// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 James Turner

#pragma once

#include <cassert>
#include <string>
#include <thread>

#include <httplib.h>

namespace simgear
{

class TestServer
{
public:
    httplib::Server svr;

    TestServer() : _port(-1) {}

    ~TestServer()
    {
        stop();
    }

    void start()
    {
        _port = svr.bind_to_any_port("127.0.0.1");
        assert(_port > 0);
        _thread = std::thread([this]() {
            svr.listen_after_bind();
        });
        svr.wait_until_ready();
    }

    void stop()
    {
        if (svr.is_running()) {
            svr.stop();
        }
        if (_thread.joinable()) {
            _thread.join();
        }
    }

    int port() const { return _port; }

    std::string url(const std::string& path) const
    {
        return "http://127.0.0.1:" + std::to_string(_port) + path;
    }

private:
    int _port;
    std::thread _thread;
};

} // namespace simgear
