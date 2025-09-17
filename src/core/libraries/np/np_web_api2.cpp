// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_web_api2.h"

namespace Libraries::Np::NpWebApi2 {

s32 PS4_SYSV_ABI sceNpWebApi2CreateRequest(s32 userCtxId, const char* api, const char* path, char* unk, void* param, s64* requestId) {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) userCtxId = {} api = {} path = {} unk = {}", userCtxId, api, path, unk);
    static s64 id = 1;
    *requestId = id++;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2AddHttpRequestHeader(s32 reqId, const char* field, const char* val) {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) reqId = {} field = {}, val = {}", reqId, field, val);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2SendRequest(s32 reqId, const void* data, u64 size, void* result) {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) reqId = {} size = {}", reqId, size);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2Initialize(s32 libHttpCtxId, u64 poolSize) {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) libHttpCtxId = {} poolSize = {}", libHttpCtxId, poolSize);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2CreateUserContext() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED)");
    static s32 id = 1;
    return id++;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("sk54bi6FtYM", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2CreateUserContext);
    LIB_FUNCTION("3EI-OSJ65Xc", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2CreateRequest);
    LIB_FUNCTION("egOOvrnF6mI", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2AddHttpRequestHeader);
    LIB_FUNCTION("lQOCF84lvzw", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2SendRequest);
    LIB_FUNCTION("+o9816YQhqQ", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2Initialize);
};

} // namespace Libraries::Np::NpWebApi2