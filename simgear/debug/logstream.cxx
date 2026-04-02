// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1998 Bernie Bright <bbright@c031.aone.net.au>

/**
 * @file
 * @brief Stream based logging mechanism.
 */

#include <cstdlib>
#include <simgear_config.h>

#include "logdelta.hxx"
#include "logstream.hxx"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <mutex>

#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>
#include <simgear/threads/SGQueue.hxx>
#include <simgear/threads/SGThread.hxx>

#include "LogCallback.hxx"
#include "simgear/debug/debug_types.h"
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/timing/timestamp.hxx>
#include <string>

#if defined (SG_WINDOWS)
// for AllocConsole, OutputDebugString
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

//////////////////////////////////////////////////////////////////////////////


// this is a global since we want to set it before LogStreamPrivate exists
static bool global_disableStderrLogging = false;

class logstream::LogStreamPrivate : public SGThread
{
private:
    /**
     * RAII object to pause the logging thread if it's running, and restart it.
     * used to safely make configuration changes.
     */
    class PauseThread
    {
    public:
        PauseThread(LogStreamPrivate* parent)
            : m_parent(parent)
            , m_wasRunning(m_parent->stop())
        {
        }

        ~PauseThread()
        {
            if (m_wasRunning) {
                m_parent->startLog();
            }
        }
    private:
        LogStreamPrivate* m_parent;
        const bool m_wasRunning;
    };

public:
    LogStreamPrivate()
    {
        bool doLogToStderr = !global_disableStderrLogging;
        _consoleLevels.set(SG_ALL, SG_ALERT);
        const auto logEnv = getenv("SG_LOGGING");
        if (logEnv) {
            auto levels = simgear::parseLogSpecFromString(logEnv);
            if (levels) {
                _consoleLevels = levels.value();
            } else {
                SG_LOG(SG_GENERAL, SG_ALERT, "Error parsing SG_LOGGING environment variable '" << logEnv << "'");
            }
        }

#if defined (SG_WINDOWS)
        /*
         * 2016-09-20(RJH) - Reworked console handling
         * 1) When started from the console use the console (when no --console)
         * 2) When started from the GUI (with --console) open a new console window
         * 3) When started from the GUI (without --console) don't open a new console
         *    window; stdout/stderr will not appear (except in logfiles as they do now)
         * 4) When started from the Console (with --console) open a new console window
         * 5) Ensure that IO redirection still works when started from the console
         *
         * Notes:
         * 1) fgfs needs to be a GUI subsystem app - which it already is
         * 2) What can't be done is to make the cmd prompt run fgfs synchronously;
         * this is only something that can be done via "start /wait fgfs".
         */

        int stderr_handle_type = GetFileType(GetStdHandle(STD_ERROR_HANDLE));
        int stdout_handle_type = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE));
        int stdout_isNull = 0;
        int stderr_isNull = 0;

        m_stderr_isRedirectedAlready = stderr_handle_type == FILE_TYPE_DISK || stderr_handle_type == FILE_TYPE_PIPE || stderr_handle_type == FILE_TYPE_CHAR;
        m_stdout_isRedirectedAlready = stdout_handle_type == FILE_TYPE_DISK || stdout_handle_type == FILE_TYPE_PIPE || stdout_handle_type == FILE_TYPE_CHAR;
        if (m_stderr_isRedirectedAlready) {
            doLogToStderr = true;
        }

        /*
         * We don't want to attach to the console if either stream has been redirected - so in this case ensure that both streams
         * are redirected as otherwise something will be lost (as Alloc or Attach Console will cause the handles that were bound
         * to disappear)
         */
        if (m_stdout_isRedirectedAlready){
			if (!m_stderr_isRedirectedAlready) {
				MessageBox(0, "Redirection only works when you use 2>&1 before using > or |\r\n(e.g. fgfs 2>&1 | more)", "Simgear Error", MB_OK | MB_ICONERROR);
				exit(3);
			}
        } else {
            /*
            * Attempt to attach to the console process of the parent process; when launched from cmd.exe this should be the console,
            * when launched via the RUN menu explorer, or another GUI app that wasn't started from the console this will fail.
            * When it fails we will redirect to the NUL device. This is to ensure that we have valid streams.
            * Later on in the initialisation sequence the --console option will be processed and this will cause the requestConsole() to
            * always open a new console, except for streams that are redirected. The same rules apply there, if both streams are redirected
            * the console will be opened, and it will contain a message to indicate that no output will be present because the streams are redirected
            */
            if (AttachConsole(ATTACH_PARENT_PROCESS) == 0) {
                /*
                * attach failed - so ensure that the streams are bound to the null device - but only when not already redirected
                */
                if (!m_stdout_isRedirectedAlready)
                {
                    stdout_isNull = true;
                    freopen("NUL", "w", stdout);
                }

                if (!m_stderr_isRedirectedAlready)
                {
                    stderr_isNull = true;
                    freopen("NUL", "w", stderr);
                }
            }
            /*
            * providing that AttachConsole succeeded - we can then either reopen the stream onto the console, or use
            * _fdopen to attached to the currently redirected (and open stream)
            */
            if (!stdout_isNull){
                if (!m_stdout_isRedirectedAlready)
                    freopen("conout$", "w", stdout);
                else
                    /*
                    * for already redirected streams we need to attach the stream to the OS handle that is open.
                    * - this comes from part of the answer http://stackoverflow.com/a/13841522
                    *   _open_osfhandle returns an FD for the Win32 Handle, which is then opened using fdopen and
                    *   hopefully safely assigned to the stream (although it does look wrong to me it works)
                    * Removing this bit will stop pipes and command line redirection (> 2> and 2>&1 from working)
                    */
                    *stdout = *_fdopen(_open_osfhandle((intptr_t) GetStdHandle(STD_OUTPUT_HANDLE), _O_WRONLY), "a");
            }

            if (!stderr_isNull){
                if (!m_stderr_isRedirectedAlready)
                    freopen("conout$", "w", stderr);
                else
                    *stderr = *_fdopen(_open_osfhandle((intptr_t) GetStdHandle(STD_ERROR_HANDLE), _O_WRONLY), "a");
            }
        }
        //http://stackoverflow.com/a/25927081
        //Clear the error state for each of the C++ standard stream objects.
        std::wcout.clear();
        std::cout.clear();
        std::wcerr.clear();
        std::cerr.clear();
