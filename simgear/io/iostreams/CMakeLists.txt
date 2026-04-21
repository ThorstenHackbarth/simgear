# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    sgstream.hxx
    gzfstream.hxx
    gzcontainerfile.hxx
    CharArrayStream.hxx
    zlibstream.hxx
    )

target_sources(SimGearCore PRIVATE
    sgstream.cxx
    gzfstream.cxx
    gzcontainerfile.cxx
    CharArrayStream.cxx
    zlibstream.cxx
    ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/io/iostreams)

if(ENABLE_TESTS)
  add_simgear_autotest(test_streams sgstream_test.cxx )
  add_simgear_autotest(test_CharArrayStream CharArrayStream_test.cxx)
  add_simgear_autotest(test_zlibstream zlibstream_test.cxx)
endif(ENABLE_TESTS)
