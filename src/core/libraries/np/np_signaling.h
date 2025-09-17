// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/network/net.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np {
struct OrbisNpOnlineId;
}

namespace Libraries::Np::NpSignaling {

constexpr int ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED = 0x80552701;
constexpr int ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT = 0x80552715;

struct OrbisNpSignalingNetInfo {
    u64 size;
    Net::OrbisNetInAddr localAddr;
    Net::OrbisNetInAddr externalAddr;
    s32 natStatus;
};

using OrbisNpSignalingHandler = PS4_SYSV_ABI void (*)(u32 ctxId, u32 subject, s32 event, s32 errorCode, void* arg);

s32 PS4_SYSV_ABI sceNpSignalingInitialize(u64 poolSize, s32 threadPrio, s32 cpuAffinity, u64 threadStackSize);
s32 PS4_SYSV_ABI sceNpSignalingCreateContext(OrbisNpOnlineId* id, OrbisNpSignalingHandler handler, void* arg, u32* ctxId);
s32 PS4_SYSV_ABI sceNpSignalingGetLocalNetInfo(u32 ctxId, OrbisNpSignalingNetInfo* info);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpSignaling