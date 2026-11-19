add_executable(zgui_tests
    ../examples/showcase/showcase_ui.cc
    unit/test_main.cc
    fuzz/random_event_sequence.cc
    unit/test_primitives.cc
    unit/test_graphics_base.cc
    unit/test_groupbox_color.cc
    unit/test_lifetime_and_manager.cc
    unit/test_phase3_edges.cc
    unit/test_layout.cc
    unit/test_scroll_area.cc
    unit/test_showcase_builds.cc
    unit/test_table_view.cc
    unit/test_tabs.cc
    unit/test_theme_and_text.cc
    unit/test_window.cc
    unit/test_widgets.cc)
target_link_libraries(zgui_tests PRIVATE zgui::testing)
target_include_directories(zgui_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(zgui_tests PRIVATE cxx_std_17)
zgui_apply_project_options(zgui_tests)
add_test(NAME zgui_tests COMMAND zgui_tests)

if(TARGET zgui_sdl2_opengl)
    add_executable(zgui_backend_text_tests unit/test_main.cc unit/test_backend_text.cc)
    target_link_libraries(zgui_backend_text_tests PRIVATE zgui::sdl2_opengl)
    target_compile_features(zgui_backend_text_tests PRIVATE cxx_std_17)
    zgui_apply_project_options(zgui_backend_text_tests)
    add_test(NAME zgui_backend_text_tests COMMAND zgui_backend_text_tests)
endif()
