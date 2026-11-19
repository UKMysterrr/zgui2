// Copyright (c) 2019, zxvnme
// Copyright (c) 2026 UKMysterrr
// SPDX-License-Identifier: MIT

#ifndef ZGUI_HH
#define ZGUI_HH

#define ZGUI_VERSION_MAJOR 0
#define ZGUI_VERSION_MINOR 1
#define ZGUI_VERSION_PATCH 0
#define ZGUI_VERSION_STRING "0.1.0"

#ifndef ZGUI_NO_LEGACY_MACROS
// Deprecated: use draw_and_update(element) instead.
#define DRAW_UPDATE(ui_element) ::zgui::draw_and_update(*(ui_element))
#endif

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace zgui {

    /// Shared ownership wrapper for values bound to widgets.
    template <typename T> class Bound {
    private:
        std::shared_ptr<T> value_;

    public:
        /// Takes shared ownership of the supplied value.
        explicit Bound(std::shared_ptr<T> value) : value_(std::move(value)) {}

        /// Returns the bound value or null when empty.
        T* get() const {
            return value_.get();
        }

        /// Returns the shared owner retained by this binding.
        const std::shared_ptr<T>& owner() const {
            return value_;
        }

        /// Reports whether a value is owned.
        explicit operator bool() const {
            return value_ != nullptr;
        }
    };

    /// Creates a shared-owned widget binding from a value.
    template <typename T> Bound<T> bound(T value) {
        return Bound<T>(std::make_shared<T>(std::move(value)));
    }

    namespace spacing {
        inline constexpr float kTabBarHeight = 24.f;
        inline constexpr float kTabLabelHorizontalPadding = 10.f;
        // Tab labels shrink their side padding down to this before text is truncated.
        inline constexpr float kTabLabelMinimumPadding = 4.f;
        [[deprecated("use spacing::kTabLabelHorizontalPadding")]] inline constexpr float
            tab_label_horizontal_padding = kTabLabelHorizontalPadding;
        inline constexpr float kGroupTitleHeight = 14.f;
        inline constexpr float kGroupTitleGap = 6.f;
        inline constexpr float kGroupInnerPadding = 8.f;
        inline constexpr float kGroupBorderTopInset = 6.f;
        // Group edge -> sub-tab strip. Equals kStackHorizontalPadding so a strip lines up
        // with the rows of groups that have no sub tabs.
        inline constexpr float kSubTabStripInset = 8.f;
        inline constexpr float kGroupBottomPad = 6.f;
        inline constexpr float kGroupSubTabTop = kGroupTitleHeight + kGroupTitleGap;
        inline constexpr float kSubTabBarHeight = 18.f;
        inline constexpr float kSubTabBarBottomGap = 6.f;
        inline constexpr float kMinimumSubTabPageHeight = 8.f;
        inline constexpr float kCheckboxSize = 11.f;
        inline constexpr float kCheckboxLabelGap = 6.f;
        inline constexpr float kSliderLabelHeight = 17.f;
        inline constexpr float kSliderTrackHeight = 4.f;
        inline constexpr float kSliderTrackInset = 1.f;
        inline constexpr float kSliderInnerTrackHeight = 2.f;
        inline constexpr float kSliderThumbWidth = 5.f;
        inline constexpr float kSliderThumbHeight = 11.f;
        inline constexpr float kSliderThumbVerticalOffset = 3.f;
        inline constexpr float kCheckboxRowPitch = 20.f;
        inline constexpr float kDefaultStackStartY = 0.f;
        inline constexpr float kGroupStackStartY = 20.f;
        inline constexpr float kCompactPageStackStartY = 2.f;
        // Pages inside a sub-tab strip (see StackLayout::page).
        inline constexpr float kPageStackStartY = 8.f;
        inline constexpr float kPageHorizontalPadding = 0.f;
        inline constexpr float kPageRowGap = 2.f;
        inline constexpr float kStackHorizontalPadding = 8.f;
        inline constexpr float kWideStackHorizontalPadding = 8.f;
        inline constexpr float kCompactPageHorizontalPadding = 4.f;
        inline constexpr float kStackRowGap = 2.f;
        inline constexpr float kLooseStackRowGap = 3.f;
        inline constexpr float kDefaultRowHeight = 14.f;
        inline constexpr float kCompactControlHeight = 15.f;
        inline constexpr float kColorButtonRowHeight = 52.f;
        [[deprecated("use spacing::kCheckboxSize")]] inline constexpr float checkbox_size =
            kCheckboxSize;
        [[deprecated("use spacing::kCheckboxLabelGap")]] inline constexpr float checkbox_label_gap =
            kCheckboxLabelGap;
        [[deprecated("use spacing::kGroupTitleHeight")]] inline constexpr float group_title_height =
            kGroupTitleHeight;
        [[deprecated(
            "use spacing::kGroupInnerPadding")]] inline constexpr float group_inner_padding =
            kGroupInnerPadding;
        [[deprecated("use spacing::kGroupSubTabTop")]] inline constexpr float group_subtab_top =
            kGroupSubTabTop;
    } // namespace spacing

    /// An 8-bit RGBA color; the default is opaque black.
    class Color {
    public:
        /// Creates opaque black.
        Color() = default;

        /// Creates an 8-bit RGBA color; alpha defaults to fully opaque.
        Color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 255)
            : r(red), g(green), b(blue), a(alpha) {}

        unsigned char r = 0;
        unsigned char g = 0;
        unsigned char b = 0;
        unsigned char a = 255;
    };

    /// Colors used by built-in widgets.
    struct Theme {
        Color window_bg{28, 30, 34};
        Color window_header{36, 38, 42};
        Color groupbox_bg{32, 34, 38};
        Color border_outer{12, 14, 17};
        Color border_inner{58, 61, 66};
        Color accent{255, 140, 0};
        Color text_primary{238, 240, 242};
        Color text_dim{157, 162, 168};
        Color control_bg{45, 48, 53};
        Color control_hover{62, 66, 72};
    };

    /// A position or size in floating-point pixels.
    class Point {
    public:
        /// Creates the zero point.
        Point() = default;

        /// Creates a point or size in floating-point pixels.
        Point(float x_value, float y_value) : x(x_value), y(y_value) {}

        float x = 0.f;
        float y = 0.f;
    };

    /// A top-left-origin rectangle with floating-point pixel dimensions.
    struct Rect {
        Point position{};
        Point size{};

        /// Tests containment using a half-open right and bottom edge.
        bool contains(Point point) const {
            return point.x >= position.x && point.y >= position.y &&
                   point.x < position.x + size.x && point.y < position.y + size.y;
        }
    };

    /// A backend-neutral vertex with pixel position, texture coordinates, and color.
    class Vertex {
    public:
        /// Creates a zero-initialized vertex.
        Vertex() = default;
        /// Creates a vertex with pixel position, texture coordinates, and color.
        Vertex(Point position_value, Point coordinates_value, Color color_value)
            : position(position_value), coordinates(coordinates_value), color(color_value) {}

        Point position{};
        Point coordinates{};
        Color color{};
    };

    /// Primitive assembly used when submitting a sequence of vertices.
    enum class VertexTopology { LINE_LIST, LINE_STRIP, TRIANGLE_LIST, TRIANGLE_STRIP };

    /// Rendering contract implemented by a platform or graphics API adapter.
    class Graphics {
    private:
        struct ClipRect {
            int x, y, w, h;
        };
        std::vector<ClipRect> clip_stack_;

    protected:
        std::vector<Vertex> vertices_;
        VertexTopology vertex_topology_ = VertexTopology::TRIANGLE_LIST;

        virtual void append_geometry(const std::vector<Vertex>& vertices, std::size_t count);
        virtual void append_geometry(const Vertex* vertices, std::size_t count);
        virtual void set_topology(VertexTopology vertex_topology);

    public:
        /// Creates a graphics adapter with an empty clip stack.
        Graphics() = default;
        /// Allows deleting a backend through the base interface.
        virtual ~Graphics() = default;
        Graphics(const Graphics&) = delete;
        Graphics& operator=(const Graphics&) = delete;
        Graphics(Graphics&&) = delete;
        Graphics& operator=(Graphics&&) = delete;

        /// Submits pending geometry for the current frame.
        virtual void draw() = 0;
        /// Flushes pending drawing; the default implementation calls draw().
        virtual void flush();
        /// Clears frame-local geometry and backend state.
        virtual void reset() = 0;
        /// Applies a pixel clip rectangle; zero dimensions clip all drawing.
        virtual void set_clip_rect(int x, int y, int w, int h) = 0;
        /// Removes the active pixel clip.
        virtual void reset_clip_rect() = 0;
        /// Draws printable ASCII text at a top-left-origin pixel position.
        virtual void draw_text(const std::string& text, Point position, Color color) = 0;
        /// Measures text using the same glyph advances as draw_text().
        virtual float measure_text_width(const std::string& text) const = 0;
        /// Returns the active drawable size in pixels, or zero when unavailable.
        virtual Point viewport_size() const {
            return Point(0.f, 0.f);
        }
        /// Truncates text to fit a pixel width and optionally records truncation.
        std::string fit_text(const std::string& text, float width, bool track = true) const;

        /// Pushes a clip intersected with all current ancestor clips.
        void push_clip_rect(int x, int y, int w, int h);
        /// Restores the parent clip; unmatched pops are ignored.
        void pop_clip_rect();
        /// Returns the number of active nested clips.
        std::size_t clip_depth() const;
        /// Appends a line to the current geometry batch.
        void draw_line(Point start, Point end, Color color);
        /// Draws an unfilled rectangle in pixel coordinates.
        void draw_rect(Point position, Point size, Color color);
        /// Draws a filled rectangle in pixel coordinates.
        void fill_rect(Point position, Point size, Color color);
        /// Compatibility alias for fill_rect().
        [[deprecated("use fill_rect")]] void draw_filled_rect(Point position, Point size,
                                                              Color color) {
            fill_rect(position, size, color);
        }
        /// Draws a rectangle with four independently colored corners.
        void draw_gradient_rect(Point position, Point size, Color top_left, Color top_right,
                                Color bottom_right, Color bottom_left);
        /// Draws a framed rectangle using the active theme's border colors.
        void draw_frame(Point position, Point size, Color fill);
    };

    /// Input state adapter; one-shot events are consumed or cleared per frame.
    class Input {
    public:
        /// Creates an empty input adapter.
        Input() = default;
        /// Allows deleting a backend through the base interface.
        virtual ~Input() = default;
        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;
        Input(Input&&) = delete;
        Input& operator=(Input&&) = delete;

        /// Reports whether a key is currently held.
        virtual bool key_pressed(int key) const = 0;
        /// Tests a point against a pixel-space rectangle.
        virtual bool mouse_over(Point position, Point size) const = 0;
        /// Returns the current pointer position in pixel coordinates.
        virtual Point mouse_position() const = 0;
        /// Reports the left-button press transition for the current frame.
        virtual bool mouse_button_pressed() const = 0;
        /// Returns the current frame's wheel delta.
        virtual int mouse_wheel_delta() const = 0;
        /// Removes one queued text byte, if available.
        virtual bool pop_text_input(char& character) = 0;
        /// Removes one queued key-press code, if available.
        virtual std::optional<int> poll_pressed_key() = 0;
        /// Maps a backend key code to display text.
        virtual std::string key_name(int key) const = 0;
        /// Reports a left cursor-key press transition.
        virtual bool text_cursor_left_pressed() const {
            return false;
        }
        /// Reports a right cursor-key press transition.
        virtual bool text_cursor_right_pressed() const {
            return false;
        }
        /// Reports a Home-key press transition.
        virtual bool text_cursor_home_pressed() const {
            return false;
        }
        /// Reports an End-key press transition.
        virtual bool text_cursor_end_pressed() const {
            return false;
        }
        /// Reports a Delete-key press transition.
        virtual bool text_delete_pressed() const {
            return false;
        }
        /// Mouse buttons that may be queried as held state.
        enum class MouseButton { left, middle, right };
        /// Reports whether a mouse button remains held.
        virtual bool mouse_button_down(MouseButton button) const {
            return button == MouseButton::left && mouse_button_pressed();
        }
        /// Returns accumulated pointer movement since the last frame reset.
        virtual Point mouse_delta() const {
            return Point(0.f, 0.f);
        }
        /// Reports a double-click transition.
        virtual bool mouse_double_clicked() const {
            return false;
        }
        /// Reports an Escape-key press transition.
        virtual bool escape_pressed() const {
            return false;
        }
    };

    /// Pointer interaction data delivered to a Viewport callback.
    struct ViewportInput {
        Point mouse_delta;
        int wheel_delta = 0;
        bool rotating = false;
        bool panning = false;
        bool double_clicked = false;
    };

    class WindowManager;
    class TextInput;

    /// Process-wide active backend, theme, focus, and diagnostics state.
    namespace core {
        inline std::shared_ptr<zgui::Graphics> graphics = nullptr;
        inline std::shared_ptr<zgui::Input> input = nullptr;
        inline zgui::WindowManager* window_manager = nullptr;
        inline zgui::TextInput* focused_text_input = nullptr;
        inline Theme default_theme{};
        inline Theme active_theme = default_theme;
        inline bool collect_text_truncations = false;
        inline std::vector<std::string> text_truncations;

        /// Checks that graphics and input were installed before UI operations.
        inline bool require_initialized() {
            const bool ready = graphics != nullptr && input != nullptr;
            assert(ready && "zgui core is not initialized; call core::initialize first");
            return ready;
        }

        /// Replaces the active theme for subsequent drawing.
        inline void set_theme(const Theme& theme) {
            active_theme = theme;
        }

        /// Restores the default theme.
        inline void reset_theme() {
            active_theme = default_theme;
        }

        /// Clears core references before the backend or graphics context is released.
        inline void shutdown() {
            focused_text_input = nullptr;
            window_manager = nullptr;
            graphics.reset();
            input.reset();
            collect_text_truncations = false;
            text_truncations.clear();
            reset_theme();
        }

        /// Installs the sole active backend pair; null arguments assert in debug builds.
        inline void initialize(std::shared_ptr<zgui::Graphics> new_graphics,
                               std::shared_ptr<zgui::Input> new_input) {
            assert(new_graphics && new_input &&
                   "core::initialize requires non-null graphics and input");
            if (!new_graphics || !new_input) {
                shutdown();
                return;
            }
            shutdown();
            graphics = std::move(new_graphics);
            input = std::move(new_input);
        }
    } // namespace core

    /// Base node for the retained widget tree.
    class UIElement {
        friend class WindowManager;

    private:
        struct ChildSnapshot {
            std::vector<UIElement*> elements;
            std::vector<std::shared_ptr<UIElement>> owners;
        };

        ChildSnapshot snapshot_children() const;

    protected:
        std::vector<UIElement*> children_;
        std::vector<std::shared_ptr<UIElement>> owned_children_;
        std::string title_;
        UIElement* parent_ = nullptr;
        Point position_{0.f, 0.f}, size_{0.f, 0.f};
        bool anchor_left_ = true;
        bool anchor_right_ = false;
        bool anchor_top_ = true;
        bool anchor_bottom_ = false;
        bool horizontal_constraints_ = false;
        bool vertical_constraints_ = false;
        float horizontal_left_fraction_ = 0.f;
        float horizontal_right_fraction_ = 1.f;
        float horizontal_left_offset_ = 0.f;
        float horizontal_right_offset_ = 0.f;
        float vertical_top_fraction_ = 0.f;
        float vertical_bottom_fraction_ = 1.f;
        float vertical_top_offset_ = 0.f;
        float vertical_bottom_offset_ = 0.f;

        void adjust_for_parent_resize(Point old_size, Point new_size);

    public:
        /// Creates an unattached element with zero position and size.
        UIElement() = default;
        /// Detaches this element from its parent and children.
        virtual ~UIElement();
        UIElement(const UIElement&) = delete;
        UIElement& operator=(const UIElement&) = delete;
        UIElement(UIElement&&) = delete;
        UIElement& operator=(UIElement&&) = delete;

        /// Adds a non-owning child; the caller must keep it alive until detached.
        void add_child(UIElement* child);
        /// Adds and retains a child using shared ownership.
        void add_child(const std::shared_ptr<UIElement>& child);
        /// Detaches a child without destroying it.
        void remove_child(UIElement* child);
        /// Detaches every child and releases children owned by this element.
        void clear_children();
        /// Changes the parent link, detaching from any previous parent.
        void set_parent(UIElement* parent);

        /// Draws this element and its visible children.
        virtual void draw();
        /// Updates this element and its children from the active input adapter.
        virtual void update();
        /// Handles a mouse press and reports whether it was consumed.
        virtual bool handle_mouse_press(Point position);
        /// Reports whether this element wants the wheel event at a point.
        virtual bool handles_mouse_wheel(Point position) const;

        /// Returns the parent-relative position in pixels.
        const Point& get_position() const;
        /// Returns the top-left position in screen coordinates.
        Point get_absolute_position() const;
        /// Sets the parent-relative top-left position in pixels.
        void set_position(const Point& position);
        /// Selects which edges retain their offsets when the parent resizes.
        void set_anchors(bool left, bool right, bool top, bool bottom);
        /// Constrains horizontal edges to parent fractions plus pixel offsets.
        void set_horizontal_constraints(float left_fraction, float right_fraction,
                                        float left_offset, float right_offset);
        /// Constrains vertical edges to parent fractions plus pixel offsets.
        void set_vertical_constraints(float top_fraction, float bottom_fraction, float top_offset,
                                      float bottom_offset);

        /// Returns the element size in pixels.
        const Point& get_size() const;
        /// Sets the element size in pixels and adjusts anchored children.
        virtual void set_size(const Point& size);
        /// Tests screen-space containment, respecting ancestor clips.
        bool contains_point(Point position) const;
        /// Returns the screen-space clip rectangle for child drawing.
        virtual Rect child_clip_rect() const;
        /// Returns the screen-space offset applied to child positions.
        virtual Point child_offset() const;
        /// Returns the lowest child edge in parent-relative pixels.
        virtual float content_bottom() const;
        /// Returns the rightmost child edge in parent-relative pixels.
        virtual float content_right() const;
        /// Counts this element and all descendants.
        virtual std::size_t widget_count() const;
        /// Counts this element and descendants intersecting visible clips.
        virtual std::size_t visible_widget_count() const;

        /// Sets the title/text used by the element.
        void set_title(std::string title);
        /// Returns the current title/text.
        const std::string& get_title() const;
    };

    /// Draws and updates one element in that order.
    inline void draw_and_update(UIElement& element) {
        element.draw();
        element.update();
    }

    /// Dereferences a pointer-like object and draws/updates its element.
    template <class P, class = decltype(*std::declval<const P&>())>
    inline void draw_and_update(const P& element) {
        draw_and_update(*element);
    }

    /// Horizontal alignment used by Label.
    enum class TextAlignment { left, center, right };

    /// Direction of a Separator rule.
    enum class SeparatorOrientation { horizontal, vertical };

    /// Draws a theme-colored divider, optionally labeled when horizontal.
    class Separator : public UIElement {
    private:
        SeparatorOrientation orientation_;

    public:
        /// Creates a horizontal separator unless a vertical direction is selected.
        explicit Separator(SeparatorOrientation orientation = SeparatorOrientation::horizontal,
                           std::string label = {});

        /// Changes the divider direction.
        void set_orientation(SeparatorOrientation orientation);
        /// Replaces the optional label; vertical separators do not display labels.
        void set_label(std::string label);
        /// Draws the divider using the active theme's inner border color.
        void draw() override;
    };

    /// Displays ASCII text with alignment and optional explicit color.
    class Label : public UIElement {
    private:
        bool has_color_override_ = false;
        Color color_override_;
        TextAlignment alignment_ = TextAlignment::left;

    public:
        /// Creates a label with optional horizontal alignment.
        explicit Label(std::string text, TextAlignment alignment = TextAlignment::left);

        /// Replaces the displayed text.
        void set_text(std::string text);
        /// Changes horizontal alignment within the label's width.
        void set_alignment(TextAlignment alignment);
        /// Overrides the theme text color until clear_color_override().
        void set_color(Color color);
        /// Resumes using the active theme's text color.
        void clear_color_override();
        /// Draws text clipped/truncated to the label bounds.
        void draw() override;
    };

    /// Activates a callback when a press is released inside the button.
    class Button : public UIElement {
    private:
        std::function<void()> on_click_;
        bool pressed_ = false;

    public:
        /// Creates a button with an optional click callback.
        explicit Button(std::string text, std::function<void()> on_click = {});

        /// Replaces the click callback.
        void set_on_click(std::function<void()> on_click);
        /// Draws the button with the active theme.
        void draw() override;
        /// Tracks press/release and invokes the callback on an inside release.
        void update() override;
        /// Begins a button press when the point is inside its bounds.
        bool handle_mouse_press(Point position) override;
    };

    /// A boolean control bound to caller-owned or shared-owned storage.
    class CheckBox : public UIElement {
    private:
        bool fallback_value_ = false;
        bool* value_ = &fallback_value_;
        std::shared_ptr<bool> owned_value_;
        std::function<void()> on_change_;

    public:
        /// Creates a checkbox with a non-owning pointer binding.
        CheckBox(std::string label, bool* value, std::function<void()> on_change = {});
        /// Creates a checkbox with a non-owning reference binding.
        CheckBox(std::string label, bool& value, std::function<void()> on_change = {})
            : CheckBox(std::move(label), &value, std::move(on_change)) {}
        /// Creates a checkbox retaining the Bound value's shared ownership.
        CheckBox(std::string label, const Bound<bool>& value, std::function<void()> on_change = {})
            : CheckBox(std::move(label), value.get(), std::move(on_change)) {
            owned_value_ = value.owner();
        }

        /// Replaces the callback invoked after a toggle.
        void set_on_change(std::function<void()> on_change);
        /// Changes the non-owning binding; null uses internal fallback storage.
        void set_value(bool* value);
        /// Draws the current bound value.
        void draw() override;
        /// Toggles the value and invokes the callback when hit.
        bool handle_mouse_press(Point position) override;
    };

    /// A button that toggles a bound boolean when released inside its bounds.
    class ToggleButton : public UIElement {
    private:
        bool fallback_value_ = false;
        bool* value_ = &fallback_value_;
        std::shared_ptr<bool> owned_value_;
        std::function<void()> on_change_;
        bool pressed_ = false;

    public:
        /// Creates a toggle button with non-owning boolean storage.
        ToggleButton(std::string label, bool* value, std::function<void()> on_change = {});
        /// Creates a toggle button with non-owning boolean storage.
        ToggleButton(std::string label, bool& value, std::function<void()> on_change = {})
            : ToggleButton(std::move(label), &value, std::move(on_change)) {}
        /// Creates a toggle button retaining the Bound value's shared ownership.
        ToggleButton(std::string label, const Bound<bool>& value,
                     std::function<void()> on_change = {});

        /// Replaces the callback invoked after a value change.
        void set_on_change(std::function<void()> on_change);
        /// Changes the boolean value and notifies only when it changes.
        void set_value(bool value);
        /// Returns the current bound value.
        bool get_value() const;
        /// Draws the button with its current toggle state.
        void draw() override;
        /// Toggles only when the press is released inside the control.
        void update() override;
        /// Begins a press when the point is inside the control.
        bool handle_mouse_press(Point position) override;
    };

    /// Selects one labeled option and stores its index in bound size_t storage.
    class RadioGroup : public UIElement {
    private:
        static constexpr float row_height_ = 18.f;

        std::vector<std::string> items_;
        std::size_t fallback_index_ = 0;
        std::size_t* selected_index_ = &fallback_index_;
        std::shared_ptr<std::size_t> owned_index_;
        std::function<void(std::size_t)> on_change_;

    public:
        /// Creates a vertical option list with non-owning selection storage.
        RadioGroup(std::vector<std::string> items, std::size_t* selected_index,
                   std::function<void(std::size_t)> on_change = {});
        /// Creates a vertical option list with non-owning selection storage.
        RadioGroup(std::vector<std::string> items, std::size_t& selected_index,
                   std::function<void(std::size_t)> on_change = {})
            : RadioGroup(std::move(items), &selected_index, std::move(on_change)) {}
        /// Creates a vertical option list retaining the Bound index.
        RadioGroup(std::vector<std::string> items, const Bound<std::size_t>& selected_index,
                   std::function<void(std::size_t)> on_change = {});

        /// Sets the selected index; returns false for an empty list or invalid index.
        bool set_selected_index(std::size_t index, bool notify = true);
        /// Returns the currently bound selection index.
        std::size_t get_selected_index() const;
        /// Replaces the callback receiving the newly selected index.
        void set_on_change(std::function<void(std::size_t)> on_change);
        /// Draws one theme-styled row per option.
        void draw() override;
        /// Selects the row containing the press point.
        bool handle_mouse_press(Point position) override;
    };

    /// Displays a bound floating-point value clamped to the normalized range [0, 1].
    class ProgressBar : public UIElement {
    private:
        float fallback_value_ = 0.f;
        float* value_ = &fallback_value_;
        std::shared_ptr<float> owned_value_;

    public:
        /// Creates a progress bar with non-owning value storage and optional text.
        explicit ProgressBar(float* value, std::string text = {});
        /// Creates a progress bar with non-owning value storage and optional text.
        explicit ProgressBar(float& value, std::string text = {})
            : ProgressBar(&value, std::move(text)) {}
        /// Creates a progress bar retaining the Bound value's shared ownership.
        explicit ProgressBar(const Bound<float>& value, std::string text = {});

        /// Sets the normalized value, clamped to [0, 1].
        void set_value(float value);
        /// Returns the current normalized value clamped to [0, 1].
        float get_value() const;
        /// Replaces the optional centered text.
        void set_text(std::string text);
        /// Draws the track, filled fraction, and optional text.
        void draw() override;
    };

    /// Shows a delayed, viewport-clamped text overlay while its bounds are hovered.
    class Tooltip : public UIElement {
    private:
        std::string text_;
        std::chrono::milliseconds delay_;
        std::optional<std::chrono::steady_clock::time_point> hover_started_;
        std::shared_ptr<UIElement> popup_;

        void dismiss();

    public:
        /// Creates an invisible hover target with a delay defaulting to 500 ms.
        explicit Tooltip(std::string text,
                         std::chrono::milliseconds delay = std::chrono::milliseconds(500));
        ~Tooltip() override;

        /// Replaces the tooltip text and restarts the hover delay.
        void set_text(std::string text);
        /// Changes the hover delay; negative durations become zero.
        void set_delay(std::chrono::milliseconds delay);
        /// Keeps the target invisible; visible text is drawn by its overlay.
        void draw() override;
        /// Opens or dismisses the single active tooltip for the hovered target.
        void update() override;
    };

    /// A draggable arithmetic value control clamped to its inclusive range.
    template <typename T> class Slider : public UIElement {
        static_assert(std::is_arithmetic<T>::value, "Slider requires an arithmetic value type");

    private:
        T fallback_value_{};
        T* value_ = &fallback_value_;
        std::shared_ptr<T> owned_value_;
        T minimum_;
        T maximum_;
        std::function<void()> on_change_;
        bool dragging_ = false;

        std::string format_value() const {
            std::ostringstream stream;
            if constexpr (std::is_floating_point<T>::value) {
                stream << std::fixed << std::setprecision(2) << *value_;
            } else {
                stream << *value_;
            }
            return stream.str();
        }

        void update_from_mouse() {
            const Point absolute = get_absolute_position();
            const float width = get_size().x;
            const float mouse_x = core::input->mouse_position().x;
            if (!std::isfinite(width) || width <= 0.f || !std::isfinite(absolute.x) ||
                !std::isfinite(mouse_x))
                return;
            const float ratio = std::clamp((mouse_x - absolute.x) / width, 0.f, 1.f);
            if (!std::isfinite(ratio))
                return;
            const long double minimum = static_cast<long double>(minimum_);
            const long double maximum = static_cast<long double>(maximum_);
            const long double mapped =
                minimum * (1.L - static_cast<long double>(ratio)) +
                maximum * static_cast<long double>(ratio);
            T next;
            if constexpr (std::is_integral<T>::value) {
                const long double rounded = std::round(mapped);
                if (rounded <= minimum)
                    next = minimum_;
                else if (rounded >= maximum)
                    next = maximum_;
                else
                    next = static_cast<T>(rounded);
            } else {
                next = static_cast<T>(std::clamp(mapped, minimum, maximum));
            }
            set_value(next);
        }

    public:
        /// Creates a slider with a non-owning pointer binding.
        Slider(std::string title, T* value, T minimum, T maximum,
               std::function<void()> on_change = {})
            : value_(value), minimum_(std::min(minimum, maximum)),
              maximum_(std::max(minimum, maximum)), on_change_(std::move(on_change)) {
            if (value == nullptr) {
                assert(value != nullptr && "slider value must not be null");
                value_ = &fallback_value_;
            }
            *value_ = std::clamp(*value_, minimum_, maximum_);
            set_title(std::move(title));
        }

        /// Creates a slider with a non-owning reference binding.
        Slider(std::string title, T& value, T minimum, T maximum,
               std::function<void()> on_change = {})
            : Slider(std::move(title), &value, minimum, maximum, std::move(on_change)) {}

        /// Creates a slider retaining the Bound value's shared ownership.
        Slider(std::string title, const Bound<T>& value, T minimum, T maximum,
               std::function<void()> on_change = {})
            : Slider(std::move(title), value.get(), minimum, maximum, std::move(on_change)) {
            owned_value_ = value.owner();
        }

        /// Returns the currently clamped value.
        T get_value() const {
            return *value_;
        }

        /// Clamps and stores a value, calling the callback only on change.
        void set_value(T value) {
            const T clamped = std::clamp(value, minimum_, maximum_);
            if (clamped == *value_)
                return;
            *value_ = clamped;
            if (on_change_)
                on_change_();
        }

        /// Replaces the callback invoked after a value change.
        void set_on_change(std::function<void()> on_change) {
            on_change_ = std::move(on_change);
        }

        /// Draws the label, value, track, and thumb.
        void draw() override {
            const Point absolute = get_absolute_position();
            const Point size = get_size();
            const Theme& theme = core::active_theme;
            auto& graphics = *core::graphics;
            const std::string value_text = format_value();
            const float value_width = graphics.measure_text_width(value_text);
            const std::string title =
                graphics.fit_text(title_, std::max(0.f, size.x - value_width - 4.f));
            const std::string fitted_value =
                graphics.fit_text(value_text, std::max(0.f, size.x - 4.f));
            graphics.draw_text(title, Point(absolute.x, absolute.y), theme.text_primary);
            graphics.draw_text(
                fitted_value,
                Point(absolute.x + size.x - graphics.measure_text_width(fitted_value), absolute.y),
                theme.text_dim);

            const float track_y = absolute.y + spacing::kSliderLabelHeight;
            graphics.fill_rect(Point(absolute.x, track_y),
                               Point(size.x, spacing::kSliderTrackHeight), theme.border_outer);
            graphics.fill_rect(Point(absolute.x + spacing::kSliderTrackInset,
                                     track_y + spacing::kSliderTrackInset),
                               Point(std::max(0.f, size.x - 2.f * spacing::kSliderTrackInset),
                                     spacing::kSliderInnerTrackHeight),
                               theme.control_bg);
            const double range = static_cast<double>(maximum_) - static_cast<double>(minimum_);
            const float ratio =
                range == 0.0
                    ? 0.f
                    : static_cast<float>(
                          (static_cast<double>(*value_) - static_cast<double>(minimum_)) / range);
            const float thumb_x =
                absolute.x + std::max(0.f, size.x - spacing::kSliderThumbWidth) * ratio;
            graphics.draw_frame(Point(thumb_x, track_y - spacing::kSliderThumbVerticalOffset),
                                Point(spacing::kSliderThumbWidth, spacing::kSliderThumbHeight),
                                dragging_ ? theme.control_hover : theme.text_dim);
        }

        /// Updates a drag until the primary mouse button is released.
        void update() override {
            if (!dragging_)
                return;
            if (!core::input->mouse_button_pressed()) {
                dragging_ = false;
                return;
            }
            update_from_mouse();
        }

        /// Starts dragging when pressed inside the slider.
        bool handle_mouse_press(Point position) override {
            if (!contains_point(position))
                return false;
            dragging_ = true;
            update_from_mouse();
            return true;
        }
    };

    namespace detail {
        struct ComboBoxData;
    }

    /// Selects one string and uses a manager-owned popup for its choices.
    class ComboBox : public UIElement {
    private:
        std::shared_ptr<detail::ComboBoxData> data_;
        std::weak_ptr<UIElement> popup_;
        WindowManager* manager_ = nullptr;

    public:
        /// Creates a combo box; an empty item list has selection index zero.
        explicit ComboBox(std::vector<std::string> items = {}, std::size_t selected = 0);

        /// Replaces choices and clamps/clears the current selection as needed.
        void set_items(std::vector<std::string> items);
        /// Selects an item, returning false for empty or out-of-range indices.
        bool set_selected_index(std::size_t index, bool notify = true);
        /// Binds the selection to non-owning index storage.
        void bind_selected_index(std::size_t* index, bool notify = false);
        /// Binds the selection to non-owning index storage.
        void bind_selected_index(std::size_t& index, bool notify = false) {
            bind_selected_index(&index, notify);
        }
        /// Binds the selection and retains the Bound value's shared ownership.
        void bind_selected_index(const Bound<std::size_t>& index, bool notify = false);
        /// Returns zero when the choice list is empty.
        std::size_t get_selected_index() const;
        /// Returns an empty string when the choice list is empty.
        const std::string& get_selected_item() const;
        /// Sets the callback receiving the newly selected index.
        void set_on_change(std::function<void(std::size_t)> on_change);
        /// Draws the control and any active popup through WindowManager.
        void draw() override;
        /// Opens the popup or consumes a popup item press.
        bool handle_mouse_press(Point position) override;
    };

    namespace detail {
        struct MultiComboBoxData;
    }

    /// Selects multiple strings using a bound vector of boolean values.
    class MultiComboBox : public UIElement {
    private:
        std::shared_ptr<detail::MultiComboBoxData> data_;
        std::weak_ptr<UIElement> popup_;
        WindowManager* manager_ = nullptr;

    public:
        /// Creates a multi-select control with non-owning value storage.
        MultiComboBox(std::vector<std::string> items, std::vector<bool>* values,
                      std::function<void()> on_change = {});
        /// Creates a multi-select control with non-owning value storage.
        MultiComboBox(std::vector<std::string> items, std::vector<bool>& values,
                      std::function<void()> on_change = {})
            : MultiComboBox(std::move(items), &values, std::move(on_change)) {}
        /// Creates a control retaining the Bound vector's shared ownership.
        MultiComboBox(std::vector<std::string> items, const Bound<std::vector<bool>>& values,
                      std::function<void()> on_change = {});

        /// Replaces choices and resizes values, preserving existing entries.
        void set_items(std::vector<std::string> items);
        /// Returns the current values; its size matches the item list.
        const std::vector<bool>& values() const;
        /// Replaces the callback invoked after toggling an item.
        void set_on_change(std::function<void()> on_change);
        /// Draws the control and any active popup.
        void draw() override;
        /// Opens the popup or consumes an item toggle.
        bool handle_mouse_press(Point position) override;
    };

    /// Edits a bound printable-ASCII string and maintains the global focus pointer.
    class TextInput : public UIElement {
    private:
        std::string fallback_value_;
        std::string* value_ = &fallback_value_;
        std::shared_ptr<std::string> owned_value_;
        std::function<void()> on_change_;
        std::size_t cursor_ = 0;
        bool password_mode_ = false;

    public:
        /// Creates an input with non-owning string storage.
        explicit TextInput(std::string* value, std::function<void()> on_change = {});
        /// Creates an input with non-owning string storage.
        explicit TextInput(std::string& value, std::function<void()> on_change = {});
        /// Creates an input retaining the Bound string's shared ownership.
        explicit TextInput(const Bound<std::string>& value, std::function<void()> on_change = {});
        /// Clears global focus if this input currently owns it.
        ~TextInput() override;

        /// Replaces the callback invoked after editing.
        void set_on_change(std::function<void()> on_change);
        /// Changes the non-owning binding; null uses internal fallback storage.
        void set_value(std::string* value);
        /// Masks displayed characters without changing the bound value.
        void set_password_mode(bool enabled);
        /// Draws the current string and caret.
        void draw() override;
        /// Processes text bytes and cursor/edit key transitions while focused.
        void update() override;
        /// Focuses the input and positions its caret when hit.
        bool handle_mouse_press(Point position) override;
    };

    /// Captures a key code after a click and displays its backend-provided name.
    class KeyBind : public UIElement {
    private:
        int fallback_key_ = 0;
        int* key_ = &fallback_key_;
        std::shared_ptr<int> owned_key_;
        std::function<void()> on_change_;
        bool listening_ = false;

    public:
        /// Creates a key binding with non-owning key-code storage.
        KeyBind(std::string title, int* key, std::function<void()> on_change = {});
        /// Creates a key binding with non-owning key-code storage.
        KeyBind(std::string title, int& key, std::function<void()> on_change = {})
            : KeyBind(std::move(title), &key, std::move(on_change)) {}
        /// Creates a key binding retaining the Bound value's shared ownership.
        KeyBind(std::string title, const Bound<int>& key, std::function<void()> on_change = {});

        /// Stores a key code and invokes the callback when it changes.
        void set_key(int key);
        /// Returns the stored key code.
        int get_key() const;
        /// Cancels an in-progress key capture without changing the binding.
        void cancel_capture();
        /// Replaces the callback invoked after a key is captured.
        void set_on_change(std::function<void()> on_change);
        /// Draws the title and backend key name.
        void draw() override;
        /// Captures one key or cancels listening on Escape.
        void update() override;
        /// Starts listening when pressed inside the control.
        bool handle_mouse_press(Point position) override;
    };

    /// Edits a color through saturation/value, hue, and alpha controls.
    class ColorPicker : public UIElement {
    private:
        enum class DragTarget { none, saturation_value, hue, alpha };

        Color fallback_value_{};
        Color* value_ = &fallback_value_;
        std::shared_ptr<Color> owned_value_;
        std::function<void()> on_change_;
        float hue_ = 0.f;
        float saturation_ = 0.f;
        float brightness_ = 0.f;
        float alpha_ = 1.f;
        DragTarget drag_target_ = DragTarget::none;

        void update_from_mouse(Point position);
        void apply_color();

    public:
        /// Creates a picker with non-owning color storage.
        explicit ColorPicker(Color* value, std::function<void()> on_change = {});
        /// Creates a picker with non-owning color storage.
        explicit ColorPicker(Color& value, std::function<void()> on_change = {})
            : ColorPicker(&value, std::move(on_change)) {}
        /// Creates a picker retaining the Bound color's shared ownership.
        explicit ColorPicker(const Bound<Color>& value, std::function<void()> on_change = {});

        /// Draws the color planes and current selection.
        void draw() override;
        /// Updates active hue, saturation/value, or alpha drags.
        void update() override;
        /// Begins interaction with one of the color planes.
        bool handle_mouse_press(Point position) override;
    };

    /// Displays a bound color and opens a color picker popup when clicked.
    class ColorSwatch : public UIElement {
    private:
        std::shared_ptr<Color> fallback_value_ = std::make_shared<Color>();
        Color* value_ = fallback_value_.get();
        std::shared_ptr<Color> owned_value_ = fallback_value_;
        std::function<void()> on_change_;
        std::weak_ptr<UIElement> picker_;
        bool vertical_layout_ = false;

    public:
        /// Creates a labeled swatch with non-owning color storage.
        ColorSwatch(std::string label, Color* value, std::function<void()> on_change = {});
        /// Creates a labeled swatch with non-owning color storage.
        ColorSwatch(std::string label, Color& value, std::function<void()> on_change = {})
            : ColorSwatch(std::move(label), &value, std::move(on_change)) {}
        /// Creates a labeled swatch retaining the Bound color.
        ColorSwatch(std::string label, const Bound<Color>& value,
                    std::function<void()> on_change = {});
        /// Creates an unlabeled swatch with non-owning color storage.
        explicit ColorSwatch(Color* value, std::function<void()> on_change = {});
        /// Creates an unlabeled swatch with non-owning color storage.
        explicit ColorSwatch(Color& value, std::function<void()> on_change = {})
            : ColorSwatch(&value, std::move(on_change)) {}
        /// Creates an unlabeled swatch retaining the Bound color.
        explicit ColorSwatch(const Bound<Color>& value, std::function<void()> on_change = {});

        /// Replaces the callback invoked after a color edit.
        void set_on_change(std::function<void()> on_change);
        /// Selects horizontal or vertical label/swatch arrangement.
        void set_vertical_layout(bool vertical);
        /// Opens or interacts with the color picker popup when hit.
        bool handle_mouse_press(Point position) override;
        /// Draws the label and current color.
        void draw() override;
    };

    /// A labeled color control sized for compact horizontal rows.
    class ColorButton : public UIElement {
    private:
        std::shared_ptr<Color> fallback_value_ = std::make_shared<Color>();
        Color* value_ = fallback_value_.get();
        std::shared_ptr<Color> owned_value_ = fallback_value_;
        std::function<void()> on_change_;
        std::weak_ptr<UIElement> picker_;
        bool pressed_ = false;

    public:
        /// Creates a button with non-owning color storage.
        ColorButton(std::string label, Color* value, std::function<void()> on_change = {});
        /// Creates a button with non-owning color storage.
        ColorButton(std::string label, Color& value, std::function<void()> on_change = {})
            : ColorButton(std::move(label), &value, std::move(on_change)) {}
        /// Creates a button retaining the Bound color.
        ColorButton(std::string label, const Bound<Color>& value,
                    std::function<void()> on_change = {});

        /// Opens the color picker when hit.
        bool handle_mouse_press(Point position) override;
        /// Tracks the button's pressed state and picker interaction.
        void update() override;
        /// Draws the label and current color.
        void draw() override;
    };

    /// Shows one scrollable page selected from a row of tabs.
    class TabBar : public UIElement {
        friend class WindowManager;

    protected:
        struct TabLayout {
            std::vector<float> widths;
            float padding = spacing::kTabLabelHorizontalPadding;
            float text_scale = 1.f;
        };

        struct Tab {
            std::string title;
            std::shared_ptr<UIElement> page;
        };

        std::vector<Tab> tabs_;
        std::size_t active_tab_ = 0;
        float tab_height_;
        float page_scroll_offset_ = 0.f;

        void layout_tabs();
        TabLayout measure_tab_layout(float available_width) const;

    public:
        /// Creates a tab bar with the requested strip height in pixels.
        explicit TabBar(float tab_height = spacing::kTabBarHeight);

        /// Adds a titled page and returns the shared page element.
        std::shared_ptr<UIElement> add_tab(std::string title);
        /// Resizes tabs and page viewports in pixels.
        void set_size(const Point& size) override;
        /// Selects a page; returns false without changing state for an invalid index.
        bool set_active_tab(std::size_t index);
        /// Returns the active page index, or zero when there are no pages.
        std::size_t get_active_tab() const;
        /// Returns the number of pages.
        std::size_t tab_count() const;
        /// Returns a page title; invalid indices yield an empty title.
        const std::string& tab_title(std::size_t index) const;
        /// Counts widgets in one page, or zero for an invalid index.
        std::size_t tab_widget_count(std::size_t index) const;
        /// Counts visible widgets in one page, or zero for an invalid index.
        std::size_t tab_visible_widget_count(std::size_t index) const;
        /// Counts the tab bar and all page widgets.
        std::size_t widget_count() const override;
        /// Counts the tab bar and widgets visible through the active page clip.
        std::size_t visible_widget_count() const override;

        /// Draws tabs and the active clipped page.
        void draw() override;
        /// Handles tab presses and active-page scrolling.
        void update() override;
        /// Selects a tab or delegates to the active page.
        bool handle_mouse_press(Point position) override;
        /// Reports wheel ownership for the active page.
        bool handles_mouse_wheel(Point position) const override;
        /// Returns the screen-space page viewport clip.
        Rect child_clip_rect() const override;
        /// Returns the active page's screen-space offset after scrolling.
        Point child_offset() const override;
        /// Returns the bottom-most tab/page content edge.
        float content_bottom() const override;
        /// Returns the right-most page content edge.
        float content_right() const override;
    };

    /// A compact TabBar intended to be placed inside a GroupBox.
    class SubTabBar : public TabBar {
    public:
        /// Creates a sub-tab strip using the shared compact height by default.
        explicit SubTabBar(float tab_height = spacing::kSubTabBarHeight);
    };

    /// Clips and scrolls child widgets when their content exceeds the viewport.
    class ScrollArea : public UIElement {
    private:
        static constexpr float scrollbar_width_ = 10.f;

        Point scroll_offset_{};
        bool horizontal_scrolling_ = false;
        bool dragging_vertical_scrollbar_ = false;
        bool dragging_horizontal_scrollbar_ = false;

        void scrollbar_visibility(bool& vertical, bool& horizontal) const;
        Point viewport_size() const;
        float maximum_vertical_scroll() const;
        float maximum_horizontal_scroll() const;
        Rect vertical_scrollbar_track() const;
        Rect horizontal_scrollbar_track() const;
        float vertical_thumb_size() const;
        float horizontal_thumb_size() const;
        void scroll_vertical_to(float mouse_y);
        void scroll_horizontal_to(float mouse_x);

    public:
        /// Enables or disables horizontal scrolling; vertical scrolling is automatic.
        void set_horizontal_scrolling(bool enabled);
        /// Reports whether horizontal scrolling is enabled.
        bool horizontal_scrolling() const;
        /// Sets the pixel scroll offset, clamped to the current content bounds.
        void set_scroll_offset(Point offset);
        /// Returns the current horizontal and vertical pixel offsets.
        Point scroll_offset() const;
        /// Resizes the viewport and clamps the scroll offset.
        void set_size(const Point& size) override;

        /// Draws clipped children and any required draggable scrollbars.
        void draw() override;
        /// Updates child widgets, wheel scrolling, and scrollbar dragging.
        void update() override;
        /// Starts scrollbar dragging or delegates a press to children.
        bool handle_mouse_press(Point position) override;
        /// Reports wheel ownership only when this area or a descendant can scroll.
        bool handles_mouse_wheel(Point position) const override;
        /// Returns the screen-space content viewport excluding borders and scrollbars.
        Rect child_clip_rect() const override;
        /// Applies the scroll offset to child positions.
        Point child_offset() const override;
    };

    /// A titled, clipped region that can automatically fit its child content.
    class GroupBox : public UIElement {
        friend class WindowManager;

    private:
        std::shared_ptr<SubTabBar> sub_tabs_;
        bool fit_content_ = true;
        TextAlignment title_alignment_ = TextAlignment::left;

        void layout_sub_tabs();

    public:
        /// Creates a group with an optional title.
        explicit GroupBox(std::string title = {});

        // The group automatically positions and sizes the strip using shared spacing.
        /// Installs a sub-tab strip and lays it out inside the group.
        void set_sub_tabs(const std::shared_ptr<SubTabBar>& tabs);
        /// Returns the installed sub-tab strip, if any.
        std::shared_ptr<SubTabBar> get_sub_tabs() const;
        /// Enables or disables automatic group sizing as rows are added.
        void set_fit_content(bool fit_content);
        /// Sets the title alignment along the top border.
        void set_title_alignment(TextAlignment alignment);
        /// Sizes the group to its content plus non-negative bottom padding.
        void fit_to_content(float bottom_padding = spacing::kGroupBottomPad);
        /// Resizes the group and relays out its sub-tab strip.
        void set_size(const Point& size) override;

        /// Draws the group background, title, and clipped contents.
        void draw() override;
        /// Updates child widgets.
        void update() override;
        /// Dispatches a press to the group or its children.
        bool handle_mouse_press(Point position) override;
        /// Reports wheel ownership for the sub-tab page.
        bool handles_mouse_wheel(Point position) const override;
        /// Returns the inset screen-space child clip.
        Rect child_clip_rect() const override;
        /// Returns the right-most content edge in parent-relative pixels.
        float content_right() const override;
        /// Counts the group and all visible or hidden page widgets.
        std::size_t widget_count() const override;
        /// Counts widgets visible through the active group/page clips.
        std::size_t visible_widget_count() const override;
    };

    /// Places child widgets in rows using pixel padding, height, and gap values.
    class StackLayout {
    private:
        UIElement& parent_;
        GroupBox* group_;
        float cursor_y_;
        float padding_;
        float gap_;

    public:
        /// Creates a layout attached to an ordinary parent element.
        explicit StackLayout(UIElement& parent, float start_y = spacing::kDefaultStackStartY,
                             float padding = spacing::kStackHorizontalPadding,
                             float gap = spacing::kStackRowGap);
        /// Creates a layout that also fits its containing GroupBox to content.
        explicit StackLayout(GroupBox& group, float start_y = spacing::kGroupStackStartY,
                             float padding = spacing::kStackHorizontalPadding,
                             float gap = spacing::kStackRowGap);

        // Layout for a page returned by TabBar::add_tab(). Uses the kPage* spacing so
        // every sub-tab page in an application gets the same rhythm.
        /// Creates a layout with shared page spacing defaults.
        static StackLayout page(UIElement& page_element, float start_y = spacing::kPageStackStartY,
                                float padding = spacing::kPageHorizontalPadding,
                                float gap = spacing::kPageRowGap);

        /// Returns the next row's y coordinate in pixels.
        float cursor_y() const;
        /// Adds a non-owning row; the parent must outlive the layout child.
        void add_row(UIElement& element, float height = spacing::kDefaultRowHeight);
        /// Adds and retains a shared-owned row widget.
        void add_row(const std::shared_ptr<UIElement>& element,
                     float height = spacing::kDefaultRowHeight);
        /// Adds a checkbox/combo row split by a clamped horizontal fraction.
        void add_two_column_row(const std::shared_ptr<CheckBox>& check_box,
                                const std::shared_ptr<ComboBox>& combo_box,
                                float left_fraction = 0.48f,
                                float height = spacing::kDefaultRowHeight);
        /// Adds three equally distributed color-button controls.
        void add_three_column_row(const std::shared_ptr<ColorButton>& first,
                                  const std::shared_ptr<ColorButton>& second,
                                  const std::shared_ptr<ColorButton>& third,
                                  float height = spacing::kColorButtonRowHeight);
    };

    /// Displays resizable columns and a scrollable set of string/color rows.
    class TableView : public UIElement {
    public:
        /// Column title, width in pixels, visibility, and optional fill weight.
        struct Column {
            std::string title;
            float width;
            bool visible = true;
            float weight = 0.f;
        };

        /// Cell text, optional text color, and a user-data marker.
        struct Cell {
            std::string text;
            std::optional<Color> color;
            bool user_data = false;
        };

        using Row = std::vector<Cell>;
        /// Selection callback value indicating that no row is selected.
        static constexpr std::size_t no_selection = static_cast<std::size_t>(-1);

    private:
        std::vector<Column> columns_;
        std::vector<Row> rows_;
        std::vector<std::optional<Color>> row_backgrounds_;
        std::optional<std::size_t> selected_row_;
        std::function<void(std::size_t)> on_selection_changed_;
        float row_height_ = 18.f;
        float scroll_offset_ = 0.f;
        bool dragging_scrollbar_ = false;
        std::optional<std::size_t> resizing_column_;
        std::vector<bool> user_fixed_columns_;
        bool zebra_stripes_ = true;
        float resize_start_x_ = 0.f;
        float resize_start_width_ = 0.f;

        float content_height() const;
        float maximum_scroll() const;
        void scroll_to_mouse(float mouse_y);
        std::vector<float> effective_column_widths(float available_width) const;

    public:
        /// Replaces columns and recalculates weighted widths.
        void set_columns(std::vector<Column> columns);
        /// Sets the viewport size in pixels and clamps the scroll offset.
        void set_size(const Point& size) override;
        /// Enables or disables alternating row backgrounds.
        void set_zebra_stripes(bool enabled);
        /// Appends a row, filling missing cells as needed.
        void add_row(Row row);
        /// Appends a row with an explicit background color.
        void add_row(Row row, Color background);
        /// Appends a row from plain text cells.
        void add_row(const std::vector<std::string>& cells);
        /// Replaces one cell; returns false for invalid row or column indices.
        bool set_cell(std::size_t row, std::size_t column, Cell cell);
        /// Hides or shows a column; invalid indices are ignored.
        void set_column_visible(std::size_t index, bool visible);
        /// Sets the callback receiving selected indices or no_selection on deselection.
        void set_on_selection_changed(std::function<void(std::size_t)> callback);
        /// Removes all rows and clears selection.
        void clear_rows();
        /// Selects a row; returns false for an invalid index.
        bool select_row(std::size_t index);
        /// Clears a selection and notifies the callback when it changes.
        void clear_selection();
        /// Returns the selected row, or nullopt when there is no selection.
        std::optional<std::size_t> selected_row() const;
        /// Returns the number of rows.
        std::size_t row_count() const;
        /// Sets a positive row height in pixels.
        void set_row_height(float height);

        /// Draws headers and only the rows intersecting the viewport.
        void draw() override;
        /// Updates scrolling and active column/scrollbar drags.
        void update() override;
        /// Toggles row selection, resizes columns, or starts scrollbar interaction.
        bool handle_mouse_press(Point position) override;
        /// Reports wheel ownership when the pointer is over the table.
        bool handles_mouse_wheel(Point position) const override;
        /// Counts the table and all rows.
        std::size_t widget_count() const override;
        /// Counts the table and visible rows.
        std::size_t visible_widget_count() const override;
    };

    /// Backward-compatible name for TableView.
    using ListView = TableView;

    /// A clipped region with render and pointer-input callbacks for custom content.
    class Viewport : public UIElement {
    private:
        std::function<void(int, int, int, int)> on_render_;
        std::function<void(const ViewportInput&)> on_input_;
        bool pointer_active_ = false;

    public:
        /// Sets a render callback receiving the pixel-space viewport rectangle.
        void set_on_render(std::function<void(int, int, int, int)> on_render);
        /// Sets the callback for drag, pan, wheel, and double-click input.
        void set_on_input(std::function<void(const ViewportInput&)> on_input);
        /// Draws the viewport frame and invokes the render callback under a clip.
        void draw() override;
        /// Updates pointer interaction while the viewport is active.
        void update() override;
        /// Starts pointer capture only for presses originating inside the viewport.
        bool handle_mouse_press(Point position) override;
        /// Returns the inset viewport clip in screen coordinates.
        Rect child_clip_rect() const override;
    };

    /// A movable, optionally resizable top-level window.
    class Window : public UIElement {
    private:
        bool dragging_ = false;
        bool resizing_ = false;
        bool resizable_ = true;
        bool status_bar_visible_ = false;
        bool visible_ = true;
        bool frame_visible_ = true;
        bool title_bar_visible_ = true;
        bool always_on_top_ = false;
        float min_width_ = 160.f;
        float min_height_ = 100.f;
        std::string status_checkbox_label_;
        bool* status_checkbox_ = nullptr;
        std::shared_ptr<bool> owned_status_checkbox_;
        Point drag_offset_{0.f, 0.f};
        Point resize_start_mouse_{0.f, 0.f};
        Point resize_start_size_{0.f, 0.f};
        std::vector<Color> status_indicators_;

    public:
        /// Creates a window with the specified title.
        explicit Window(std::string title);

        /// Sets minimum dimensions in pixels.
        void set_minimum_size(float width, float height);
        /// Derives the minimum dimensions from current content.
        void set_minimum_size_from_content();
        /// Returns the current minimum size in pixels.
        Point get_minimum_size() const;
        /// Enables or disables pointer-based resizing.
        void set_resizable(bool resizable);
        /// Returns whether resizing is enabled.
        bool is_resizable() const;
        /// Hides the frame when frameless is true.
        void set_frameless(bool frameless);
        /// Shows or hides the title bar.
        void set_title_bar_visible(bool visible);
        /// Places the window above ordinary windows when true.
        void set_always_on_top(bool always_on_top);
        /// Returns whether the window is always on top.
        bool is_always_on_top() const;
        /// Shows or hides the window.
        void set_visible(bool visible);
        /// Returns whether the window is visible.
        bool is_visible() const;
        /// Sets size in pixels, respecting the minimum dimensions.
        void set_size(const Point& size) override;
        /// Shows or hides the status bar.
        void set_status_bar_visible(bool visible);
        /// Binds a non-owning checkbox value displayed in the status bar.
        void set_status_checkbox(std::string label, bool* value);
        /// Binds a non-owning checkbox value displayed in the status bar.
        void set_status_checkbox(std::string label, bool& value) {
            set_status_checkbox(std::move(label), &value);
        }
        /// Binds a status checkbox and retains the Bound value's ownership.
        void set_status_checkbox(std::string label, const Bound<bool>& value);
        /// Replaces the colored status indicators.
        void set_status_indicators(std::vector<Color> indicators);
        /// Begins dragging/resizing or delegates to a child.
        bool handle_mouse_press(Point position) override;
        /// Draws the frame, status bar, and clipped child widgets.
        void draw() override;
        /// Updates dragging, resizing, and child widgets.
        void update() override;
        /// Returns the screen-space content rectangle.
        Rect child_clip_rect() const override;
    };

    /// Owns top-level windows and overlays and dispatches frames/input.
    class WindowManager {
    private:
        std::vector<std::shared_ptr<Window>> windows_;
        std::vector<std::shared_ptr<UIElement>> overlays_;
        std::weak_ptr<UIElement> tooltip_overlay_;
        const UIElement* tooltip_owner_ = nullptr;
        bool mouse_was_pressed_ = false;

    public:
        /// Creates an empty manager.
        WindowManager() = default;
        /// Detaches manager registrations from windows before destruction.
        ~WindowManager();
        WindowManager(const WindowManager&) = delete;
        WindowManager& operator=(const WindowManager&) = delete;
        WindowManager(WindowManager&&) = delete;
        WindowManager& operator=(WindowManager&&) = delete;

        /// Adds and owns a window.
        void add_window(const std::shared_ptr<Window>& window);
        /// Removes a registered window.
        void remove_window(const std::shared_ptr<Window>& window);
        /// Moves a registered window to the top of its z-order group.
        void bring_to_front(const std::shared_ptr<Window>& window);
        /// Returns registered windows in draw order.
        const std::vector<std::shared_ptr<Window>>& windows() const;
        /// Adds an owned overlay drawn above all windows.
        void show_overlay(const std::shared_ptr<UIElement>& overlay);
        /// Dismisses an overlay if it is currently registered.
        void dismiss_overlay(const UIElement* overlay);
        /// Shows one tooltip overlay at a time; returns false if another target owns it.
        bool show_tooltip(const UIElement* owner, const std::shared_ptr<UIElement>& tooltip);
        /// Dismisses the tooltip belonging to the target, if any.
        void dismiss_tooltip(const UIElement* owner);
        /// Reports whether a target currently owns the active tooltip.
        bool has_tooltip(const UIElement* owner) const;
        /// Returns whether any overlays are active.
        bool has_overlays() const;
        /// Validates all registered windows and optionally writes the first diagnostic.
        bool validate_layout(std::string* error = nullptr) const;
        /// Validates one window and optionally writes the first diagnostic.
        bool validate_layout(const Window& window, std::string* error = nullptr) const;

        /// Draws windows followed by overlays.
        void draw();
        /// Updates windows, overlays, and input dispatch.
        void update();
    };

} // namespace zgui

#endif // ZGUI_HH
