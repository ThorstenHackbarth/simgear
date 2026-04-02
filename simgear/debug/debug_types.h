// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2001 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Enums used in debug macros
 */

#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

/** \file debug_types.h
 *  Define the various logging classes and priorities
 */

/**
 * Define the possible classes/categories of logging messages
 */
typedef enum {
    SG_ALL = 0, // Special value to indicate all classes
    SG_GENERAL = 1,
    SG_TERRAIN,
    SG_ASTRO,
    SG_FLIGHT,
    SG_INPUT,
    SG_GL,
    SG_VIEW,
    SG_COCKPIT,

    SG_MATH,
    SG_EVENT,
    SG_AIRCRAFT,
    SG_AUTOPILOT,
    SG_IO,
    SG_CLIPPER,
    SG_NETWORK,
    SG_ATC,
    SG_NASAL,
    SG_INSTR,
    SG_SYSTEMS,
    SG_AI,
    SG_ENVIRONMENT,
    SG_SOUND,
    SG_NAVAID,
    SG_GUI,
    SG_TERRASYNC,
    SG_PARTICLES,
    SG_HEADLESS,
    // SG_OSG (OSG notify) - will always be displayed regardless of FG log settings as OSG log level is configured
    // separately and thus it makes more sense to allow these message through.
    SG_OSG,

    SG_MAX_LOG_CLASS
} sgDebugClass;


/**
 * Define the possible logging priorities (and their order).
 *
 * Caution - unfortunately, this enum is exposed to Nasal via the logprint()
 * function as an integer parameter. Therefore, new values should only be
 * appended, or the priority Nasal reports to compiled code will change.
 */
typedef enum {
    SG_UNSET_LOG_PRIORITY = 0,
    SG_BULK = 1, // For frequent messages
    SG_DEBUG,    // Less frequent debug type messages
    SG_INFO,     // Informatory messages
    SG_WARN,     // Possible impending problem
    SG_ALERT,    // Very possible impending problem
    SG_POPUP,    // Severe enough to alert using a pop-up window

    SG_DEV_WARN,  // Warning for developers, translated to other priority
    SG_DEV_ALERT, // Alert for developers, translated

    SG_MANDATORY_INFO, // information, but should always be shown

    SG_LOG_PRIORITY_DISABLED = 255,
} sgDebugPriority;

// implemented in logstream.cxx

const std::string& debugClassToString(sgDebugClass c);
const std::string& debugPriorityToString(sgDebugPriority p);

namespace simgear {
// value struct for storing levels
struct LogLevels {
    LogLevels();

    std::string to_string() const;

    std::array<sgDebugPriority, SG_MAX_LOG_CLASS> levels;

    void set(sgDebugClass c, sgDebugPriority p)
    {
        levels[static_cast<int>(c)] = p;
    }

    sgDebugPriority get(sgDebugClass c) const;
};

bool shouldLog(sgDebugClass c, sgDebugPriority p, const LogLevels& logLevels);

void setAllClassesPriority(sgDebugPriority p, LogLevels& logLevels);

/**
     * @brief all the category names. Order will match sgDebugClass
     *
     * @return vector of strings
     */
std::vector<std::string> logCategoryNames();

// combine log-levels, so that the more verbose of each is used.
LogLevels& operator+=(LogLevels& a, const LogLevels& b);

bool operator==(const LogLevels& a, const LogLevels& b);

void setLogLevelFromString(LogLevels& a, const std::string& spec);

/**
        @brief convert a string value to a log priority.
        throws std::invalid_argument if the string is not valid
     */
sgDebugPriority priorityFromString(const std::string& s);

sgDebugClass debugClassFromString(const std::string& s);

std::optional<LogLevels> parseLogSpecFromString(const std::string& spec);
} // namespace simgear
