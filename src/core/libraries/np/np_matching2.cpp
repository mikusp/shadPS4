// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <arpa/inet.h>
#include <deque>
#include <future>
#include <mutex>

#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/net.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_matching2.h"
#include "core/libraries/np/np_matching2_requests.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/np/object_manager.h"
#include "core/libraries/system/userservice.h"
#include "core/shadnet/shadnet.h"
#include "cppcodec/base64_rfc4648.hpp"

#include "magic_enum/magic_enum.hpp"

namespace Libraries::Np::NpMatching2 {

using base64 = cppcodec::base64_rfc4648;
using MatchingContext = Core::ShadNet::MatchingContext;

static bool g_initialized = false;
static OrbisNpMatching2ContextId contextId = 1;

struct NpMatching2LobbyEvent {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2Event event;
    void* data;
};

struct NpMatching2RoomEvent {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2Event event;
    void* data;
};

struct NpMatching2RoomMessage {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2RoomMemberId roomMemberId;
    OrbisNpMatching2Event event;
    void* data;
};

struct NpMatching2SignalingEvent {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2RoomMemberId roomMemberId;
    OrbisNpMatching2Event event;
    int errorCode;
};

static std::mutex g_events_mutex;
// static std::deque<NpMatching2ContextEvent> g_ctx_events;
static std::deque<NpMatching2LobbyEvent> g_lobby_events;
static std::deque<NpMatching2RoomEvent> g_room_events;
static std::deque<NpMatching2RoomMessage> g_room_messages;
static std::deque<NpMatching2SignalingEvent> g_signaling_events;
static std::mutex g_responses_mutex;
static std::deque<std::function<void()>> g_responses;

struct OrbisNpMatching2CreateContextParameter {
    Libraries::Np::OrbisNpId* npId;
    void* npCommunicationId;
    void* npPassphrase;
    Libraries::Np::OrbisNpServiceLabel serviceLabel;
    u64 size;
};

static_assert(sizeof(OrbisNpMatching2CreateContextParameter) == 0x28);

using NpMatching2ContextManager =
    ObjectManager<MatchingContext, 1, ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID,
                  ORBIS_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND, ORBIS_NP_MATCHING2_ERROR_CONTEXT_MAX>;

static NpMatching2ContextManager ctxManager;

int PS4_SYSV_ABI sceNpMatching2CreateContext(const OrbisNpMatching2CreateContextParameter* param,
                                             OrbisNpMatching2ContextId* ctxId) {
    LOG_DEBUG(Lib_NpMatching2, "called, npId = {}, serviceLabel = {}, size = {}",
              param->npId->handle.data, param->serviceLabel, param->size);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!param || param->size != 0x28 || !ctxId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto id = ctxManager.CreateObject();
    if (id < 0) {
        return id;
    }

    *ctxId = id;

    return ORBIS_OK;
}

struct OrbisNpMatching2CreateContextParameterA {
    Libraries::UserService::OrbisUserServiceUserId userId;
    Libraries::Np::OrbisNpServiceLabel serviceLabel;
    u64 size;
};

static_assert(sizeof(OrbisNpMatching2CreateContextParameterA) == 16);

int PS4_SYSV_ABI sceNpMatching2CreateContextA(const OrbisNpMatching2CreateContextParameterA* param,
                                              OrbisNpMatching2ContextId* ctxId) {
    LOG_DEBUG(Lib_NpMatching2, "called, userId = {}, serviceLabel = {}, size = {}", param->userId,
              param->serviceLabel, param->size);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!param || param->size != 0x10 || !ctxId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto id = ctxManager.CreateObject();
    if (id < 0) {
        return id;
    }

    *ctxId = id;

    return ORBIS_OK;
}

static std::optional<OrbisNpMatching2RequestOptParam> defaultRequestOptParam = std::nullopt;

auto GetOptParam(OrbisNpMatching2RequestOptParam* requestOpt) {
    return requestOpt ? *requestOpt
                      : (defaultRequestOptParam ? defaultRequestOptParam
                                                : std::optional<OrbisNpMatching2RequestOptParam>{});
}

// struct OrbisNpMatching2RoomMemberDataInternal {
//     OrbisNpMatching2RoomMemberDataInternal* next;
//     u64 joinDateTicks;
//     Libraries::Np::OrbisNpPeerAddress user;
//     Libraries::Np::OrbisNpOnlineId onlineId;
//     u8 pad[4];
//     OrbisNpMatching2RoomMemberId memberId;
//     OrbisNpMatching2TeamId teamId;
//     OrbisNpMatching2NatType natType;
//     OrbisNpMatching2Flags flags;
//     void* roomGroup;
//     OrbisNpMatching2BinAttr* roomMemberInternalBinAttr;
//     u64 roomMemberInternalBinAttrs;
// };

constexpr Libraries::Np::OrbisNpOnlineId g_onlineId = {"shadps4", 0, {}};
Libraries::Np::OrbisNpId g_npId = {g_onlineId};
template <bool A>
int PS4_SYSV_ABI sceNpMatching2CreateJoinRoomA(OrbisNpMatching2ContextId ctxId,
                                               OrbisNpMatching2CreateJoinRoomRequest* request,
                                               OrbisNpMatching2RequestOptParam* requestOpt,
                                               OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "called{}, ctxId = {}, requestOpt = {}", A ? " A" : "", ctxId,
              fmt::ptr(requestOpt));

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    MatchingContext* ctx = nullptr;
    if (auto ret = ctxManager.GetObject(ctxId, &ctx); ret < 0) {
        return ret;
    }

