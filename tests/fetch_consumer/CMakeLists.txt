cmake_minimum_required(VERSION 3.14)
project(zgui_fetch_consumer LANGUAGES CXX)
include(FetchContent)

set(ZGUI_BUILD_BACKEND_SDL2_OPENGL ON CACHE BOOL "")
FetchContent_Declare(zgui
    GIT_REPOSITORY https://github.com/UKMysterrr/zgui2.git
    GIT_TAG main)
FetchContent_MakeAvailable(zgui)

add_executable(zgui_fetch_consumer ../install_consumer/main.cc)
target_compile_definitions(zgui_fetch_consumer PRIVATE ZGUI_CONSUMER_BACKEND)
target_link_libraries(zgui_fetch_consumer PRIVATE
    zgui::zgui zgui::sdl2_opengl zgui::testing)
