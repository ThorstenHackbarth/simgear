# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: James Turner <james@flightgear.org>

set(HEADERS
    Catalog.hxx
    Package.hxx
    Install.hxx
    Root.hxx
    Delegate.hxx
    PackageCommon.hxx
    )

target_sources(SimGearCore PRIVATE
    Catalog.cxx
    Package.cxx
    Install.cxx
    Root.cxx
    Delegate.cxx
# internal helpers
    md5.h md5.c
    ioapi.c ioapi.h
    unzip.h unzip.c
    unzip_buffer.hxx unzip_buffer.cxx
    ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/package)

if(ENABLE_PKGUTIL)
  add_simgear_test(sg_pkgutil pkgutil.cxx)
endif()

if(ENABLE_TESTS)

add_simgear_autotest(catalog_test CatalogTest.cxx)
target_link_libraries(catalog_test httplib::httplib)
set_target_properties(catalog_test PROPERTIES
        COMPILE_DEFINITIONS "SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"" )

endif(ENABLE_TESTS)
