if(NOT TARGET zgui_sdl2_opengl)
    message(FATAL_ERROR
        "ZGUI_BUILD_EXAMPLES requires the SDL2/OpenGL backend. Install SDL2 and OpenGL development packages.")
endif()

add_executable(zgui_showcase
    showcase/main.cc
    showcase/showcase_ui.cc)
target_link_libraries(zgui_showcase PRIVATE zgui::sdl2_opengl)
target_compile_features(zgui_showcase PRIVATE cxx_std_17)
zgui_apply_project_options(zgui_showcase)
