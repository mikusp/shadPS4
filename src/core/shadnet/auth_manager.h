//  SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/logging/log.h"
#include "common/singleton.h"
#include "common/types.h"
#include "externals/httplib.h"
#include "nlohmann/json.hpp"

struct AuthManager {
    using json = nlohmann::json;

    static AuthManager& Instance() {
        return *Common::Singleton<AuthManager>::Instance();
    }

    std::string token() {
        using namespace std::chrono_literals;
        if (bearer_token.empty()) {
            login();
        }
        if (10min <
            duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - fetch_time)) {
            refresh();
        }

        return bearer_token;
    }

private:
    const std::string BASE_URL = "http://127.0.0.1:3000";

    void login() {
        httplib::Client client(BASE_URL);
        auto res =
            client.Post("/auth/login", "{\"email\":\"foo2@example.com\", \"password\":\"foo\"}",
                        "application/json");

        if (res->status == httplib::StatusCode::OK_200) {
            LOG_DEBUG(ShadNet, "{}", res->body);
            auto json = json::parse(res->body);
            for (auto& [key, value] : json.items()) {
                if (key == "access_token") {
                    if (!value.is_string()) {
                        LOG_ERROR(ShadNet, "access_token is not a string");
                        return;
                    }
                    bearer_token = value.get<std::string>();
                    fetch_time = std::chrono::steady_clock::now();
                } else if (key == "refresh_token") {
                    if (!value.is_string()) {
                        LOG_ERROR(ShadNet, "refresh_token is not a string");
                        return;
                    }
                    refresh_token = value.get<std::string>();
                } else {
                    LOG_WARNING(ShadNet, "unknown key {} in login response", key);
                }
            }
        } else {
            LOG_ERROR(ShadNet, "login to shadpsn failed");
        }
    }

    void refresh() {
        httplib::Client client(BASE_URL);
        auto res = client.Post("/auth/refresh", "{\"refresh_token\":\"" + refresh_token + "\"}",
                               "application/json");

        if (res->status == httplib::StatusCode::OK_200) {
            auto json = json::parse(res->body);
            for (auto& [key, value] : json.items()) {
                if (key == "access_token") {
                    if (!value.is_string()) {
                        LOG_ERROR(ShadNet, "access_token is not a string");
                        return;
                    }
                    bearer_token = value.get<std::string>();
                    fetch_time = std::chrono::steady_clock::now();
                } else if (key == "refresh_token") {
                    if (!value.is_string()) {
                        LOG_ERROR(ShadNet, "refresh_token is not a string");
                        return;
                    }
                    refresh_token = value.get<std::string>();
                } else {
                    LOG_WARNING(ShadNet, "unknown key {} in login response", key);
                }
            }
        } else {
            LOG_ERROR(ShadNet, "login to shadpsn failed");
        }
    }

    std::string bearer_token;
    std::string refresh_token;
    std::chrono::steady_clock::time_point fetch_time;
};