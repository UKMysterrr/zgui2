cmake_minimum_required(VERSION 3.14)
project(zgui_packed_tests LANGUAGES CXX)

enable_testing()

set(ZGUI_PACKED_DIR "${CMAKE_CURRENT_LIST_DIR}/../../dist")
add_library(zgui STATIC "${ZGUI_PACKED_DIR}/zgui.cc")
add_library(zgui::zgui ALIAS zgui)
target_include_directories(zgui PUBLIC "${ZGUI_PACKED_DIR}")
target_compile_features(zgui PUBLIC cxx_std_17)

function(zgui_apply_project_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wold-style-cast
            -Wnon-virtual-dtor
            -Woverloaded-virtual
            -Wnull-dereference
            -Wformat=2
            -Werror)
    endif()
endfunction()

zgui_apply_project_options(zgui)

add_library(zgui_testing STATIC
    "${CMAKE_CURRENT_LIST_DIR}/../support/mock_graphics.cc"
    "${CMAKE_CURRENT_LIST_DIR}/../support/mock_input.cc"
    "${CMAKE_CURRENT_LIST_DIR}/../support/test_context.cc")
add_library(zgui::testing ALIAS zgui_testing)
target_include_directories(zgui_testing PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/../support")
target_link_libraries(zgui_testing PUBLIC zgui::zgui)
target_compile_features(zgui_testing PUBLIC cxx_std_17)
zgui_apply_project_options(zgui_testing)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/.." "${CMAKE_CURRENT_BINARY_DIR}/core-tests")
