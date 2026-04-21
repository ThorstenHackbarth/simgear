# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2018 Thomas Geymayer

set(HEADERS
  integer_sequence.hxx
  type_traits.hxx
)

target_sources(SimGearCore PRIVATE ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/std)

if(ENABLE_TESTS)
  add_simgear_autotest(test_integer_sequence integer_sequence_test.cxx)
  add_simgear_autotest(test_type_traits type_traits_test.cxx)
endif()
