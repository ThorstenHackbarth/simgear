# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2019 Richard Harrison

set(HEADERS
    Emesary.hxx
    INotification.hxx
    IReceiver.hxx
    ITransmitter.hxx
    ReceiptStatus.hxx
    Transmitter.hxx
    notifications.hxx
    )

target_sources(SimGearCore PRIVATE Emesary.cxx ${HEADERS})
install(FILES ${HEADERS} DESTINATION include/simgear/emesary)


if(ENABLE_TESTS)

add_simgear_autotest(test_emesary test_emesary.cxx)
set_target_properties(test_emesary PROPERTIES
  COMPILE_DEFINITIONS "SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"" )

endif(ENABLE_TESTS)
