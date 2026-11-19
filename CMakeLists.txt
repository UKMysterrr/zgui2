cmake_minimum_required(VERSION 3.14)
project(zgui VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(ZGUI_BUILD_SHARED "Build zgui as a shared library" OFF)
option(ZGUI_BUILD_BACKEND_SDL2_OPENGL "Build the SDL2/OpenGL backend when available" ON)
option(ZGUI_BUILD_EXAMPLES "Build examples" OFF)
option(ZGUI_WARNINGS_AS_ERRORS "Treat project compiler warnings as errors" OFF)
option(ZGUI_NO_EXCEPTIONS "Build the core and tests without C++ exceptions where supported" OFF)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(_zgui_tests_default ON)
    set(_zgui_install_default ON)
else()
    set(_zgui_tests_default OFF)
    set(_zgui_install_default OFF)
endif()
option(ZGUI_BUILD_TESTS "Build the headless tests" ${_zgui_tests_default})
option(ZGUI_INSTALL "Install zgui and its CMake package" ${_zgui_install_default})

if(ZGUI_BUILD_SHARED)
    add_library(zgui SHARED zgui.cc)
else()
    add_library(zgui STATIC zgui.cc)
endif()
add_library(zgui::zgui ALIAS zgui)
target_include_directories(zgui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
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
            -Wformat=2)
        if(ZGUI_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
        if(ZGUI_NO_EXCEPTIONS AND
           ("${target}" STREQUAL "zgui" OR
            "${target}" STREQUAL "zgui_testing" OR
            "${target}" STREQUAL "zgui_tests"))
            target_compile_options(${target} PRIVATE -fno-exceptions)
        endif()
    endif()
endfunction()

zgui_apply_project_options(zgui)

add_library(zgui_testing STATIC
    tests/support/mock_graphics.cc
    tests/support/mock_input.cc
    tests/support/test_context.cc)
add_library(zgui::testing ALIAS zgui_testing)
target_include_directories(zgui_testing PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tests/support>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/zgui/testing>)
target_link_libraries(zgui_testing PUBLIC zgui::zgui)
target_compile_features(zgui_testing PUBLIC cxx_std_17)
zgui_apply_project_options(zgui_testing)

if(ZGUI_INSTALL)
    set(zgui_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/zgui")
    install(TARGETS zgui zgui_testing
        EXPORT zguiTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES zgui.hh DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(FILES
        tests/support/mock_graphics.hh
        tests/support/mock_input.hh
        tests/support/test_context.hh
        tests/support/test_harness.hh
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/zgui/testing")
    install(EXPORT zguiTargets
        FILE zguiTargets.cmake
        NAMESPACE zgui::
        DESTINATION "${zgui_INSTALL_CMAKEDIR}")

    configure_package_config_file(
        cmake/zguiConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/zguiConfig.cmake"
        INSTALL_DESTINATION "${zgui_INSTALL_CMAKEDIR}")
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/zguiConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion)
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/zguiConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/zguiConfigVersion.cmake"
        DESTINATION "${zgui_INSTALL_CMAKEDIR}")
endif()

find_package(OpenGL QUIET)
find_package(SDL2 QUIET CONFIG)
set(_zgui_sdl2_target "")
if(TARGET SDL2::SDL2)
    set(_zgui_sdl2_target SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    set(_zgui_sdl2_target SDL2::SDL2-static)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2_PC QUIET IMPORTED_TARGET sdl2)
        if(TARGET PkgConfig::SDL2_PC)
            set(_zgui_sdl2_target PkgConfig::SDL2_PC)
        endif()
    endif()
endif()

if(ZGUI_BUILD_BACKEND_SDL2_OPENGL)
    if(_zgui_sdl2_target AND OpenGL_FOUND)
        add_library(zgui_sdl2_opengl backends/sdl2_opengl/zgui_impl_sdl2_opengl.cc)
        add_library(zgui::sdl2_opengl ALIAS zgui_sdl2_opengl)
        target_link_libraries(zgui_sdl2_opengl PUBLIC zgui::zgui PRIVATE
            ${_zgui_sdl2_target} OpenGL::GL)
        target_compile_features(zgui_sdl2_opengl PUBLIC cxx_std_17)
        zgui_apply_project_options(zgui_sdl2_opengl)
    else()
        message(STATUS
            "Skipping zgui_sdl2_opengl: SDL2 and OpenGL development packages are required")
    endif()
endif()

if(ZGUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ZGUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
