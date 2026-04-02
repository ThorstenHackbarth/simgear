# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

include (SimGearComponent)

set(HEADERS
    debug_types.h
    logstream.hxx BufferedLogCallback.hxx
    LogCallback.hxx LogEntry.hxx
    ErrorReportingCallback.hxx logdelta.hxx
    Reporting.hxx)

set(SOURCES
    debug_types.cxx
    logstream.cxx BufferedLogCallback.cxx
    LogCallback.cxx LogEntry.cxx logdelta.cxx
    ErrorReportingCallback.cxx
    Reporting.cxx
    )

simgear_component(debug debug "${SOURCES}" "${HEADERS}")
