#!/usr/bin/env python3
"""Compile TouchOSC Mk2 documents into Parser Mini's compact page format."""

from __future__ import annotations

import argparse
import math
import re
import struct
import sys
import zlib
from pathlib import Path
from xml.etree import ElementTree as ET


TYPES = {
    "BUTTON": 1,
    "FADER": 2,
    "RADIAL": 3,
    "XY": 4,
    "RADAR": 5,
    "ENCODER": 6,
    "GRID": 7,
    "LABEL": 8,
    "TEXT": 9,
}

FLAGS = {
    "VISIBLE": 1 << 0,
    "INTERACTIVE": 1 << 1,
    "BACKGROUND": 1 << 2,
    "OUTLINE": 1 << 3,
    "BRACKETS": 1 << 4,
    "CIRCLE": 1 << 5,
    "CURSOR": 1 << 6,
    "GRIDX": 1 << 7,
    "GRIDY": 1 << 8,
    "BAR": 1 << 9,
    "LINES": 1 << 10,
    "CENTERED": 1 << 11,
    "INVERTED": 1 << 12,
    "LOCKX": 1 << 13,
    "LOCKY": 1 << 14,
    "SEND": 1 << 15,
    "RECEIVE": 1 << 16,
    "RELATIVE": 1 << 17,
    "EXCLUSIVE": 1 << 18,
    "TEXTWRAP": 1 << 19,
    "TEXTCLIP": 1 << 20,
    "MONO": 1 << 21,
}


class CompileError(ValueError):
    pass


def direct(element: ET.Element | None, tag: str) -> ET.Element | None:
    if element is None:
        return None
    return next((child for child in element if child.tag == tag), None)


def child_text(element: ET.Element | None, tag: str, default: str = "") -> str:
    child = direct(element, tag)
    return default if child is None or child.text is None else child.text


def number(value: object, default: float = 0.0) -> float:
    try:
        result = float(value)
        return result if math.isfinite(result) else default
    except (TypeError, ValueError):
        return default


def js_round(value: object) -> int:
    return math.floor(number(value) + 0.5)


def yes(value: object) -> bool:
    return value is True or str(value) == "1"


def string_value(value: object) -> str:
    if value is None:
        return ""
    return str(value)


def js_number_string(value: float) -> str:
    if value == 0:
        return "0"
    if value.is_integer():
        return str(int(value))
    return format(value, ".15g")


def properties(node: ET.Element) -> dict[str, object]:
    result: dict[str, object] = {}
    container = direct(node, "properties")
    if container is None:
        return result
    for item in container:
        key = child_text(item, "key")
        value = direct(item, "value")
        if value is None:
            continue
        children = list(value)
        if children:
            result[key] = {
                child.tag: number(child.text) for child in children
            }
        else:
            result[key] = value.text or ""
    return result


def values(node: ET.Element) -> dict[str, object]:
    result: dict[str, object] = {}
    container = direct(node, "values")
    if container is None:
        return result
    for item in container:
        key = child_text(item, "key")
        raw = child_text(item, "default")
        result[key] = raw if key == "text" else number(raw)
    return result


def rgba(value: object, default: tuple[float, float, float, float]) -> bytes:
    source = value if isinstance(value, dict) else {}
    channels = []
    for index, key in enumerate(("r", "g", "b", "a")):
        channel = number(source.get(key, default[index]), default[index])
        channels.append(max(0, min(255, js_round(channel * 255))))
    return bytes(channels)


def first_truthy(*values_to_check: object) -> object:
    for value in values_to_check:
        if value not in (None, "", False):
            return value
    return 0


