// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <string_view>
#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/file_sys/fs.h"
#include "core/game_hook.h"

#include <dlfcn.h>

namespace Core::GameHook {

struct hooks_metadata {
    const char* game_id;
    const char* app_version;
    u32 firmware_ver;
};

using HookFun = bool(*)(u64);

struct hook {
    const char* library_name;
    HookFun func;
};

std::map<std::string, HookFun> loaded_hooks;

void Init(std::string game_id, std::string app_version) {
    const auto hooks_dir = Common::FS::GetUserPath(Common::FS::PathType::HooksDir);
    const auto hook_file = hooks_dir / fmt::format("{}.so", game_id);

    void* handle = dlopen(hook_file.c_str(), RTLD_LAZY);
    if (!handle) {
        if (std::filesystem::exists(hook_file)) {
            LOG_ERROR(Core, "Hook file {} failed to load: {}", hook_file.string(), dlerror());
        }
        return;
    }

    const auto meta = static_cast<hooks_metadata*>(dlsym(handle, "hooks_metadata"));
    if (!meta) {
        LOG_ERROR(Core, "Hook file {} lacks required symbol hooks_metadata", hook_file.string());
        return;
    }

    if (!(strcmp(game_id.c_str(), meta->game_id) == 0 && strcmp(app_version.c_str(), meta->app_version) == 0)) {
        LOG_ERROR(Core, "Hook file is not compatible with that version: {} != {}, {} != {}", game_id, meta->game_id, app_version, meta->app_version);
        return;
    }

    auto hooks = static_cast<hook*>(dlsym(handle, "hooks"));
    if (!hooks) {
        LOG_ERROR(Core, "Hooks file is missing hooks?");
        return;
    }

    while (hooks->library_name) {
        loaded_hooks.emplace(hooks->library_name, hooks->func);
        hooks++;
    }

    LOG_INFO(Core, "Hooks file {} successfully loaded", hook_file.string());
}

void OnModuleLoaded(std::string library_name, u64 module_base_addr) {
    if (const auto hook = loaded_hooks.find(library_name); hook != loaded_hooks.end()) {
        LOG_INFO(Core, "Hooking module {}", library_name);
        hook->second(module_base_addr);
    }
}

}