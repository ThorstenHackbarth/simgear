# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    CSSBorder.hxx
    ListDiff.hxx
    ResourceManager.hxx
    SVGpreserveAspectRatio.hxx
    argparse.hxx
    hash_utils.hxx
    inputvalue.hxx
    inputcolor.hxx
    make_new.hxx
    sg_dir.hxx
    sg_path.hxx
    stdint.hxx
    strutils.hxx
    test_macros.hxx
    type_utils.hxx
    )

set(SOURCES
    CSSBorder.cxx
    ResourceManager.cxx
    SVGpreserveAspectRatio.cxx
    argparse.cxx
    inputvalue.cxx
    inputcolor.cxx
    sg_dir.cxx
    sg_path.cxx
    sg_hash.cxx
    strutils.cxx
    texcoord.cxx
    )

if (WINDOWS)
    target_sources(SimGearCore PRIVATE sgversionhelpers.hxx)
endif()

if (APPLE)
    list(APPEND SOURCES CocoaHelpers.mm)
endif()

target_sources(SimGearCore PRIVATE ${SOURCES}
    sg_hash.hxx
    texcoord.hxx
    lru_cache.hxx
    ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/misc)

if(ENABLE_TESTS)

add_simgear_autotest(test_argparse argparse_test.cxx)
add_simgear_autotest(test_CSSBorder CSSBorder_test.cxx)
add_simgear_autotest(test_strutils strutils_test.cxx)
add_simgear_autotest(test_path path_test.cxx )
add_simgear_autotest(test_sg_dir sg_dir_test.cxx)
add_simgear_autotest(test_hash_utils hash_utils_test.cxx)
add_simgear_autotest(test_type_utils type_utils_test.cxx)

endif(ENABLE_TESTS)

add_simgear_autotest(SVGpreserveAspectRatio SVGpreserveAspectRatio_test.cxx)
add_simgear_autotest(utf8tolatin1 utf8tolatin1_test.cxx)