def osc_info(
    node: ET.Element,
    name: str,
    parent_name: str,
    index: int,
    count: int,
) -> dict[str, object]:
    messages = direct(node, "messages")
    osc = direct(messages, "osc")
    if (
        osc is None
        or not yes(child_text(osc, "enabled"))
        or not (yes(child_text(osc, "send")) or yes(child_text(osc, "receive")))
    ):
        return {"address": "", "count": 0, "send": False, "receive": False}

    address = ""
    path = direct(osc, "path")
    for partial in list(path) if path is not None else []:
        kind = child_text(partial, "type")
        raw = child_text(partial, "value")
        if kind == "CONSTANT":
            address += raw
        elif kind == "PROPERTY":
            if raw == "name":
                address += name
            elif raw == "parent.name":
                address += parent_name
            else:
                raise CompileError(f"Unsupported path property {raw}")
        elif kind == "INDEX":
            low = number(child_text(partial, "scaleMin", "0"))
            high = number(child_text(partial, "scaleMax", "1"))
            # TouchOSC evaluates INDEX as the raw zero-based position in the
            # parent's child list, then applies out = min + in * (max - min).
            scaled = low + index * (high - low)
            if child_text(partial, "conversion") == "INTEGER":
                address += str(js_round(scaled))
            else:
                address += js_number_string(scaled)
        else:
            raise CompileError(f"Unsupported OSC path partial {kind}")

    argument_count = 0
    string_argument = False
    arguments = direct(osc, "arguments")
    for partial in list(arguments) if arguments is not None else []:
        kind = child_text(partial, "type")
        conversion = child_text(partial, "conversion")
        value = child_text(partial, "value")
        if (
            kind == "VALUE"
            and conversion == "FLOAT"
            and value in ("x", "y")
            and not string_argument
        ):
            argument_count += 1
        elif (
            kind == "VALUE"
            and conversion == "STRING"
            and value == "text"
            and argument_count == 0
            and not string_argument
        ):
            string_argument = True
        else:
            raise CompileError(
                "OSC arguments must be one text string or up to two x/y floats"
            )
    if argument_count > 2:
        raise CompileError("A maximum of two OSC float arguments is supported")
    return {
        "address": address,
        "count": 3 if string_argument else argument_count,
        "send": yes(child_text(osc, "send")),
        "receive": yes(child_text(osc, "receive")),
    }


def append_u8(output: bytearray, value: int) -> None:
    output.append(int(value) & 0xFF)


def append_u16(output: bytearray, value: int) -> None:
    output.extend(struct.pack("<H", int(value) & 0xFFFF))


def append_u32(output: bytearray, value: int) -> None:
    output.extend(struct.pack("<I", int(value) & 0xFFFFFFFF))


def append_float(output: bytearray, value: float) -> None:
    output.extend(struct.pack("<f", float(value)))


