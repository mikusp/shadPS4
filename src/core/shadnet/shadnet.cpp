//  SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later


#include <magic_enum/magic_enum.hpp>

#include "core/shadnet/auth_manager.h"
#include "core/shadnet/matching_json.h"
#include "core/shadnet/shadnet.h"
#include "core/libraries/np/np_manager.h"

namespace Core::ShadNet {

using namespace Libraries::Np::NpMatching2;
using json = nlohmann::json;

void to_json(json& j, const WsEnvelope& e) {
    j = json{
        {"id", e.id},
        {"type", e.type},
        {"payload", e.payload}
    };
}

void from_json(const json& j, WsMessage& msg) {
    j.at("id").get_to(msg.request_id);
    if (j.contains("error_code")) {
        j.at("error_code").get_to(msg.error_code);
    }
    if (j.contains("error")) {
        j.at("error").get_to(msg.error);
    }
    if (j.contains("payload")) {
        j.at("payload").get_to(msg.payload);
    }
}

void from_json(const json& j, WsEvent& ev) {
    j.at("type").get_to(ev.type);
    j.at("ev").get_to(ev.ev);
}

struct NpMatching2ContextEvent {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2Event event;
    OrbisNpMatching2EventCause cause;
    int errorCode;
};

std::function<void(const NpMatching2ContextEvent*)> npMatching2ContextCallback = nullptr;

int MatchingContext::Start(OrbisNpMatching2ContextId ctxId, u64 timeout) {
    if (websocket.isOnMessageCallbackRegistered()) {
        // make it a proper state machine
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED;
    }

    LOG_DEBUG(ShadNet, "starting context ctxId = {}", this->ctxId);

    this->ctxId = ctxId;

    ix::initNetSystem();

    websocket.setUrl("ws://127.0.0.1:3000/matching/v1/ws");
    websocket.setExtraHeaders({
        {"X-NP-TITLE-ID", Libraries::Np::NpManager::g_np_title_id.id},
        {"Authorization", std::format("Bearer {}", AuthManager::Instance().token())}
    });
    websocket.setPingInterval(10);

    auto timeoutSec = 20;
    if (timeout > 10'000'000) {
        timeoutSec = timeout / 1'000'000;
    }
    websocket.setHandshakeTimeout(timeoutSec);

    websocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        switch (msg->type) {
        case ix::WebSocketMessageType::Open: {
            LOG_DEBUG(ShadNet, "ws connection opened for ctxId = {}", this->ctxId);
            NpMatching2ContextEvent ev {
                .contextId = this->ctxId,
                .event = ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED,
                .cause = ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION,
                .errorCode = 0
            };
            npMatching2ContextCallback(&ev);
            break;
        }
        case ix::WebSocketMessageType::Message: {
            if (msg->binary) {
                LOG_ERROR(ShadNet, "received binary ws message");
                break;
            }
            LOG_DEBUG(ShadNet, "text message: {}", msg->str);
            try {
                this->HandleMessage(msg->str);
            }
            catch (const json::exception& e) {
                LOG_ERROR(ShadNet, "json error when handling message: {}", e.what());
            }
            catch (...) {
                LOG_ERROR(ShadNet, "handling message failed");
            }
            break;
        }
        case ix::WebSocketMessageType::Close: {
            LOG_DEBUG(ShadNet, "close message, code = {}, reason = {}", msg->closeInfo.code, msg->closeInfo.reason);
            break;
        }
        case ix::WebSocketMessageType::Error: {
            LOG_DEBUG(ShadNet, "error message, http_status = {}, reason = {}", msg->errorInfo.http_status, msg->errorInfo.reason);
            break;
        }
        default: {
            LOG_DEBUG(ShadNet, "message type {}", magic_enum::enum_name(msg->type));
            break;
        }
        }
    });

    websocket.start();

    return 0;
}

int MatchingContext::CreateJoinRoom(const OrbisNpMatching2CreateJoinRoomRequest& req, const OrbisNpMatching2RequestOptParam* optParam) {
    return SendRequest(req, optParam);
}

int MatchingContext::CreateJoinRoom(const OrbisNpMatching2CreateJoinRoomRequestA& req, const OrbisNpMatching2RequestOptParam* optParam) {
    return SendRequest(req, optParam);
}

int MatchingContext::JoinRoom(const OrbisNpMatching2JoinRoomRequest& req, const OrbisNpMatching2RequestOptParam* optParam) {
    return SendRequest(req, optParam);
}

int MatchingContext::SearchRoom(const OrbisNpMatching2SearchRoomRequest& req, const OrbisNpMatching2RequestOptParam* optParam) {
    return SendRequest(req, optParam);
}

