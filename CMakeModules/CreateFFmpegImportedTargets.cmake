# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: James Turner <james@flightgear.org>

# helper function to find botht the static or import library and the DLL,
# needed on windows
function(find_ffmpeg_library retval basename)

    find_library(imported_fflib_${basename}
        NAMES ${basename}
        PATH_SUFFIXES lib
    )

    if (imported_fflib_${basename})
        set(${retval} ${imported_fflib_${basename}} PARENT_SCOPE)
    else()
        message(STATUS "Couldn't find link library for ${basename}")
        return()
    endif()

    if (MSVC)
        find_library(imported_ffdll_${basename}
            NAMES ${basename}
            PATH_SUFFIXES bin
        )

        if (imported_ffdll_${basename})
            set(${retval}_dll ${imported_ffdll_${basename}} PARENT_SCOPE)
        endif()
    endif()

endfunction()


set (Simgear_FFmpeg_Components avcodec avformat avutil swscale)

foreach (comp ${Simgear_FFmpeg_Components})
    string(TOUPPER ${comp} _ffmpeg_module_UC)
    set(tgt imported_${_ffmpeg_module_UC})
    add_library(${tgt} SHARED IMPORTED GLOBAL)
    add_library(FFmpeg::${comp} ALIAS ${tgt})

    find_ffmpeg_library(releaseLib ${comp})

    set_property(TARGET ${tgt} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${${_ffmpeg_module_UC}_INCLUDE_DIRS})

# no worrying about debug/RelWithDbgInfo support for the moment,
# 99%, FFmpeg is built in release mode
    if (MSVC)
        set_property(TARGET ${tgt} PROPERTY IMPORTED_IMPLIB ${releaseLib})
        set_property(TARGET ${tgt} PROPERTY IMPORTED_LOCATION ${releaseLib_dll})
    else()
        set_property(TARGET ${tgt} PROPERTY IMPORTED_LOCATION ${releaseLib})
    endif()
endforeach()
