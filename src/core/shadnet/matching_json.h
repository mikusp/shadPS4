//  SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/np/np_matching2_requests.h"
#include "nlohmann/json.hpp"


namespace Libraries::Np::NpMatching2 {

using json = nlohmann::json;

struct OrbisNpPeerAddressOwned {
    OrbisNpId npId;
    OrbisNpPlatformType platform;
};

struct OrbisNpMatching2BinAttrOwned {
    OrbisNpMatching2AttributeId id;
    std::vector<u8> data;
};

struct OrbisNpMatching2RoomMemberDataInternalOwned {
    u64 joinDateTicks;
    OrbisNpPeerAddressOwned user;
    Libraries::Np::OrbisNpOnlineId onlineId;
    u8 pad[4];
    OrbisNpMatching2RoomMemberId memberId;
    OrbisNpMatching2TeamId teamId;
    OrbisNpMatching2NatType natType;
    OrbisNpMatching2Flags flags;
    std::optional<OrbisNpMatching2RoomGroup> roomGroup;
    std::vector<OrbisNpMatching2BinAttrOwned> roomMemberInternalBinAttr;
};

struct OrbisNpMatching2RoomDataInternalOwned {
    u16 publicSlots;
    u16 privateSlots;
    u16 openPublicSlots;
    u16 openPrivateSlots;
    u16 maxSlot;
    OrbisNpMatching2ServerId serverId;
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2RoomId roomId;
    u64 passwdSlotMask;
    u64 joinedSlotMask;
    std::vector<OrbisNpMatching2RoomGroup> roomGroup;
    OrbisNpMatching2Flags flags;
    u8 pad[4];
    std::vector<OrbisNpMatching2BinAttrOwned> internalBinAttr;

    std::vector<OrbisNpMatching2BinAttr> internalBinAttrView;
    OrbisNpMatching2RoomDataInternal view();
};

struct OrbisNpMatching2CreateJoinRoomResponseOwned {
    OrbisNpMatching2RoomDataInternalOwned roomData;
    std::vector<OrbisNpMatching2RoomMemberDataInternalOwned> members;

    OrbisNpMatching2RoomDataInternal roomDataView;
    std::vector<OrbisNpMatching2RoomMemberDataInternal> membersView;
    std::vector<std::vector<OrbisNpMatching2BinAttr>> membersBinAttrs;
    OrbisNpMatching2CreateJoinRoomResponse view();
};

std::string request_tag(const OrbisNpMatching2CreateJoinRoomRequest&);
std::string request_tag(const OrbisNpMatching2CreateJoinRoomRequestA&);
void to_json(json& j, const OrbisNpMatching2CreateJoinRoomRequest& req);
void to_json(json& j, const OrbisNpMatching2CreateJoinRoomRequestA& req);
// void from_json(const json& j, OrbisNpMatching2CreateJoinRoomResponse& res);
void from_json(const json& j, OrbisNpMatching2CreateJoinRoomResponseOwned& res);


}
