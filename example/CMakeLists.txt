if(NOT TARGET zgui::sdl2_opengl)
    message(FATAL_ERROR
        "ZGUI_BUILD_EXAMPLES requires the SDL2/OpenGL backend and its development packages.")
endif()

add_executable(zgui_example main.cc)
target_link_libraries(zgui_example PRIVATE zgui::sdl2_opengl)
zgui_apply_project_options(zgui_example)