    if (auto ret = request->Validate(); ret < 0) {
        return ret;
    }

    auto id = ctx->CreateJoinRoom(*request, requestOpt);

    *requestId = id;

    return ORBIS_OK;
    // auto signaling = request->signalingParam;
    // LOG_DEBUG(Lib_NpMatching2, "signaling {}, type = {}, flag = {}, mainMember = {}",
    //           fmt::ptr(signaling), signaling ? signaling->type : -1,
    //           signaling ? signaling->flag : -1, signaling ? signaling->mainMember : -1);

    // LOG_DEBUG(Lib_NpMatching2,
    //           "maxSlot = {}, teamId = {}, worldId = {}, lobbyId = {}",
    //           request->maxSlot, request->teamId, request->worldId, request->lobbyId);
    // LOG_DEBUG(Lib_NpMatching2, "roomPasswd = {}, groupConfig = {}, joinGroupLabel = {}", fmt::ptr(request->roomPasswd), fmt::ptr(request->groupConfig), fmt::ptr(request->joinGroupLabel));
    // LOG_DEBUG(Lib_NpMatching2, "internalBinAttrs = {}, externalSearchIntAttrs = {}, externalSearchBinAttrs = {}", request->internalBinAttrs, request->externalSearchIntAttrs, request->externalSearchBinAttrs);
    // LOG_DEBUG(Lib_NpMatching2, "externalBinAttrs = {}, memberInternalBinAttrs = {}", request->externalBinAttrs, request->memberInternalBinAttrs);

    // for (int i = 0; i < request->internalBinAttrs; ++i) {
    //     LOG_DEBUG(Lib_NpMatching2, "internalBinAttr[{}] = [id = {:#x}, data = {}]", i, request->internalBinAttr[i].id, base64::encode(request->internalBinAttr[i].data, request->internalBinAttr[i].dataSize));
    // }
    // for (int i = 0; i < request->externalBinAttrs; ++i) {
    //     LOG_DEBUG(Lib_NpMatching2, "externalBinAttr[{}] = [id = {:#x}, data = {}]", i, request->externalBinAttr[i].id, base64::encode(request->externalBinAttr[i].data, request->externalBinAttr[i].dataSize));
    // }
    // for (int i = 0; i < request->externalSearchBinAttrs; ++i) {
    //     LOG_DEBUG(Lib_NpMatching2, "externalSearchBinAttr[{}] = [id = {:#x}, data = {}]", i, request->externalSearchBinAttr[i].id, base64::encode(request->externalSearchBinAttr[i].data, request->externalSearchBinAttr[i].dataSize));
    // }
    // for (int i = 0; i < request->externalSearchIntAttrs; ++i) {
    //     LOG_DEBUG(Lib_NpMatching2, "externalSearchIntAttr[{}] = [id = {:#x}, attr = {}]", i, request->externalSearchIntAttr[i].id, request->externalSearchIntAttr[i].attr);
    // }
    // for (int i = 0; i < request->memberInternalBinAttrs; ++i) {
    //     LOG_DEBUG(Lib_NpMatching2, "memberInternalBinAttr[{}] = [id = {:#x}, data = {}]", i, request->memberInternalBinAttr[i].id, base64::encode(request->memberInternalBinAttr[i].data, request->memberInternalBinAttr[i].dataSize));
    // }

