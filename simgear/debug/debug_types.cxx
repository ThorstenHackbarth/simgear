// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1998 Bernie Bright <bbright@c031.aone.net.au>

#include <simgear_config.h>

#include <algorithm>
#include <iostream>
#include <sstream>

#include <simgear/debug/debug_types.h>
#include <simgear/misc/strutils.hxx>
#include <simgear/structure/exception.hxx>

namespace {

static const std::vector<std::string> global_priorityNames = {
    "UNKN",
    "BULK",
    "DBUG",
    "INFO",
    "WARN",
    "ALRT",
    "POPU",
    "WARN",
    "ALRT",
    "INFO",
    "OFF"};

struct LogClassMapping {
    const sgDebugClass c;
    const std::string name;
    const std::vector<std::string> aliases;

    LogClassMapping(sgDebugClass cc, const std::string& n, const std::vector<std::string>& al = {}) : c(cc),
                                                                                                      name(n),
                                                                                                      aliases(al) {};
};

const std::initializer_list<LogClassMapping> log_class_mappings = {
    LogClassMapping(SG_ALL, "all"),
    LogClassMapping(SG_TERRAIN, "terrain"),
    LogClassMapping(SG_ASTRO, "astro"),
    LogClassMapping(SG_FLIGHT, "flight"),
    LogClassMapping(SG_INPUT, "input"),
    LogClassMapping(SG_GL, "gl", {"opengl"}),
    LogClassMapping(SG_VIEW, "view"),
    LogClassMapping(SG_COCKPIT, "cockpit"),
    LogClassMapping(SG_GENERAL, "general"),
    LogClassMapping(SG_MATH, "math"),
    LogClassMapping(SG_EVENT, "event"),
    LogClassMapping(SG_AIRCRAFT, "aircraft"),
    LogClassMapping(SG_AUTOPILOT, "autopilot"),
    LogClassMapping(SG_IO, "io"),
    LogClassMapping(SG_CLIPPER, "clipper"),
    LogClassMapping(SG_NETWORK, "network"),
    LogClassMapping(SG_INSTR, "instrumentation", {"instruments"}),
    LogClassMapping(SG_ATC, "atc"),
    LogClassMapping(SG_NASAL, "nasal"),
    LogClassMapping(SG_SYSTEMS, "systems"),
    LogClassMapping(SG_AI, "ai"),
    LogClassMapping(SG_ENVIRONMENT, "environment"),
    LogClassMapping(SG_SOUND, "sound"),
    LogClassMapping(SG_NAVAID, "navaid"),
    LogClassMapping(SG_GUI, "gui"),
    LogClassMapping(SG_TERRASYNC, "terrasync"),
    LogClassMapping(SG_PARTICLES, "particles"),
    LogClassMapping(SG_HEADLESS, "headless"),
    LogClassMapping(SG_OSG, "osg", {"openscenegraph"}),

}; // namespace

} // namespace


const std::string& debugClassToString(sgDebugClass c)
{
    auto it = std::find_if(log_class_mappings.begin(), log_class_mappings.end(), [c](const LogClassMapping& lm) {
        return lm.c == c;
    });

    // return 'none'
    if (it == log_class_mappings.end()) {
        return log_class_mappings.begin()->name;
    }

    return it->name;
}

const std::string& debugPriorityToString(sgDebugPriority p)
{
    if (p == SG_LOG_PRIORITY_DISABLED) {
        return global_priorityNames.back();
    }

    if (static_cast<int>(p) >= global_priorityNames.size()) {
        return global_priorityNames.at(0);
    }

    return global_priorityNames.at(static_cast<int>(p));
}

