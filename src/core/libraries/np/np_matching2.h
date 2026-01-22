// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/network/net.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpMatching2 {

using OrbisNpMatching2ContextId = u16;
using OrbisNpMatching2Event = u16;
using OrbisNpMatching2EventCause = u8;

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpMatching2