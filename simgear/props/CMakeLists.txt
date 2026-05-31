# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
    AtomicChangeListener.hxx
    condition.hxx
    easing_functions.hxx
    ExtendedPropertyAdapter.hxx
    PropertyBasedElement.hxx
    PropertyBasedMgr.hxx
    PropertyInterpolationMgr.hxx
    PropertyInterpolator.hxx
    propertyObject.hxx
    props.hxx
    props_io.hxx
    propsfwd.hxx
    tiedpropertylist.hxx
    vectorPropTemplates.hxx
    )

target_sources(SimGearCore PRIVATE
    AtomicChangeListener.cxx
    condition.cxx
    easing_functions.cxx
    PropertyBasedElement.cxx
    PropertyBasedMgr.cxx
    PropertyInterpolationMgr.cxx
    PropertyInterpolator.cxx
    propertyObject.cxx
    props.cxx
    props_io.cxx
    ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/props)

if(ENABLE_TESTS)

add_simgear_autotest(test_props props_test.cxx)
add_simgear_autotest(test_propertyObject propertyObject_test.cxx)
add_simgear_autotest(test_easing_functions easing_functions_test.cxx)

endif(ENABLE_TESTS)

add_simgear_benchmark(bench_props bench_props.cxx)
