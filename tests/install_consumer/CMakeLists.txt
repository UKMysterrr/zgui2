cmake_minimum_required(VERSION 3.14)
project(zgui_install_consumer LANGUAGES CXX)

find_package(zgui 0.1 CONFIG REQUIRED)

add_executable(zgui_install_consumer main.cc)
target_link_libraries(zgui_install_consumer PRIVATE zgui::zgui)
