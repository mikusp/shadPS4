// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "common/types.h"
#include "core/libraries/error_codes.h"
#include "http.h"
#include "http_connection.h"
#include "http_object.h"

int ConvertCurlCodeToOrbis(CURLcode e);

namespace Libraries::Http {

struct HttpRequest : public HttpObject {
    ~HttpRequest() override {}
    HttpRequest();
    HttpRequest(HttpConnection& connection_);

    int connectionId;
    s32 method;
    std::string url;
    u64 contentLength;
    std::unordered_multimap<std::string, std::string> headers;
    OrbisHttpEpollHandle epoll = -1;
    void* arg = nullptr;
    bool done{};
    CURLcode result_code{};

    int GetStatusCode();
    std::pair<int, u64> GetResponseContentLength();
    std::pair<u8*, u64> GetResponseHeaders();
    u64 ReceiveData(char* bytes, u64 size);
    u64 ReceiveHeader(char* data, u64 bytes);

    int ReadData(char* buf, u64 size);
    int Send(const void* postData, size_t size);
    int SendBlocking(const void* postData, size_t size);

private:
    CURL* curl = nullptr;
    curl_slist* headers_list = nullptr;
    friend class HttpEpoll;
    std::vector<std::string> headers_cache{};
    std::vector<u8> response_data{};
    std::vector<u8> response_headers{};
    u64 response_index{};

    bool PrepareCurl();
};

} // namespace Libraries::Http