// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2020 James Turner

/**
 * @file
 * @brief Base class for log callbacks
 */

#include <simgear_config.h>

#include "simgear/debug/debug_types.h"
#include <simgear/debug/LogCallback.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/timing/timestamp.hxx>

#if defined(SG_WINDOWS)
    // for AllocConsole, OutputDebugString
    #include <fcntl.h>
    #include <io.h>
    #include <windows.h>
#endif

using namespace simgear;

// LogCallback::LogCallback(const simgear::LogLevels& levels) : _logLevels(levels)
// {
// }

void LogCallback::operator()(sgDebugClass c, sgDebugPriority p,
                             const char* file, int line, const std::string& aMessage)
{
    // override me
}

bool LogCallback::doProcessEntry(const LogEntry& e)
{
    return false;
}

void LogCallback::processEntry(const LogEntry& e)
{
    if (doProcessEntry(e))
        return; // derived class used the new API

    // call the old API
    (*this)(e.debugClass, e.debugPriority, e.file, e.line, e.message);
}


bool LogCallback::shouldLog(sgDebugClass c, sgDebugPriority p) const
{
    return simgear::shouldLog(c, p, _logLevels);
}

void LogCallback::setLogLevels(const simgear::LogLevels& levels)
{
    _logLevels = levels;
}

///////////////////////////////////////////////////////////////////////////////


FileLogCallback::FileLogCallback(const std::string& tag, const SGPath& aPath) : simgear::LogCallback(tag)
{
    m_file.open(aPath, std::ios_base::out | std::ios_base::trunc);
    logTimer.stamp();
}

void FileLogCallback::operator()(sgDebugClass c, sgDebugPriority p,
                                 const char* file, int line, const std::string& message)
{
    if (!shouldLog(c, p)) return;

    m_file
        << std::fixed
        << std::setprecision(2)
        << std::setw(8)
        << std::right
        << (logTimer.elapsedMSec() / 1000.0)
        << std::setw(8)
        << std::left
        << " [" + debugPriorityToString(p) + "]:"
        << std::setw(10)
        << std::left
        << debugClassToString(c);
    if (file) {
        /* <line> can be -ve to indicate that m_fileLine was false, but we
        want to show file:line information regardless of m_fileLine. */
        m_file
            << file
            << ":"
            << abs(line)
            << ": ";
    }
    m_file
        << message << std::endl;
    //m_file << debugClassToString(c) << ":" << (int)p
    //    << ":" << file << ":" << line << ":" << message << std::endl;
}


///////////////////////////////////////////////////////////////////////////////


StderrLogCallback::StderrLogCallback() : simgear::LogCallback("console")
{
    logTimer.stamp();
}

StderrLogCallback::~StderrLogCallback()
{
#if defined(SG_WINDOWS)
    FreeConsole();
#endif
}

void StderrLogCallback::operator()(sgDebugClass c, sgDebugPriority p,
                                   const char* file, int line, const std::string& aMessage)
{
    if (!shouldLog(c, p)) return;

    if (file && line > 0) {
        fprintf(stderr, "%8.2f %s:%i: [%.8s]:%-10s %s\n", logTimer.elapsedMSec() / 1000.0, file, line, debugPriorityToString(p).c_str(), debugClassToString(c).c_str(), aMessage.c_str());
    } else {
        fprintf(stderr, "%8.2f [%.8s]:%-10s %s\n", logTimer.elapsedMSec() / 1000.0, debugPriorityToString(p).c_str(), debugClassToString(c).c_str(), aMessage.c_str());
    }

    fflush(stderr);
}

///////////////////////////////////////////////////////////////////////////////

#ifdef SG_WINDOWS

WinDebugLogCallback::WinDebugLogCallback() : simgear::LogCallback("console")
{
}

void WinDebugLogCallback::operator()(sgDebugClass c, sgDebugPriority p,
                                     const char* file, int line, const std::string& aMessage)
{
    if (!shouldLog(c, p)) return;

    std::ostringstream os;
    os << debugClassToString(c) << ":" << aMessage << std::endl;
    OutputDebugStringA(os.str().c_str());
}

#endif