#endif

        if (doLogToStderr) {
            m_callbacks.push_back(new simgear::StderrLogCallback());
        }

#if defined (SG_WINDOWS)
        const char* winDebugEnv = ::getenv("SG_WINDEBUG");
        const bool b = winDebugEnv ? simgear::strutils::to_bool(std::string{winDebugEnv}) : false;
        if (b) {
            m_callbacks.push_back(new simgear::WinDebugLogCallback());
        }
#endif
        const char* fl = getenv("SG_LOG_FILE_LINE");
        m_fileLine = fl ? simgear::strutils::to_bool(std::string{fl}) : false;
    }

    ~LogStreamPrivate()
    {
        removeCallbacks();

        // house-keeping, avoid leak warnings if we exit before disabling
        // startup logging
        {
            std::lock_guard<std::mutex> g(m_lock);
            clearStartupEntriesLocked();
        }
    }

    std::mutex m_lock;
    SGBlockingQueue<simgear::LogEntry> m_entries;

    // log entries posted during startup
    std::vector<simgear::LogEntry> m_startupEntries;
    bool m_startupLogging = false;

    typedef std::vector<simgear::LogCallback*> CallbackVec;
    CallbackVec m_callbacks;
    /// subset of callbacks which correspond to stdout / console,
	/// and hence should dynamically reflect console logging settings
	CallbackVec m_consoleCallbacks;

    // combined log levels for all callbacks, for fast testing in would_log()
    simgear::LogLevels _combinedLogLevels;

    // this is the 'main' logging levels, set by the command line options
    // or GUI. Individual callbacks can set custom levels
    simgear::LogLevels _consoleLevels;

    bool m_isRunning = false;
#if defined (SG_WINDOWS)
    // track whether the console was redirected on launch (in the constructor, which is called early on)
    bool m_stderr_isRedirectedAlready = false;
    bool m_stdout_isRedirectedAlready = false;
