// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/system/userservice.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpMatching2 {

using OrbisNpMatching2ContextId = u16;
using OrbisNpMatching2Event = u16;
using OrbisNpMatching2EventCause = u8;
using OrbisNpMatching2ContextCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId ctxId, OrbisNpMatching2Event event, OrbisNpMatching2EventCause cause, s32 errorCode, void* arg);
using OrbisNpMatching2ContextCallback_ = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId ctxId, OrbisNpMatching2Event event, OrbisNpMatching2EventCause cause, s32 errorCode);

constexpr int ORBIS_NP_MATCHING2_ERROR_ALREADY_INITIALIZED = 0x80550C02;
constexpr int ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED = 0x80550C03;
constexpr int ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT = 0x80550C0A;

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpMatching2
