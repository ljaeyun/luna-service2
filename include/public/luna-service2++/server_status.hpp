// Copyright (c) 2014-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <luna-service2/lunaservice.h>

#include <iostream>
#include <functional>
#include <memory>
#include "error.hpp"

namespace LS {

typedef std::function<bool(bool)> ServerStatusCallback;

class ServerStatus
{
    friend class Handle;

public:
    ServerStatus() : _handle(nullptr), _cookie(nullptr) { }

    ServerStatus(const ServerStatus &) = delete;
    ServerStatus& operator=(const ServerStatus &) = delete;

    ServerStatus(ServerStatus &&other) noexcept
        : _handle(other._handle)
        , _cookie(other._cookie)
        , _callback(std::move(other._callback))
    {
        other._cookie = nullptr;
    }

    ServerStatus &operator=(ServerStatus &&other)
    {
        if (_cookie)
        {
            Error error;

            if (!LSCancelServerStatus(_handle, _cookie, error.get()))
            {
                throw error;
            }
        }
        _handle = other._handle;
        _cookie = other._cookie;
        _callback = std::move(other._callback);

        other._cookie = nullptr;

        return *this;
    }

    ~ServerStatus()
    {
        if (_cookie)
        {
            Error error;

            if (!LSCancelServerStatus(_handle, _cookie, error.get()))
                error.logError("LS_FAILED_TO_UNREG_SRV_STAT");
        }
    }

    void *get() { return _cookie; }
    const void *get() const { return _cookie; }

    void cancel()
    {
        Error error;

        if (!LSCancelServerStatus(_handle, _cookie, error.get()))
            throw error;

        _cookie = nullptr;
    }

    explicit operator bool() const { return _cookie; }

private:
    LSHandle *_handle;
    void *_cookie;
    std::unique_ptr<ServerStatusCallback> _callback;

private:
    ServerStatus(LSHandle *_handle, const char *service_name, const ServerStatusCallback &callback)
        : _handle(_handle)
        , _cookie(nullptr)
        , _callback(new ServerStatusCallback(callback))
    {
        Error error;

        if (!LSRegisterServerStatusEx(_handle, service_name, ServerStatus::callbackFunc,
                                      _callback.get(), &_cookie, error.get()))
        {
            throw error;
        }
    }

    static bool callbackFunc(LSHandle *, const char *, bool connected, void *ctx)
    {
        ServerStatusCallback callback = *(static_cast<ServerStatusCallback *>(ctx));

        return callback(connected);
    }

    friend std::ostream &operator<<(std::ostream &os, const ServerStatus &status)
    { return os << "LUNA SERVER STATUS [" << status._cookie << "]"; }
};

} //namespace LS;