#endif
    bool m_developerMode = false;
    bool m_fileLine = false;

    // test suite mode.
    bool m_testMode = false;

    std::vector<std::string> _popupMessages;

    void startLog()
    {
        std::lock_guard<std::mutex> g(m_lock);
        if (m_isRunning) return;
        m_isRunning = true;
        start();
    }

    void setStartupLoggingEnabled(bool on)
    {
        if (m_startupLogging == on) {
            return;
        }

        {
            std::lock_guard<std::mutex> g(m_lock);
            m_startupLogging = on;
            clearStartupEntriesLocked();
        }
    }

    void clearStartupEntriesLocked()
    {
        m_startupEntries.clear();
    }

    void run() override
    {
        while (1) {
            simgear::LogEntry entry(m_entries.pop());
            // special marker entry detected, terminate the thread since we are
            // making a configuration change or quitting the app
            if ((entry.debugPriority == SG_MANDATORY_INFO) && entry.file && !strcmp(entry.file, "done")) {
                return;
            }
            {
                std::lock_guard<std::mutex> g(m_lock);
                if (m_startupLogging) {
                    // save to the startup list for not-yet-added callbacks to
                    // pull down on startup
                    m_startupEntries.push_back(entry);
                }
            }
            // submit to each installed callback in turn
            for (simgear::LogCallback* cb : m_callbacks) {
                cb->processEntry(entry);
            }
        } // of main thread loop
    }

    bool stop()
    {
        {
            std::lock_guard<std::mutex> g(m_lock);
            if (!m_isRunning) {
                return false;
            }

            // log a special marker value, which will cause the thread to wakeup,
            // and then exit
            log(SG_ALL, SG_MANDATORY_INFO, "done", -1, "", "", false);
        }
        join();

        m_isRunning = false;
        return true;
    }

    void addCallback(simgear::LogCallback* cb)
    {
        PauseThread pause(this);
        m_callbacks.push_back(cb);
        _combinedLogLevels += cb->logLevels();

        // we clear startup entries not using this, so always safe to run
        // this code, container will simply be empty
        for (const auto& entry : m_startupEntries) {
            cb->processEntry(entry);
        }
    }

    void removeCallback(simgear::LogCallback* cb)
    {
        PauseThread pause(this);
        auto it = std::find(m_callbacks.begin(), m_callbacks.end(), cb);
        if (it != m_callbacks.end()) {
            m_callbacks.erase(it);
        }
    }

    void removeCallbacks()
    {
        PauseThread pause(this);
        for (simgear::LogCallback* cb : m_callbacks) {
            delete cb;
        }
        m_callbacks.clear();
    }

    void setLogLevels(sgDebugClass c, sgDebugPriority p, const std::string& tag)
    {
        PauseThread pause(this);
        // we *could* re-generate the combined log levels here,
        // but probably not worth it
        for (auto cb : m_callbacks) {
            if (cb->tag() != tag) {
                continue;
            }

            auto levels = cb->logLevels();
            levels.set(c, p);
            cb->setLogLevels(levels);
            _combinedLogLevels += levels;
        }

        if (tag == "console") {
            _consoleLevels.set(c, p);
        }
    }

    void setLogLevels(const simgear::LogLevels& levels, const std::string& tag)
    {
        if (tag == "console") {
            if (levels == _consoleLevels) {
                // no change, so skip the work. Because we have a tied property on
                // /sim/logigng/classes, this is quite important
                return;
            }
        }

        PauseThread pause(this);
        // we *could* re-generate the combined log levels here,
        // but probably not worth it
        for (auto cb : m_callbacks) {
            if (cb->tag() != tag) {
                continue;
            }

            cb->setLogLevels(levels);
        }

        _combinedLogLevels += levels;
        if (tag == "console") {
            _consoleLevels = levels;
        }
    }


    bool would_log( sgDebugClass c, sgDebugPriority p,
            const char* file, int line, const char* function,
            bool freeFilename ) const
    {
        // Testing mode, so always log.
        if (m_testMode) return true;

        // SG_OSG (OSG notify) - will always be displayed regardless of FG log settings as OSG log level is configured
        // separately and thus it makes more sense to allow these message through.
        if (c == SG_OSG) return true;

        p = translatePriority(p, file, line, function, freeFilename);
        return simgear::shouldLog(c, p, _combinedLogLevels);
    }

    void log( sgDebugClass c, sgDebugPriority p,
            const char* fileName, int line, const char* function,
            const std::string& msg, bool freeFilename)
    {
        auto tp = translatePriority(p, fileName, line, function, freeFilename);
        if (!m_fileLine) {
            /* This prevents output of file:line in StderrLogCallback. */
            line = -line;
        }

        simgear::LogEntry entry(c, tp, p, fileName, line, function, msg, freeFilename);
        m_entries.push(entry);
    }

    sgDebugPriority translatePriority(sgDebugPriority in,
            const char* filename, int line, const char* function,
            bool freeFilename) const
    {
        in = logDeltaAdd(in, filename, line, function, freeFilename);
        if (in == SG_DEV_WARN) {
            return m_developerMode ? SG_WARN : SG_DEBUG;
        }

        if (in == SG_DEV_ALERT) {
            return m_developerMode ? SG_ALERT : SG_WARN;
        }

        return in;
    }
};

