cmake_minimum_required(VERSION 3.16)
project(text-scale VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find OBS installation or fallback to local headers
if(NOT DEFINED OBS_INCLUDE_DIR)
    set(OBS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/obs-src/libobs")
endif()

include_directories(
    src
    ${OBS_INCLUDE_DIR}
)

if(DEFINED OBS_LIB_DIR)
    link_directories(${OBS_LIB_DIR})
endif()

# Plugin Module
add_library(text-scale MODULE
    src/plugin-main.cpp
    src/text-scale-filter.cpp
    src/text-measurement.cpp
)

set_target_properties(text-scale PROPERTIES
    PREFIX ""
    OUTPUT_NAME "text-scale"
)

if(WIN32)
    target_link_libraries(text-scale PRIVATE
        obs
        gdiplus
    )
    if(MINGW)
        target_link_options(text-scale PRIVATE
            -static-libgcc
            -static-libstdc++
        )
    endif()
elseif(APPLE)
    target_link_options(text-scale PRIVATE
        -Wl,-undefined,dynamic_lookup
    )
    set_target_properties(text-scale PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "plugin"
    )
else() # Linux
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBOBS REQUIRED libobs)
    target_include_directories(text-scale PRIVATE ${LIBOBS_INCLUDE_DIRS})
    target_link_libraries(text-scale PRIVATE ${LIBOBS_LIBRARIES})
endif()

# Unit tests executable
add_executable(test_math
    tests/test_math.cpp
)
if(MINGW)
    target_link_options(test_math PRIVATE -static -static-libgcc -static-libstdc++)
endif()

# Output package directory layout
add_custom_command(TARGET text-scale POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/output/bin/64bit"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/output/data"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:text-scale>" "${CMAKE_CURRENT_BINARY_DIR}/output/bin/64bit/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" "${CMAKE_CURRENT_BINARY_DIR}/output/data"
)

# Target to install directly to OBS Studio User plugins folder
set(OBS_PLUGIN_INSTALL_DIR "$ENV{APPDATA}/obs-studio/plugins/text-scale")

add_custom_target(install_to_obs
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OBS_PLUGIN_INSTALL_DIR}/bin/64bit"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OBS_PLUGIN_INSTALL_DIR}/data"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:text-scale>" "${OBS_PLUGIN_INSTALL_DIR}/bin/64bit/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data" "${OBS_PLUGIN_INSTALL_DIR}/data"
    COMMENT "Installing TextScale to OBS user directory: ${OBS_PLUGIN_INSTALL_DIR}"
    DEPENDS text-scale
)