namespace simgear {

LogLevels::LogLevels()
{
    // default priority
    levels.fill(SG_UNSET_LOG_PRIORITY);
    levels[SG_ALL] = SG_LOG_PRIORITY_DISABLED;
}

std::string LogLevels::to_string() const
{
    std::ostringstream os;
    os << "all=" << debugPriorityToString(levels[SG_ALL]);
    // syntax here is deliberately compatible with parseLogSpecFromString
    for (int i = 1; i < SG_MAX_LOG_CLASS; ++i) {
        if (levels[i] != SG_UNSET_LOG_PRIORITY) {
            os << "," << debugClassToString(static_cast<sgDebugClass>(i)) << "=" << debugPriorityToString(levels[i]);
        }
    }
    return os.str();
}

sgDebugPriority LogLevels::get(sgDebugClass c) const
{
    const auto p = levels[static_cast<int>(c)];
    return p == SG_UNSET_LOG_PRIORITY ? levels[SG_ALL] : p;
}


bool shouldLog(sgDebugClass c, sgDebugPriority p, const LogLevels& logLevels)
{
    return logLevels.get(c) <= p;
}

void setAllClassesPriority(sgDebugPriority p, LogLevels& logLevels)
{
    logLevels.levels[SG_ALL] = p;
}

// combine log-levels, so that the more verbose of each is used.
LogLevels& operator+=(LogLevels& a, const LogLevels& b)
{
    for (int i = 0; i < SG_MAX_LOG_CLASS; ++i) {
        auto& l = a.levels[i];
        if (l == SG_UNSET_LOG_PRIORITY) {
            l = b.levels[i];
        } else if (b.levels[i] == SG_UNSET_LOG_PRIORITY) {
            // nothing to do
        } else {
            l = std::min(l, b.levels[i]);
        }
    }
    return a;
}

bool operator==(const LogLevels& a, const LogLevels& b)
{
    return a.levels == b.levels;
}

void setLogLevelFromString(LogLevels& a, const std::string& spec)
{
    // convert to lowercase?

    const auto sep = spec.find('=');
    if (sep == std::string::npos) {
        a.levels[SG_ALL] = priorityFromString(spec);
        return;
    }

    const auto classPart = spec.substr(0, sep);
    const auto priorityPart = spec.substr(sep + 1);

    const auto c = debugClassFromString(classPart);
    const auto pri = priorityFromString(priorityPart);

    a.levels[static_cast<int>(c)] = pri;
}

std::optional<LogLevels> parseLogSpecFromString(const std::string& spec)
{
    using namespace strutils;
    const auto lcSpec = lowercase(spec);

    try {
        LogLevels result;
        if (lcSpec.empty() || (lcSpec == "all")) {
            // what should this do?
        } else if (lcSpec == "none") {
            result.levels[SG_ALL] = SG_MANDATORY_INFO;
        } else {
            // merge each piece in turn
            const auto pieces = split_on_any_of(lcSpec, "|,");
            for (auto p : pieces) {
                setLogLevelFromString(result, p);
            }
        }

        return result;
    } catch (std::exception& e) {
        // don't use SG_LOG here :)
        std::cerr << "error parsing log classes '" << spec << "':" << e.what() << std::endl;
        return std::nullopt;
    }
}

sgDebugPriority priorityFromString(const std::string& s)
{
    const auto ls = strutils::lowercase(s);
    if (ls == "bulk") return SG_BULK;
    if (ls == "debug") return SG_DEBUG;
    if (ls == "info") return SG_INFO;
    if (ls == "warn") return SG_WARN;
    if (ls == "alert") return SG_ALERT;
    if ((ls == "none") || (ls == "off")) return SG_LOG_PRIORITY_DISABLED;

    // check the 'output' names so we can round-trip /sim/logging/
    // these are upper-case, <sigh>
    const auto us = strutils::uppercase(s);
    auto it = std::find(global_priorityNames.begin(), global_priorityNames.end(), us);
    if (it != global_priorityNames.end()) {
        return static_cast<sgDebugPriority>(std::distance(global_priorityNames.begin(), it));
    }

    throw std::invalid_argument("Couldn't parse log priority:" + s);
}

string_list logCategoryNames()
{
    string_list result;
    result.reserve(SG_MAX_LOG_CLASS);

    for (const auto& mapping : log_class_mappings) {
        result.push_back(mapping.name);
    }
    return result;
}

sgDebugClass debugClassFromString(const std::string& us)
{
    const auto s = strutils::lowercase(us);
    for (const auto& mapping : log_class_mappings) {
        if (mapping.name == s || std::find(mapping.aliases.begin(), mapping.aliases.end(), s) != mapping.aliases.end()) {
            return mapping.c;
        }
    }

    throw std::invalid_argument("Couldn't parse log class:" + s);
}

} // namespace simgear
