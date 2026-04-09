// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2020 James Turner

/**
 * @file
 * @brief Base class for log callbacks
 */

#pragma once

#include <string>

#include "LogEntry.hxx"
#include "debug_types.h"
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/timing/timestamp.hxx>

namespace simgear {

/**
 * @brief Base class for log callbacks.
 *
 * Derived classes should override `doProcessEntry` to handle log entries, returning true
 * if they logged the message or false if they skipped it for whatever reason.
 *
 * Log Callbacks all run on a shared, dedicated worker thread, so must be thread safe,
 * but will be synchronized with each other.
 */
class LogCallback
{
public:
    virtual ~LogCallback() = default;

    // newer API: return true if you handled the message, otherwise
    // the old API will be called
    virtual bool doProcessEntry(const LogEntry& e);

    // old API, kept for compatibility
    virtual void operator()(sgDebugClass c, sgDebugPriority p,
                            const char* file, int line, const std::string& aMessage);

    void setLogLevels(const simgear::LogLevels& levels);
    const simgear::LogLevels& logLevels() const { return _logLevels; }

    void processEntry(const LogEntry& e);

    const std::string& tag() const { return _tag; }

protected:
    LogCallback(const std::string& tag) : _tag(tag) {};

    bool shouldLog(sgDebugClass c, sgDebugPriority p) const;
private:
    std::string _tag;
    simgear::LogLevels _logLevels;
};

/**
 * Log to a file on disk.
 */
class FileLogCallback : public simgear::LogCallback
{
public:
    SGTimeStamp logTimer;
    FileLogCallback(const std::string& tag, const SGPath& aPath);

    void operator()(sgDebugClass c, sgDebugPriority p,
                    const char* file, int line, const std::string& message) override;

private:
    sg_ofstream m_file;
};

class StderrLogCallback : public simgear::LogCallback
{
public:
    SGTimeStamp logTimer;
    StderrLogCallback();
    ~StderrLogCallback();

    void operator()(sgDebugClass c, sgDebugPriority p,
                    const char* file, int line, const std::string& aMessage) override;
};

#ifdef SG_WINDOWS

/**
 * @brief Windows-specific log callback which outputs to the Windows debug output stream
 */
class WinDebugLogCallback : public simgear::LogCallback
{
public:
    WinDebugLogCallback();
    void operator()(sgDebugClass c, sgDebugPriority p,
                    const char* file, int line, const std::string& aMessage) override;
};

#endif

} // namespace simgear