def compile_page(path: Path) -> dict[str, object]:
    try:
        xml = zlib.decompress(path.read_bytes())
        document = ET.fromstring(xml)
    except (OSError, zlib.error, ET.ParseError) as error:
        raise CompileError(f"{path.name}: could not decompress or parse: {error}") from error

    root = direct(document, "node")
    if root is None or root.get("type") != "GROUP":
        raise CompileError(f"{path.name}: missing TouchOSC document root")
    root_properties = properties(root)
    root_frame = root_properties.get("frame", {})
    if not isinstance(root_frame, dict):
        root_frame = {}
    controls: list[dict[str, object]] = []

    def visit(
        node: ET.Element,
        offset_x: int,
        offset_y: int,
        parent_name: str,
        parent_grid: int,
        index: int,
        count: int,
    ) -> None:
        control_type = node.get("type", "")
        props = properties(node)
        vals = values(node)
        frame_value = props.get("frame", {})
        frame = frame_value if isinstance(frame_value, dict) else {}
        if control_type == "GROUP":
            raise CompileError(f"{path.name}: nested GROUP controls are not supported")
        if control_type not in TYPES:
            raise CompileError(f"{path.name}: unsupported control type {control_type}")
        compiled_frame = {
            "x": js_round(offset_x + number(frame.get("x", 0))),
            "y": js_round(offset_y + number(frame.get("y", 0))),
            "w": js_round(frame.get("w", 0)),
            "h": js_round(frame.get("h", 0)),
        }

        orientation = max(0, min(3, js_round(props.get("orientation", 0))))
        flags = orientation << 22
        if yes(props.get("visible")):
            flags |= FLAGS["VISIBLE"]
        if yes(props.get("interactive")) and control_type not in ("GRID", "LABEL", "TEXT"):
            flags |= FLAGS["INTERACTIVE"]
        for property_name, flag_name in (
            ("background", "BACKGROUND"),
            ("outline", "OUTLINE"),
            ("cursor", "CURSOR"),
            ("bar", "BAR"),
            ("lines", "LINES"),
            ("centered", "CENTERED"),
            ("inverted", "INVERTED"),
            ("lockX", "LOCKX"),
            ("lockY", "LOCKY"),
            ("exclusive", "EXCLUSIVE"),
            ("textWrap", "TEXTWRAP"),
            ("textClip", "TEXTCLIP"),
        ):
            if yes(props.get(property_name)):
                flags |= FLAGS[flag_name]
        if string_value(props.get("outlineStyle")) == "1":
            flags |= FLAGS["BRACKETS"]
        if string_value(props.get("shape")) == "2":
            flags |= FLAGS["CIRCLE"]
        if string_value(props.get("response")) == "1":
            flags |= FLAGS["RELATIVE"]
        if string_value(props.get("font")) == "1":
            flags |= FLAGS["MONO"]
        if yes(props.get("gridX")) or (
            yes(props.get("grid")) and string_value(props.get("orientation")) != "1"
        ):
            flags |= FLAGS["GRIDY" if control_type == "FADER" else "GRIDX"]
        if yes(props.get("gridY")) or (
            yes(props.get("grid")) and string_value(props.get("orientation")) == "1"
        ):
            flags |= FLAGS["GRIDY"]

        name = string_value(props.get("name"))
        info = (
            {"address": "", "count": 0, "send": False, "receive": False}
            if control_type == "GRID"
            else osc_info(node, name, parent_name, index, count)
        )
        if info["send"]:
            flags |= FLAGS["SEND"]
        if info["receive"]:
            flags |= FLAGS["RECEIVE"]

        display_text = string_value(vals.get("text", ""))
        text_limit = int(number(props.get("textLength", 0)))
        if control_type == "LABEL" and text_limit > 0:
            display_text = display_text[:text_limit]
        record = {
            "type": TYPES[control_type],
            "flags": flags,
            "frame": compiled_frame,
            "color": rgba(props.get("color"), (1, 1, 1, 1)),
            "grid_color": rgba(props.get("gridColor"), (0, 0, 0, 0.25)),
            "grid_x": int(number(first_truthy(props.get("gridStepsX"), props.get("gridSteps"), 0))),
            "grid_y": int(number(first_truthy(props.get("gridStepsY"), props.get("gridSteps"), 0))),
            "value_count": int(info["count"]),
            "parent": parent_grid,
            "x": number(vals.get("x", 0)),
            "y": number(vals.get("y", 0)),
            "address": str(info["address"]),
            "text_color": rgba(props.get("textColor"), (1, 1, 1, 1)),
            "text_size": max(1, js_round(first_truthy(props.get("textSize"), 16))),
            "align_h": int(number(first_truthy(props.get("textAlignH"), 2))),
            "align_v": int(number(first_truthy(props.get("textAlignV"), 2))),
            "text": display_text,
        }
        record_index = len(controls)
        controls.append(record)
        if len(controls) > 64:
            raise CompileError(f"{path.name}: layout has more than 64 rendered controls")

        if control_type == "GRID":
            children = direct(node, "children")
            child_nodes = [
                child
                for child in (list(children) if children is not None else [])
                if child.tag == "node"
            ]
            for child_index, child in enumerate(child_nodes):
                visit(
                    child,
                    compiled_frame["x"],
                    compiled_frame["y"],
                    name,
                    record_index,
                    child_index,
                    len(child_nodes),
                )

    children = direct(root, "children")
    top_nodes = [
        child
        for child in (list(children) if children is not None else [])
        if child.tag == "node"
    ]
    for child_index, child in enumerate(top_nodes):
        visit(child, 0, 0, "", 255, child_index, len(top_nodes))
    if not controls:
        raise CompileError(f"{path.name}: layout has no supported controls")

    output = bytearray(b"TLAY")
    append_u8(output, 2)
    append_u8(output, len(controls))
    append_u16(output, int(number(root_frame.get("w", 368), 368)))
    append_u16(output, int(number(root_frame.get("h", 448), 448)))
    output.extend(rgba(root_properties.get("color"), (0, 0, 0, 1)))
    append_u16(output, 0)

    for control in controls:
        frame = control["frame"]
        append_u8(output, control["type"])
        append_u32(output, control["flags"])
        append_u16(output, frame["x"])
        append_u16(output, frame["y"])
        append_u16(output, frame["w"])
        append_u16(output, frame["h"])
        output.extend(control["color"])
        output.extend(control["grid_color"])
        append_u8(output, control["grid_x"])
        append_u8(output, control["grid_y"])
        append_u8(output, control["value_count"])
        append_u8(output, control["parent"])
        append_float(output, control["x"])
        append_float(output, control["y"])
        address = control["address"].encode("utf-8")
        if len(address) > 95:
            raise CompileError(f"{path.name}: OSC address is too long: {control['address']}")
        append_u8(output, len(address))
        output.extend(address)
        output.extend(control["text_color"])
        append_u16(output, control["text_size"])
        append_u8(output, control["align_h"])
        append_u8(output, control["align_v"])
        text = control["text"].encode("utf-8")
        if len(text) > 127:
            raise CompileError(f"{path.name}: label or text is longer than 127 bytes")
        append_u8(output, len(text))
        output.extend(text)

    return {
        "data": bytes(output),
        "count": len(controls),
        "width": int(number(root_frame.get("w", 368), 368)),
        "height": int(number(root_frame.get("h", 448), 448)),
        "name": path.name,
    }


def combine_pages(pages: list[dict[str, object]]) -> bytes:
    if not 1 <= len(pages) <= 8:
        raise CompileError("Select between one and eight layout pages")
    total = sum(int(page["count"]) for page in pages)
    if total > 64:
        raise CompileError("The combined layout has more than 64 rendered controls")
    output = bytearray(b"TLAY")
    append_u8(output, 3)
    append_u8(output, total)
    first = pages[0]["data"]
    output.extend(first[6:14])
    append_u16(output, len(pages))
    for page in pages[1:]:
        output.extend(page["data"][6:14])

    for page_index, page in enumerate(pages):
        data = page["data"]
        position = 16
        for _ in range(int(page["count"])):
            start = position
            address_length = data[start + 33]
            position = start + 34 + address_length
            text_length = data[position + 8]
            end = position + 9 + text_length
            if end > len(data):
                raise CompileError(f"{page['name']}: compiled page data is invalid")
            append_u8(output, data[start])
            append_u8(output, page_index)
            output.extend(data[start + 1 : end])
            position = end
        if position != len(data):
            raise CompileError(f"{page['name']}: compiled page has trailing data")
    if len(output) > 32 * 1024:
        raise CompileError("The combined layout exceeds the 32 KB device limit")
    return bytes(output)


def natural_key(path: Path) -> list[object]:
    return [int(part) if part.isdigit() else part.lower() for part in re.split(r"(\d+)", path.name)]


def write_header(path: Path, data: bytes, revision: int, sources: list[Path]) -> None:
    lines = [
        "// Generated private layout bundle. This file is excluded from Git.",
        f"// Sources: {', '.join(source.name for source in sources)}",
        "#pragma once",
        "",
        f"#define TPM_DEFAULT_LAYOUT_REVISION {revision}",
        "static const uint8_t kPrivateDefaultLayoutData[] PROGMEM = {",
    ]
    for offset in range(0, len(data), 16):
        chunk = data[offset : offset + 16]
        lines.append("  " + ", ".join(f"0x{byte:02x}" for byte in chunk) + ",")
    lines.extend(
        [
            "};",
            "#define TPM_DEFAULT_LAYOUT_DATA kPrivateDefaultLayoutData",
            "#define TPM_DEFAULT_LAYOUT_SIZE sizeof(kPrivateDefaultLayoutData)",
            "",
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Compile one or more .tosc files into a Parser Mini page set."
    )
    parser.add_argument("inputs", nargs="+", type=Path)
    parser.add_argument("--binary", type=Path, help="write the compact .bin file")
    parser.add_argument("--header", type=Path, help="write a private C++ data header")
    parser.add_argument("--revision", type=int, default=1)
    args = parser.parse_args()
    if not args.binary and not args.header:
        parser.error("provide --binary, --header, or both")
    if args.revision < 1:
        parser.error("--revision must be at least 1")

    inputs = sorted(args.inputs, key=natural_key)
    try:
        pages = [compile_page(path) for path in inputs]
        combined = combine_pages(pages)
    except CompileError as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1

    if args.binary:
        args.binary.parent.mkdir(parents=True, exist_ok=True)
        args.binary.write_bytes(combined)
    if args.header:
        args.header.parent.mkdir(parents=True, exist_ok=True)
        write_header(args.header, combined, args.revision, inputs)
    print(
        f"Compiled {len(pages)} pages, "
        f"{sum(int(page['count']) for page in pages)} controls, "
        f"{len(combined)} bytes."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
