// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_signaling.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpSignaling {

constexpr int ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED = 0x80552701;
constexpr int ORBIS_NP_SIGNALING_ERROR_ALREADY_INITIALIZED = 0x80552702;
constexpr int ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT = 0x80552715;

static bool g_initialized = false;
static u32 contextId = 1;

int PS4_SYSV_ABI sceNpSignalingCreateContext(Libraries::Np::OrbisNpId* npId, void* handler,
                                             void* arg, u32* ctxId) {
    LOG_DEBUG(Lib_NpSignaling, "called");

    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!npId || !ctxId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    *ctxId = contextId++;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSignalingCreateContextA(Libraries::UserService::OrbisUserServiceUserId userId,
                                              void* handler, void* arg, u32* ctxId) {
    LOG_DEBUG(Lib_NpSignaling, "called");

    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!ctxId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    *ctxId = contextId++;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSignalingGetLocalNetInfo(u32 ctxId, OrbisNetInfo* info) {
    LOG_DEBUG(Lib_NpSignaling, "called, ctxId = {}", ctxId);

    if (!g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!info || info->size == sizeof(OrbisNetInfo)) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpSignalingInitialize(size_t poolSize, int threadPrio, int affinityMask,
                                          size_t stackSize) {
    LOG_DEBUG(Lib_NpSignaling,
              "called, poolSize = {}, threadPrio = {}, affinityMask = {}, stackSize = {}", poolSize,
              threadPrio, affinityMask, stackSize);

    if (g_initialized) {
        return ORBIS_NP_SIGNALING_ERROR_ALREADY_INITIALIZED;
    }

    g_initialized = true;

    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("5yYjEdd4t8Y", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingCreateContext);
    LIB_FUNCTION("dDLNFdY8dws", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingCreateContextA);
    LIB_FUNCTION("U8AQMlOFBc8", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingGetLocalNetInfo);
    LIB_FUNCTION("3KOuC4RmZZU", "libSceNpSignaling", 1, "libSceNpSignaling",
                 sceNpSignalingInitialize);
};

} // namespace Libraries::Np::NpSignaling