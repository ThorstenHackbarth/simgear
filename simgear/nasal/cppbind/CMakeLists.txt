# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.org>

set(HEADERS
  cppbind_fwd.hxx
  Ghost.hxx
  NasalCallContext.hxx
  NasalCode.hxx
  NasalContext.hxx
  NasalHash.hxx
  NasalMe.hxx
  NasalMethodHolder.hxx
  NasalObject.hxx
  NasalObjectHolder.hxx
  NasalString.hxx
  from_nasal.hxx
  to_nasal.hxx
)

set(DETAIL_HEADERS
  detail/from_nasal_helper.hxx
  detail/nasal_traits.hxx
  detail/to_nasal_helper.hxx
)

target_sources(SimGearCore PRIVATE
  Ghost.cxx
  NasalCode.cxx
  NasalContext.cxx
  NasalHash.cxx
  NasalString.cxx
  NasalObject.cxx
  detail/from_nasal_helper.cxx
  detail/to_nasal_helper.cxx
  ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/nasal/cppbind)
target_sources(SimGearCore PRIVATE ${DETAIL_HEADERS})
install(FILES ${DETAIL_HEADERS} DESTINATION include/simgear/nasal/cppbind/detail)

add_simgear_autotest(cppbind_ghost test/cppbind_test_ghost.cxx)
add_simgear_autotest(cppbind_misc test/cppbind_test.cxx)
add_simgear_autotest(nasal_gc_test test/nasal_gc_test.cxx)
add_simgear_autotest(nasal_num test/nasal_num_test.cxx)
