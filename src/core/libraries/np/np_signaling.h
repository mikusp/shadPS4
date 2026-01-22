// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/network/net.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpSignaling {

struct OrbisNetInfo {
    size_t size;
    Net::OrbisNetInAddr local_addr;
    Net::OrbisNetInAddr mapped_addr;
    int nat_status;
};

static_assert(sizeof(OrbisNetInfo) == 24, "OrbisNetInfo size is incorrect");

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpSignaling