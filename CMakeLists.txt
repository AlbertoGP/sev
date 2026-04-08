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
    src/state_io.c
    src/clay/renderer.c
    src/clay/init.c
    src/command/keyboard.c
    src/command/keymap.c
    src/command/macro.c
    src/command/message.c
    src/command/minibuf.c
    src/command/mode.c
    src/command/scheme.c
    src/display/cursor.c
    src/display/decoration.c
    src/display/icon.c
    src/display/layout.c
    src/display/message.c
    src/display/minibuf.c
    src/display/mode_icon.c
    src/display/buf_render.c
    src/display/pane.c
    src/display/scale.c
    src/display/welcome.c
    src/display/status.c
    src/display/tab.c
    src/display/theme.c
    src/display/tooltip.c
    src/display/vline.c
    src/display/which_key.c
    src/text/buffer.c
    src/text/buffer_edit.c
    src/text/change.c
    src/text/gap.c
    src/text/line.c
    src/text/mark.c
    src/text/mode.c
    src/text/point.c
    src/text/register.c
    src/text/search.c
    src/text/var.c
    src/text/jump_list.c
    src/text/treesitter.c
)

# Create executable
add_executable(app ${SRC_FILES})

# ------------------------------------------------------------
# Tree-sitter (vendored, always static)
# ------------------------------------------------------------
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(vendored/tree-sitter EXCLUDE_FROM_ALL)

# Build tree-sitter-scheme grammar directly (its CMakeLists has an invalid
# version string, so we compile the sources ourselves)
add_library(tree-sitter-scheme STATIC
    vendored/tree-sitter-scheme/src/parser.c
)
set_target_properties(tree-sitter-scheme PROPERTIES C_STANDARD 11 POSITION_INDEPENDENT_CODE ON)
target_include_directories(tree-sitter-scheme PRIVATE vendored/tree-sitter-scheme/src)

target_include_directories(app PRIVATE
    vendored/tree-sitter/lib/include
    vendored/tree-sitter-scheme/bindings/c
)

