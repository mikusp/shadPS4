// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <arpa/inet.h>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_signaling.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpSignaling {

s32 PS4_SYSV_ABI sceNpSignalingInitialize(u64 poolSize, s32 threadPrio, s32 cpuAffinity, u64 threadStackSize) {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called poolSize = {} threadPrio = {} cpuAffinity = {} threadStackSize = {}", poolSize, threadPrio, cpuAffinity, threadStackSize);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingCreateContext(OrbisNpOnlineId* id, OrbisNpSignalingHandler handler, void* arg, u32* ctxId) {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called id = {}", id->data);
    *ctxId = 1;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingCreateContextA(Libraries::UserService::OrbisUserServiceUserId userId, OrbisNpSignalingHandler handler, void* arg, u32* ctxId) {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called userId = {}", userId);
    *ctxId = 1;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSignalingGetLocalNetInfo(u32 ctxId, OrbisNpSignalingNetInfo* info) {
    LOG_ERROR(Lib_NpSignaling, "(STUBBED) called ctxId = {}", ctxId);

    if (info == nullptr) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    info->localAddr = (Net::OrbisNetInAddr)(u32)inet_addr("192.168.1.107");
    info->externalAddr = (Net::OrbisNetInAddr)(u32)inet_addr("94.172.117.172");
    info->natStatus = 1;

    return ORBIS_OK;
}


void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("3KOuC4RmZZU", "libSceNpSignaling", 1, "libSceNpSignaling", sceNpSignalingInitialize);
    LIB_FUNCTION("5yYjEdd4t8Y", "libSceNpSignaling", 1, "libSceNpSignaling", sceNpSignalingCreateContext);
    LIB_FUNCTION("dDLNFdY8dws", "libSceNpSignaling", 1, "libSceNpSignaling", sceNpSignalingCreateContextA);
    LIB_FUNCTION("U8AQMlOFBc8", "libSceNpSignaling", 1, "libSceNpSignaling", sceNpSignalingGetLocalNetInfo);
};

} // namespace Libraries::Np::NpSignaling