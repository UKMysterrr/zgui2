# Widgets

Unless noted otherwise, widget positions and sizes are relative to their parent
and measured in floating-point pixels. Values may be bound by pointer/reference
(non-owning) or by `Bound<T>` (shared ownership). Callbacks run synchronously
when the corresponding value changes.

## Label

Displays text with left, center, or right alignment. `set_color()` overrides
the theme text color until `clear_color_override()`. Text is truncated to fit;
empty and narrow widths are safe.

```cpp
void add_label(zgui::UIElement& parent) {
    auto label = std::make_shared<zgui::Label>("Status");
    label->set_alignment(zgui::TextAlignment::center);
    label->set_color(zgui::core::active_theme.accent);
    parent.add_child(label);
}
```

## Separator

Construct with `SeparatorOrientation::horizontal` (the default) or
`SeparatorOrientation::vertical`; a horizontal rule can include a centered
label. Its parent controls the pixel bounds, and the rule uses the theme's
inner-border color. A vertical separator ignores the optional label.

```cpp
void add_separator(zgui::UIElement& parent) {
    auto separator = std::make_shared<zgui::Separator>(
        zgui::SeparatorOrientation::horizontal, "Advanced");
    separator->set_size({240.f, 18.f});
    parent.add_child(separator);
}
```

## ScrollArea

`ScrollArea` clips its children and scrolls vertically when their bounds exceed
the viewport. Horizontal scrolling is opt-in with `set_horizontal_scrolling`;
both axes have draggable scrollbars when needed. Wheel input is offered to the
innermost scrollable child first, then handed to an ancestor when that child
reaches the corresponding vertical boundary.

```cpp
void add_scroll_area(zgui::UIElement& parent) {
    auto area = std::make_shared<zgui::ScrollArea>();
    area->set_size({240.f, 160.f});
    area->set_horizontal_scrolling(true);

    auto content = std::make_shared<zgui::Label>("Scrollable content");
    content->set_position({0.f, 220.f});
    content->set_size({320.f, 16.f});
    area->add_child(content);
    parent.add_child(area);
}
```

## Button

Construct with a label and optional `void()` callback. `set_on_click()` replaces
the callback. A click activates only when the release occurs inside the button.

```cpp
void add_button(zgui::UIElement& parent, int& count) {
    auto button = std::make_shared<zgui::Button>("Apply", [&count] { ++count; });
    button->set_size({100.f, 20.f});
    parent.add_child(button);
}
```

## CheckBox

Construct with a label, a `bool*`, `bool&`, or `Bound<bool>`, and an optional
change callback. The callback runs on a toggle; external value changes are
reflected on the next draw. Recommended row height:
`spacing::kCheckboxRowPitch`.

```cpp
void add_checkbox(zgui::UIElement& parent) {
    auto enabled = zgui::bound(false);
    parent.add_child(std::make_shared<zgui::CheckBox>("Enabled", enabled));
}
```

## ToggleButton and RadioGroup

`ToggleButton` behaves like a button while retaining a bound boolean. It toggles
on release inside the control, and its change callback runs only when the value
changes. `RadioGroup` displays options vertically and binds the selected
`std::size_t`; its constructor clamps an initially out-of-range index, while
`set_selected_index()` rejects later invalid indices.

```cpp
void add_selection_controls(zgui::UIElement& parent) {
    auto pinned = zgui::bound(false);
    auto mode = zgui::bound(std::size_t{0});
    parent.add_child(std::make_shared<zgui::ToggleButton>("Pinned", pinned));

    auto radios = std::make_shared<zgui::RadioGroup>(
        std::vector<std::string>{"Compact", "Comfortable", "Spacious"}, mode);
    radios->set_size({180.f, 54.f});
    parent.add_child(radios);
}
```

## ProgressBar

`ProgressBar` binds a floating-point value clamped to `[0, 1]`; an optional
string is centered over the themed track. Caller-owned references and
`Bound<float>` are supported.

```cpp
void add_progress(zgui::UIElement& parent) {
    auto progress = zgui::bound(0.65f);
    auto bar = std::make_shared<zgui::ProgressBar>(progress, "Downloading");
    bar->set_size({240.f, 20.f});
    parent.add_child(bar);
}
```

## Tooltip

`Tooltip` is an invisible hover target: position and size it over the control
that should trigger the text. After a 500 ms default delay it opens one
theme-styled overlay, clamped to the graphics viewport. Leaving the target
dismisses it; `WindowManager` allows only one tooltip at a time.

```cpp
void add_tooltip(zgui::Window& window) {
    auto tooltip = std::make_shared<zgui::Tooltip>("A short explanation");
    tooltip->set_position({20.f, 20.f});
    tooltip->set_size({100.f, 20.f});
    window.add_child(tooltip);
}
```

## Slider

`Slider<T>` accepts arithmetic types and a value pointer/reference/`Bound<T>`,
minimum, maximum, and optional `void()` callback. Reversed bounds are normalized;
values are clamped at construction and on assignment. Integral values round to
the nearest step.

```cpp
void add_slider(zgui::UIElement& parent) {
    auto scale = zgui::bound(1.0f);
    parent.add_child(std::make_shared<zgui::Slider<float>>("Scale", scale, 0.f, 2.f));
}
```

## ComboBox

Construct with a vector of labels and optional selected index. Bind selection
with `bind_selected_index()` or pass `Bound<std::size_t>`; `set_on_change()`
receives the selected index. Invalid indices return `false`; an empty list has
index zero and an empty selected string. Its popup is clipped to the viewport
and can flip above the control.

```cpp
void add_combo(zgui::UIElement& parent) {
    auto mode = zgui::bound(std::size_t{0});
    auto combo = std::make_shared<zgui::ComboBox>(
        std::vector<std::string>{"Windowed", "Fullscreen"});
    combo->bind_selected_index(mode);
    parent.add_child(combo);
}
```

Popup widgets use the active `WindowManager`; add their owning window to the
manager before interacting with them.

## MultiComboBox

Construct with labels and a `std::vector<bool>` pointer/reference or
`Bound<std::vector<bool>>`. The values vector is resized when the item list
changes, preserving existing entries and initializing new entries to false.
The optional callback runs when a choice toggles.

```cpp
void add_multi_combo(zgui::UIElement& parent) {
    auto flags = zgui::bound(std::vector<bool>{false, true});
    parent.add_child(std::make_shared<zgui::MultiComboBox>(
        std::vector<std::string>{"One", "Two"}, flags));
}
```

## TextInput

Construct with a string pointer, reference, or `Bound<std::string>` and an
optional `void()` callback. Clicking focuses it; only printable ASCII is
accepted. Backspace, delete, cursor keys, Home, and End are handled by `Input`.
Destroying the focused input clears the global focus pointer.
`set_password_mode(true)` masks the display with asterisks; the bound string
still contains the original text and is not securely erased.

```cpp
void add_text_input(zgui::UIElement& parent) {
    auto name = zgui::bound(std::string("player"));
    parent.add_child(std::make_shared<zgui::TextInput>(name));
}
```

## KeyBind

Construct with a title, key-code pointer/reference/`Bound<int>`, and optional
change callback. Clicking starts listening; Escape cancels; the next key press
is stored. Key naming is delegated to the backend.

```cpp
void add_key_bind(zgui::UIElement& parent) {
    auto key = zgui::bound(0);
    parent.add_child(std::make_shared<zgui::KeyBind>("Toggle", key));
}
```

## ColorPicker

Construct with a `Color*`, `Color&`, or `Bound<Color>` plus optional change
callback. It provides saturation/value, hue, and alpha controls. HSV state is
kept to avoid losing hue for grey colors; channel values round-trip within one
8-bit step.

```cpp
void add_color_picker(zgui::UIElement& parent) {
    auto color = zgui::bound(zgui::Color(255, 128, 0, 255));
    parent.add_child(std::make_shared<zgui::ColorPicker>(color));
}
```

## ColorSwatch

Construct with an optional label and color pointer/reference/`Bound<Color>`, or
with only a color. It opens a picker popup; `set_vertical_layout()` changes its
label/swatch arrangement. Color values may be shared-owned with `Bound<Color>`.

```cpp
void add_swatch(zgui::UIElement& parent) {
    auto color = zgui::bound(zgui::Color(20, 140, 220));
    parent.add_child(std::make_shared<zgui::ColorSwatch>("Accent", color));
}
```

## ColorButton

Construct with a label and color pointer/reference/`Bound<Color>`. Clicking
opens the color picker; changes are reflected in the button and optional
callback. This is sized for a row of three controls by
`StackLayout::add_three_column_row()`.

```cpp
void add_color_button(zgui::UIElement& parent) {
    auto color = zgui::bound(zgui::Color(255, 140, 0));
    parent.add_child(std::make_shared<zgui::ColorButton>("Accent", color));
}
```

## TabBar

