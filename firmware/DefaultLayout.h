// SPDX-FileCopyrightText: 2026 Sebastian Tomczak
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>

// DefaultLayout.local.h is generated from private .tosc inputs and excluded
// from Git. Public builds use the inert defaults below.
#if __has_include("DefaultLayout.local.h")
#include "DefaultLayout.local.h"
#endif

#ifndef TPM_DEFAULT_LAYOUT_REVISION
#define TPM_DEFAULT_LAYOUT_REVISION 0
#endif

#ifndef TPM_DEFAULT_LAYOUT_DATA
#define TPM_DEFAULT_LAYOUT_DATA nullptr
#endif

#ifndef TPM_DEFAULT_LAYOUT_SIZE
#define TPM_DEFAULT_LAYOUT_SIZE 0
#endif

namespace DefaultLayout {

inline uint32_t revision() {
  return static_cast<uint32_t>(TPM_DEFAULT_LAYOUT_REVISION);
}

inline const uint8_t *data() {
  return TPM_DEFAULT_LAYOUT_DATA;
}

inline size_t size() {
  return static_cast<size_t>(TPM_DEFAULT_LAYOUT_SIZE);
}

inline bool available() {
  return revision() > 0 && data() != nullptr && size() >= 16;
}

}  // namespace DefaultLayout
