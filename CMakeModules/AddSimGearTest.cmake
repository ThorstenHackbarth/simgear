
function(add_simgear_test _name _sources)
    add_executable(${_name} ${_sources})
    target_link_libraries(${_name} SimGearCore Threads::Threads)

    # for simgear_config.h
    target_include_directories(${_name} PRIVATE ${PROJECT_BINARY_DIR}/simgear)
endfunction()


function(add_simgear_autotest _name _sources)    
    add_executable(${_name} ${_sources})
    target_link_libraries(${_name} SimGearCore Threads::Threads)

    # for simgear_config.h
    target_include_directories(${_name} PRIVATE ${PROJECT_BINARY_DIR}/simgear)

    add_test(${_name} ${EXECUTABLE_OUTPUT_PATH}/${_name})
    set_tests_properties(${_name} PROPERTIES TIMEOUT 30) 
endfunction()


function(add_simgear_scene_autotest _name _sources)
    # No-op when the scene library is not being built (headless / wicked backend
    # until the wicked scene port lands).
    if(NOT TARGET SimGearScene)
        return()
    endif()
    add_executable(${_name} ${_sources})
    target_link_libraries(${_name} SimGearScene Threads::Threads)

    # for simgear_config.h
    target_include_directories(${_name} PRIVATE ${PROJECT_BINARY_DIR}/simgear)

    add_test(${_name} ${EXECUTABLE_OUTPUT_PATH}/${_name})
endfunction()


# Test registration for the OSG-to-Wicked migration. Tests added with this
# macro carry the 'migration' label so the per-commit run-tests.sh runs them
# as a focused subset (`ctest -L migration`). Tests run on every backend that
# provides SimGearCore, since the migration tests exercise backend-neutral
# code such as opaque handles, geo↔ECEF conversions, and the property bridge.
function(add_simgear_wicked_autotest _name _sources)
    add_executable(${_name} ${_sources})
    target_link_libraries(${_name} SimGearCore Threads::Threads)

    # for simgear_config.h
    target_include_directories(${_name} PRIVATE ${PROJECT_BINARY_DIR}/simgear)

    add_test(${_name} ${EXECUTABLE_OUTPUT_PATH}/${_name})
    set_tests_properties(${_name} PROPERTIES
        TIMEOUT 30
        LABELS  migration)
endfunction()
