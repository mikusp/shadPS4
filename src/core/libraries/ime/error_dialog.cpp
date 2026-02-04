// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <utility>
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/system/commondialog.h"
#include "error_dialog.h"
#include "imgui/imgui_layer.h"
#include "imgui/imgui_std.h"

static constexpr ImVec2 BUTTON_SIZE{100.0f, 30.0f};

namespace Libraries::ErrorDialog {

using CommonDialog::Error;
using CommonDialog::Result;
using CommonDialog::Status;

class ErrorDialogUi final : public ImGui::Layer {
    bool first_render{false};

    Status* status{nullptr};
    std::string err_message{};

public:
    explicit ErrorDialogUi(Status* status = nullptr, std::string err_message = "")
        : status(status), err_message(std::move(err_message)) {
        if (status && *status == Status::RUNNING) {
            first_render = true;
            AddLayer(this);
        }
    }
    ~ErrorDialogUi() override {
        Finish();
    }
    ErrorDialogUi(const ErrorDialogUi& other) = delete;
    ErrorDialogUi(ErrorDialogUi&& other) noexcept
        : Layer(other), status(other.status), err_message(std::move(other.err_message)) {
        other.status = nullptr;
    }
    ErrorDialogUi& operator=(ErrorDialogUi other) {
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
static ErrorDialogUi g_dialog_ui;

struct Param {
    s32 size;
    s32 errorCode;
    OrbisUserServiceUserId userId;
    s32 _reserved;
};

Error PS4_SYSV_ABI sceErrorDialogClose() {
    LOG_DEBUG(Lib_ErrorDialog, "called");
    if (g_status != Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    g_dialog_ui.Finish();
    return Error::OK;
}

Status PS4_SYSV_ABI sceErrorDialogGetStatus() {
    LOG_TRACE(Lib_ErrorDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

Error PS4_SYSV_ABI sceErrorDialogInitialize() {
    LOG_DEBUG(Lib_ErrorDialog, "called");
    if (g_status != Status::NONE) {
        return Error::ALREADY_INITIALIZED;
    }
    g_status = Status::INITIALIZED;
    return Error::OK;
}

Error PS4_SYSV_ABI sceErrorDialogOpen(const Param* param) {
    if (g_status != Status::INITIALIZED && g_status != Status::FINISHED) {
        LOG_INFO(Lib_ErrorDialog, "called without initialize");
        return Error::INVALID_STATE;
    }
    if (param == nullptr) {
        LOG_DEBUG(Lib_ErrorDialog, "called param:(NULL)");
        return Error::ARG_NULL;
    }
    const auto err = static_cast<u32>(param->errorCode);
    LOG_DEBUG(Lib_ErrorDialog, "called param->errorCode = {:#x}", err);
    ASSERT(param->size == sizeof(Param));

    const std::string err_message = fmt::format("An error has occurred. \nCode: {:#X}", err);
    g_status = Status::RUNNING;
    g_dialog_ui = ErrorDialogUi{&g_status, err_message};
    return Error::OK;
}

int PS4_SYSV_ABI sceErrorDialogOpenDetail() {
    LOG_ERROR(Lib_ErrorDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceErrorDialogOpenWithReport() {
    LOG_ERROR(Lib_ErrorDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceErrorDialogTerminate() {
    LOG_DEBUG(Lib_ErrorDialog, "called");
    if (g_status == Status::RUNNING) {
        sceErrorDialogClose();
    }
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    g_status = Status::NONE;
    return Error::OK;
}

Status PS4_SYSV_ABI sceErrorDialogUpdateStatus() {
    LOG_TRACE(Lib_ErrorDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

Error PS4_SYSV_ABI sceInvitationDialogInitialize() {
    LOG_DEBUG(Lib_ErrorDialog, "called");
    if (g_status != Status::NONE) {
        return Error::ALREADY_INITIALIZED;
    }
    g_status = Status::INITIALIZED;
    return Error::OK;
}

struct DialogBaseParam {
    u64 size;
    u8 reserved[36];
    u32 magic;
} __attribute__((__aligned__(8)));

static_assert(sizeof(DialogBaseParam) == 0x30);

struct OrbisNpSessionId {
    char data[45];
    char term;
    char pad[2];
};

typedef union {
    struct {
        u64* accountIds;
        u32 accountIdNum;
        u32 pad;
    } UserSelecttDisableAddress;
    struct {
        u32 userMaxCount;
    } UserSelectEnableAddress;
} SceInvitationDialogAddressInfoA;

struct OrbisInvitationDialogAddressParamA {
    u32 addressType;
    u32 pad;
    SceInvitationDialogAddressInfoA addressInfo;
};

typedef union {
    struct {
        char* userMessage;
        OrbisNpSessionId* sessionId;
        OrbisInvitationDialogAddressParamA addressParam;
    } SendInfo;
    struct {
        u8 reserved[64];
    } RecvInfo;
} OrbisInvitationDialogDataParamA;

typedef union {
    struct {
        char* userMessage;
        OrbisNpSessionId* sessionId;
        OrbisInvitationDialogAddressParamA addressParam;
    } SendInfo;
    struct {
        u8 reserved[64];
    } RecvInfo;
} OrbisInvitationDialogDataParam;

struct InvitationParamA {
    DialogBaseParam base;
    u32 size;
    u32 dialogMode;
    u32 userId;
    u32 pad;
    void* callbackArg;
    OrbisInvitationDialogDataParamA* dataParam;
    u8 reserved[64];
};

void* g_callback_arg = nullptr;

struct InvitationParam {
    DialogBaseParam base;
    u32 size;
    u32 dialogMode;
    u32 userId;
    u32 pad;
    void* callbackArg;
    OrbisInvitationDialogDataParam* dataParam;
    u8 reserved[64];
};

static_assert(sizeof(InvitationParam) == 0x90);

Error PS4_SYSV_ABI sceInvitationDialogOpen(const InvitationParam* param) {
    if (g_status != Status::INITIALIZED && g_status != Status::FINISHED) {
        LOG_INFO(Lib_ErrorDialog, "called without initialize");
        return Error::INVALID_STATE;
    }
    if (param == nullptr) {
        LOG_DEBUG(Lib_ErrorDialog, "called param:(NULL)");
        return Error::ARG_NULL;
    }
    // const auto err = static_cast<u32>(param->errorCode);
    // LOG_DEBUG(Lib_ErrorDialog, "called param->errorCode = {:#x}", err);
    LOG_DEBUG(Lib_ErrorDialog, "called");
    ASSERT(param->size == sizeof(InvitationParam));

    g_callback_arg = param->callbackArg;
    const std::string err_message = "invitation dialog stub";
    g_status = Status::RUNNING;
    g_dialog_ui = ErrorDialogUi{&g_status, err_message};
    return Error::OK;
}

Error PS4_SYSV_ABI sceInvitationDialogOpenA(const InvitationParamA* param) {
    if (g_status != Status::INITIALIZED && g_status != Status::FINISHED) {
        LOG_INFO(Lib_ErrorDialog, "called without initialize");
        return Error::INVALID_STATE;
    }
    if (param == nullptr) {
        LOG_DEBUG(Lib_ErrorDialog, "called param:(NULL)");
        return Error::ARG_NULL;
    }
    // const auto err = static_cast<u32>(param->errorCode);
    // LOG_DEBUG(Lib_ErrorDialog, "called param->errorCode = {:#x}", err);
    LOG_DEBUG(Lib_ErrorDialog, "called");
    ASSERT(param->size == sizeof(InvitationParamA));

    g_callback_arg = param->callbackArg;
    const std::string err_message = "invitation dialog stub";
    g_status = Status::RUNNING;
    g_dialog_ui = ErrorDialogUi{&g_status, err_message};
    return Error::OK;
}

Status PS4_SYSV_ABI sceInvitationDialogUpdateStatus() {
    LOG_DEBUG(Lib_ErrorDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

struct OrbisInvitationDialogUserList {
    u32 count;
    struct {
        Np::OrbisNpOnlineId onlineId;
        Np::OrbisNpAccountId accountId;
    } users[16];
};

struct OrbisInvitationDialogResultA {
    void* callbackArg;
    int errorCode;
    Libraries::CommonDialog::Result result;
    OrbisInvitationDialogUserList* sentUsers;
    u8 reserved[32];
};

struct OrbisInvitationDialogResult {
    void* callbackArg;
    int errorCode;
    Libraries::CommonDialog::Result result;
    OrbisInvitationDialogUserList* sentUsers;
    u8 reserved[32];
};

s32 PS4_SYSV_ABI sceInvitationDialogGetResult(OrbisInvitationDialogResult* result) {
    LOG_ERROR(Lib_ErrorDialog, "(STUBBED) called");
    // Simulates behavior of user pressing circle to cancel the dialog.
    // Result::OK would mean a headset was connected.
    result->callbackArg = g_callback_arg;
    result->errorCode = 0;
    result->result = Libraries::CommonDialog::Result::OK;
    if (result->sentUsers) {
        result->sentUsers->count = 0;
    }
    // strncpy(result->sentUsers->users[0].onlineId.data, "FOO", strlen("FOO"));
    // result->sentUsers->users[0].accountId = 0xCAFE;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogGetResultA(OrbisInvitationDialogResultA* result) {
    LOG_ERROR(Lib_ErrorDialog, "(STUBBED) called");
    // Simulates behavior of user pressing circle to cancel the dialog.
    // Result::OK would mean a headset was connected.
    result->callbackArg = g_callback_arg;
    result->errorCode = 0;
    result->result = Libraries::CommonDialog::Result::OK;
    if (result->sentUsers) {
        LOG_WARNING(Lib_ErrorDialog, "requiring sentUsers");
        result->sentUsers->count = 0;
        // strncpy(result->sentUsers->users[0].onlineId.data, "FOO", strlen("FOO"));
        // result->sentUsers->users[0].accountId = 0xCAFE;
    }
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("ekXHb1kDBl0", "libSceErrorDialog", 1, "libSceErrorDialog", sceErrorDialogClose);
    LIB_FUNCTION("t2FvHRXzgqk", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogGetStatus);
    LIB_FUNCTION("I88KChlynSs", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogInitialize);
    LIB_FUNCTION("M2ZF-ClLhgY", "libSceErrorDialog", 1, "libSceErrorDialog", sceErrorDialogOpen);
    LIB_FUNCTION("jrpnVQfJYgQ", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogOpenDetail);
    LIB_FUNCTION("wktCiyWoDTI", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogOpenWithReport);
    LIB_FUNCTION("9XAxK2PMwk8", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogTerminate);
    LIB_FUNCTION("WWiGuh9XfgQ", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogUpdateStatus);

    LIB_FUNCTION("XvA5KS56wcs", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogInitialize);
    LIB_FUNCTION("0zU0G+wiVLA", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogOpen);
    LIB_FUNCTION("sAxbHhAWMXM", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogOpenA);
    LIB_FUNCTION("9+g9iOq+7kg", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogUpdateStatus);
    LIB_FUNCTION("8XKR6wa64iQ", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogGetResult);
    LIB_FUNCTION("WuuUhuKOxwQ", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogGetResultA);
};

} // namespace Libraries::ErrorDialog