// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_tus.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpTus {

s32 PS4_SYSV_ABI sceNpTssCreateNpTitleCtx(OrbisNpServiceLabel service, OrbisNpOnlineId* npId) {
    LOG_ERROR(Lib_NpTus, "(STUBBED) service = {:#x}, npId = {}", service, npId->data);
    static s32 id = 1;
    return id++;
}

s32 PS4_SYSV_ABI sceNpTssCreateNpTitleCtxA(OrbisNpServiceLabel service, Libraries::UserService::OrbisUserServiceUserId userId) {
    LOG_ERROR(Lib_NpTus, "(STUBBED) service = {:#x}, userId = {}", service, userId);
    static s32 id = 1;
    return id++;
}

s32 PS4_SYSV_ABI sceNpTusCreateNpTitleCtx(OrbisNpServiceLabel service, OrbisNpOnlineId* npId) {
    LOG_ERROR(Lib_NpTus, "(STUBBED) service = {:#x}, npId = {}", service, npId->data);
    static s32 id = 1;
    return id++;
}

s32 PS4_SYSV_ABI sceNpTusCreateRequest(s32 titleCtxId) {
    LOG_ERROR(Lib_NpTus, "(STUBBED) titleCtxId = {}", titleCtxId);
    static s32 id = 0;
    return id++;
}

s32 PS4_SYSV_ABI sceNpTusDeleteRequest(s32 reqId) {
    LOG_ERROR(Lib_NpTus, "(STUBBED) reqId = {}", reqId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpTssGetSmallStorage(s32 reqId, void* data, u64 maxSize, u64* size, void* option) {
    LOG_ERROR(Lib_NpTus, "(STUBBED) reqId = {} maxSize = {}", reqId, maxSize);
    *size = 1;
    ((char*)data)[0] = '\0';
    return ORBIS_OK;
}

using OrbisNpTssSlotId = s32;

struct OrbisNpTssDataStatus {
    u64 tick;
    s32 status;
    u64 contentLength;
};

s32 PS4_SYSV_ABI sceNpTssGetData(s32 reqId, OrbisNpTssSlotId slot, OrbisNpTssDataStatus* status, u64 statusSize, void* data, u64 recvSize, void* param) {
    LOG_ERROR(Lib_NpTus, "(STUBBED) reqId = {} slot = {}", reqId, slot);

    if (status == nullptr) {
        return 0x8055070c;
    }

    status->status = 0;
    status->contentLength = 0;

    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("sRVb2Cf0GHg", "libSceNpTus", 1, "libSceNpTus", sceNpTssCreateNpTitleCtx);
    LIB_FUNCTION("lBtrk+7lk14", "libSceNpTus", 1, "libSceNpTus", sceNpTssCreateNpTitleCtxA);
    LIB_FUNCTION("BIkMmUfNKWM", "libSceNpTus", 1, "libSceNpTus", sceNpTusCreateNpTitleCtx);
    LIB_FUNCTION("3bh2aBvvmvM", "libSceNpTus", 1, "libSceNpTus", sceNpTusCreateRequest);
    LIB_FUNCTION("CcIH40dYS88", "libSceNpTus", 1, "libSceNpTus", sceNpTusDeleteRequest);
    LIB_FUNCTION("lL+Z3zCKNTs", "libSceNpTus", 1, "libSceNpTus", sceNpTssGetSmallStorage);
    LIB_FUNCTION("-SUR+UoLS6c", "libSceNpTus", 1, "libSceNpTus", sceNpTssGetData);
};

} // namespace Libraries::Np::NpTus