/////////////////////////////////////////////////////////////////////////////

static std::unique_ptr<logstream> global_logstream;
static std::mutex global_logStreamLock;

logstream::logstream()
{
    d.reset(new LogStreamPrivate);
    d->startLog();
}

logstream::~logstream()
{
    d->stop();
    d.reset();
}

void logstream::setLogLevels(sgDebugClass c, sgDebugPriority p, const std::string& tag)
{
    d->setLogLevels(c, p, tag);
}

void logstream::setLogLevels(const simgear::LogLevels& levels, const std::string& tag)
{
    d->setLogLevels(levels, tag);
}

simgear::LogLevels logstream::consoleLogLevels() const
{
    return d->_consoleLevels;
}

void logstream::setDeveloperMode(bool devMode)
{
    d->m_developerMode = devMode;
}

bool logstream::inDeveloperMode() const
{
    return d->m_developerMode;
}


void logstream::setFileLine(bool fileLine)
{
    d->m_fileLine = fileLine;
}

void
logstream::addCallback(simgear::LogCallback* cb)
{
    d->addCallback(cb);
}

void
logstream::removeCallback(simgear::LogCallback* cb)
{
    d->removeCallback(cb);
}

void
logstream::log( sgDebugClass c, sgDebugPriority p,
        const char* fileName, int line, const char* function,
        const std::string& msg)
{
    d->log(c, p, fileName, line, function, msg, false);
}

void
logstream::logCopyingFilename( sgDebugClass c, sgDebugPriority p,
         const char* fileName, int line, const char* function,
         const std::string& msg)
{
    d->log(c, p, strdup(fileName), line, strdup(function), msg, true);
}


void logstream::hexdump(sgDebugClass c, sgDebugPriority p,
        const char* fileName, int line, const char* function,
        const void *mem, unsigned int len, unsigned int columns)
{
    unsigned int i, j;
    char temp[3000], temp1[3000];
    *temp = 0;

    for (i = 0; i < len + ((len % columns) ? (columns - len % columns) : 0); i++)
    {
        if (strlen(temp) > 500) return;
        /* print offset */
        if (i % columns == 0)
        {
            snprintf(temp1, 3000, "0x%06x: ", i);
            strcat(temp, temp1);
        }

        /* print hex data */
        if (i < len)
        {
            snprintf(temp1, 3000, "%02x ", 0xFF & ((char*)mem)[i]);
            strcat(temp, temp1);
        }
        else /* end of block, just aligning for ASCII dump */
        {
            strcat(temp, "   ");
        }

        /* print ASCII dump */
        if (i % columns == (columns - 1))
        {
            for (j = i - (columns - 1); j <= i; j++)
            {
                if (j >= len) /* end of block, not really printing */
                {
                    strcat(temp, " ");
                }
                else if (((((char*)mem)[j]) & (char)0x7f) > 32) /* printable char */
                {
                    char t2[2];
                    t2[0] = 0xFF & ((char*)mem)[j];
                    t2[1] = 0;
                    strcat(temp, t2);
                }
                else /* other char */
                {
                    strcat(temp, ".");
                }
            }
            log(c, p, fileName, line, function, temp );
            *temp = 0;
        }
    }
}

void
logstream::popup( const std::string& msg)
{
    std::lock_guard<std::mutex> g(d->m_lock);
    d->_popupMessages.push_back(msg);
}

