// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <deque>
#include <mutex>

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_manager.h"
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

struct NpMatching2InternalEvent {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2Event event;
    OrbisNpMatching2EventCause cause;
    int errorCode;
};

static std::mutex g_events_mutex;
static std::deque<NpMatching2InternalEvent> g_events;
static std::mutex g_responses_mutex;
static std::deque<std::function<void()>> g_responses;

using OrbisNpMatching2ContextCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId,
                                                              OrbisNpMatching2Event event,
                                                              OrbisNpMatching2EventCause cause,
                                                              int errorCode, void* userdata);
using OrbisNpMatching2RoomEventCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId,
                                                                OrbisNpMatching2RoomId roomId,
                                                                OrbisNpMatching2Event event,
                                                                void* data, void* userdata);
using OrbisNpMatching2SignalingCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId,
                                                                OrbisNpMatching2RoomId roomId,
                                                                OrbisNpMatching2RoomMemberId roomMemberId,
                                                                OrbisNpMatching2Event event,
                                                                int errorCode, void* userdata);

std::function<PS4_SYSV_ABI void(const NpMatching2InternalEvent&)> npMatching2ContextCallback =
    nullptr;

constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST = 0x0002;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED = 0x6F02;
constexpr OrbisNpMatching2EventCause ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION = 11;

int PS4_SYSV_ABI sceNpMatching2RegisterContextCallback(OrbisNpMatching2ContextCallback callback,
                                                       void* userdata) {
    LOG_DEBUG(Lib_NpMatching2, "called, userdata = {}", userdata);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    npMatching2ContextCallback = [callback, userdata](auto arg) {
        callback(arg.contextId, arg.event, arg.cause, arg.errorCode, userdata);
    };

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2RegisterRoomEventCallback(OrbisNpMatching2ContextId ctxId,
                                                         OrbisNpMatching2RoomEventCallback callback,
                                                         void* userdata) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    //
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2RegisterSignalingCallback(OrbisNpMatching2ContextId ctxId,
                                                         OrbisNpMatching2SignalingCallback callback,
                                                         void* userdata) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    //
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2ContextStart(OrbisNpMatching2ContextId ctxId, u64 timeout) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, timeout = {}", ctxId, timeout);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    std::scoped_lock lk{g_events_mutex};
    g_events.emplace_back(ctxId, ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED,
                          ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION, 0);

    return ORBIS_OK;
}

struct OrbisNpMatching2InitializeParameter {
    u64 poolSize;
    //
};

void ProcessEvents() {
    if (npMatching2ContextCallback) {
        LOG_DEBUG(Lib_NpMatching2, "processing context callbacks");

        std::scoped_lock lk{g_events_mutex};

        while (!g_events.empty()) {
            npMatching2ContextCallback(g_events.front());
            g_events.pop_front();
        }
    }

    std::scoped_lock lk{g_responses_mutex};
    while (!g_responses.empty()) {
        (g_responses.front())();
        g_responses.pop_front();
    }
}

int PS4_SYSV_ABI sceNpMatching2Initialize(void* param) {
    LOG_DEBUG(Lib_NpMatching2, "called");

    if (g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_ALREADY_INITIALIZED;
    }

    g_initialized = true;
    Libraries::Np::NpManager::RegisterNpCallback("NpMatching2", ProcessEvents);

    return ORBIS_OK;
}

using OrbisNpMatching2RequestCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId,
                                                              OrbisNpMatching2RequestId,
                                                              OrbisNpMatching2Event, int, void*,
                                                              void*);

struct OrbisNpMatching2RequestOptParam {
    OrbisNpMatching2RequestCallback callback;
    void* arg;
    u32 timeout;
    u16 appId;
    u8 dummy[2];
};

static std::optional<OrbisNpMatching2RequestOptParam> defaultRequestOptParam = std::nullopt;

