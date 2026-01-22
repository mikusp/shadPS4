// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_matching2.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpMatching2 {

constexpr int ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED = 0x80550c01;
constexpr int ORBIS_NP_MATCHING2_ERROR_ALREADY_INITIALIZED = 0x80550c02;
constexpr int ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT = 0x80550c15;

static bool g_initialized = false;
static OrbisNpMatching2ContextId contextId = 1;

struct OrbisNpMatching2CreateContextParameter {
    Libraries::Np::OrbisNpId npId;
    void* npCommunicationId;
    void* npCommunicationPassphrase;
    Libraries::Np::OrbisNpServiceLabel serviceLabel;
    u64 size;
};

int PS4_SYSV_ABI sceNpMatching2CreateContext(const OrbisNpMatching2CreateContextParameter* param,
                                             OrbisNpMatching2ContextId* ctxId) {
    LOG_DEBUG(Lib_NpMatching2, "called");

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!ctxId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    *ctxId = contextId++;

    return ORBIS_OK;
}

struct OrbisNpMatching2CreateContextParameterA {
    Libraries::UserService::OrbisUserServiceUserId userId;
    Libraries::Np::OrbisNpServiceLabel serviceLabel;
    u64 size;
};

int PS4_SYSV_ABI sceNpMatching2CreateContextA(const OrbisNpMatching2CreateContextParameterA* param,
                                              OrbisNpMatching2ContextId* ctxId) {
    LOG_DEBUG(Lib_NpMatching2, "called");

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!ctxId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    *ctxId = contextId++;

    return ORBIS_OK;
}

using OrbisNpMatching2ContextCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId,
                                                              OrbisNpMatching2Event event,
                                                              OrbisNpMatching2EventCause cause,
                                                              int errorCode, void* userdata);

struct NpMatching2ContextCallback {
    OrbisNpMatching2ContextCallback callback;
    void* userdata;
};

NpMatching2ContextCallback npMatching2ContextCallback{nullptr, nullptr};

const OrbisNpMatching2Event ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED = 0x6F02;
const OrbisNpMatching2EventCause ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION = 11;

int PS4_SYSV_ABI sceNpMatching2RegisterContextCallback(OrbisNpMatching2ContextCallback callback,
                                                       void* userdata) {
    LOG_DEBUG(Lib_NpMatching2, "called, userdata = {}", userdata);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    npMatching2ContextCallback.callback = callback;
    npMatching2ContextCallback.userdata = userdata;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2ContextStart(OrbisNpMatching2ContextId ctxId, u64 timeout) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, timeout = {}", ctxId, timeout);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    if (npMatching2ContextCallback.callback) {
        (npMatching2ContextCallback.callback)(ctxId, ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED,
                                              ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION, 0,
                                              npMatching2ContextCallback.userdata);
    }

    return ORBIS_OK;
}

struct OrbisNpMatching2InitializeParameter {
    u64 poolSize;
    //
};

int PS4_SYSV_ABI sceNpMatching2Initialize(void* param) {
    LOG_DEBUG(Lib_NpMatching2, "called");

    if (g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_ALREADY_INITIALIZED;
    }

    g_initialized = true;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SetDefaultRequestOptParam(OrbisNpMatching2ContextId ctxId,
                                                         void* requestOpt) {
    LOG_DEBUG(Lib_NpMatching2, "called");

    if (g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_ALREADY_INITIALIZED;
    }
    if (!requestOpt) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    //
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("10t3e5+JPnU", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2Initialize);
    LIB_FUNCTION("YfmpW719rMo", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2CreateContext);
    LIB_FUNCTION("ajvzc8e2upo", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2CreateContextA);
    LIB_FUNCTION("fQQfP87I7hs", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterContextCallback);
    LIB_FUNCTION("7vjNQ6Z1op0", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2ContextStart);
    LIB_FUNCTION("+8e7wXLmjds", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetDefaultRequestOptParam);
};

} // namespace Libraries::Np::NpMatching2