std::string
logstream::get_popup()
{
    std::string rv;
    std::lock_guard<std::mutex> g(d->m_lock);
    if (!d->_popupMessages.empty()) {
        rv = d->_popupMessages.front();
        d->_popupMessages.erase(d->_popupMessages.begin());
    }
    return rv;
}

bool
logstream::has_popup()
{
    std::lock_guard<std::mutex> g(d->m_lock);
    return !d->_popupMessages.empty();
}

bool
logstream::would_log( sgDebugClass c, sgDebugPriority p,
        const char* file, int line, const char* function,
        bool freeFilename ) const
{
    return d->would_log(c, p, file, line, function, freeFilename);
}

sgDebugPriority
logstream::get_all_log_priority() const
{
    return d->_consoleLevels.levels[SG_ALL];
}

sgDebugPriority logstream::get_log_priority(sgDebugClass c) const
{
    return d->_consoleLevels.levels[c];
}

void logstream::set_log_priority(sgDebugPriority p)
{
    d->setLogLevels(SG_ALL, p, "console");
}

std::string logstream::getLogClassesAsString() const
{
    return d->_consoleLevels.to_string();
}


logstream&
sglog()
{
    // Force initialization of cerr.
    static std::ios_base::Init initializer;

    // http://www.aristeia.com/Papers/DDJ_Jul_Aug_2004_revised.pdf
    // in the absence of portable memory barrier ops in Simgear,
    // let's keep this correct & safe
    std::lock_guard<std::mutex> g(global_logStreamLock);

    if( !global_logstream )
        global_logstream.reset(new logstream);
    return *(global_logstream.get());
}

simgear::LogCallback*
logstream::logToFile(const std::string& tag, const SGPath& aPath)
{
    auto cb = new simgear::FileLogCallback(tag, aPath);
    d->addCallback(cb);
    return cb;
}

void logstream::setStartupLoggingEnabled(bool enabled)
{
    d->setStartupLoggingEnabled(enabled);
}

void logstream::requestConsole(bool ignoreErrors)
{
#if defined (SG_WINDOWS)
    const bool stderrAlreadyRedirected = d->m_stderr_isRedirectedAlready;
    const bool stdoutAlreadyRedirected = d->m_stdout_isRedirectedAlready;

    /*
     * 2016-09-20(RJH) - Reworked console handling
     * This is part of the reworked console handling for Win32. This is for building as a Win32 GUI Subsystem where no
     * console is allocated on launch. If building as a console app then the startup will ensure that a console is created - but
     * we don't need to handle that.
     * The new handling is quite simple:
     *              1. The constructor will ensure that these streams exists. It will attach to the
     *                 parent command prompt if started from the command prompt, otherwise the
     *                 stdout/stderr will be bound to the NUL device.
     *              2. with --console a window will always appear regardless of where the process was
     *                 started from. Any non redirected streams will be redirected
     *              3. You cannot use --console and either redirected stream.
     *
     * This is called after the Private Log Stream constructor so we need to undo any console that it has attached to.
     */

    if (!stderrAlreadyRedirected && !stdoutAlreadyRedirected) {
        FreeConsole();
        if (AllocConsole()) {
            if (!stdoutAlreadyRedirected)
                freopen("conout$", "w", stdout);

            if (!stderrAlreadyRedirected)
                freopen("conout$", "w", stderr);

            //http://stackoverflow.com/a/25927081
            //Clear the error state for each of the C++ standard stream objects.
            std::wcout.clear();
            std::cout.clear();
            std::wcerr.clear();
            std::cerr.clear();
        }
    } else {
        if (!ignoreErrors)
            MessageBox(0, "--console ignored because stdout or stderr redirected with > or 2>", "Simgear Error", MB_OK | MB_ICONERROR);
    }
#endif
}

void
logstream::setTestingMode( bool testMode )
{
    d->m_testMode = testMode;
    if (testMode) d->removeCallbacks();
}

void logstream::disableStderrLogging()
{
    global_disableStderrLogging = true;
}

namespace simgear
{

void requestConsole(bool ignoreErrors)
{
    sglog().requestConsole(ignoreErrors);
}


void shutdownLogging()
{
    std::lock_guard<std::mutex> g(global_logStreamLock);
    global_logstream.reset();
}

} // of namespace simgear
