// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <functional>
#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_matching2.h"
#include "core/tls.h"

namespace Libraries::Np::NpMatching2 {

static bool g_initialized = false;
static std::optional<std::function<std::remove_pointer_t<OrbisNpMatching2ContextCallback_>>> callback = {};

s32 PS4_SYSV_ABI sceNpMatching2Initialize() {
    LOG_DEBUG(Lib_NpMatching2, "called");
    if (g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_ALREADY_INITIALIZED;
    }

    g_initialized = true;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpMatching2RegisterContextCallback(OrbisNpMatching2ContextCallback cb, void* arg) {
    LOG_ERROR(Lib_NpMatching2, "(DUMMY) called");
    
    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    using namespace std::placeholders;

    callback = std::bind(cb, _1, _2, _3, _4, arg);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpMatching2CreateContext(const void* param, OrbisNpMatching2ContextId* ctxId) {
    LOG_ERROR(Lib_NpMatching2, "(DUMMY) called");

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!ctxId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    static OrbisNpMatching2ContextId id = 1;
    *ctxId = id++;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpMatching2CreateContextA(const void* param, OrbisNpMatching2ContextId* ctxId) {
    LOG_ERROR(Lib_NpMatching2, "(DUMMY) called");

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!ctxId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    static OrbisNpMatching2ContextId id = 1;
    *ctxId = id++;

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpMatching2ContextStart(OrbisNpMatching2ContextId ctxId, u64 timeout) {
    LOG_ERROR(Lib_NpMatching2, "(DUMMY) called ctxId = {} timeout = {}", ctxId, timeout);
    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    if (callback) {
        LOG_ERROR(Lib_NpMatching2, "reporting CONTEXT_STARTED");
        (*callback)(ctxId, 0x6F02, 11, 0);
    }
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("10t3e5+JPnU", "libSceNpMatching2", 1, "libSceNpMatching2", sceNpMatching2Initialize);
    LIB_FUNCTION("fQQfP87I7hs", "libSceNpMatching2", 1, "libSceNpMatching2", sceNpMatching2RegisterContextCallback);
    LIB_FUNCTION("YfmpW719rMo", "libSceNpMatching2", 1, "libSceNpMatching2", sceNpMatching2CreateContext);
    LIB_FUNCTION("ajvzc8e2upo", "libSceNpMatching2", 1, "libSceNpMatching2", sceNpMatching2CreateContextA);
    // LIB_FUNCTION("+8e7wXLmjds", "libSceNpMatching2", 1, "libSceNpMatching2", sceNpMatching2SetDefaultRequestOptParam);
    LIB_FUNCTION("7vjNQ6Z1op0", "libSceNpMatching2", 1, "libSceNpMatching2", sceNpMatching2ContextStart);
};

} // namespace Libraries::Np::NpMatching2
