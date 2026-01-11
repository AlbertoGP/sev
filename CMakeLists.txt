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
    src/layout/layout.c
    src/layout/toggle_button.c
    src/layout/current_mode.c
    src/subeditor/buffer.c
    src/subeditor/gap.c
    src/subeditor/mark.c
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

    # Preload resources folder
    set(EMCC_ASSETS "--preload-file ${CMAKE_SOURCE_DIR}/resources@/resources")
    set_target_properties(app PROPERTIES LINK_FLAGS "${EMCC_ASSETS}")

    # Add vendored SDL libraries
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
    set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
    add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)

    # Explicitly propagate include directories
    target_include_directories(app PUBLIC
        vendored/SDL/include
        vendored/SDL_ttf/include
    )
    
    add_subdirectory(
        vendored/chibi-scheme
        ${CMAKE_BINARY_DIR}/chibi-build
        EXCLUDE_FROM_ALL
    )

    # Link the libraries
    target_link_libraries(app PRIVATE
        SDL3
        SDL3_ttf::SDL3_ttf
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
