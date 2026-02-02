# minimum version
cmake_minimum_required(VERSION 3.16)

project(chess CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set the output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(FETCHCONTENT_QUIET FALSE)
include(cmake/get_cpm.cmake)

# set flag to compile only the chessvalidator
option(CHESS_VALIDATOR_ONLY "Compile only the chess validator" OFF)

CPMAddPackage("gh:TheLartians/Format.cmake@1.8.3")

# add external chess lib to use as a validator for the tools
CPMAddPackage(
        NAME disservin
        GITHUB_REPOSITORY InfiniBrains/chess-library
        DOWNLOAD_ONLY YES
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
include_directories(${disservin_SOURCE_DIR}/include)

# add magic_enum lib
CPMAddPackage(
        NAME magic_enum
        VERSION 0.9.5
        GITHUB_REPOSITORY Neargye/magic_enum
        DOWNLOAD_ONLY YES
        OPTIONS
        "MAGIC_ENUM_BUILD_EXAMPLES OFF"
        "MAGIC_ENUM_BUILD_TESTS OFF"
        "MAGIC_ENUM_BUILD_INSTALL OFF"
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
)
include_directories(${magic_enum_SOURCE_DIR}/include)

IF(NOT CHESS_VALIDATOR_ONLY)

    CPMAddPackage(
            NAME SDL2
            GITHUB_REPOSITORY libsdl-org/SDL
            GIT_TAG release-2.32.10
            OPTIONS "SDL2_DISABLE_INSTALL ON"
            "SDL_WERROR OFF"
            "SDL_TEST OFF"
    )
    find_package(SDL2 REQUIRED)
    if(SDL2_ADDED)
        file(GLOB SDL_HEADERS "${SDL_SOURCE_DIR}/include/*.h")

        # Create a target that copies headers at build time, when they change
        add_custom_target(
                sdl_copy_headers_in_build_dir
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${SDL_SOURCE_DIR}/include"
                "${CMAKE_BINARY_DIR}/SDLHeaders/SDL2"
                DEPENDS ${SDL_HEADERS}
        )

        # Make SDL depend from it
        add_dependencies(SDL2 sdl_copy_headers_in_build_dir)

        # And add the directory where headers have been copied as an interface include dir
        target_include_directories(SDL2 INTERFACE "${CMAKE_BINARY_DIR}/SDLHeaders")

        set(SDL2_INCLUDE_DIR ${SDL2_SOURCE_DIR}/include)
    endif()
    include_directories(${SDL2_INCLUDE_DIR})

    CPMAddPackage(GITHUB_REPOSITORY libsdl-org/SDL_image
            GIT_TAG release-2.8.8
            OPTIONS
            "SDL2IMAGE_INSTALL OFF"
            "SDL2IMAGE_SAMPLES OFF"
            "SDL2IMAGE_VENDORED ON"
            "GIT_SUBMODULES_RECURSE ON"
    )
    find_package(SDL_image REQUIRED)
    include_directories(${SDL_image_SOURCE_DIR})

    # add imgui
    CPMAddPackage(
            NAME IMGUI
            URL "https://github.com/ocornut/imgui/archive/refs/tags/v1.92.5.zip"
    )
    IF(IMGUI_ADDED)
        add_library(IMGUI STATIC)

        target_sources( IMGUI
                PRIVATE
                ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
                ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
                ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
                ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
                ${IMGUI_SOURCE_DIR}/imgui.cpp

                PRIVATE
                ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
                ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
                ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
        target_include_directories( IMGUI
                PUBLIC ${IMGUI_SOURCE_DIR}
                PUBLIC ${IMGUI_SOURCE_DIR}/backends
        )
        find_package(OpenGL REQUIRED)
        if(EMSCRIPTEN)
            target_link_libraries(IMGUI PUBLIC ${OPENGL_gl_LIBRARY} SDL2 ${CMAKE_DL_LIBS})
        else()
            target_link_libraries(IMGUI PUBLIC ${OPENGL_gl_LIBRARY} SDL2 ${CMAKE_DL_LIBS})
        endif()
    ENDIF()
    include_directories(${IMGUI_SOURCE_DIR} ${IMGUI_SOURCE_DIR}/backends)

endif() # NOT CHESS_VALIDATOR_ONLY

# chess library
file(GLOB_RECURSE CHESS_BOT_FILES CONFIGURE_DEPENDS "chess-bot/*.cpp" "chess-bot/*.h")
add_library(chessbot STATIC ${CHESS_BOT_FILES})
set_target_properties(chessbot PROPERTIES LINKER_LANGUAGE CXX)
include_directories(chess-bot)

# chess cli
file(GLOB_RECURSE CHESS_CLI_FILES CONFIGURE_DEPENDS "chess-cli/*.cpp" "chess-cli/*.h")
add_executable(chesscli ${CHESS_CLI_FILES})
target_link_libraries(chesscli PUBLIC chessbot)

if(NOT CHESS_VALIDATOR_ONLY)
# chess gui
file(GLOB_RECURSE CHESS_GUI_FILES CONFIGURE_DEPENDS "chess-gui/*.cpp" "chess-gui/*.h")
add_executable(chessgui ${CHESS_GUI_FILES})
target_link_libraries(chessgui PUBLIC chessbot IMGUI SDL2::SDL2 SDL2_image)
endif() # NOT CHESS_VALIDATOR_ONLY

CPMAddPackage(
        NAME CPMLicenses.cmake
        GITHUB_REPOSITORY cpm-cmake/CPMLicenses.cmake
        VERSION 0.0.5
)
write_license_disclaimer("third_party.txt" "${CPM_PACKAGES}")