# ------------------------------------------------------------
# Platform-specific configuration
# ------------------------------------------------------------
if(EMSCRIPTEN)
    message(STATUS "Configuring for Emscripten / WASM")

    # Optimization flags for WASM performance
    set(CMAKE_C_FLAGS_RELEASE "-O3 -flto")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-O3 -flto")
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
    endif()

    # Set output suffix to .html
    set_target_properties(app PROPERTIES SUFFIX ".html")

    # Preload resources, scheme, and chibi-scheme lib (required for standard env)
    # Note: scheme files go to /resources/ to match RESOURCES_PATH in scheme.c
    set(EMCC_ASSETS
        "--preload-file ${CMAKE_SOURCE_DIR}/resources@/resources"
        "--preload-file ${CMAKE_SOURCE_DIR}/scheme@/resources"
        # Bundle the full chibi-scheme lib, but exclude files that are never
        # needed at runtime: the snow package manager and SRFI test suites.
        # This saves ~900 KB from app.data without any fragile dep tracking.
        "--preload-file ${CMAKE_SOURCE_DIR}/vendored/chibi-scheme/lib@/lib"
        "--exclude-file ${CMAKE_SOURCE_DIR}/vendored/chibi-scheme/lib/chibi/snow"
        "--exclude-file */test.sld"
    )
    list(JOIN EMCC_ASSETS " " EMCC_ASSETS_STR)
    set_target_properties(app PROPERTIES LINK_FLAGS "${EMCC_ASSETS_STR}")

    # Allow WASM linear memory to grow beyond its initial size (required for
    # Chibi Scheme's GC heap allocation; without this the binary has a fixed
    # memory ceiling and any malloc beyond it returns NULL / aborts).
    target_link_options(app PRIVATE
        -sALLOW_MEMORY_GROWTH=1
        -sMAXIMUM_MEMORY=268435456
    )

    # Disable unused SDL3 subsystems to reduce WASM binary size
    set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
    set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
    set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
    set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
    set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
    set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
    set(SDL_TRAY OFF CACHE BOOL "" FORCE)
    set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
    set(SDL_POWER OFF CACHE BOOL "" FORCE)
    set(SDL_GPU OFF CACHE BOOL "" FORCE)

    # Add vendored SDL libraries
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
    # Only SVG is used (src/display/icon.c); disable all other image formats
    set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
    set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_ANI OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_BMP OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_GIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_JPG OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_LBM OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_PCX OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_PNG OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_PNM OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_QOI OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TGA OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_XCF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_XPM OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_XV OFF CACHE BOOL "" FORCE)
    add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)
    # Disable color emoji SVG rendering (not needed for monospace code editor)
    set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
    set(SDLTTF_PLUTOSVG OFF CACHE BOOL "" FORCE)
    add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)

    # Explicitly propagate include directories
    target_include_directories(app PUBLIC
        vendored/SDL/include
        vendored/SDL_image/include
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

    # Force static build so modules are embedded (required for WASM)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(EMBED_MODULES ON CACHE BOOL "" FORCE)

    add_subdirectory(
        vendored/chibi-scheme
        ${CMAKE_BINARY_DIR}/chibi-build
        EXCLUDE_FROM_ALL
    )

    # Link the libraries
    target_link_libraries(app PRIVATE
        SDL3::SDL3-static
        SDL3_image::SDL3_image-static
        SDL3_ttf::SDL3_ttf-static
        chibi::libchibi-scheme
        tree-sitter
        tree-sitter-scheme
    )

else()
    # ------------------------------------------------------------
    # Desktop build
    # ------------------------------------------------------------

    # Default to vendored deps on macOS (no system SDL3 packages),
    # system packages on Linux
    if(APPLE)
        option(USE_VENDORED "Build vendored SDL3/chibi instead of using system packages" ON)
    else()
        option(USE_VENDORED "Build vendored SDL3/chibi instead of using system packages" OFF)
    endif()

    message(STATUS "Configuring for desktop build (vendored=${USE_VENDORED})")

    # Copy resources and scheme folders for desktop builds
    add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:app>/resources
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/scheme $<TARGET_FILE_DIR:app>/scheme
    )

    if(USE_VENDORED)
        # Build SDL3, SDL3_image, SDL3_ttf, and chibi-scheme from vendored submodules
        add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

        set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
        set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_ANI OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_BMP OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_GIF OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_JPG OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_LBM OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_PCX OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_PNG OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_PNM OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_QOI OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_TGA OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_XCF OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_XPM OFF CACHE BOOL "" FORCE)
        set(SDLIMAGE_XV OFF CACHE BOOL "" FORCE)
        add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)

        set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
        set(SDLTTF_PLUTOSVG OFF CACHE BOOL "" FORCE)
        add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)

        target_include_directories(app PUBLIC
            vendored/SDL/include
            vendored/SDL_image/include
            vendored/SDL_ttf/include
        )

        # Build chibi-scheme from vendored source
        add_subdirectory(
            vendored/chibi-scheme
            ${CMAKE_BINARY_DIR}/chibi-build
            EXCLUDE_FROM_ALL
        )

        target_link_libraries(app PUBLIC
            SDL3::SDL3-static
            SDL3_image::SDL3_image-static
            SDL3_ttf::SDL3_ttf-static
            chibi::libchibi-scheme
            tree-sitter
            tree-sitter-scheme
        )

        # Copy vendored chibi-scheme stdlib next to binary so it can find modules
        add_custom_command(TARGET app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/vendored/chibi-scheme/lib $<TARGET_FILE_DIR:app>/lib
        )
    else()
        # Use system SDL3 packages (Linux)
        find_package(SDL3 REQUIRED)
        find_package(SDL3_image REQUIRED)
        find_package(SDL3_ttf REQUIRED)

        target_link_libraries(app PUBLIC SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf chibi-scheme tree-sitter tree-sitter-scheme m)
    endif()

endif()