int PS4_SYSV_ABI sceNpMatching2SetDefaultRequestOptParam(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2RequestOptParam* requestOpt) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!requestOpt) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    defaultRequestOptParam = *requestOpt;

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2GetServerId(OrbisNpMatching2ContextId ctxId,
                                           OrbisNpMatching2ServerId* serverId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!serverId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    *serverId = 0xac;

    return ORBIS_OK;
}

struct OrbisNpMatching2GetWorldInfoListRequest {
    OrbisNpMatching2ServerId serverId;
};

struct OrbisNpMatching2World {
    OrbisNpMatching2World* next;
    OrbisNpMatching2WorldId worldId;
    u32 lobbiesNum;
    u32 maxLobbyMembersNum;
    u32 lobbyMembersNum;
    u32 roomsNum;
    u32 roomMembersNum;
    u8 pad[3];
};

struct OrbisNpMatching2GetWorldInfoListResponse {
    OrbisNpMatching2World* world;
    u64 worldNum;
};

int PS4_SYSV_ABI sceNpMatching2GetWorldInfoList(OrbisNpMatching2ContextId ctxId,
                                                OrbisNpMatching2GetWorldInfoListRequest* request,
                                                OrbisNpMatching2RequestOptParam* requestOpt,
                                                OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, request.serverId = {}, requestOpt = {}", ctxId,
              request ? request->serverId : 0xFFFF, (void*)requestOpt);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    static OrbisNpMatching2RequestId id = 1;
    *requestId = id++;

    auto optParam = requestOpt
                        ? requestOpt
                        : (defaultRequestOptParam ? &defaultRequestOptParam.value() : nullptr);

    if (optParam) {
        LOG_DEBUG(Lib_NpMatching2, "optParam.timeout = {}, optParam.appId = {}", optParam->timeout,
                  optParam->appId);
        std::scoped_lock lk{g_responses_mutex};
        g_responses.emplace_back([=]() {
            OrbisNpMatching2World w{nullptr, 1, 10, 0, 10, 0, {}};
            OrbisNpMatching2GetWorldInfoListResponse resp{&w, 1};
            optParam->callback(ctxId, *requestId,
                               ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST, 0, &resp,
                               optParam->arg);
        });
    }

    return ORBIS_OK;
}

struct OrbisNpMatching2RangeFilter {
    u32 start;
    u32 max;
};

struct OrbisNpMatching2SearchRoomRequest {
    int option;
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2RangeFilter rangeFilter;
    OrbisNpMatching2Flags flags1;
    OrbisNpMatching2Flags flags2;
    void* intFilter;
    u64 intFilters;
    void* binFilter;
    u64 binFilters;
    OrbisNpMatching2AttributeId* attr;
    u64 attrs;
};

int PS4_SYSV_ABI sceNpMatching2SearchRoom(OrbisNpMatching2ContextId ctxId, OrbisNpMatching2SearchRoomRequest* request, OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, (void*)requestOpt);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    static OrbisNpMatching2RequestId id = 1;
    *requestId = id++;

    auto optParam = requestOpt
                        ? requestOpt
                        : (defaultRequestOptParam ? &defaultRequestOptParam.value() : nullptr);

    if (optParam) {
        LOG_DEBUG(Lib_NpMatching2, "optParam.timeout = {}, optParam.appId = {}", optParam->timeout,
                  optParam->appId);
        std::scoped_lock lk{g_responses_mutex};
        g_responses.emplace_back([=]() {
            //
        });
    }

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
    LIB_FUNCTION("p+2EnxmaAMM", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterRoomEventCallback);
    LIB_FUNCTION("0UMeWRGnZKA", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterSignalingCallback);
    LIB_FUNCTION("7vjNQ6Z1op0", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2ContextStart);
    LIB_FUNCTION("LhCPctIICxQ", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetServerId);
    LIB_FUNCTION("rJNPJqDCpiI", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetWorldInfoList);
    LIB_FUNCTION("+8e7wXLmjds", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetDefaultRequestOptParam);
    LIB_FUNCTION("VqZX7POg2Mk", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SearchRoom);
};

} // namespace Libraries::Np::NpMatching2