# Layout

Every `UIElement` has a parent-relative position and size in floating-point
pixels. Child clipping is determined by the parent; hit testing also respects
ancestor clips. Children added with `shared_ptr` are owned by the parent.
`add_child(UIElement*)` is non-owning, so the caller must keep that child alive.

## Anchors and constraints

Anchors preserve the selected edges' offsets when a parent changes size. For
example, anchoring left and right stretches a child horizontally while keeping
its left and right margins fixed.

Constraints specify each edge as a fraction of the parent extent plus a pixel
offset:

```cpp
void place_split(zgui::UIElement& parent, zgui::UIElement& child) {
    parent.add_child(&child);
    child.set_horizontal_constraints(0.25f, 0.75f, 8.f, -8.f);
    child.set_vertical_constraints(0.f, 1.f, 8.f, -8.f);
}
```

The child spans the middle half of the parent width and nearly the full
height. Fractions must be ordered within `[0, 1]`; offsets can be negative.

## StackLayout

`StackLayout` places rows vertically and applies horizontal fill constraints.
Its defaults use `spacing::kDefaultRowHeight`, `kStackHorizontalPadding`, and
`kStackRowGap`. Use `StackLayout::page()` for pages returned by
`TabBar::add_tab()`; page defaults omit horizontal padding and reserve the tab
strip at the top.

```cpp
void build_group(zgui::GroupBox& group) {
    auto layout = zgui::StackLayout(group);
    auto enabled = zgui::bound(true);
    layout.add_row(std::make_shared<zgui::CheckBox>("Enabled", enabled),
                   zgui::spacing::kCheckboxRowPitch);
    layout.add_row(std::make_shared<zgui::Label>("Details"));
    group.fit_to_content();
}
```

`add_two_column_row()` lays out a checkbox and combo box at a chosen split
fraction. `add_three_column_row()` lays out three color buttons. Their row
heights default to the corresponding `spacing::` constants.

## Tabs and groups

`TabBar::add_tab()` returns the page element to populate. Only the active page
is drawn, and its content is clipped to the page viewport; tall pages scroll
vertically. An invalid `set_active_tab()` index returns `false` without
changing the active tab. `GroupBox::set_sub_tabs()` positions the sub-tab strip
using shared spacing. Call `fit_to_content()` to size a group around its
children.

## Layout validation

`WindowManager::validate_layout()` checks registered windows and reports the
first problem through an optional error string. It verifies finite, non-negative
sizes, child bounds, and overlaps. The overload accepting a `Window` validates
one window without registering it. Run it in debug builds and headless tests
after constructing or resizing a layout.
