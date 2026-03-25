# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: James Turner <james@flightgear.org>
if (NOT SYSTEM_EXPAT)
	add_subdirectory(expat)
endif()

if (NOT SYSTEM_UDNS)
	if(NOT c-ares_FOUND)
		add_subdirectory(udns)
	endif()
endif()

add_subdirectory(tiny_gltf)
add_subdirectory(glcore)
add_subdirectory(thinks_fmm)