    // static OrbisNpMatching2RequestId id = 10;
    // *requestId = id++;

    // if (auto optParam = GetOptParam(requestOpt); optParam) {
    //     LOG_DEBUG(Lib_NpMatching2, "optParam.timeout = {}, optParam.appId = {}", optParam->timeout,
    //               optParam->appId);
    //     std::scoped_lock lk{g_responses_mutex};
    //     auto reqIdCopy = *requestId;
    //     auto requestCopy = *request;
    //     g_responses.emplace_back([=]() {


    //         OrbisNpMatching2RoomMemberDataInternal me{
    //             nullptr,
    //             0,
    //             {&g_npId, Libraries::Np::OrbisNpPlatformType::PS4},
    //             g_onlineId,
    //             {0, 0, 0, 0},
    //             0x1FE8,
    //             requestCopy.teamId,
    //             1,
    //             0,
    //             nullptr,
    //             nullptr,
    //             0};
    //         OrbisNpMatching2RoomDataInternal room{requestCopy.maxSlot,
    //                                               0,
    //                                               static_cast<u16>(requestCopy.maxSlot - 1u),
    //                                               0,
    //                                               15,
    //                                               0xac,
    //                                               requestCopy.worldId,
    //                                               requestCopy.lobbyId,
    //                                               0x123456,
    //                                               0,
    //                                               0,
    //                                               nullptr,
    //                                               0,
    //                                               0,
    //                                               {0, 0, 0, 0},
    //                                               nullptr,
    //                                               0};
    //         auto* a = reinterpret_cast<OrbisNpMatching2RoomMemberDataInternalA*>(&me);
    //         OrbisNpMatching2CreateJoinRoomResponseA resp{&room, {a, 1, a, a}};
    //         LOG_DEBUG(Lib_NpMatching2, "foo2 {}", fmt::ptr(&resp));
    //         optParam->callback(ctxId, reqIdCopy,
    //                            A ? ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM_A
    //                              : ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM,
    //                            0, &resp, optParam->arg);

    //         auto foo = std::async(std::launch::async, [=](){
    //             using namespace std::chrono_literals;
    //             std::this_thread::sleep_for(5s);
    //             LOG_ERROR(Lib_NpMatching2, "sending member joined");
    //             std::scoped_lock lk{g_events_mutex};
    //             OrbisNpOnlineId onlineId {"shadow", 0, {}};
    //             OrbisNpId npId {onlineId};
    //             OrbisNpMatching2RoomMemberDataInternal roomMemberData {
    //                 nullptr,
    //                 0,
    //                 {&npId, OrbisNpPlatformType::PS4, {}},
    //                 onlineId,
    //                 {},
    //                 0x6644,
    //                 requestCopy.teamId,
    //                 0,
    //                 0,
    //                 nullptr,
    //                 nullptr,
    //                 0
    //             };

    //             OrbisNpMatching2RoomMemberUpdateInfo info {
    //                 &roomMemberData
    //             };
    //             g_room_events.emplace_back(ctxId, 0x123456, ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED, &info);
    //             g_signaling_events.emplace_back(ctxId, 0x123456, 0x1FE8,
    //                                             ORBIS_NP_MATCHING2_SIGNALING_EVENT_ESTABLISHED, 0);
    //             return 0;
    //         });
    //         // std::scoped_lock lk{g_events_mutex};
    //         // g_signaling_events.emplace_back(ctxId, 0x123456, 0x1FE8,
    //         //                                 ORBIS_NP_MATCHING2_SIGNALING_EVENT_ESTABLISHED, 0);
    //     });
    // }
    // return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2RegisterContextCallback(OrbisNpMatching2ContextCallback callback,
                                                       void* userdata) {
    LOG_DEBUG(Lib_NpMatching2, "called, userdata = {}", userdata);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    MatchingContext::SetContextCallback(callback, userdata);

    return ORBIS_OK;
}

using OrbisNpMatching2LobbyEventCallback =
    PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId, OrbisNpMatching2LobbyId lobbyId,
                          OrbisNpMatching2Event event, void* data, void* userdata);

std::function<void(const NpMatching2LobbyEvent*)> npMatching2LobbyCallback = nullptr;

int PS4_SYSV_ABI sceNpMatching2RegisterLobbyEventCallback(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2LobbyEventCallback callback, void* userdata) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    npMatching2LobbyCallback = [callback, userdata](auto arg) {
        callback(arg->contextId, arg->lobbyId, arg->event, arg->data, userdata);
    };

    return ORBIS_OK;
}

using OrbisNpMatching2RoomEventCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId,
                                                                OrbisNpMatching2RoomId roomId,
                                                                OrbisNpMatching2Event event,
                                                                const void* data, void* userdata);

std::function<void(const NpMatching2RoomEvent*)> npMatching2RoomCallback = nullptr;

int PS4_SYSV_ABI sceNpMatching2RegisterRoomEventCallback(OrbisNpMatching2ContextId ctxId,
                                                         OrbisNpMatching2RoomEventCallback callback,
                                                         void* userdata) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    MatchingContext* ctx = nullptr;
    if (auto ret = ctxManager.GetObject(ctxId, &ctx); ret < 0) {
        return ret;
    }

    ctx->SetRoomCallback(callback, userdata);

    return ORBIS_OK;
}

using OrbisNpMatching2RoomMessageCallback =
    PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId, OrbisNpMatching2RoomId roomId,
                          OrbisNpMatching2RoomMemberId roomMemberId, OrbisNpMatching2Event event,
                          void* data, void* userdata);

std::function<void(const NpMatching2RoomMessage*)> npMatching2RoomMessageCallback = nullptr;

int PS4_SYSV_ABI sceNpMatching2RegisterRoomMessageCallback(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2RoomMessageCallback callback, void* userdata) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    npMatching2RoomMessageCallback = [callback, userdata](auto arg) {
        callback(arg->contextId, arg->roomId, arg->roomMemberId, arg->event, arg->data, userdata);
    };

    return ORBIS_OK;
}

// std::function<void(const NpMatching2SignalingEvent*)> npMatching2SignalingCallback = nullptr;

int PS4_SYSV_ABI sceNpMatching2RegisterSignalingCallback(OrbisNpMatching2ContextId ctxId,
                                                         OrbisNpMatching2SignalingCallback callback,
                                                         void* userdata) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    MatchingContext* ctx = nullptr;
    if (auto ret = ctxManager.GetObject(ctxId, &ctx); ret < 0) {
        return ret;
    }

    ctx->SetSignalingCallback(callback, userdata);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2ContextStart(OrbisNpMatching2ContextId ctxId, u64 timeout) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, timeout = {}", ctxId, timeout);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    MatchingContext* ctx = nullptr;
    if (auto ret = ctxManager.GetObject(ctxId, &ctx); ret < 0) {
        return ret;
    }

    return ctx->Start(ctxId, timeout);

    // std::scoped_lock lk{g_events_mutex};
    
    // if (Config::getIsConnectedToNetwork() && Config::getPSNSignedIn()) {
    //     g_ctx_events.emplace_back(ctxId, ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED,
    //                               ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION, 0);
    //     // if (npMatching2ContextCallback) {
    //     //         npMatching2ContextCallback(&g_ctx_events.front());
    //     // }
    // } else {
    //     // error confirmed with a real console disconnected from the internet
    //     constexpr int ORBIS_NET_ERROR_RESOLVER_ETIMEDOUT = 0x804101e2;
    //     g_ctx_events.emplace_back(ctxId, ORBIS_NP_MATCHING2_CONTEXT_EVENT_START_OVER,
    //                               ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ERROR,
    //                               ORBIS_NET_ERROR_RESOLVER_ETIMEDOUT);
    // }
}

int PS4_SYSV_ABI sceNpMatching2ContextStop(OrbisNpMatching2ContextId ctxId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    std::scoped_lock lk{g_events_mutex};
    // g_ctx_events.emplace_back(ctxId, ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED,
    //                           ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION, 0);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2ContextDestroy(OrbisNpMatching2ContextId ctxId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    return ORBIS_OK;
}

