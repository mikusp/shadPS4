// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_profile_dialog.h"
#include "core/libraries/system/commondialog.h"
#include "imgui/imgui_layer.h"
#include "imgui/imgui_std.h"

static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};

namespace Libraries::Np::NpProfileDialog {

using CommonDialog::Error;
using CommonDialog::Result;
using CommonDialog::Status;

class ProfileDialogUi final : public ImGui::Layer {
    bool first_render{false};

    Status* status{nullptr};
    std::string err_message{};

public:
    explicit ProfileDialogUi(Status* status = nullptr, std::string err_message = "")
        : status(status), err_message(std::move(err_message)) {
        if (status && *status == Status::RUNNING) {
            first_render = true;
            AddLayer(this);
        }
    }
    ~ProfileDialogUi() override {
        Finish();
    }
    ProfileDialogUi(const ProfileDialogUi& other) = delete;
    ProfileDialogUi(ProfileDialogUi&& other) noexcept
        : Layer(other), status(other.status), err_message(std::move(other.err_message)) {
        other.status = nullptr;
    }
    ProfileDialogUi& operator=(ProfileDialogUi other) {
        using std::swap;
        swap(status, other.status);
        swap(err_message, other.err_message);
        if (status && *status == Status::RUNNING) {
            first_render = true;
            AddLayer(this);
        }
        return *this;
    }

    void Finish() {
        if (status) {
            *status = Status::FINISHED;
        }
        status = nullptr;
        RemoveLayer(this);
    }

    void Draw() override {
        using namespace ImGui;
        if (status == nullptr || *status != Status::RUNNING) {
            return;
        }
        const auto& io = GetIO();

        const ImVec2 window_size{
            std::min(io.DisplaySize.x, 500.0f),
            std::min(io.DisplaySize.y, 300.0f),
        };

        CentralizeNextWindow();
        SetNextWindowSize(window_size);
        SetNextWindowCollapsed(false);
        if (first_render || !io.NavActive) {
            SetNextWindowFocus();
        }
        KeepNavHighlight();
        if (Begin("Error Dialog##ErrorDialog", nullptr,
                  ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
            const auto ws = GetWindowSize();

            DrawPrettyBackground();
            const char* begin = &err_message.front();
            const char* end = &err_message.back() + 1;
            SetWindowFontScale(1.3f);
            DrawCenteredText(begin, end,
                             GetContentRegionAvail() - ImVec2{0.0f, 15.0f + BUTTON_SIZE.y});
            SetWindowFontScale(1.0f);

            SetCursorPos({
                ws.x / 2.0f - BUTTON_SIZE.x / 2.0f,
                ws.y - 10.0f - BUTTON_SIZE.y,
            });
            if (Button("OK", BUTTON_SIZE)) {
                Finish();
            }
            if (first_render) {
                SetItemCurrentNavFocus();
            }
        }
        End();

        first_render = false;
    }
};

static auto g_status = Status::NONE;
static ProfileDialogUi g_dialog_ui;

struct DialogBaseParam {
    u64 size;
    u8 reserved[36];
    u32 magic;
} __attribute__((__aligned__(8)));

static_assert(sizeof(DialogBaseParam) == 0x30);

struct OrbisNpProfileDialogParam {
    DialogBaseParam base;
    u64 size;
    u32 dialogMode;
    u32 userId;
    OrbisNpId* npId;
    u64 unk;
    void* arg;
    u8 reserved[40];
};

static_assert(sizeof(OrbisNpProfileDialogParam) == 128);

void* g_callback_arg = nullptr;

s32 PS4_SYSV_ABI sceNpProfileDialogOpen(OrbisNpProfileDialogParam* param) {
    if (g_status != Status::INITIALIZED && g_status != Status::FINISHED) {
        LOG_INFO(Lib_NpProfileDialog, "called without initialize");
        return std::to_underlying(Error::INVALID_STATE);
    }
    if (param == nullptr) {
        LOG_DEBUG(Lib_NpProfileDialog, "called param:(NULL)");
        return std::to_underlying(Error::ARG_NULL);
    }
    LOG_ERROR(Lib_NpProfileDialog, "called, size = {}, dialogMode = {}, userId = {}", param->size, param->dialogMode, param->userId);

    g_callback_arg = param->arg;
    std::string err_message = std::format("npId: {}", param->npId->handle.data);
    g_status = Status::RUNNING;
    g_dialog_ui = ProfileDialogUi{&g_status, err_message};
    return std::to_underlying(Error::OK);
}

s32 PS4_SYSV_ABI sceNpProfileDialogClose() {
    LOG_DEBUG(Lib_NpProfileDialog, "called");
    if (g_status != Status::RUNNING) {
        return std::to_underlying(Error::NOT_RUNNING);
    }
    g_dialog_ui.Finish();
    return std::to_underlying(Error::OK);
}

struct OrbisNpProfileDialogResult {
    int errorCode;
    Libraries::CommonDialog::Result result;
    void* arg;
    u8 reserved[32];
};

s32 PS4_SYSV_ABI sceNpProfileDialogGetResult(OrbisNpProfileDialogResult* result) {
    LOG_ERROR(Lib_NpProfileDialog, "called");
    // Simulates behavior of user pressing circle to cancel the dialog.
    // Result::OK would mean a headset was connected.
    result->arg = g_callback_arg;
    result->errorCode = 0;
    result->result = Libraries::CommonDialog::Result::OK;
    // strncpy(result->sentUsers->users[0].onlineId.data, "FOO", strlen("FOO"));
    // result->sentUsers->users[0].accountId = 0xCAFE;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpProfileDialogGetStatus() {
    LOG_DEBUG(Lib_NpProfileDialog, "called status={}", magic_enum::enum_name(g_status));
    return std::to_underlying(g_status);
}

s32 PS4_SYSV_ABI sceNpProfileDialogInitialize() {
    LOG_DEBUG(Lib_NpProfileDialog, "called");
    if (g_status != Status::NONE) {
        return std::to_underlying(Error::ALREADY_INITIALIZED);
    }
    g_status = Status::INITIALIZED;
    return std::to_underlying(Error::OK);
}

s32 PS4_SYSV_ABI sceNpProfileDialogOpenA() {
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpProfileDialogTerminate() {
    LOG_DEBUG(Lib_NpProfileDialog, "called");
    if (g_status == Status::RUNNING) {
        sceNpProfileDialogClose();
    }
    if (g_status == Status::NONE) {
        return std::to_underlying(Error::NOT_INITIALIZED);
    }
    g_status = Status::NONE;
    return std::to_underlying(Error::OK);
}

s32 PS4_SYSV_ABI sceNpProfileDialogUpdateStatus() {
    LOG_DEBUG(Lib_NpProfileDialog, "(STUBBED) called");
    return std::to_underlying(g_status);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("uj9Cz7Tk0cc", "libSceNpProfileDialogCompat", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogOpen);
    LIB_FUNCTION("wkwjz0Xdo2A", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogClose);
    LIB_FUNCTION("8rhLl1-0W-o", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogGetResult);
    LIB_FUNCTION("3BqoiFOjSsk", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogGetStatus);
    LIB_FUNCTION("Lg+NCE6pTwQ", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogInitialize);
    LIB_FUNCTION("uj9Cz7Tk0cc", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogOpen);
    LIB_FUNCTION("nrQRlLKzdwE", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogOpenA);
    LIB_FUNCTION("0Sp9vJcB1-w", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogTerminate);
    LIB_FUNCTION("haVZE9FgKqE", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogUpdateStatus);
};

} // namespace Libraries::Np::NpProfileDialog