Construct with a tab-strip height. `add_tab()` returns the page to populate;
`set_active_tab()` returns false for an invalid index. Only the active page is
drawn, and tall page content is clipped and scrollable.

```cpp
void add_tabs(zgui::UIElement& parent) {
    auto tabs = std::make_shared<zgui::TabBar>();
    auto page = tabs->add_tab("General");
    page->add_child(std::make_shared<zgui::Label>("General settings"));
    parent.add_child(tabs);
}
```

## SubTabBar

`SubTabBar` uses the smaller `spacing::kSubTabBarHeight`. Attach it to a
`GroupBox` with `set_sub_tabs()`; the group automatically positions the strip.
Tab index handling and page clipping match `TabBar`.

```cpp
void add_subtabs(zgui::GroupBox& group) {
    auto tabs = std::make_shared<zgui::SubTabBar>();
    auto page = tabs->add_tab("Basic");
    page->add_child(std::make_shared<zgui::Label>("Basic options"));
    group.set_sub_tabs(tabs);
}
```

## GroupBox

Construct with an optional title. Add rows with `StackLayout(group)` and call
`fit_to_content()` to size the group to its children. `set_fit_content(false)`
disables automatic sizing. Nested sub-tabs are supported.
`set_title_alignment()` changes the title alignment; the default is left.

```cpp
void add_group(zgui::UIElement& parent) {
    auto group = std::make_shared<zgui::GroupBox>("General");
    zgui::StackLayout layout(*group);
    layout.add_row(std::make_shared<zgui::Label>("Options"));
    group->set_position({8.f, 8.f});
    parent.add_child(group);
}
```

## StackLayout

This helper is not a widget; it positions child rows and applies horizontal
constraints. Use `StackLayout::page()` for tab pages,
`add_two_column_row()` for a checkbox/combo pair, and
`add_three_column_row()` for color buttons. Default row and gap dimensions are
in `spacing::`.

```cpp
void add_rows(zgui::UIElement& parent) {
    auto layout = zgui::StackLayout(parent);
    layout.add_row(std::make_shared<zgui::Label>("First"));
    layout.add_row(std::make_shared<zgui::Label>("Second"), 20.f);
}
```

## TableView and ListView

`TableView` accepts `Column` definitions, string rows or `Row` cells, optional
cell/row colors, and selection callbacks. Clicking a selected row deselects it;
the callback receives `TableView::no_selection`. `clear_selection()` also
notifies when the selection changes. Invalid `set_cell()` and selection
indices return false; clearing rows clears selection. Only visible rows are
drawn, column widths can be weighted or resized, and hidden columns are omitted.
`ListView` is a compatibility alias.

```cpp
void add_table(zgui::UIElement& parent) {
    auto table = std::make_shared<zgui::TableView>();
    table->set_columns({{"Name", 120.f}, {"State", 80.f}});
    table->add_row(std::vector<std::string>{"Alpha", "Ready"});
    table->set_on_selection_changed([](std::size_t row) {
        if (row == zgui::TableView::no_selection) {
            // No row is selected.
        }
    });
    parent.add_child(table);
}
```

## Viewport

Use `set_on_render()` to draw into its clipped screen-space rectangle, supplied
as integer `x`, `y`, `width`, and `height`. `set_on_input()` receives drag,
pan, wheel, and double-click information when the pointer is active.

```cpp
void add_viewport(zgui::UIElement& parent) {
    auto view = std::make_shared<zgui::Viewport>();
    view->set_on_render([](int, int, int, int) {});
    view->set_on_input([](const zgui::ViewportInput&) {});
    parent.add_child(view);
}
```

## Window

Construct with a title, add child widgets, set a pixel size, and register it
with `WindowManager`. Windows can be hidden, frameless, non-resizable, or kept
always on top; minimum size can be explicit or derived from content.

```cpp
void add_window(zgui::WindowManager& manager) {
    auto window = std::make_shared<zgui::Window>("Tools");
    window->set_size({320.f, 220.f});
    window->add_child(std::make_shared<zgui::Label>("Ready"));
    manager.add_window(window);
}
```

## WindowManager

The manager owns registered windows and overlays. `update()` dispatches input;
`draw()` renders windows and then overlays. `show_overlay()` and
`dismiss_overlay()` manage transient popup elements. `validate_layout()` checks
registered window bounds and overlaps and optionally returns a diagnostic.

```cpp
void render_ui(zgui::WindowManager& manager) {
    manager.update();
    manager.draw();
    std::string diagnostic;
    (void)manager.validate_layout(&diagnostic);
}
```