void ProcessEvents() {
    {
        std::scoped_lock lk{g_events_mutex};

        // if (npMatching2ContextCallback) {
        //     while (!g_ctx_events.empty()) {
        //         npMatching2ContextCallback(&g_ctx_events.front());
        //         g_ctx_events.pop_front();
        //     }
        // }
        if (npMatching2LobbyCallback) {
            while (!g_lobby_events.empty()) {
                npMatching2LobbyCallback(&g_lobby_events.front());
                g_lobby_events.pop_front();
            }
        }
        if (npMatching2RoomCallback) {
            while (!g_room_events.empty()) {
                npMatching2RoomCallback(&g_room_events.front());
                g_room_events.pop_front();
            }
        }
        if (npMatching2RoomMessageCallback) {
            while (!g_room_messages.empty()) {
                LOG_INFO(Lib_NpMatching2, "calling room message");
                npMatching2RoomMessageCallback(&g_room_messages.front());
                g_room_messages.pop_front();
            }
        }
        // if (npMatching2SignalingCallback) {
        //     while (!g_signaling_events.empty()) {
        //         npMatching2SignalingCallback(&g_signaling_events.front());
        //         g_signaling_events.pop_front();
        //     }
        // }
    }

    std::scoped_lock lk{g_responses_mutex};
    while (!g_responses.empty()) {
        (g_responses.front())();
        g_responses.pop_front();
    }
}

struct OrbisNpMatching2InitializeParameter {
    u64 poolSize;
    //
};

int PS4_SYSV_ABI sceNpMatching2Initialize(OrbisNpMatching2InitializeParameter* param) {
    LOG_DEBUG(Lib_NpMatching2, "called");

    if (g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_ALREADY_INITIALIZED;
    }

    g_initialized = true;
    Libraries::Np::NpManager::RegisterNpCallback("NpMatching2", ProcessEvents);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2Terminate() {
    LOG_DEBUG(Lib_NpMatching2, "called");

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    g_initialized = false;
    Libraries::Np::NpManager::DeregisterNpCallback("NpMatching2");

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SetDefaultRequestOptParam(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2RequestOptParam* requestOpt) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!requestOpt) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    MatchingContext* ctx = nullptr;
    if (auto ret = ctxManager.GetObject(ctxId, &ctx); ret < 0) {
        return ret;
    }

    ctx->SetDefaultRequestOptParam(*requestOpt);

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
    const OrbisNpMatching2World* world;
    u64 worldNum;
};

int PS4_SYSV_ABI sceNpMatching2GetWorldInfoList(OrbisNpMatching2ContextId ctxId,
                                                OrbisNpMatching2GetWorldInfoListRequest* request,
                                                OrbisNpMatching2RequestOptParam* requestOpt,
                                                OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, request.serverId = {}, requestOpt = {}", ctxId,
              request ? request->serverId : 0xFFFF, fmt::ptr(requestOpt));

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    static OrbisNpMatching2RequestId id = 8866;
    *requestId = id++;

    if (auto optParam = GetOptParam(requestOpt); optParam) {
        LOG_DEBUG(Lib_NpMatching2, "optParam.timeout = {}, optParam.appId = {}", optParam->timeout,
                  optParam->appId);
        auto reqIdCopy = *requestId;
        std::scoped_lock lk{g_responses_mutex};
        g_responses.emplace_back([=]() {
            OrbisNpMatching2World w{nullptr, 51966, 0, 0, 0, 1, 1, {}};
            OrbisNpMatching2GetWorldInfoListResponse resp{&w, 1};
            LOG_DEBUG(Lib_NpMatching2, "foo {}", fmt::ptr(&resp));
            optParam->callback(ctxId, reqIdCopy,
                               ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST, 0,
                               reinterpret_cast<const void*>(&resp), optParam->arg);
        });
    }

    return ORBIS_OK;
}

// struct OrbisNpMatching2RoomMemberDataInternalList {
//     OrbisNpMatching2RoomMemberDataInternal* members;
//     u64 membersNum;
//     OrbisNpMatching2RoomMemberDataInternal* me;
//     OrbisNpMatching2RoomMemberDataInternal* owner;
// };

struct OrbisNpMatching2JoinRoomResponse {
    OrbisNpMatching2RoomDataInternal* roomData;
    OrbisNpMatching2RoomMemberDataInternalList members;
};

