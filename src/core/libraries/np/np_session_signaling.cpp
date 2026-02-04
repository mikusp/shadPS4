// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_session_signaling.h"

namespace Libraries::Np::NpSessionSignaling {

s32 PS4_SYSV_ABI sceNpSessionSignalingInitialize() {
    LOG_ERROR(Lib_NpSessionSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSessionSignalingCreateContext2() {
    LOG_ERROR(Lib_NpSessionSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSessionSignalingDestroyContext() {
    LOG_ERROR(Lib_NpSessionSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSessionSignalingTerminate() {
    LOG_ERROR(Lib_NpSessionSignaling, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("ysmw6J-P8Ak", "libSceNpSessionSignaling", 1, "libSceNpSessionSignaling",
                 sceNpSessionSignalingInitialize);
    LIB_FUNCTION("aBuX0PX-T7I", "libSceNpSessionSignaling", 1, "libSceNpSessionSignaling",
                 sceNpSessionSignalingCreateContext2);
    LIB_FUNCTION("Z9Q9LzQDXf0", "libSceNpSessionSignaling", 1, "libSceNpSessionSignaling",
                 sceNpSessionSignalingDestroyContext);
    LIB_FUNCTION("CqJuNXo5yiM", "libSceNpSessionSignaling", 1, "libSceNpSessionSignaling",
                 sceNpSessionSignalingTerminate);
};

} // namespace Libraries::Np::NpSessionSignaling