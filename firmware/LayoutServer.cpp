// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#include "LayoutServer.h"

#include <WiFi.h>

namespace {
constexpr size_t kMaximumUploadBytes = 32 * 1024;

const char kUploadPage[] PROGMEM = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>TouchOSC Parser Mini</title><style>
body{font:16px system-ui;max-width:38rem;margin:2rem auto;padding:0 1rem;background:#111;color:#eee}
h1{font-size:1.55rem}h2{font-size:1.15rem}section{border:1px solid #555;padding:1rem;margin:1rem 0}input,button{box-sizing:border-box;width:100%;padding:.8rem;font:inherit}button{margin-top:1rem;background:#eee;color:#111;border:0;font-weight:650}.check{display:flex;align-items:center;gap:.7rem;margin-top:1rem}.check input{width:auto;transform:scale(1.35)}.danger{background:#802828;color:#fff}button:disabled{opacity:.45}pre{white-space:pre-wrap;color:#9f9}small{color:#aaa}.notice{line-height:1.45;color:#bbb}</style></head>
<body><h1>TouchOSC Parser Mini</h1><p>Select one or more TouchOSC Mk2 <code>.tosc</code> documents. They are parsed locally in this browser, then installed on the controller as an ordered page set.</p>
<section><h2>Layout pages</h2><input id="file" type="file" accept=".tosc" multiple><small>Select several files together for a multi-page layout. Files are ordered naturally by filename.</small><button id="upload">Install selected layout pages</button><button id="remove" class="danger">Remove installed layout</button><pre id="status">Checking device…</pre></section>
<section><h2>OSC connection</h2><form id="osc"><label>Target IPv4 address</label><input id="target" name="target" inputmode="decimal" required><label>Computer receive port</label><input id="sendPort" name="send_port" type="number" min="1" max="65535" required><label>Device receive port</label><input id="receivePort" name="receive_port" type="number" min="1" max="65535" required><label class="check"><input id="sendName" name="send_name" type="checkbox">Include device name in layout OSC addresses</label><button>Save OSC settings</button></form><pre id="oscStatus">Loading current settings…</pre></section>
<p><small>Supported controls: button, fader, radial, XY, radar, encoder, grid, label and text. The root GROUP is canvas metadata; other GROUP controls are rejected.</small></p>
<p class="notice"><small>TouchOSC is fantastic software. TouchOSC Parser Mini is an independent community project and is not affiliated with, endorsed by, sponsored by, or associated with TouchOSC or its developers. TouchOSC is a trademark of its respective owner.</small></p>
<script>
const $=id=>document.getElementById(id), enc=new TextEncoder();
const TYPES={BUTTON:1,FADER:2,RADIAL:3,XY:4,RADAR:5,ENCODER:6,GRID:7,LABEL:8,TEXT:9};
const F={VISIBLE:1<<0,INTERACTIVE:1<<1,BACKGROUND:1<<2,OUTLINE:1<<3,BRACKETS:1<<4,CIRCLE:1<<5,CURSOR:1<<6,GRIDX:1<<7,GRIDY:1<<8,BAR:1<<9,LINES:1<<10,CENTERED:1<<11,INVERTED:1<<12,LOCKX:1<<13,LOCKY:1<<14,SEND:1<<15,RECEIVE:1<<16,RELATIVE:1<<17,EXCLUSIVE:1<<18,TEXTWRAP:1<<19,TEXTCLIP:1<<20,MONO:1<<21};
const direct=(e,t)=>Array.from(e?.children||[]).find(c=>c.tagName===t);
const text=(e,t,d='')=>direct(e,t)?.textContent??d;
function props(n){const out={};const ps=direct(n,'properties');for(const p of ps?.children||[]){const k=text(p,'key'),v=direct(p,'value');if(!v)continue;if(v.children.length){const o={};for(const c of v.children)o[c.tagName]=Number(c.textContent);out[k]=o}else out[k]=v.textContent}return out}
function values(n){const out={};const vs=direct(n,'values');for(const v of vs?.children||[]){const k=text(v,'key'),raw=text(v,'default','');out[k]=k==='text'?raw:(Number(raw)||0)}return out}
const yes=v=>String(v)==='1'||v===true;
const rgba=(v,d=[1,1,1,1])=>{v=v||{r:d[0],g:d[1],b:d[2],a:d[3]};return ['r','g','b','a'].map((k,i)=>Math.max(0,Math.min(255,Math.round(Number(v[k]??d[i])*255))))};
function oscInfo(n,p,index,count){const m=direct(n,'messages'),o=direct(m,'osc');if(!o||!yes(text(o,'enabled'))||(!yes(text(o,'send'))&&!yes(text(o,'receive'))))return {address:'',count:0,send:false,receive:false};let address='';for(const q of direct(o,'path')?.children||[]){const kind=text(q,'type'),raw=text(q,'value');if(kind==='CONSTANT')address+=raw;else if(kind==='PROPERTY'){if(raw==='name')address+=p.name||'';else if(raw==='parent.name')address+=p.parentName||'';else throw Error(`Unsupported path property ${raw}`)}else if(kind==='INDEX'){const lo=Number(text(q,'scaleMin','0')),hi=Number(text(q,'scaleMax','1'));const scaled=lo+index*(hi-lo);address+=text(q,'conversion')==='INTEGER'?String(Math.round(scaled)):String(scaled)}else throw Error(`Unsupported OSC path partial ${kind}`)}let ac=0,stringArgument=false;for(const q of direct(o,'arguments')?.children||[]){const kind=text(q,'type'),conversion=text(q,'conversion'),value=text(q,'value');if(kind==='VALUE'&&conversion==='FLOAT'&&['x','y'].includes(value)&&!stringArgument)ac++;else if(kind==='VALUE'&&conversion==='STRING'&&value==='text'&&ac===0&&!stringArgument)stringArgument=true;else throw Error('OSC arguments must be one text string or up to two x/y floats')}if(ac>2)throw Error('A maximum of two OSC float arguments is supported');return {address,count:stringArgument?3:ac,send:yes(text(o,'send')),receive:yes(text(o,'receive'))}}
function compile(xml){const doc=new DOMParser().parseFromString(xml,'application/xml');if(doc.querySelector('parsererror'))throw Error('The decompressed file is not valid XML');const root=direct(doc.documentElement,'node');if(!root||root.getAttribute('type')!=='GROUP')throw Error('Missing TouchOSC document root');const rp=props(root),rf=rp.frame||{},controls=[];function visit(n,ox,oy,parentName,parentGrid,index,count){const type=n.getAttribute('type'),p=props(n),v=values(n),f=p.frame||{};if(type==='GROUP')throw Error('Nested GROUP controls are not supported');if(!TYPES[type])throw Error(`Unsupported control type ${type}`);const frame={x:Math.round(ox+Number(f.x||0)),y:Math.round(oy+Number(f.y||0)),w:Math.round(Number(f.w||0)),h:Math.round(Number(f.h||0))};const orientation=Math.max(0,Math.min(3,Math.round(Number(p.orientation||0))));let flags=orientation<<22;if(yes(p.visible))flags|=F.VISIBLE;if(yes(p.interactive)&&!['GRID','LABEL','TEXT'].includes(type))flags|=F.INTERACTIVE;if(yes(p.background))flags|=F.BACKGROUND;if(yes(p.outline))flags|=F.OUTLINE;if(String(p.outlineStyle)==='1')flags|=F.BRACKETS;if(String(p.shape)==='2')flags|=F.CIRCLE;if(yes(p.cursor))flags|=F.CURSOR;if(yes(p.bar))flags|=F.BAR;if(yes(p.lines))flags|=F.LINES;if(yes(p.centered))flags|=F.CENTERED;if(yes(p.inverted))flags|=F.INVERTED;if(yes(p.lockX))flags|=F.LOCKX;if(yes(p.lockY))flags|=F.LOCKY;if(String(p.response)==='1')flags|=F.RELATIVE;if(yes(p.exclusive))flags|=F.EXCLUSIVE;if(yes(p.gridX)||(yes(p.grid)&&String(p.orientation)!=='1'))flags|=type==='FADER'?F.GRIDY:F.GRIDX;if(yes(p.gridY)||(yes(p.grid)&&String(p.orientation)==='1'))flags|=F.GRIDY;if(yes(p.textWrap))flags|=F.TEXTWRAP;if(yes(p.textClip))flags|=F.TEXTCLIP;if(String(p.font)==='1')flags|=F.MONO;const oi=type==='GRID'?{address:'',count:0,send:false,receive:false}:oscInfo(n,{name:p.name||'',parentName},index,count);if(oi.send)flags|=F.SEND;if(oi.receive)flags|=F.RECEIVE;let displayText=String(v.text??'');const limit=Number(p.textLength||0);if(type==='LABEL'&&limit>0)displayText=displayText.slice(0,limit);const rec={type:TYPES[type],flags,frame,color:rgba(p.color),gridColor:rgba(p.gridColor,[0,0,0,.25]),textColor:rgba(p.textColor),gx:Number(p.gridStepsX||p.gridSteps||0),gy:Number(p.gridStepsY||p.gridSteps||0),vc:oi.count,parent:parentGrid,x:Number(v.x||0),y:Number(v.y||0),address:oi.address,text:displayText,textSize:Math.max(1,Math.round(Number(p.textSize||16))),alignH:Number(p.textAlignH||2),alignV:Number(p.textAlignV||2)};const recIndex=controls.push(rec)-1;if(controls.length>64)throw Error('Layout has more than 64 rendered controls');if(type==='GRID'){const ch=direct(n,'children'),kids=Array.from(ch?.children||[]).filter(e=>e.tagName==='node');kids.forEach((kid,i)=>visit(kid,frame.x,frame.y,p.name||'',recIndex,i,kids.length))}}
const ch=direct(root,'children'),kids=Array.from(ch?.children||[]).filter(e=>e.tagName==='node');kids.forEach((n,i)=>visit(n,0,0,'',255,i,kids.length));if(!controls.length)throw Error('Layout has no supported controls');const bytes=[];const u8=v=>bytes.push(v&255),u16=v=>{u8(v);u8(v>>8)},u32=v=>{u16(v);u16(v>>>16)},flt=v=>{const b=new ArrayBuffer(4);new DataView(b).setFloat32(0,v,true);bytes.push(...new Uint8Array(b))},put=a=>bytes.push(...a);put(enc.encode('TLAY'));u8(2);u8(controls.length);u16(Number(rf.w||368));u16(Number(rf.h||448));put(rgba(rp.color,[0,0,0,1]));u16(0);for(const c of controls){u8(c.type);u32(c.flags);u16(c.frame.x);u16(c.frame.y);u16(c.frame.w);u16(c.frame.h);put(c.color);put(c.gridColor);u8(c.gx);u8(c.gy);u8(c.vc);u8(c.parent);flt(c.x);flt(c.y);const a=enc.encode(c.address);if(a.length>95)throw Error(`OSC address is too long: ${c.address}`);u8(a.length);put(a);put(c.textColor);u16(c.textSize);u8(c.alignH);u8(c.alignV);const t=enc.encode(c.text);if(t.length>127)throw Error('Label or text content is longer than 127 bytes');u8(t.length);put(t)}return {data:new Uint8Array(bytes),count:controls.length,width:Number(rf.w||368),height:Number(rf.h||448)}}
function combinePages(pages){if(!pages.length||pages.length>8)throw Error('Select between one and eight layout pages');const total=pages.reduce((n,p)=>n+p.count,0);if(total>64)throw Error('The combined layout has more than 64 rendered controls');const bytes=[];const u8=v=>bytes.push(v&255),u16=v=>{u8(v);u8(v>>8)},put=a=>bytes.push(...a);put(enc.encode('TLAY'));u8(3);u8(total);const first=pages[0].data;put(first.slice(6,14));u16(pages.length);for(let page=1;page<pages.length;page++)put(pages[page].data.slice(6,14));pages.forEach((layout,page)=>{const data=layout.data;let pos=16;for(let index=0;index<layout.count;index++){const start=pos,alen=data[start+33];pos=start+34+alen;const tlen=data[pos+8],end=pos+9+tlen;if(end>data.length)throw Error('Compiled page data is invalid');u8(data[start]);u8(page);put(data.slice(start+1,end));pos=end}if(pos!==data.length)throw Error('Compiled page has trailing data')});return {data:new Uint8Array(bytes),count:total,pages:pages.length}}
$('upload').onclick=async()=>{const files=Array.from($('file').files).sort((a,b)=>a.name.localeCompare(b.name,undefined,{numeric:true,sensitivity:'base'}));if(!files.length)return $('status').textContent='Choose at least one .tosc file first.';$('upload').disabled=true;try{if(typeof DecompressionStream==='undefined')throw Error('This browser does not support local deflate decompression');if(files.length>8)throw Error('A maximum of eight layout pages is supported');const parsed=[];for(const file of files){$('status').textContent=`Parsing ${file.name}…`;const xml=await new Response(file.stream().pipeThrough(new DecompressionStream('deflate'))).text();parsed.push(compile(xml))}const layout=combinePages(parsed);$('status').textContent=`Parsed ${layout.count} controls across ${layout.pages} page${layout.pages===1?'':'s'}. Uploading…`;const response=await fetch('/layout',{method:'PUT',headers:{'Content-Type':'application/octet-stream'},body:layout.data});const result=await response.text();if(!response.ok)throw Error(result);$('status').textContent=result}catch(e){$('status').textContent='Error: '+e.message}finally{$('upload').disabled=false}};
async function loadLayoutStatus(){try{$('status').textContent=await fetch('/status').then(r=>r.text())}catch(e){$('status').textContent='Could not read layout status: '+e.message}}
$('remove').onclick=async()=>{if(!confirm('Remove the installed layout from this device?'))return;$('remove').disabled=true;try{const response=await fetch('/layout',{method:'DELETE'});const result=await response.text();if(!response.ok)throw Error(result);$('status').textContent=result}catch(e){$('status').textContent='Error: '+e.message}finally{$('remove').disabled=false}};
async function loadSettings(){try{const s=await fetch('/settings').then(r=>r.json());$('target').value=s.oscTarget;$('sendPort').value=s.oscSendPort;$('receivePort').value=s.oscReceivePort;$('sendName').checked=s.oscIncludeDeviceName;$('oscStatus').textContent=`Device: ${s.deviceName} · ${s.ip}`}catch(e){$('oscStatus').textContent='Could not read settings: '+e.message}}
$('osc').onsubmit=async e=>{e.preventDefault();const button=e.submitter;button.disabled=true;$('oscStatus').textContent='Saving…';try{const body=new URLSearchParams(new FormData(e.target));const response=await fetch('/osc',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const result=await response.text();if(!response.ok)throw Error(result);$('oscStatus').textContent=result}catch(error){$('oscStatus').textContent='Error: '+error.message}finally{button.disabled=false}};loadLayoutStatus();loadSettings();
</script></body></html>)HTML";

bool parsePort(const String &text, uint16_t &port) {
  if (text.isEmpty()) return false;
  char *end = nullptr;
  const long value = strtol(text.c_str(), &end, 10);
  if (!end || *end != '\0' || value < 1 || value > 65535) return false;
  port = static_cast<uint16_t>(value);
  return true;
}
}  // namespace

void LayoutServer::begin(TouchOscLayout &layout, DeviceSettings &settings,
                         ConfigStore &store,
                         LayoutSettingsHandler settingsHandler,
                         LayoutChangeHandler changeHandler) {
  layout_ = &layout;
  settings_ = &settings;
  store_ = &store;
  settingsHandler_ = settingsHandler;
  changeHandler_ = changeHandler;
  if (!configured_) configureRoutes();
}

void LayoutServer::configureRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/layout", HTTP_PUT,
             [this]() { handleUploadComplete(); },
             [this]() { handleUploadData(); });
  server_.on("/layout", HTTP_DELETE, [this]() { handleRemove(); });
  server_.on("/status", HTTP_GET, [this]() {
    String result = layout_ && layout_->active()
        ? String("active pages=") + layout_->pageCount() +
              " controls=" + layout_->controlCount()
        : String("no active layout");
    server_.send(200, "text/plain", result);
  });
  server_.on("/settings", HTTP_GET, [this]() { handleSettings(); });
  server_.on("/osc", HTTP_POST, [this]() { handleOscSave(); });
  server_.onNotFound([this]() { server_.send(404, "text/plain", "Not found"); });
  configured_ = true;
}

