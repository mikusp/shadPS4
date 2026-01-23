// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpMatching2 {

using OrbisNpMatching2AttributeId = u16;
using OrbisNpMatching2ContextId = u16;
using OrbisNpMatching2Event = u16;
using OrbisNpMatching2EventCause = u8;
using OrbisNpMatching2Flags = u32;
using OrbisNpMatching2LobbyId = u64;
using OrbisNpMatching2RequestId = u16;
using OrbisNpMatching2RoomId = u64;
using OrbisNpMatching2RoomMemberId = u16;
using OrbisNpMatching2ServerId = u16;
using OrbisNpMatching2WorldId = u32;

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpMatching2