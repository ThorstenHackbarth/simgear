# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    debug_types.h
    logstream.hxx BufferedLogCallback.hxx
    LogCallback.hxx LogEntry.hxx
    ErrorReportingCallback.hxx logdelta.hxx
    Reporting.hxx)

target_sources(SimGearCore PRIVATE
    debug_types.cxx
    logstream.cxx BufferedLogCallback.cxx
    LogCallback.cxx LogEntry.cxx logdelta.cxx
    ErrorReportingCallback.cxx
    Reporting.cxx
    ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/debug)