void LayoutServer::handleSettings() {
  if (!settings_) {
    server_.send(500, "application/json", "{\"error\":\"unavailable\"}");
    return;
  }
  String json;
  json.reserve(180);
  json += F("{\"deviceName\":\"");
  json += settings_->deviceName;
  json += F("\",\"ip\":\"");
  json += WiFi.localIP().toString();
  json += F("\",\"oscTarget\":\"");
  json += settings_->oscTarget;
  json += F("\",\"oscSendPort\":");
  json += settings_->oscSendPort;
  json += F(",\"oscReceivePort\":");
  json += settings_->oscReceivePort;
  json += F(",\"oscIncludeDeviceName\":");
  json += settings_->oscIncludeDeviceName ? F("true") : F("false");
  json += '}';
  server_.send(200, "application/json", json);
}

void LayoutServer::handleOscSave() {
  if (!settings_ || !store_) {
    server_.send(500, "text/plain", "Settings are unavailable");
    return;
  }
  const String targetText = server_.arg("target");
  IPAddress target;
  uint16_t sendPort = 0;
  uint16_t receivePort = 0;
  if (!target.fromString(targetText) || target == IPAddress(0, 0, 0, 0) ||
      !parsePort(server_.arg("send_port"), sendPort) ||
      !parsePort(server_.arg("receive_port"), receivePort)) {
    server_.send(400, "text/plain", "Enter a valid IPv4 address and ports from 1 to 65535");
    return;
  }
  settings_->oscTarget = target.toString();
  settings_->oscSendPort = sendPort;
  settings_->oscReceivePort = receivePort;
  settings_->oscIncludeDeviceName = server_.hasArg("send_name");
  if (!store_->save(*settings_)) {
    server_.send(500, "text/plain", "Could not save OSC settings");
    return;
  }
  if (settingsHandler_) settingsHandler_();
  server_.send(200, "text/plain",
               String("Saved. Sending to ") + settings_->oscTarget + ":" +
                   settings_->oscSendPort + " and listening on " +
                   settings_->oscReceivePort + ". Device name " +
                   (settings_->oscIncludeDeviceName ? "included." : "not included."));
}

void LayoutServer::loop(bool wifiConnected) {
  if (wifiConnected && !running_) {
    server_.begin();
    running_ = true;
    Serial.println("layout upload http=ready");
  } else if (!wifiConnected && running_) {
    server_.stop();
    running_ = false;
  }
  if (running_) server_.handleClient();
}

void LayoutServer::handleRoot() {
  server_.send_P(200, "text/html", kUploadPage);
}

void LayoutServer::handleUploadData() {
  HTTPRaw &upload = server_.raw();
  if (upload.status == RAW_START) {
    uploadFailed_ = false;
    uploadBytes_ = 0;
    FFat.remove(TouchOscLayout::kUploadPath);
    uploadFile_ = FFat.open(TouchOscLayout::kUploadPath, FILE_WRITE);
    if (!uploadFile_) uploadFailed_ = true;
  } else if (upload.status == RAW_WRITE) {
    uploadBytes_ += upload.currentSize;
    if (uploadBytes_ > kMaximumUploadBytes || !uploadFile_ ||
        uploadFile_.write(upload.buf, upload.currentSize) != upload.currentSize) {
      uploadFailed_ = true;
    }
  } else if (upload.status == RAW_END || upload.status == RAW_ABORTED) {
    if (uploadFile_) uploadFile_.close();
    if (upload.status == RAW_ABORTED) uploadFailed_ = true;
  }
}

void LayoutServer::handleUploadComplete() {
  if (uploadFile_) uploadFile_.close();
  if (uploadFailed_ || uploadBytes_ == 0 || !layout_) {
    FFat.remove(TouchOscLayout::kUploadPath);
    server_.send(400, "text/plain", "Upload failed or exceeded 32 KB");
    return;
  }
  if (!layout_->installUploaded()) {
    server_.send(400, "text/plain", layout_->lastError());
    return;
  }
  if (changeHandler_) changeHandler_();
  server_.send(200, "text/plain",
               String("Installed ") + layout_->pageCount() + " page" +
                   (layout_->pageCount() == 1 ? "" : "s") + " with " +
                   layout_->controlCount() + " controls.");
}

void LayoutServer::handleRemove() {
  if (!layout_) {
    server_.send(500, "text/plain", "Layout storage is unavailable");
    return;
  }
  if (!layout_->removeActive()) {
    server_.send(500, "text/plain", layout_->lastError());
    return;
  }
  if (changeHandler_) changeHandler_();
  server_.send(200, "text/plain",
               "Layout removed. The device is showing its Parser Mini home screen.");
}