int MatchingContext::SignalingGetPingInfo(const OrbisNpMatching2SignalingGetPingInfoRequest& req, const OrbisNpMatching2RequestOptParam* optParam) {
    return SendRequest(req, optParam);
}

void MatchingContext::SetDefaultRequestOptParam(const OrbisNpMatching2RequestOptParam& optParam) {
    std::scoped_lock lk{this->mutex};
    this->optParam = optParam;
}

// call under lock as it might access internal state
auto MatchingContext::GetRequestCallback(std::optional<OrbisNpMatching2RequestOptParam> requestOptParam) {
    std::function<OrbisNpMatching2RequestFn> cb = nullptr;
    void* arg = nullptr;
    if (requestOptParam) {
        cb = requestOptParam->callback;
        arg = requestOptParam->arg;
    }
    else if (this->optParam) {
        cb = this->optParam->callback;
        arg = this->optParam->arg;
    }

    return [cb, arg](auto ctxId, auto reqId, auto ev, int errorCode, auto data) {
        if (cb) {
            cb(ctxId, reqId, ev, errorCode, data, arg);
        }
    };
}


template<typename T>
int MatchingContext::SendRequest(const T& req, const OrbisNpMatching2RequestOptParam* optParam) {
    json j = req;
    WsEnvelope envelope {this->reqId++, request_tag(req), j.dump()};

    {
        std::scoped_lock lk{this->mutex};
        this->pendingRequests.emplace(envelope.id, std::make_tuple(request_tag(req), optParam ? std::make_optional(*optParam) : std::nullopt));
    }

    json e = envelope;
    websocket.send(e.dump());

    return envelope.id;
}

void MatchingContext::SetContextCallback(OrbisNpMatching2ContextCallback cb, void* userdata) {
    npMatching2ContextCallback = [cb, userdata](auto arg) {
        cb(arg->contextId, arg->event, arg->cause, arg->errorCode, userdata);
    };
}

void MatchingContext::SetRoomCallback(OrbisNpMatching2RoomCallback cb, void* userdata) {
    this->roomCallback = [cb, userdata](auto ctxId, auto roomId, auto event, const void* data) {
        cb(ctxId, roomId, event, data, userdata);
    };
}

class Finalizer {
    std::function<void()> f;

public:
    explicit Finalizer(std::function<void()> f) : f(f) {}
    ~Finalizer() { f(); }
};

void MatchingContext::HandleResponse(const WsMessage& response) {
    Finalizer f([this, response]{
        std::scoped_lock lk{this->mutex};
        this->pendingRequests.erase(response.request_id);
    });

    std::unique_lock lk{this->mutex};
    auto [type, optParam] = this->pendingRequests.at(response.request_id);
    auto cb = GetRequestCallback(optParam);
    lk.unlock();

    if (response.error_code) {
        LOG_ERROR(ShadNet, "matching request {} failed with {} (code {})", response.request_id, response.error, response.error_code);

    }
    else {
        LOG_DEBUG(ShadNet, "matching request {} response received", response.request_id);
        if (type == "create_join_room") {
            auto resp = response.payload.get<OrbisNpMatching2CreateJoinRoomResponseOwned>();
            auto view = resp.view();
            cb(this->ctxId, response.request_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM, ORBIS_OK, &view);
        }
        else if (type == "search_room") {
            auto resp = response.payload.get<OrbisNpMatching2SearchRoomResponseOwned>();
            auto view = resp.view();
            cb(this->ctxId, response.request_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM, ORBIS_OK, &view);
        }
        else if (type == "join_room") {
            // it's the same response as createjoin
            auto resp = response.payload.get<OrbisNpMatching2CreateJoinRoomResponseOwned>();
            auto view = resp.view();
            cb(this->ctxId, response.request_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM, ORBIS_OK, &view);
        }
        ///
        else {
            LOG_ERROR(ShadNet, "unhandled response type: {}", type);
        }
    }
}

void MatchingContext::HandleEvent(const WsEvent& event) {
    if (event.type == "member_joined") {
        auto evData = event.ev.get<OrbisNpMatching2RoomMemberUpdateInfoOwned>();
        auto view = evData.view();
        this->roomCallback(this->ctxId, evData.roomId, ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED, &view);
    }
    else {
        LOG_ERROR(ShadNet, "unhandled event type: {}", event.type);
    }
}

void MatchingContext::HandleMessage(const std::string& wsMessage) {
    auto j = json::parse(wsMessage);

    if (j.contains("id")) {
        auto message = j.get<WsMessage>();
        HandleResponse(message);
    }
    else {
        auto ev = j.get<WsEvent>();
        HandleEvent(ev);
    }
}

} // namespace Core::ShadNet
