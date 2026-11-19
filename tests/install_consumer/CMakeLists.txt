cmake_minimum_required(VERSION 3.14)
project(zgui_install_consumer LANGUAGES CXX)

find_package(zgui 0.1 CONFIG REQUIRED)

add_executable(zgui_install_consumer main.cc)
target_link_libraries(zgui_install_consumer PRIVATE zgui::zgui)
target_link_libraries(zgui_install_consumer PRIVATE zgui::testing)

if(ZGUI_CONSUMER_BACKEND)
    find_package(zgui CONFIG REQUIRED COMPONENTS sdl2_opengl)
    target_compile_definitions(zgui_install_consumer PRIVATE ZGUI_CONSUMER_BACKEND)
    target_link_libraries(zgui_install_consumer PRIVATE zgui::sdl2_opengl)
endif()
