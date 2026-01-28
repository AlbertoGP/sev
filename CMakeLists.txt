cmake_minimum_required(VERSION 3.16)
project(sdl-clay-setup C)

# ------------------------------------------------------------
# Sources
# ------------------------------------------------------------
set(SRC_FILES
    src/init.c
    src/iterate.c
    src/event.c
    src/quit.c
    src/keyboard.c
    src/keymap.c
    src/clay/renderer.c
    src/clay/init.c
    src/layout/layout.c
    src/layout/pane.c
    src/layout/status.c
    src/layout/tab.c
    src/subeditor/buffer.c
    src/subeditor/gap.c
    src/subeditor/line.c
    src/subeditor/mark.c
    src/subeditor/message.c
    src/scheme/scheme.c
)

# Create executable
add_executable(app ${SRC_FILES})

# ------------------------------------------------------------
# Platform-specific configuration
# ------------------------------------------------------------
if(EMSCRIPTEN)
    message(STATUS "Configuring for Emscripten / WASM")

    # Set output suffix to .html
    set_target_properties(app PROPERTIES SUFFIX ".html")

    # Preload resources folder and chibi-scheme lib (required for standard env)
    set(EMCC_ASSETS
        "--preload-file ${CMAKE_SOURCE_DIR}/resources@/resources"
        "--preload-file ${CMAKE_SOURCE_DIR}/vendored/chibi-scheme/lib@/lib"
    )
    list(JOIN EMCC_ASSETS " " EMCC_ASSETS_STR)
    set_target_properties(app PROPERTIES LINK_FLAGS "${EMCC_ASSETS_STR}")

    # Add vendored SDL libraries
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
    set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
    add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)

    # Explicitly propagate include directories
    target_include_directories(app PUBLIC
        vendored/SDL/include
        vendored/SDL_ttf/include
    )
    
    # Build native bootstrap first (clear emscripten env vars)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/native-chibi
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env --unset=CC --unset=CXX --unset=AR --unset=RANLIB
            ${CMAKE_COMMAND}
            ${CMAKE_SOURCE_DIR}/vendored/chibi-scheme
            -DCMAKE_BUILD_TYPE=Release
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/native-chibi
        RESULT_VARIABLE bootstrap_config_result
    )
    if(NOT bootstrap_config_result EQUAL 0)
        message(FATAL_ERROR "Failed to configure native chibi-scheme bootstrap")
    endif()
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build . --target chibi-scheme-bootstrap
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/native-chibi
        RESULT_VARIABLE bootstrap_build_result
    )
    if(NOT bootstrap_build_result EQUAL 0)
        message(FATAL_ERROR "Failed to build native chibi-scheme bootstrap")
    endif()

    set(CHIBI_BOOTSTRAP_EXECUTABLE "${CMAKE_BINARY_DIR}/native-chibi/chibi-scheme-bootstrap" CACHE FILEPATH "" FORCE)
    
    add_subdirectory(
        vendored/chibi-scheme
        ${CMAKE_BINARY_DIR}/chibi-build
        EXCLUDE_FROM_ALL
    )

    # Link the libraries
    target_link_libraries(app PRIVATE
        SDL3::SDL3-static
        SDL3_ttf::SDL3_ttf-static
        chibi::libchibi-scheme
    )

else()
    # ------------------------------------------------------------
    # Desktop build (Linux)
    # ------------------------------------------------------------
    message(STATUS "Configuring for desktop build")

    # Copy resources folder for desktop builds
    add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:app>/resources
    )

    # Use system SDL3 packages
    find_package(SDL3 REQUIRED)
    find_package(SDL3_ttf REQUIRED)

    target_link_libraries(app PUBLIC SDL3::SDL3 SDL3_ttf::SDL3_ttf)

    add_subdirectory(vendored/chibi-scheme)
    target_link_libraries(app PRIVATE chibi::libchibi-scheme)

endif()
