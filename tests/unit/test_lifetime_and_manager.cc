// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/test_context.hh"
#include "../support/test_harness.hh"

namespace {
    class InspectableElement : public zgui::UIElement {
    public:
        std::size_t child_count() const {
            return children_.size();
        }

        bool attached() const {
            return parent_ != nullptr;
        }
    };

    class MutatingOverlay : public zgui::UIElement {
    public:
        zgui::WindowManager* manager = nullptr;
        bool remove_on_draw = false;
        bool remove_on_update = false;

        void draw() override {
            if (remove_on_draw)
                manager->dismiss_overlay(this);
        }

        void update() override {
            if (remove_on_update)
                manager->dismiss_overlay(this);
        }
    };

    class MutatingWindow : public zgui::Window {
    public:
        explicit MutatingWindow(std::string title) : Window(std::move(title)) {}
        std::function<void()> on_draw;
        std::function<void()> on_update;

        void draw() override {
            if (on_draw)
                on_draw();
        }

        void update() override {
            if (on_update)
                on_update();
        }
    };
} // namespace

TEST(raw_child_destruction_detaches_from_parent) {
    InspectableElement parent;
    {
        InspectableElement child;
        parent.add_child(&child);
        CHECK_EQ(parent.child_count(), 1u);
        CHECK(child.attached());
    }
    CHECK_EQ(parent.child_count(), 0u);
}

TEST(shared_child_reparenting_transfers_ownership_once) {
    auto first = std::make_shared<InspectableElement>();
    auto second = std::make_shared<InspectableElement>();
    auto child = std::make_shared<InspectableElement>();
    std::weak_ptr<InspectableElement> weak_child = child;
    first->add_child(child);
    second->add_child(child.get());
    CHECK_EQ(first->child_count(), 0u);
    CHECK_EQ(second->child_count(), 1u);
    child.reset();
    CHECK(!weak_child.expired());
    second->clear_children();
    CHECK(weak_child.expired());
}

TEST(overlay_can_dismiss_during_draw_and_update) {
    zgui_test::Harness harness;
    auto overlay = std::make_shared<MutatingOverlay>();
    overlay->manager = &harness.manager;
    overlay->remove_on_draw = true;
    harness.manager.show_overlay(overlay);
    harness.manager.draw();
    CHECK(!harness.manager.has_overlays());

    overlay->remove_on_draw = false;
    overlay->remove_on_update = true;
    harness.manager.show_overlay(overlay);
    harness.manager.update();
    CHECK(!harness.manager.has_overlays());
}

TEST(window_can_remove_itself_during_draw_and_update) {
    zgui_test::Harness harness;
    auto window = std::make_shared<MutatingWindow>("mutating");
    std::weak_ptr<MutatingWindow> weak_window = window;
    window->on_draw = [&harness, weak_window]() {
        if (auto current = weak_window.lock())
            harness.manager.remove_window(current);
    };
    harness.manager.add_window(window);
    harness.manager.draw();
    CHECK(harness.manager.windows().empty());

    auto updating = std::make_shared<MutatingWindow>("updating");
    std::weak_ptr<MutatingWindow> weak_updating = updating;
    updating->on_update = [&harness, weak_updating]() {
        if (auto current = weak_updating.lock())
            harness.manager.remove_window(current);
    };
    harness.manager.add_window(updating);
    harness.manager.update();
    CHECK(harness.manager.windows().empty());
}

TEST(focused_text_input_clears_global_pointer_on_destruction) {
    std::string value;
    {
        zgui::TextInput input(value);
        zgui::core::focused_text_input = &input;
    }
    CHECK(zgui::core::focused_text_input == nullptr);
}

TEST(popup_widget_can_die_before_its_manager) {
    zgui_test::Harness harness;
    std::vector<bool> values(1, false);
    auto combo = std::make_shared<zgui::ComboBox>(std::vector<std::string>{"one"});
    combo->set_size({100.f, 20.f});
    zgui::core::window_manager = &harness.manager;
    CHECK(combo->handle_mouse_press({1.f, 1.f}));
    CHECK(harness.manager.has_overlays());
    combo.reset();
    harness.manager.draw();
    harness.manager.update();
    CHECK(harness.manager.has_overlays());

    auto multi_combo =
        std::make_shared<zgui::MultiComboBox>(std::vector<std::string>{"one"}, &values);
    multi_combo->set_size({100.f, 20.f});
    CHECK(multi_combo->handle_mouse_press({1.f, 1.f}));
    multi_combo.reset();
    harness.manager.draw();
    harness.manager.update();

    zgui::Color color(10, 20, 30);
    auto swatch = std::make_shared<zgui::ColorSwatch>("swatch", color);
    swatch->set_position({10.f, 10.f});
    swatch->set_size({100.f, 80.f});
    swatch->set_vertical_layout(true);
    CHECK(swatch->handle_mouse_press({60.f, 80.f}));
    CHECK(harness.manager.has_overlays());
    swatch.reset();
    harness.manager.draw();
    harness.manager.update();

    auto color_button = std::make_shared<zgui::ColorButton>("color", color);
    color_button->set_position({10.f, 10.f});
    color_button->set_size({120.f, 50.f});
    CHECK(color_button->handle_mouse_press({20.f, 20.f}));
    CHECK(harness.manager.has_overlays());
    color_button.reset();
    harness.manager.draw();
    harness.manager.update();
}

TEST(manager_can_die_before_external_popup_widget) {
    auto graphics = std::make_shared<zgui_test::MockGraphics>();
    auto input = std::make_shared<zgui_test::MockInput>();
    auto combo = std::make_shared<zgui::ComboBox>(std::vector<std::string>{"one"});
    combo->set_size({100.f, 20.f});
    std::vector<bool> values(1, false);
    auto multi_combo =
        std::make_shared<zgui::MultiComboBox>(std::vector<std::string>{"one"}, values);
    multi_combo->set_size({100.f, 20.f});
    zgui::Color color(10, 20, 30);
    auto swatch = std::make_shared<zgui::ColorSwatch>("swatch", color);
    swatch->set_position({10.f, 10.f});
    swatch->set_size({100.f, 80.f});
    swatch->set_vertical_layout(true);
    auto color_button = std::make_shared<zgui::ColorButton>("color", color);
    color_button->set_size({120.f, 50.f});
    {
        zgui::WindowManager manager;
        zgui::core::graphics = graphics;
        zgui::core::input = input;
        zgui::core::window_manager = &manager;
        CHECK(combo->handle_mouse_press({1.f, 1.f}));
        CHECK(multi_combo->handle_mouse_press({1.f, 1.f}));
        CHECK(swatch->handle_mouse_press({60.f, 80.f}));
        CHECK(color_button->handle_mouse_press({20.f, 20.f}));
        CHECK(manager.has_overlays());
    }
    CHECK(zgui::core::window_manager == nullptr);
    combo.reset();
    multi_combo.reset();
    swatch.reset();
    color_button.reset();
    zgui::core::graphics.reset();
    zgui::core::input.reset();
}
