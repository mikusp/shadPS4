// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::GameHook {

void Init(std::string game_id, std::string app_version);
void OnModuleLoaded(std::string library_name, u64 module_base_addr);

}