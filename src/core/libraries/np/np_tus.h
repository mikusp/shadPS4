// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpTus {

s32 PS4_SYSV_ABI sceNpTssGetSmallStorage(s32 reqId, void* data, u64 maxSize, u64* size, void* option);
s32 PS4_SYSV_ABI sceNpTusCreateRequest(s32 titleCtxId);
s32 PS4_SYSV_ABI sceNpTusDeleteRequest(s32 reqId);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpTus