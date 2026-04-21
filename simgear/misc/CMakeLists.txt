# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    CSSBorder.hxx
    ListDiff.hxx
    ResourceManager.hxx
    SVGpreserveAspectRatio.hxx
    argparse.hxx
    inputvalue.hxx
    inputcolor.hxx
    make_new.hxx
    sg_dir.hxx
    sg_path.hxx
    stdint.hxx
    strutils.hxx
    test_macros.hxx
    )

set(SOURCES
    CSSBorder.cxx
    ResourceManager.cxx
    SVGpreserveAspectRatio.cxx
    argparse.cxx
    inputvalue.cxx
    inputcolor.cxx
    interpolator.cxx
    sg_dir.cxx
    sg_path.cxx
    sg_hash.cxx
    strutils.cxx
    tabbed_values.cxx
    texcoord.cxx
    )

if (WINDOWS)
    target_sources(SimGearCore PRIVATE sgversionhelpers.hxx)
endif()

if (APPLE)
    list(APPEND SOURCES CocoaHelpers.mm)
endif()

target_sources(SimGearCore PRIVATE ${SOURCES}
    interpolator.hxx
    sg_hash.hxx
    stopwatch.hxx
    tabbed_values.hxx
    texcoord.hxx
    lru_cache.hxx
    ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/misc)

if(ENABLE_TESTS)

add_simgear_autotest(test_argparse argparse_test.cxx)
add_simgear_autotest(test_CSSBorder CSSBorder_test.cxx)
add_simgear_autotest(test_tabbed_values tabbed_values_test.cxx)
add_simgear_autotest(test_strutils strutils_test.cxx)
add_simgear_autotest(test_path path_test.cxx )
add_simgear_autotest(test_sg_dir sg_dir_test.cxx)

endif(ENABLE_TESTS)

add_boost_test(SVGpreserveAspectRatio
  SOURCES SVGpreserveAspectRatio_test.cxx
  LIBRARIES SimGearCore
)

add_boost_test(utf8tolatin1
  SOURCES utf8tolatin1_test.cxx
  LIBRARIES SimGearCore
)