int PS4_SYSV_ABI sceNpMatching2JoinRoom(OrbisNpMatching2ContextId ctxId,
                                        OrbisNpMatching2JoinRoomRequest* request,
                                        OrbisNpMatching2RequestOptParam* requestOpt,
                                        OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    MatchingContext* ctx = nullptr;
    if (auto ret = ctxManager.GetObject(ctxId, &ctx); ret < 0) {
        return ret;
    }

    if (auto ret = request->Validate(); ret < 0) {
        return ret;
    }

    auto id = ctx->JoinRoom(*request, requestOpt);

    *requestId = id;

    return ORBIS_OK;
}

struct OrbisNpMatching2LeaveRoomRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2PresenceOptionData optData;
};

int PS4_SYSV_ABI sceNpMatching2LeaveRoom(OrbisNpMatching2ContextId ctxId,
                                         OrbisNpMatching2LeaveRoomRequest* request,
                                         OrbisNpMatching2RequestOptParam* requestOpt,
                                         OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    static OrbisNpMatching2RequestId id = 500;
    *requestId = id++;

    if (auto optParam = GetOptParam(requestOpt); optParam) {
        LOG_DEBUG(Lib_NpMatching2, "optParam.timeout = {}, optParam.appId = {}", optParam->timeout,
                  optParam->appId);
        std::scoped_lock lk{g_responses_mutex};
        auto reqIdCopy = *requestId;
        g_responses.emplace_back([=]() {
            optParam->callback(ctxId, reqIdCopy, ORBIS_NP_MATCHING2_REQUEST_EVENT_LEAVE_ROOM, 0,
                               nullptr, optParam->arg);
        });
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SearchRoom(OrbisNpMatching2ContextId ctxId,
                                          OrbisNpMatching2SearchRoomRequest* request,
                                          OrbisNpMatching2RequestOptParam* requestOpt,
                                          OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    MatchingContext* ctx = nullptr;
    if (auto ret = ctxManager.GetObject(ctxId, &ctx); ret < 0) {
        return ret;
    }

    if (auto ret = request->Validate(); ret < 0) {
        return ret;
    }

    auto id = ctx->SearchRoom(*request, requestOpt);

    *requestId = id;

    return ORBIS_OK;
}

struct OrbisNpMatching2SetUserInfoRequest {
    OrbisNpMatching2ServerId serverId;
    u8 padding[6];
    OrbisNpMatching2BinAttr* userBinAttr;
    u64 userBinAttrs;
};

int PS4_SYSV_ABI sceNpMatching2SetUserInfo(OrbisNpMatching2ContextId ctxId,
                                           OrbisNpMatching2SetUserInfoRequest* request,
                                           OrbisNpMatching2RequestOptParam* requestOpt,
                                           OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    static OrbisNpMatching2RequestId id = 100;
    *requestId = id++;

    if (auto optParam = GetOptParam(requestOpt); optParam) {
        LOG_DEBUG(Lib_NpMatching2, "optParam.timeout = {}, optParam.appId = {}", optParam->timeout,
                  optParam->appId);
        std::scoped_lock lk{g_responses_mutex};
        auto reqIdCopy = *requestId;
        g_responses.emplace_back([=]() {
            optParam->callback(ctxId, reqIdCopy, ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_USER_INFO, 0,
                               nullptr, optParam->arg);
        });
    }

    return ORBIS_OK;
}

enum class OrbisNpMatching2Cast : u8 {
    Broadcast = 1,
    Unicast = 2,
    Multicast = 3,
    Team = 4,
};

union OrbisNpMatching2Addressee {
    OrbisNpMatching2RoomMemberId unicast;
    struct {
        OrbisNpMatching2RoomMemberId* members;
        u64 len;
    } multicast;
    OrbisNpMatching2TeamId teamId;
};

struct OrbisNpMatching2SendRoomMessageRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2Cast cast;
    u8 pad[3];
    int unk;
    OrbisNpMatching2Addressee addressee;
    void* message;
    u64 messageLen;
};

int PS4_SYSV_ABI sceNpMatching2SendRoomMessage(OrbisNpMatching2ContextId ctxId,
                                               OrbisNpMatching2SendRoomMessageRequest* request,
                                               OrbisNpMatching2RequestOptParam* requestOpt,
                                               OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto cast = std::to_underlying(request->cast);
    LOG_DEBUG(Lib_NpMatching2, "roomId = {}, cast = {}, addressee = {}", request->roomId,
              magic_enum::enum_name(request->cast),
              cast == 0 ? "broadcast"
                        : (cast == 1 ? std::to_string(request->addressee.unicast)
                                     : (cast == 2 ? "multicast"
                                                  : std::to_string(request->addressee.teamId))));

    static OrbisNpMatching2RequestId id = 1000;
    *requestId = id++;
    auto reqIdCopy = *requestId;

    if (auto optParam = GetOptParam(requestOpt); optParam) {
        LOG_DEBUG(Lib_NpMatching2, "optParam.timeout = {}, optParam.appId = {}", optParam->timeout,
                  optParam->appId);
        std::scoped_lock lk{g_responses_mutex};
        g_responses.emplace_back([=]() {
            optParam->callback(ctxId, reqIdCopy, ORBIS_NP_MATCHING2_REQUEST_EVENT_SEND_ROOM_MESSAGE,
                               0, nullptr, optParam->arg);
        });
    }
    if (request->cast == OrbisNpMatching2Cast::Broadcast) {
        std::scoped_lock lk{g_events_mutex};
        g_room_messages.emplace_back(ctxId, request->roomId, 0x1FE8,
                                     ORBIS_NP_MATCHING2_ROOM_MSG_EVENT_MESSAGE_A, request->message);
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SetRoomDataExternal(OrbisNpMatching2ContextId ctxId, void* request,
                                                   OrbisNpMatching2RequestOptParam* requestOpt,
                                                   OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    static OrbisNpMatching2RequestId id = 800;
    *requestId = id++;

    if (auto optParam = GetOptParam(requestOpt); optParam) {
        LOG_DEBUG(Lib_NpMatching2, "optParam.timeout = {}, optParam.appId = {}", optParam->timeout,
                  optParam->appId);
        std::scoped_lock lk{g_responses_mutex};
        auto reqIdCopy = *requestId;
        g_responses.emplace_back([=]() {
            optParam->callback(ctxId, reqIdCopy,
                               ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_EXTERNAL, 0, nullptr,
                               optParam->arg);
        });
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SetRoomDataInternal(OrbisNpMatching2ContextId ctxId, void* request,
                                                   OrbisNpMatching2RequestOptParam* requestOpt,
                                                   OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    static OrbisNpMatching2RequestId id = 200;
    *requestId = id++;

    if (auto optParam = GetOptParam(requestOpt); optParam) {
        LOG_DEBUG(Lib_NpMatching2, "optParam.timeout = {}, optParam.appId = {}", optParam->timeout,
                  optParam->appId);
        std::scoped_lock lk{g_responses_mutex};
        auto reqIdCopy = *requestId;
        g_responses.emplace_back([=]() {
            optParam->callback(ctxId, reqIdCopy,
                               ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_INTERNAL, 0, nullptr,
                               optParam->arg);
        });
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SignalingGetConnectionStatus(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2RoomId roomId,
    OrbisNpMatching2RoomMemberId roomMemberId, int* connectionStatus, Libraries::Net::OrbisNetInAddr* addr, u16* port) {
    LOG_ERROR(Lib_NpMatching2, "(STUBBED) ctxId = {}, roomId = {}, roomMemberId = {}", ctxId, roomId, roomMemberId);

    if (connectionStatus) {
        *connectionStatus = 1;
        // if (addr) {
        //     addr->inaddr_addr = inet_addr("10.0.0.66");
        // }
        // if (port) {
        //     *port = 6666;
        // }
    }

    return ORBIS_OK;
}

union OrbisNpMatching2SignalingConnectionInfo {
    u32 ping;
    u32 bps;
    //
};

int PS4_SYSV_ABI sceNpMatching2SignalingGetConnectionInfo(OrbisNpMatching2ContextId ctxId, OrbisNpMatching2RoomId roomId,
    OrbisNpMatching2RoomMemberId roomMemberId, int info, OrbisNpMatching2SignalingConnectionInfo* connInfo) {
    LOG_ERROR(Lib_NpMatching2, "(STUBBED) ctxId = {}, roomId = {}, roomMemberId = {}, info = {}", ctxId, roomId, roomMemberId, info);

    if (connInfo) {
        if (info == 1) {
            connInfo->ping = 2500;
        }
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNpMatching2SignalingGetPingInfo(OrbisNpMatching2ContextId ctxId, OrbisNpMatching2SignalingGetPingInfoRequest* request,
                                                   OrbisNpMatching2RequestOptParam* requestOpt,
                                                   OrbisNpMatching2RequestId* requestId) {
    LOG_DEBUG(Lib_NpMatching2, "ctxId = {}, roomId = {}", ctxId, request ? request->roomId : -1);

    if (!g_initialized) {
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    MatchingContext* ctx = nullptr;
    if (auto ret = ctxManager.GetObject(ctxId, &ctx); ret < 0) {
        return ret;
    }

    if (auto ret = request->Validate(); ret < 0) {
        return ret;
    }

    // auto id = ctx->SignalingGetPingInfo(*request, requestOpt);

    // *requestId = id;

    static OrbisNpMatching2RequestId id = 27331;
    *requestId = id++;

    if (auto optParam = GetOptParam(requestOpt); optParam) {
        LOG_DEBUG(Lib_NpMatching2, "optParam.timeout = {}, optParam.appId = {}", optParam->timeout,
                  optParam->appId);
        std::scoped_lock lk{g_responses_mutex};
        auto reqIdCopy = *requestId;
        g_responses.emplace_back([=]() {
            OrbisNpMatching2SignalingGetPingInfoResponse data {
                .serverId = 1,
                .worldId = 51966,
                .roomId = request->roomId,
                .pingUs = 2500,
            };
            optParam->callback(ctxId, reqIdCopy,
                               ORBIS_NP_MATCHING2_REQUEST_EVENT_SIGNALING_GET_PING_INFO, 0, &data,
                               optParam->arg);
        });
    }

    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("10t3e5+JPnU", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2Initialize);
    LIB_FUNCTION("Mqp3lJ+sjy4", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2Terminate);
    LIB_FUNCTION("YfmpW719rMo", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2CreateContext);
    LIB_FUNCTION("ajvzc8e2upo", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2CreateContextA);
    LIB_FUNCTION("zCWZmXXN600", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2CreateJoinRoomA<false>);
    LIB_FUNCTION("V6KSpKv9XJE", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2CreateJoinRoomA<true>);
    LIB_FUNCTION("fQQfP87I7hs", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterContextCallback);
    LIB_FUNCTION("4Nj7u5B5yCA", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterLobbyEventCallback);
    LIB_FUNCTION("p+2EnxmaAMM", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterRoomEventCallback);
    LIB_FUNCTION("uBESzz4CQws", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterRoomMessageCallback);
    LIB_FUNCTION("0UMeWRGnZKA", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2RegisterSignalingCallback);
    LIB_FUNCTION("Nz-ZE7ur32I", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2ContextDestroy);
    LIB_FUNCTION("7vjNQ6Z1op0", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2ContextStart);
    LIB_FUNCTION("-f6M4caNe8k", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2ContextStop);
    LIB_FUNCTION("LhCPctIICxQ", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetServerId);
    LIB_FUNCTION("rJNPJqDCpiI", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2GetWorldInfoList);
    LIB_FUNCTION("CSIMDsVjs-g", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2JoinRoom);
    LIB_FUNCTION("BD6kfx442Do", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2LeaveRoom);
    LIB_FUNCTION("+8e7wXLmjds", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetDefaultRequestOptParam);
    LIB_FUNCTION("VqZX7POg2Mk", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SearchRoom);
    LIB_FUNCTION("Iw2h0Jrrb5U", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SendRoomMessage);
    LIB_FUNCTION("meEjIdbjAA0", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetUserInfo);
    LIB_FUNCTION("q7GK98-nYSE", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetRoomDataExternal);
    LIB_FUNCTION("S9D8JSYIrjE", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SetRoomDataInternal);
    LIB_FUNCTION("tHD5FPFXtu4", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SignalingGetConnectionStatus);
    LIB_FUNCTION("twVupeaYYrk", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SignalingGetConnectionInfo);
    LIB_FUNCTION("wUmwXZHaX1w", "libSceNpMatching2", 1, "libSceNpMatching2",
                 sceNpMatching2SignalingGetPingInfo);
};

} // namespace Libraries::Np::NpMatching2