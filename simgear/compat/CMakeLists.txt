# SPDX-License-Identifier: LGPL-2.1-or-later
# Unit tests for simgear/compat/ replacement headers (used when SIMGEAR_NO_BOOST=ON)

include(SimGearComponent)

set(HEADERS
    optional.hxx
    functional_hash.hxx
    tokenizer.hxx
    call_traits.hxx
    iterator_facade.hxx
    iterator_adaptor.hxx
    algorithm_string.hxx
)

simgear_component(compat compat "" "${HEADERS}")

if(ENABLE_TESTS)
    add_simgear_autotest(test_compat_optional     optional_test.cxx)
    add_simgear_autotest(test_compat_hash         functional_hash_test.cxx)
    add_simgear_autotest(test_compat_tokenizer    tokenizer_test.cxx)
    add_simgear_autotest(test_compat_call_traits  call_traits_test.cxx)
    add_simgear_autotest(test_compat_iterator     iterator_test.cxx)
endif()
