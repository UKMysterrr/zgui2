// Copyright (c) 2019, zxvnme
// Copyright (c) 2026 UKMysterrr
// SPDX-License-Identifier: MIT

#include "zgui.hh"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>

namespace zgui::detail {
    struct ComboBoxData {
        std::vector<std::string> items;
        std::size_t selected = 0;
        std::size_t* bound_index = nullptr;
        std::shared_ptr<std::size_t> owned_index;
        std::function<void(std::size_t)> on_change;

        std::size_t current_index() const {
            if (items.empty())
                return 0;
            const std::size_t index = bound_index == nullptr ? selected : *bound_index;
            return std::min(index, items.size() - 1);
        }

        void set_index(std::size_t index) {
            selected = index;
            if (bound_index != nullptr)
                *bound_index = index;
        }
    };

    struct MultiComboBoxData {
        std::vector<std::string> items;
        std::vector<bool> fallback_values;
        std::vector<bool>* values = nullptr;
        std::shared_ptr<std::vector<bool>> owned_values;
        std::function<void()> on_change;
    };
} // namespace zgui::detail

namespace {
    constexpr std::size_t max_text_truncations = 1024;

    struct HsvColor {
        float hue;
        float saturation;
        float value;
        float alpha;
    };

    HsvColor to_hsv(zgui::Color color) {
        const float red = static_cast<float>(color.r) / 255.f;
        const float green = static_cast<float>(color.g) / 255.f;
        const float blue = static_cast<float>(color.b) / 255.f;
        const float maximum = std::max({red, green, blue});
        const float minimum = std::min({red, green, blue});
        const float difference = maximum - minimum;

        float hue = 0.f;
        if (difference > 0.f) {
            if (maximum == red)
                hue = 60.f * std::fmod((green - blue) / difference, 6.f);
            else if (maximum == green)
                hue = 60.f * ((blue - red) / difference + 2.f);
            else
                hue = 60.f * ((red - green) / difference + 4.f);
        }
        if (hue < 0.f)
            hue += 360.f;
        return {hue, maximum == 0.f ? 0.f : difference / maximum, maximum,
                static_cast<float>(color.a) / 255.f};
    }

    zgui::Color from_hsv(float hue, float saturation, float value, float alpha) {
        hue = std::fmod(std::max(0.f, hue), 360.f);
        saturation = std::clamp(saturation, 0.f, 1.f);
        value = std::clamp(value, 0.f, 1.f);
        alpha = std::clamp(alpha, 0.f, 1.f);
        const float chroma = value * saturation;
        const float x = chroma * (1.f - std::fabs(std::fmod(hue / 60.f, 2.f) - 1.f));
        const float match = value - chroma;
        float red = 0.f;
        float green = 0.f;
        float blue = 0.f;

        if (hue < 60.f) {
            red = chroma;
            green = x;
        } else if (hue < 120.f) {
            red = x;
            green = chroma;
        } else if (hue < 180.f) {
            green = chroma;
            blue = x;
        } else if (hue < 240.f) {
            green = x;
            blue = chroma;
        } else if (hue < 300.f) {
            red = x;
            blue = chroma;
        } else {
            red = chroma;
            blue = x;
        }

        const auto channel = [match](float component) {
            return static_cast<unsigned char>(
                std::lround(std::clamp(component + match, 0.f, 1.f) * 255.f));
        };
        return zgui::Color(channel(red), channel(green), channel(blue),
                           static_cast<unsigned char>(std::lround(alpha * 255.f)));
    }

    bool same_color(zgui::Color left, zgui::Color right) {
        return left.r == right.r && left.g == right.g && left.b == right.b && left.a == right.a;
    }

    bool inside_rect(zgui::Point point, zgui::Point position, zgui::Point size) {
        return point.x >= position.x && point.y >= position.y && point.x < position.x + size.x &&
               point.y < position.y + size.y;
    }

    std::string truncate_with_ellipsis(const zgui::Graphics& graphics, const std::string& text,
                                       float width, bool track = true) {
        if (width <= 0.f)
            return {};
        if (graphics.measure_text_width(text) <= width)
            return text;
        if (track && zgui::core::collect_text_truncations &&
            zgui::core::text_truncations.size() < max_text_truncations)
            zgui::core::text_truncations.push_back(text);
        constexpr const char* ellipsis = "..";
        if (graphics.measure_text_width(ellipsis) > width)
            return {};

        std::size_t low = 0;
        std::size_t high = text.size();
        while (low < high) {
            const std::size_t middle = low + (high - low + 1) / 2;
            if (graphics.measure_text_width(text.substr(0, middle) + ellipsis) <= width)
                low = middle;
            else
                high = middle - 1;
        }
        return text.substr(0, low) + ellipsis;
    }

    void draw_outline(zgui::Graphics& graphics, zgui::Point position, zgui::Point size,
                      zgui::Color color) {
        graphics.draw_line(position, zgui::Point(position.x + size.x, position.y), color);
        graphics.draw_line(zgui::Point(position.x + size.x, position.y),
                           zgui::Point(position.x + size.x, position.y + size.y), color);
        graphics.draw_line(zgui::Point(position.x + size.x, position.y + size.y),
                           zgui::Point(position.x, position.y + size.y), color);
        graphics.draw_line(zgui::Point(position.x, position.y + size.y), position, color);
    }

    class ClipScope {
    private:
        zgui::Graphics& graphics_;

    public:
        ClipScope(zgui::Graphics& graphics, int x, int y, int w, int h) : graphics_(graphics) {
            graphics_.flush();
            graphics_.push_clip_rect(x, y, w, h);
        }

        ~ClipScope() {
            graphics_.flush();
            graphics_.pop_clip_rect();
        }
    };

    class TooltipPopup : public zgui::UIElement {
    public:
        explicit TooltipPopup(std::string text) {
            set_title(std::move(text));
        }

        void draw() override {
            if (!zgui::core::require_initialized())
                return;
            const zgui::Point position = get_absolute_position();
            const zgui::Point size = get_size();
            const zgui::Theme& theme = zgui::core::active_theme;
            auto& graphics = *zgui::core::graphics;
            graphics.draw_frame(position, size, theme.window_header);
            const std::string text = graphics.fit_text(title_, std::max(0.f, size.x - 8.f), false);
            graphics.draw_text(text, zgui::Point(position.x + 4.f, position.y + 5.f),
                               theme.text_primary);
        }
    };

    class ComboBoxPopup : public zgui::UIElement {
    private:
        std::shared_ptr<zgui::detail::ComboBoxData> data_;
        zgui::WindowManager* manager_;
        static constexpr float row_height_ = 16.f;

    public:
        ComboBoxPopup(std::shared_ptr<zgui::detail::ComboBoxData> data,
                      zgui::WindowManager* manager, zgui::Point position, float width)
            : data_(std::move(data)), manager_(manager) {
            set_position(position);
            set_size(
                zgui::Point(width, row_height_ * static_cast<float>(data_->items.size()) + 2.f));
        }

        void draw() override {
            const zgui::Point position = get_absolute_position();
            const zgui::Point size = get_size();
            const zgui::Theme& theme = zgui::core::active_theme;
            auto& graphics = *zgui::core::graphics;
            graphics.draw_frame(position, size, theme.control_bg);
            ClipScope clip(graphics, static_cast<int>(position.x + 1.f),
                           static_cast<int>(position.y + 1.f),
                           std::max(0, static_cast<int>(size.x - 2.f)),
                           std::max(0, static_cast<int>(size.y - 2.f)));
            for (std::size_t i = 0; i < data_->items.size(); ++i) {
                const float y = position.y + 1.f + static_cast<float>(i) * row_height_;
                if (y >= position.y + size.y - 1.f)
                    break;
                const zgui::Point row_position(position.x + 1.f, y);
                const zgui::Point row_size(std::max(0.f, size.x - 2.f),
                                           std::min(row_height_, position.y + size.y - 1.f - y));
                const bool hovered = zgui::core::input->mouse_over(row_position, row_size);
                if (hovered || i == data_->current_index()) {
                    graphics.fill_rect(row_position, row_size,
                                       hovered ? theme.control_hover : theme.window_header);
                }
                const std::string text = truncate_with_ellipsis(graphics, data_->items[i],
                                                                std::max(0.f, row_size.x - 8.f));
                graphics.draw_text(text, zgui::Point(position.x + 5.f, y + 2.f),
                                   theme.text_primary);
            }
        }

        bool handle_mouse_press(zgui::Point position) override {
            if (!contains_point(position))
                return false;
            const auto row =
                static_cast<std::size_t>((position.y - get_absolute_position().y) / row_height_);
            if (row < data_->items.size() && row != data_->current_index()) {
                data_->set_index(row);
                if (data_->on_change)
                    data_->on_change(row);
            }
            manager_->dismiss_overlay(this);
            return true;
        }
    };

    class MultiComboBoxPopup : public zgui::UIElement {
    private:
        std::shared_ptr<zgui::detail::MultiComboBoxData> data_;
        static constexpr float row_height_ = 16.f;

    public:
        MultiComboBoxPopup(std::shared_ptr<zgui::detail::MultiComboBoxData> data,
                           zgui::Point position, float width)
            : data_(std::move(data)) {
            set_position(position);
            set_size(
                zgui::Point(width, row_height_ * static_cast<float>(data_->items.size()) + 2.f));
        }

        void draw() override {
            const zgui::Point position = get_absolute_position();
            const zgui::Theme& theme = zgui::core::active_theme;
            auto& graphics = *zgui::core::graphics;
            graphics.draw_frame(position, get_size(), theme.control_bg);
            for (std::size_t i = 0; i < data_->items.size(); ++i) {
                const float y = position.y + 1.f + static_cast<float>(i) * row_height_;
                const zgui::Point row_position(position.x + 1.f, y);
                const zgui::Point row_size(std::max(0.f, get_size().x - 2.f), row_height_);
                const bool hovered = zgui::core::input->mouse_over(row_position, row_size);
                if (hovered)
                    graphics.fill_rect(row_position, row_size, theme.control_hover);
                graphics.draw_frame(zgui::Point(position.x + 4.f, y + 3.f), zgui::Point(10.f, 10.f),
                                    theme.control_bg);
                if ((*data_->values)[i])
                    graphics.fill_rect(zgui::Point(position.x + 6.f, y + 5.f),
                                       zgui::Point(6.f, 6.f), theme.accent);
                const std::string text = truncate_with_ellipsis(graphics, data_->items[i],
                                                                std::max(0.f, row_size.x - 24.f));
                graphics.draw_text(text, zgui::Point(position.x + 20.f, y + 2.f),
                                   theme.text_primary);
            }
        }

        bool handle_mouse_press(zgui::Point position) override {
            if (!contains_point(position))
                return false;
            const float row_position = position.y - get_absolute_position().y - 1.f;
            if (row_position >= 0.f) {
                const auto row = static_cast<std::size_t>(row_position / row_height_);
                if (row < data_->items.size()) {
                    (*data_->values)[row] = !(*data_->values)[row];
                    if (data_->on_change)
                        data_->on_change();
                }
            }
            return true;
        }
    };
} // namespace

std::string zgui::Graphics::fit_text(const std::string& text, float width, bool track) const {
    return truncate_with_ellipsis(*this, text, width, track);
}

void zgui::Graphics::set_topology(zgui::VertexTopology vertex_topology) {
    this->vertex_topology_ = vertex_topology;
}

void zgui::Graphics::append_geometry(const std::vector<zgui::Vertex>& vertices, std::size_t count) {
    for (std::size_t i = 0; i < count; i++) {
        this->vertices_.push_back(vertices[i]);
    }
}

void zgui::Graphics::flush() {
    draw();
}

void zgui::Graphics::push_clip_rect(int x, int y, int w, int h) {
    ClipRect clip{x, y, std::max(0, w), std::max(0, h)};
    if (!clip_stack_.empty()) {
        const ClipRect& parent = clip_stack_.back();
        const int left = std::max(clip.x, parent.x);
        const int top = std::max(clip.y, parent.y);
        const int right = std::min(clip.x + clip.w, parent.x + parent.w);
        const int bottom = std::min(clip.y + clip.h, parent.y + parent.h);
        clip = {left, top, std::max(0, right - left), std::max(0, bottom - top)};
    }
    clip_stack_.push_back(clip);
    set_clip_rect(clip.x, clip.y, clip.w, clip.h);
}

void zgui::Graphics::pop_clip_rect() {
    if (clip_stack_.empty())
        return;

    clip_stack_.pop_back();
    if (clip_stack_.empty()) {
        reset_clip_rect();
    } else {
        const ClipRect& clip = clip_stack_.back();
        set_clip_rect(clip.x, clip.y, clip.w, clip.h);
    }
}

std::size_t zgui::Graphics::clip_depth() const {
    return clip_stack_.size();
}

void zgui::Graphics::draw_line(zgui::Point start, zgui::Point end, zgui::Color color) {
    const std::vector<zgui::Vertex> vertices = {zgui::Vertex(start, zgui::Point(0.f, 0.f), color),
                                                zgui::Vertex(end, zgui::Point(0.f, 0.f), color)};

    this->set_topology(zgui::VertexTopology::LINE_LIST);
    this->append_geometry(vertices, 2);
}

void zgui::Graphics::draw_rect(zgui::Point position, zgui::Point size, zgui::Color color) {
    const zgui::Point top_right(position.x + size.x, position.y);
    const zgui::Point bottom_right(position.x + size.x, position.y + size.y);
    const zgui::Point bottom_left(position.x, position.y + size.y);
    draw_line(position, top_right, color);
    draw_line(top_right, bottom_right, color);
    draw_line(bottom_right, bottom_left, color);
    draw_line(bottom_left, position, color);
}

void zgui::Graphics::fill_rect(zgui::Point position, zgui::Point size, zgui::Color color) {
    const std::vector<zgui::Vertex> vertices = {
        zgui::Vertex(position, zgui::Point(0.f, 0.f), color),
        zgui::Vertex(zgui::Point(position.x + size.x, position.y), zgui::Point(0.f, 0.f), color),
        zgui::Vertex(zgui::Point(position.x, position.y + size.y), zgui::Point(0.f, 0.f), color),
        zgui::Vertex(zgui::Point(position.x, position.y + size.y), zgui::Point(0.f, 0.f), color),
        zgui::Vertex(zgui::Point(position.x + size.x, position.y), zgui::Point(0.f, 0.f), color),
        zgui::Vertex(zgui::Point(position.x + size.x, position.y + size.y), zgui::Point(0.f, 0.f),
                     color)};

    this->set_topology(zgui::VertexTopology::TRIANGLE_LIST);
    this->append_geometry(vertices, 6);
}

void zgui::Graphics::draw_gradient_rect(zgui::Point position, zgui::Point size,
                                        zgui::Color top_left, zgui::Color top_right,
                                        zgui::Color bottom_right, zgui::Color bottom_left) {
    const std::vector<zgui::Vertex> vertices = {
        zgui::Vertex(position, zgui::Point(0.f, 0.f), top_left),
        zgui::Vertex(zgui::Point(position.x + size.x, position.y), zgui::Point(0.f, 0.f),
                     top_right),
        zgui::Vertex(zgui::Point(position.x + size.x, position.y + size.y), zgui::Point(0.f, 0.f),
                     bottom_right),
        zgui::Vertex(position, zgui::Point(0.f, 0.f), top_left),
        zgui::Vertex(zgui::Point(position.x + size.x, position.y + size.y), zgui::Point(0.f, 0.f),
                     bottom_right),
        zgui::Vertex(zgui::Point(position.x, position.y + size.y), zgui::Point(0.f, 0.f),
                     bottom_left)};
    set_topology(zgui::VertexTopology::TRIANGLE_LIST);
    append_geometry(vertices, vertices.size());
}

void zgui::Graphics::draw_frame(zgui::Point position, zgui::Point size, zgui::Color fill) {
    const zgui::Theme& theme = zgui::core::active_theme;
    fill_rect(position, size, theme.border_outer);
    if (size.x > 2.f && size.y > 2.f) {
        fill_rect(zgui::Point(position.x + 1.f, position.y + 1.f),
                  zgui::Point(size.x - 2.f, size.y - 2.f), theme.border_inner);
        if (size.x > 4.f && size.y > 4.f) {
            fill_rect(zgui::Point(position.x + 2.f, position.y + 2.f),
                      zgui::Point(size.x - 4.f, size.y - 4.f), fill);
        }
    }
}

zgui::UIElement::~UIElement() {
    if (parent_ != nullptr)
        parent_->remove_child(this);
    clear_children();
}

void zgui::UIElement::add_child(zgui::UIElement* child) {
    if (child == nullptr) {
        assert(child != nullptr && "child must not be null");
        return;
    }
    for (UIElement* ancestor = this; ancestor != nullptr; ancestor = ancestor->parent_) {
        if (ancestor == child) {
            assert(ancestor != child && "adding child would create a parent cycle");
            return;
        }
    }
    if (child->parent_ == this) {
        if (std::find(children_.begin(), children_.end(), child) == children_.end())
            children_.push_back(child);
        return;
    }

    std::shared_ptr<UIElement> keep_alive;
    if (child->parent_ != nullptr) {
        UIElement* old_parent = child->parent_;
        const auto owner =
            std::find_if(old_parent->owned_children_.begin(), old_parent->owned_children_.end(),
                         [child](const auto& owned) { return owned.get() == child; });
        if (owner != old_parent->owned_children_.end())
            keep_alive = *owner;
        old_parent->remove_child(child);
    }

    if (std::find(children_.begin(), children_.end(), child) == children_.end())
        children_.push_back(child);
    if (keep_alive && std::none_of(owned_children_.begin(), owned_children_.end(),
                                   [child](const auto& owned) { return owned.get() == child; }))
        owned_children_.push_back(std::move(keep_alive));
    child->parent_ = this;
}

void zgui::UIElement::add_child(const std::shared_ptr<zgui::UIElement>& child) {
    if (!child) {
        assert(child && "child must not be null");
        return;
    }
    add_child(child.get());
    if (std::none_of(owned_children_.begin(), owned_children_.end(),
                     [&child](const auto& owned) { return owned == child; }))
        owned_children_.push_back(child);
}

void zgui::UIElement::remove_child(zgui::UIElement* child) {
    if (child == nullptr)
        return;
    children_.erase(std::remove(children_.begin(), children_.end(), child), children_.end());
    if (child->parent_ == this)
        child->parent_ = nullptr;
    owned_children_.erase(
        std::remove_if(owned_children_.begin(), owned_children_.end(),
                       [child](const auto& owned) { return owned.get() == child; }),
        owned_children_.end());
}

void zgui::UIElement::clear_children() {
    for (UIElement* child : children_) {
        if (child != nullptr && child->parent_ == this)
            child->parent_ = nullptr;
    }
    children_.clear();
    owned_children_.clear();
}

void zgui::UIElement::set_parent(zgui::UIElement* parent) {
    if (parent == parent_)
        return;
    if (parent != nullptr) {
        parent->add_child(this);
        return;
    }

    if (parent_ != nullptr) {
        std::shared_ptr<UIElement> keep_alive;
        const auto owner =
            std::find_if(parent_->owned_children_.begin(), parent_->owned_children_.end(),
                         [this](const auto& owned) { return owned.get() == this; });
        if (owner != parent_->owned_children_.end())
            keep_alive = *owner;
        parent_->remove_child(this);
    }
}

void zgui::UIElement::draw() {
    if (!zgui::core::require_initialized())
        return;
    const zgui::Rect clip = child_clip_rect();
    ClipScope child_clip(*zgui::core::graphics, static_cast<int>(clip.position.x),
                         static_cast<int>(clip.position.y), static_cast<int>(clip.size.x),
                         static_cast<int>(clip.size.y));
    for (const auto child : children_)
        child->draw();
}

void zgui::UIElement::update() {
    if (!zgui::core::require_initialized())
        return;
    for (const auto child : children_)
        child->update();
}

bool zgui::UIElement::handle_mouse_press(zgui::Point position) {
    for (auto child = children_.rbegin(); child != children_.rend(); ++child) {
        if ((*child)->contains_point(position) && (*child)->handle_mouse_press(position))
            return true;
    }
    return false;
}

bool zgui::UIElement::handles_mouse_wheel(zgui::Point position) const {
    for (auto child = children_.rbegin(); child != children_.rend(); ++child) {
        if ((*child)->contains_point(position) && (*child)->handles_mouse_wheel(position))
            return true;
    }
    return false;
}

const zgui::Point& zgui::UIElement::get_position() const {
    return position_;
}

zgui::Point zgui::UIElement::get_absolute_position() const {
    if (parent_ == nullptr)
        return position_;
    const zgui::Point parent_position = parent_->get_absolute_position();
    const zgui::Point offset = parent_->child_offset();
    return zgui::Point(parent_position.x + offset.x + position_.x,
                       parent_position.y + offset.y + position_.y);
}

void zgui::UIElement::set_position(const zgui::Point& position) {
    position_ = position;
}

void zgui::UIElement::set_anchors(bool left, bool right, bool top, bool bottom) {
    anchor_left_ = left;
    anchor_right_ = right;
    anchor_top_ = top;
    anchor_bottom_ = bottom;
    horizontal_constraints_ = false;
    vertical_constraints_ = false;
}

void zgui::UIElement::set_horizontal_constraints(float left_fraction, float right_fraction,
                                                 float left_offset, float right_offset) {
    if (left_fraction < 0.f || right_fraction > 1.f || left_fraction > right_fraction) {
        assert(left_fraction >= 0.f && right_fraction <= 1.f && left_fraction <= right_fraction &&
               "horizontal constraints must be ordered within [0, 1]");
        return;
    }
    horizontal_constraints_ = true;
    horizontal_left_fraction_ = left_fraction;
    horizontal_right_fraction_ = right_fraction;
    horizontal_left_offset_ = left_offset;
    horizontal_right_offset_ = right_offset;
    if (parent_ != nullptr) {
        const zgui::Point parent_size = parent_->get_size();
        adjust_for_parent_resize(parent_size, parent_size);
    }
}

void zgui::UIElement::set_vertical_constraints(float top_fraction, float bottom_fraction,
                                               float top_offset, float bottom_offset) {
    if (top_fraction < 0.f || bottom_fraction > 1.f || top_fraction > bottom_fraction) {
        assert(top_fraction >= 0.f && bottom_fraction <= 1.f && top_fraction <= bottom_fraction &&
               "vertical constraints must be ordered within [0, 1]");
        return;
    }
    vertical_constraints_ = true;
    vertical_top_fraction_ = top_fraction;
    vertical_bottom_fraction_ = bottom_fraction;
    vertical_top_offset_ = top_offset;
    vertical_bottom_offset_ = bottom_offset;
    if (parent_ != nullptr) {
        const zgui::Point parent_size = parent_->get_size();
        adjust_for_parent_resize(parent_size, parent_size);
    }
}

const zgui::Point& zgui::UIElement::get_size() const {
    return size_;
}

void zgui::UIElement::set_size(const zgui::Point& size) {
    const zgui::Point old_size = size_;
    size_ = size;
    for (UIElement* child : children_)
        child->adjust_for_parent_resize(old_size, size_);
}

void zgui::UIElement::adjust_for_parent_resize(zgui::Point old_size, zgui::Point new_size) {
    float delta_x = old_size.x > 0.f ? new_size.x - old_size.x : 0.f;
    float delta_y = old_size.y > 0.f ? new_size.y - old_size.y : 0.f;
    zgui::Point position = position_;
    zgui::Point size = size_;

    if (horizontal_constraints_) {
        position.x = new_size.x * horizontal_left_fraction_ + horizontal_left_offset_;
        const float right = new_size.x * horizontal_right_fraction_ + horizontal_right_offset_;
        size.x = std::max(0.f, right - position.x);
    } else if (delta_x != 0.f) {
        if (anchor_left_ && anchor_right_)
            size.x = std::max(0.f, size.x + delta_x);
        else if (!anchor_left_ && anchor_right_)
            position.x += delta_x;
        else if (!anchor_left_ && !anchor_right_)
            position.x += delta_x * 0.5f;
    }
    if (vertical_constraints_) {
        position.y = new_size.y * vertical_top_fraction_ + vertical_top_offset_;
        const float bottom = new_size.y * vertical_bottom_fraction_ + vertical_bottom_offset_;
        size.y = std::max(0.f, bottom - position.y);
    } else if (delta_y != 0.f) {
        if (anchor_top_ && anchor_bottom_)
            size.y = std::max(0.f, size.y + delta_y);
        else if (!anchor_top_ && anchor_bottom_)
            position.y += delta_y;
        else if (!anchor_top_ && !anchor_bottom_)
            position.y += delta_y * 0.5f;
    }

    set_position(position);
    set_size(size);
}

bool zgui::UIElement::contains_point(zgui::Point position) const {
    const zgui::Point absolute = get_absolute_position();
    if (!inside_rect(position, absolute, size_))
        return false;
    for (const UIElement* ancestor = parent_; ancestor != nullptr; ancestor = ancestor->parent_) {
        if (!ancestor->child_clip_rect().contains(position))
            return false;
    }
    return true;
}

zgui::Rect zgui::UIElement::child_clip_rect() const {
    return {get_absolute_position(), size_};
}

zgui::Point zgui::UIElement::child_offset() const {
    return zgui::Point(0.f, 0.f);
}

float zgui::UIElement::content_bottom() const {
    float bottom = 0.f;
    for (const UIElement* child : children_)
        bottom = std::max(bottom, child->get_position().y + child->get_size().y);
    return bottom;
}

float zgui::UIElement::content_right() const {
    float right = 0.f;
    for (const UIElement* child : children_)
        right = std::max(right, child->get_position().x + child->get_size().x);
    return right;
}

std::size_t zgui::UIElement::widget_count() const {
    std::size_t count = 1;
    for (const UIElement* child : children_)
        count += child->widget_count();
    return count;
}

std::size_t zgui::UIElement::visible_widget_count() const {
    std::size_t count = 1;
    const Rect clip = child_clip_rect();
    for (const UIElement* child : children_) {
        const Point position = child->get_absolute_position();
        const Point size = child->get_size();
        const bool intersects =
            position.x < clip.position.x + clip.size.x && clip.position.x < position.x + size.x &&
            position.y < clip.position.y + clip.size.y && clip.position.y < position.y + size.y;
        if (intersects)
            count += child->visible_widget_count();
    }
    return count;
}

void zgui::UIElement::set_title(std::string title) {
    title_ = std::move(title);
}

const std::string& zgui::UIElement::get_title() const {
    return title_;
}

zgui::Label::Label(std::string text, zgui::TextAlignment alignment) : alignment_(alignment) {
    set_title(std::move(text));
}

zgui::Separator::Separator(zgui::SeparatorOrientation orientation, std::string label)
    : orientation_(orientation) {
    set_title(std::move(label));
}

void zgui::Separator::set_orientation(zgui::SeparatorOrientation orientation) {
    orientation_ = orientation;
}

void zgui::Separator::set_label(std::string label) {
    set_title(std::move(label));
}

void zgui::Separator::draw() {
    if (!zgui::core::require_initialized())
        return;

    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    auto& graphics = *zgui::core::graphics;
    const zgui::Color color = zgui::core::active_theme.border_inner;
    if (orientation_ == SeparatorOrientation::vertical) {
        graphics.draw_line(zgui::Point(position.x + size.x / 2.f, position.y),
                           zgui::Point(position.x + size.x / 2.f, position.y + size.y), color);
        return;
    }

    const float center_y = position.y + size.y / 2.f;
    if (title_.empty()) {
        graphics.draw_line(zgui::Point(position.x, center_y),
                           zgui::Point(position.x + size.x, center_y), color);
        return;
    }

    const std::string label = graphics.fit_text(title_, size.x, false);
    const float label_width = graphics.measure_text_width(label);
    const float left_end =
        position.x + std::max(0.f, (size.x - label_width) / 2.f - spacing::kGroupTitleGap);
    const float right_start =
        position.x + std::min(size.x, (size.x + label_width) / 2.f + spacing::kGroupTitleGap);
    if (left_end > position.x)
        graphics.draw_line(zgui::Point(position.x, center_y), zgui::Point(left_end, center_y),
                           color);
    graphics.draw_text(
        label,
        zgui::Point(position.x + (size.x - label_width) / 2.f, position.y + (size.y - 12.f) / 2.f),
        zgui::core::active_theme.text_primary);
    if (right_start < position.x + size.x)
        graphics.draw_line(zgui::Point(right_start, center_y),
                           zgui::Point(position.x + size.x, center_y), color);
}

void zgui::Label::set_text(std::string text) {
    set_title(std::move(text));
}

void zgui::Label::set_alignment(zgui::TextAlignment alignment) {
    alignment_ = alignment;
}

void zgui::Label::set_color(zgui::Color color) {
    color_override_ = color;
    has_color_override_ = true;
}

void zgui::Label::clear_color_override() {
    has_color_override_ = false;
}

void zgui::Label::draw() {
    const zgui::Point position = get_absolute_position();
    auto& graphics = *zgui::core::graphics;
    const std::string text = truncate_with_ellipsis(graphics, title_, size_.x);
    const float text_width = graphics.measure_text_width(text);
    float x = position.x;
    if (alignment_ == zgui::TextAlignment::center)
        x += (size_.x - text_width) / 2.f;
    else if (alignment_ == zgui::TextAlignment::right)
        x += size_.x - text_width;
    const zgui::Color color =
        has_color_override_ ? color_override_ : zgui::core::active_theme.text_primary;
    graphics.draw_text(text, zgui::Point(x, position.y), color);
}

zgui::Button::Button(std::string text, std::function<void()> on_click)
    : on_click_(std::move(on_click)) {
    set_title(std::move(text));
}

void zgui::Button::set_on_click(std::function<void()> on_click) {
    on_click_ = std::move(on_click);
}

void zgui::Button::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    const bool hovered = contains_point(zgui::core::input->mouse_position());
    const zgui::Color fill = pressed_ || hovered ? theme.control_hover : theme.control_bg;
    auto& graphics = *zgui::core::graphics;
    graphics.draw_frame(position, size, fill);
    const std::string text = truncate_with_ellipsis(graphics, title_, std::max(0.f, size.x - 8.f));
    const float text_width = graphics.measure_text_width(text);
    graphics.draw_text(
        text,
        zgui::Point(position.x + (size.x - text_width) / 2.f, position.y + (size.y - 12.f) / 2.f),
        theme.text_primary);
}

void zgui::Button::update() {
    if (!pressed_ || zgui::core::input->mouse_button_pressed())
        return;
    pressed_ = false;
    if (contains_point(zgui::core::input->mouse_position()) && on_click_)
        on_click_();
}

bool zgui::Button::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    pressed_ = true;
    return true;
}

zgui::CheckBox::CheckBox(std::string label, bool* value, std::function<void()> on_change)
    : value_(value != nullptr ? value : &fallback_value_), on_change_(std::move(on_change)) {
    assert(value != nullptr && "checkbox value must not be null");
    set_title(std::move(label));
}

void zgui::CheckBox::set_on_change(std::function<void()> on_change) {
    on_change_ = std::move(on_change);
}

void zgui::CheckBox::set_value(bool* value) {
    if (value == nullptr) {
        assert(value != nullptr && "checkbox value must not be null");
        return;
    }
    owned_value_.reset();
    value_ = value;
}

void zgui::CheckBox::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    const zgui::Point box_position(position.x, position.y + 1.f);
    graphics.draw_frame(box_position, zgui::Point(spacing::kCheckboxSize, spacing::kCheckboxSize),
                        theme.control_bg);
    if (*value_) {
        graphics.fill_rect(zgui::Point(box_position.x + 2.f, box_position.y + 2.f),
                           zgui::Point(7.f, 7.f), theme.accent);
    }
    const std::string text = truncate_with_ellipsis(
        graphics, title_,
        std::max(0.f, size.x - spacing::kCheckboxSize - spacing::kCheckboxLabelGap));
    graphics.draw_text(text,
                       zgui::Point(position.x + spacing::kCheckboxSize + spacing::kCheckboxLabelGap,
                                   position.y - 1.f),
                       theme.text_primary);
}

bool zgui::CheckBox::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    *value_ = !*value_;
    if (on_change_)
        on_change_();
    return true;
}

zgui::ToggleButton::ToggleButton(std::string label, bool* value, std::function<void()> on_change)
    : value_(value != nullptr ? value : &fallback_value_), on_change_(std::move(on_change)) {
    assert(value != nullptr && "toggle button value must not be null");
    set_title(std::move(label));
}

zgui::ToggleButton::ToggleButton(std::string label, const zgui::Bound<bool>& value,
                                 std::function<void()> on_change)
    : ToggleButton(std::move(label), value.get(), std::move(on_change)) {
    owned_value_ = value.owner();
}

void zgui::ToggleButton::set_on_change(std::function<void()> on_change) {
    on_change_ = std::move(on_change);
}

void zgui::ToggleButton::set_value(bool value) {
    if (*value_ == value)
        return;
    *value_ = value;
    if (on_change_)
        on_change_();
}

bool zgui::ToggleButton::get_value() const {
    return *value_;
}

void zgui::ToggleButton::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    const bool hovered = contains_point(zgui::core::input->mouse_position());
    const zgui::Color fill =
        *value_ ? theme.accent : (pressed_ || hovered ? theme.control_hover : theme.control_bg);
    auto& graphics = *zgui::core::graphics;
    graphics.draw_frame(position, size, fill);
    const std::string text = truncate_with_ellipsis(graphics, title_, std::max(0.f, size.x - 8.f));
    const float text_width = graphics.measure_text_width(text);
    graphics.draw_text(
        text,
        zgui::Point(position.x + (size.x - text_width) / 2.f, position.y + (size.y - 12.f) / 2.f),
        theme.text_primary);
}

void zgui::ToggleButton::update() {
    if (!pressed_ || zgui::core::input->mouse_button_pressed())
        return;
    pressed_ = false;
    if (contains_point(zgui::core::input->mouse_position()))
        set_value(!*value_);
}

bool zgui::ToggleButton::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    pressed_ = true;
    return true;
}

zgui::RadioGroup::RadioGroup(std::vector<std::string> items, std::size_t* selected_index,
                             std::function<void(std::size_t)> on_change)
    : items_(std::move(items)),
      selected_index_(selected_index != nullptr ? selected_index : &fallback_index_),
      on_change_(std::move(on_change)) {
    assert(selected_index != nullptr && "radio group selection must not be null");
    if (items_.empty())
        *selected_index_ = 0;
    else
        *selected_index_ = std::min(*selected_index_, items_.size() - 1);
}

zgui::RadioGroup::RadioGroup(std::vector<std::string> items,
                             const zgui::Bound<std::size_t>& selected_index,
                             std::function<void(std::size_t)> on_change)
    : RadioGroup(std::move(items), selected_index.get(), std::move(on_change)) {
    owned_index_ = selected_index.owner();
}

bool zgui::RadioGroup::set_selected_index(std::size_t index, bool notify) {
    if (index >= items_.size())
        return false;
    if (*selected_index_ != index) {
        *selected_index_ = index;
        if (notify && on_change_)
            on_change_(index);
    }
    return true;
}

std::size_t zgui::RadioGroup::get_selected_index() const {
    return *selected_index_;
}

void zgui::RadioGroup::set_on_change(std::function<void(std::size_t)> on_change) {
    on_change_ = std::move(on_change);
}

void zgui::RadioGroup::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    const std::size_t visible_rows = static_cast<std::size_t>(std::max(0.f, size.y) / row_height_);
    const std::size_t row_count = std::min(items_.size(), visible_rows);
    for (std::size_t index = 0; index < row_count; ++index) {
        const float row_y = position.y + static_cast<float>(index) * row_height_;
        const zgui::Point radio_position(position.x + 2.f, row_y + 3.f);
        graphics.draw_frame(radio_position, zgui::Point(10.f, 10.f), theme.control_bg);
        if (*selected_index_ == index)
            graphics.fill_rect(zgui::Point(radio_position.x + 3.f, radio_position.y + 3.f),
                               zgui::Point(4.f, 4.f), theme.accent);
        const std::string text =
            truncate_with_ellipsis(graphics, items_[index], std::max(0.f, size.x - 18.f));
        graphics.draw_text(text, zgui::Point(position.x + 17.f, row_y + 2.f), theme.text_primary);
    }
}

bool zgui::RadioGroup::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    const float local_y = position.y - get_absolute_position().y;
    const auto index = static_cast<std::size_t>(local_y / row_height_);
    if (index < items_.size())
        set_selected_index(index);
    return true;
}

zgui::ProgressBar::ProgressBar(float* value, std::string text)
    : value_(value != nullptr ? value : &fallback_value_) {
    assert(value != nullptr && "progress bar value must not be null");
    *value_ = std::clamp(*value_, 0.f, 1.f);
    set_title(std::move(text));
}

zgui::ProgressBar::ProgressBar(const zgui::Bound<float>& value, std::string text)
    : ProgressBar(value.get(), std::move(text)) {
    owned_value_ = value.owner();
}

void zgui::ProgressBar::set_value(float value) {
    *value_ = std::clamp(value, 0.f, 1.f);
}

float zgui::ProgressBar::get_value() const {
    return std::clamp(*value_, 0.f, 1.f);
}

void zgui::ProgressBar::set_text(std::string text) {
    set_title(std::move(text));
}

void zgui::ProgressBar::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    graphics.draw_frame(position, size, theme.control_bg);

    const zgui::Point inner_position(position.x + 1.f, position.y + 1.f);
    const zgui::Point inner_size(std::max(0.f, size.x - 2.f), std::max(0.f, size.y - 2.f));
    const float value = get_value();
    if (inner_size.x > 0.f && inner_size.y > 0.f && value > 0.f) {
        graphics.fill_rect(inner_position, zgui::Point(inner_size.x * value, inner_size.y),
                           theme.accent);
    }

    if (!title_.empty()) {
        const std::string text = truncate_with_ellipsis(graphics, title_, inner_size.x, false);
        const float text_width = graphics.measure_text_width(text);
        graphics.draw_text(text,
                           zgui::Point(position.x + (size.x - text_width) / 2.f,
                                       position.y + (size.y - 12.f) / 2.f),
                           theme.text_primary);
    }
}

zgui::Tooltip::Tooltip(std::string text, std::chrono::milliseconds delay)
    : text_(std::move(text)), delay_(std::max(std::chrono::milliseconds::zero(), delay)) {}

zgui::Tooltip::~Tooltip() {
    dismiss();
}

void zgui::Tooltip::dismiss() {
    if (zgui::core::window_manager != nullptr)
        zgui::core::window_manager->dismiss_tooltip(this);
    popup_.reset();
    hover_started_.reset();
}

void zgui::Tooltip::set_text(std::string text) {
    text_ = std::move(text);
    dismiss();
}

void zgui::Tooltip::set_delay(std::chrono::milliseconds delay) {
    delay_ = std::max(std::chrono::milliseconds::zero(), delay);
    dismiss();
}

void zgui::Tooltip::draw() {}

void zgui::Tooltip::update() {
    if (!zgui::core::require_initialized())
        return;
    const zgui::Point mouse = zgui::core::input->mouse_position();
    if (!contains_point(mouse)) {
        dismiss();
        return;
    }

    zgui::WindowManager* manager = zgui::core::window_manager;
    if (popup_ && (manager == nullptr || !manager->has_tooltip(this)))
        popup_.reset();
    const auto now = std::chrono::steady_clock::now();
    if (!hover_started_)
        hover_started_ = now;
    if (popup_ || manager == nullptr || now - *hover_started_ < delay_)
        return;

    auto popup = std::make_shared<TooltipPopup>(text_);
    const zgui::Point viewport = zgui::core::graphics->viewport_size();
    const float width =
        std::min(std::max(40.f, zgui::core::graphics->measure_text_width(text_) + 8.f),
                 std::max(0.f, viewport.x));
    const float height = std::min(22.f, std::max(0.f, viewport.y));
    popup->set_size(zgui::Point(width, height));

    const zgui::Point absolute = get_absolute_position();
    zgui::Point position(absolute.x, absolute.y + get_size().y + 4.f);
    if (position.y + height > viewport.y)
        position.y = absolute.y - height - 4.f;
    position.x = std::clamp(position.x, 0.f, std::max(0.f, viewport.x - width));
    position.y = std::clamp(position.y, 0.f, std::max(0.f, viewport.y - height));
    popup->set_position(position);
    if (manager->show_tooltip(this, popup))
        popup_ = std::move(popup);
}

zgui::ComboBox::ComboBox(std::vector<std::string> items, std::size_t selected)
    : data_(std::make_shared<zgui::detail::ComboBoxData>()) {
    set_items(std::move(items));
    if (!data_->items.empty())
        data_->set_index(std::min(selected, data_->items.size() - 1));
}

void zgui::ComboBox::set_items(std::vector<std::string> items) {
    data_->items = std::move(items);
    data_->set_index(
        data_->items.empty() ? 0 : std::min(data_->current_index(), data_->items.size() - 1));
}

bool zgui::ComboBox::set_selected_index(std::size_t index, bool notify) {
    if (index >= data_->items.size())
        return false;
    if (data_->current_index() != index) {
        data_->set_index(index);
        if (notify && data_->on_change)
            data_->on_change(index);
    }
    return true;
}

void zgui::ComboBox::bind_selected_index(std::size_t* index, bool notify) {
    if (index == nullptr) {
        assert(index != nullptr && "combo selected index must not be null");
        return;
    }
    data_->owned_index.reset();
    data_->bound_index = index;
    if (data_->items.empty()) {
        *index = 0;
        data_->selected = 0;
        return;
    }
    const std::size_t selected = std::min(*index, data_->items.size() - 1);
    data_->set_index(selected);
    if (notify && data_->on_change)
        data_->on_change(selected);
}

void zgui::ComboBox::bind_selected_index(const zgui::Bound<std::size_t>& index, bool notify) {
    if (!index) {
        assert(index && "combo selected index must not be null");
        return;
    }
    data_->owned_index = index.owner();
    data_->bound_index = index.get();
    if (data_->items.empty()) {
        *data_->bound_index = 0;
        data_->selected = 0;
        return;
    }
    const std::size_t selected = std::min(*data_->bound_index, data_->items.size() - 1);
    data_->set_index(selected);
    if (notify && data_->on_change)
        data_->on_change(selected);
}

std::size_t zgui::ComboBox::get_selected_index() const {
    return data_->current_index();
}

const std::string& zgui::ComboBox::get_selected_item() const {
    static const std::string empty;
    return data_->items.empty() ? empty : data_->items[data_->current_index()];
}

void zgui::ComboBox::set_on_change(std::function<void(std::size_t)> on_change) {
    data_->on_change = std::move(on_change);
}

void zgui::ComboBox::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    graphics.draw_frame(position, size, theme.control_bg);
    if (!data_->items.empty()) {
        const std::string text = truncate_with_ellipsis(
            graphics, data_->items[data_->current_index()], std::max(0.f, size.x - 22.f));
        graphics.draw_text(text, zgui::Point(position.x + 4.f, position.y + (size.y - 12.f) / 2.f),
                           theme.text_primary);
    }
    const float arrow_x = position.x + size.x - 10.f;
    const float arrow_y = position.y + size.y / 2.f;
    graphics.draw_line(zgui::Point(arrow_x, arrow_y - 2.f),
                       zgui::Point(arrow_x + 3.f, arrow_y + 1.f), theme.text_dim);
    graphics.draw_line(zgui::Point(arrow_x + 3.f, arrow_y + 1.f),
                       zgui::Point(arrow_x + 6.f, arrow_y - 2.f), theme.text_dim);
}

bool zgui::ComboBox::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    if (data_->items.empty())
        return true;
    manager_ = zgui::core::window_manager;
    if (manager_ == nullptr) {
        assert(manager_ != nullptr && "ComboBox requires an active WindowManager");
        return true;
    }
    if (!popup_.expired())
        return true;
    const zgui::Point absolute = get_absolute_position();
    auto popup = std::make_shared<ComboBoxPopup>(
        data_, manager_, zgui::Point(absolute.x, absolute.y + size_.y), size_.x);
    const zgui::Point viewport = zgui::core::graphics->viewport_size();
    zgui::Point popup_size = popup->get_size();
    if (viewport.x > 0.f)
        popup_size.x = std::min(popup_size.x, viewport.x);
    if (viewport.y > 0.f)
        popup_size.y = std::min(popup_size.y, viewport.y);
    popup->set_size(popup_size);
    zgui::Point popup_position = popup->get_position();
    if (viewport.y > 0.f) {
        if (popup_position.y + popup_size.y > viewport.y && absolute.y >= popup_size.y)
            popup_position.y = absolute.y - popup_size.y;
        popup_position.y =
            std::clamp(popup_position.y, 0.f, std::max(0.f, viewport.y - popup_size.y));
    }
    if (viewport.x > 0.f)
        popup_position.x =
            std::clamp(popup_position.x, 0.f, std::max(0.f, viewport.x - popup_size.x));
    popup->set_position(popup_position);
    popup_ = popup;
    manager_->show_overlay(popup);
    return true;
}

zgui::MultiComboBox::MultiComboBox(std::vector<std::string> items, std::vector<bool>* values,
                                   std::function<void()> on_change)
    : data_(std::make_shared<zgui::detail::MultiComboBoxData>()) {
    assert(values != nullptr && "multi-combo value vector must not be null");
    data_->values = values != nullptr ? values : &data_->fallback_values;
    data_->on_change = std::move(on_change);
    set_items(std::move(items));
}

zgui::MultiComboBox::MultiComboBox(std::vector<std::string> items,
                                   const zgui::Bound<std::vector<bool>>& values,
                                   std::function<void()> on_change)
    : MultiComboBox(std::move(items), values.get(), std::move(on_change)) {
    data_->owned_values = values.owner();
}

void zgui::MultiComboBox::set_items(std::vector<std::string> items) {
    data_->items = std::move(items);
    data_->values->resize(data_->items.size(), false);
}

const std::vector<bool>& zgui::MultiComboBox::values() const {
    return *data_->values;
}

void zgui::MultiComboBox::set_on_change(std::function<void()> on_change) {
    data_->on_change = std::move(on_change);
}

void zgui::MultiComboBox::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    graphics.draw_frame(position, size, theme.control_bg);

    std::string summary;
    for (std::size_t i = 0; i < data_->items.size(); ++i) {
        if (!(*data_->values)[i])
            continue;
        if (!summary.empty())
            summary += ", ";
        summary += data_->items[i];
    }
    if (summary.empty())
        summary = "None";

    {
        ClipScope clip(graphics, static_cast<int>(position.x + 4.f),
                       static_cast<int>(position.y + 1.f),
                       std::max(0, static_cast<int>(size.x - 18.f)),
                       std::max(0, static_cast<int>(size.y - 2.f)));
        graphics.draw_text(truncate_with_ellipsis(graphics, summary, std::max(0.f, size.x - 18.f)),
                           zgui::Point(position.x + 4.f, position.y + (size.y - 12.f) / 2.f),
                           theme.text_primary);
    }
    const float arrow_x = position.x + size.x - 10.f;
    const float arrow_y = position.y + size.y / 2.f;
    graphics.draw_line(zgui::Point(arrow_x, arrow_y - 2.f),
                       zgui::Point(arrow_x + 3.f, arrow_y + 1.f), theme.text_dim);
    graphics.draw_line(zgui::Point(arrow_x + 3.f, arrow_y + 1.f),
                       zgui::Point(arrow_x + 6.f, arrow_y - 2.f), theme.text_dim);
}

bool zgui::MultiComboBox::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    if (data_->items.empty())
        return true;
    manager_ = zgui::core::window_manager;
    if (manager_ == nullptr) {
        assert(manager_ != nullptr && "MultiComboBox requires an active WindowManager");
        return true;
    }
    if (!popup_.expired())
        return true;
    const zgui::Point absolute = get_absolute_position();
    auto popup = std::make_shared<MultiComboBoxPopup>(
        data_, zgui::Point(absolute.x, absolute.y + size_.y), size_.x);
    popup_ = popup;
    manager_->show_overlay(popup);
    return true;
}

zgui::TextInput::TextInput(std::string* value, std::function<void()> on_change)
    : value_(value != nullptr ? value : &fallback_value_), on_change_(std::move(on_change)) {
    assert(value != nullptr && "text input value must not be null");
    cursor_ = value_->size();
}

zgui::TextInput::TextInput(std::string& value, std::function<void()> on_change)
    : TextInput(&value, std::move(on_change)) {}

zgui::TextInput::TextInput(const zgui::Bound<std::string>& value, std::function<void()> on_change)
    : TextInput(value.get(), std::move(on_change)) {
    owned_value_ = value.owner();
}

zgui::TextInput::~TextInput() {
    if (zgui::core::focused_text_input == this)
        zgui::core::focused_text_input = nullptr;
}

void zgui::TextInput::set_on_change(std::function<void()> on_change) {
    on_change_ = std::move(on_change);
}

void zgui::TextInput::set_value(std::string* value) {
    if (value == nullptr) {
        assert(value != nullptr && "text input value must not be null");
        return;
    }
    if (value_ == value)
        return;
    owned_value_.reset();
    value_ = value;
    cursor_ = value_->size();
}

void zgui::TextInput::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    auto& graphics = *zgui::core::graphics;
    graphics.draw_frame(position, size, zgui::Color(189, 189, 189));

    const float text_x = position.x + 3.f;
    const float text_y = position.y + (size.y - 12.f) / 2.f;
    const float available_width = std::max(0.f, size.x - 6.f);
    const std::string before_cursor = value_->substr(0, std::min(cursor_, value_->size()));
    const float cursor_width = graphics.measure_text_width(before_cursor);
    const float text_width = graphics.measure_text_width(*value_);
    const float scroll = std::clamp(cursor_width - available_width + 1.f, 0.f,
                                    std::max(0.f, text_width - available_width));
    ClipScope clip(graphics, static_cast<int>(position.x + 3.f), static_cast<int>(position.y + 1.f),
                   std::max(0, static_cast<int>(size.x - 6.f)),
                   std::max(0, static_cast<int>(size.y - 2.f)));
    graphics.draw_text(*value_, zgui::Point(text_x - scroll, text_y),
                       zgui::core::active_theme.text_primary);
    if (zgui::core::focused_text_input == this) {
        const zgui::Point cursor(text_x + cursor_width - scroll, position.y + 3.f);
        graphics.fill_rect(zgui::Point(cursor.x - 1.f, cursor.y - 1.f),
                           zgui::Point(3.f, std::max(0.f, size.y - 4.f)), zgui::Color(0, 0, 0));
        graphics.fill_rect(cursor, zgui::Point(1.f, std::max(0.f, size.y - 6.f)),
                           zgui::core::active_theme.text_primary);
    }
}

void zgui::TextInput::update() {
    if (zgui::core::focused_text_input != this)
        return;

    cursor_ = std::min(cursor_, value_->size());
    bool changed = false;
    if (zgui::core::input->text_cursor_left_pressed() && cursor_ > 0)
        --cursor_;
    if (zgui::core::input->text_cursor_right_pressed() && cursor_ < value_->size())
        ++cursor_;
    if (zgui::core::input->text_cursor_home_pressed())
        cursor_ = 0;
    if (zgui::core::input->text_cursor_end_pressed())
        cursor_ = value_->size();
    if (zgui::core::input->text_delete_pressed() && cursor_ < value_->size()) {
        value_->erase(cursor_, 1);
        changed = true;
    }
    char character = 0;
    while (zgui::core::input->pop_text_input(character)) {
        if (character == '\b') {
            if (cursor_ > 0) {
                value_->erase(cursor_ - 1, 1);
                --cursor_;
                changed = true;
            }
        } else if (static_cast<unsigned char>(character) >= 0x20 &&
                   static_cast<unsigned char>(character) <= 0x7e) {
            value_->insert(value_->begin() + static_cast<std::string::difference_type>(cursor_),
                           character);
            ++cursor_;
            changed = true;
        }
    }
    if (changed && on_change_)
        on_change_();
}

bool zgui::TextInput::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    zgui::core::focused_text_input = this;
    auto& graphics = *zgui::core::graphics;
    const Point absolute = get_absolute_position();
    const float available_width = std::max(0.f, get_size().x - 6.f);
    const float current_width =
        graphics.measure_text_width(value_->substr(0, std::min(cursor_, value_->size())));
    const float scroll =
        std::clamp(current_width - available_width + 1.f, 0.f,
                   std::max(0.f, graphics.measure_text_width(*value_) - available_width));
    const float target_x = position.x - (absolute.x + 3.f) + scroll;
    float nearest_distance = std::numeric_limits<float>::max();
    cursor_ = 0;
    for (std::size_t index = 0; index <= value_->size(); ++index) {
        const float distance =
            std::abs(graphics.measure_text_width(value_->substr(0, index)) - target_x);
        if (distance < nearest_distance) {
            nearest_distance = distance;
            cursor_ = index;
        }
    }
    return true;
}

zgui::KeyBind::KeyBind(std::string title, int* key, std::function<void()> on_change)
    : key_(key != nullptr ? key : &fallback_key_), on_change_(std::move(on_change)) {
    assert(key != nullptr && "key binding value must not be null");
    set_title(std::move(title));
}

zgui::KeyBind::KeyBind(std::string title, const zgui::Bound<int>& key,
                       std::function<void()> on_change)
    : KeyBind(std::move(title), key.get(), std::move(on_change)) {
    owned_key_ = key.owner();
}

void zgui::KeyBind::set_key(int key) {
    if (*key_ == key)
        return;
    *key_ = key;
    if (on_change_)
        on_change_();
}

int zgui::KeyBind::get_key() const {
    return *key_;
}

void zgui::KeyBind::set_on_change(std::function<void()> on_change) {
    on_change_ = std::move(on_change);
}

void zgui::KeyBind::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    const std::string key_text =
        listening_ ? "[...]" : "[" + zgui::core::input->key_name(*key_) + "]";
    const float required_button_width = graphics.measure_text_width(key_text) + 4.f;
    const float button_width =
        std::min(size.x, std::min(92.f, std::max(required_button_width, size.x * 0.4f)));
    const float button_x = position.x + size.x - button_width;
    const std::string title =
        truncate_with_ellipsis(graphics, title_, std::max(0.f, button_x - position.x - 2.f));
    graphics.draw_text(title, position, theme.text_primary);
    graphics.draw_frame(zgui::Point(button_x, position.y), zgui::Point(button_width, size.y),
                        listening_ ? theme.control_hover : theme.control_bg);
    const std::string fitted_key =
        truncate_with_ellipsis(graphics, key_text, std::max(0.f, button_width - 4.f));
    const float text_width = graphics.measure_text_width(fitted_key);
    graphics.draw_text(fitted_key,
                       zgui::Point(button_x + (button_width - text_width) / 2.f,
                                   position.y + (size.y - 12.f) / 2.f),
                       theme.text_primary);
}

void zgui::KeyBind::update() {
    if (!listening_)
        return;
    if (zgui::core::input->escape_pressed()) {
        listening_ = false;
        return;
    }
    const std::optional<int> key = zgui::core::input->poll_pressed_key();
    if (key) {
        listening_ = false;
        set_key(*key);
    }
}

bool zgui::KeyBind::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    while (zgui::core::input->poll_pressed_key()) {
    }
    listening_ = true;
    return true;
}

zgui::ColorPicker::ColorPicker(zgui::Color* value, std::function<void()> on_change)
    : value_(value != nullptr ? value : &fallback_value_), on_change_(std::move(on_change)) {
    assert(value != nullptr && "color picker value must not be null");
    const HsvColor hsv = to_hsv(*value_);
    hue_ = hsv.hue;
    saturation_ = hsv.saturation;
    brightness_ = hsv.value;
    alpha_ = hsv.alpha;
    set_size(zgui::Point(264.f, 212.f));
}

zgui::ColorPicker::ColorPicker(const zgui::Bound<zgui::Color>& value,
                               std::function<void()> on_change)
    : ColorPicker(value.get(), std::move(on_change)) {
    owned_value_ = value.owner();
}

void zgui::ColorPicker::apply_color() {
    const zgui::Color updated = from_hsv(hue_, saturation_, brightness_, alpha_);
    if (same_color(*value_, updated))
        return;
    *value_ = updated;
    if (on_change_)
        on_change_();
}

void zgui::ColorPicker::update_from_mouse(zgui::Point position) {
    const zgui::Point absolute = get_absolute_position();
    const float sv_x = absolute.x + 10.f;
    const float sv_y = absolute.y + 28.f;
    switch (drag_target_) {
    case DragTarget::saturation_value:
        saturation_ = std::clamp((position.x - sv_x) / 160.f, 0.f, 1.f);
        brightness_ = 1.f - std::clamp((position.y - sv_y) / 128.f, 0.f, 1.f);
        break;
    case DragTarget::hue:
        hue_ = std::clamp((position.y - sv_y) / 128.f, 0.f, 1.f) * 360.f;
        break;
    case DragTarget::alpha:
        alpha_ = std::clamp((position.x - sv_x) / 160.f, 0.f, 1.f);
        break;
    case DragTarget::none:
        return;
    }
    apply_color();
}

void zgui::ColorPicker::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    graphics.draw_frame(position, get_size(), theme.window_bg);
    graphics.fill_rect(position, zgui::Point(get_size().x, 20.f), theme.window_header);
    graphics.draw_text("Color picker", zgui::Point(position.x + 8.f, position.y + 3.f),
                       theme.text_primary);

    const zgui::Point sv_position(position.x + 10.f, position.y + 28.f);
    const zgui::Point sv_size(160.f, 128.f);
    graphics.draw_frame(sv_position, sv_size, theme.border_outer);
    const zgui::Color pure_hue = from_hsv(hue_, 1.f, 1.f, 1.f);
    graphics.draw_gradient_rect(zgui::Point(sv_position.x + 1.f, sv_position.y + 1.f),
                                zgui::Point(sv_size.x - 2.f, sv_size.y - 2.f),
                                zgui::Color(255, 255, 255), pure_hue, zgui::Color(0, 0, 0),
                                zgui::Color(0, 0, 0));

    const zgui::Point hue_position(position.x + 178.f, position.y + 28.f);
    constexpr int hue_segments = 36;
    for (int i = 0; i < hue_segments; ++i) {
        const float segment_y = hue_position.y + 128.f * static_cast<float>(i) / hue_segments;
        const float segment_height = 128.f / hue_segments + 0.5f;
        graphics.fill_rect(zgui::Point(hue_position.x, segment_y),
                           zgui::Point(14.f, segment_height),
                           from_hsv(360.f * static_cast<float>(i) / hue_segments, 1.f, 1.f, 1.f));
    }
    draw_outline(graphics, hue_position, zgui::Point(14.f, 128.f), theme.border_outer);

    const zgui::Point preview_position(position.x + 204.f, position.y + 28.f);
    graphics.draw_frame(preview_position, zgui::Point(48.f, 42.f), theme.border_outer);
    graphics.fill_rect(zgui::Point(preview_position.x + 2.f, preview_position.y + 2.f),
                       zgui::Point(44.f, 38.f), *value_);

    const zgui::Point alpha_position(position.x + 10.f, position.y + 166.f);
    const zgui::Point alpha_size(160.f, 10.f);
    for (int i = 0; i < 20; ++i) {
        const zgui::Color checker = i % 2 == 0 ? theme.border_inner : theme.window_bg;
        graphics.fill_rect(
            zgui::Point(alpha_position.x + 8.f * static_cast<float>(i), alpha_position.y),
            zgui::Point(8.f, alpha_size.y), checker);
    }
    const zgui::Color transparent(value_->r, value_->g, value_->b, 0);
    graphics.draw_gradient_rect(alpha_position, alpha_size, transparent, *value_, *value_,
                                transparent);
    draw_outline(graphics, alpha_position, alpha_size, theme.border_outer);

    const float saturation_x = sv_position.x + saturation_ * sv_size.x;
    const float brightness_y = sv_position.y + (1.f - brightness_) * sv_size.y;
    draw_outline(graphics, zgui::Point(saturation_x - 3.f, brightness_y - 3.f),
                 zgui::Point(6.f, 6.f), theme.text_primary);
    const float hue_y = hue_position.y + hue_ / 360.f * 128.f;
    graphics.draw_line(zgui::Point(hue_position.x - 2.f, hue_y),
                       zgui::Point(hue_position.x + 16.f, hue_y), theme.text_primary);
    const float alpha_x = alpha_position.x + alpha_ * alpha_size.x;
    graphics.draw_line(zgui::Point(alpha_x, alpha_position.y - 2.f),
                       zgui::Point(alpha_x, alpha_position.y + alpha_size.y + 2.f),
                       theme.text_primary);

    std::ostringstream hex;
    hex << '#' << std::uppercase << std::hex << std::setfill('0') << std::setw(2)
        << static_cast<unsigned int>(value_->r) << std::setw(2)
        << static_cast<unsigned int>(value_->g) << std::setw(2)
        << static_cast<unsigned int>(value_->b) << std::setw(2)
        << static_cast<unsigned int>(value_->a);
    graphics.draw_text(hex.str(), zgui::Point(position.x + 10.f, position.y + 188.f),
                       theme.text_primary);
}

void zgui::ColorPicker::update() {
    if (drag_target_ == DragTarget::none)
        return;
    if (!zgui::core::input->mouse_button_pressed()) {
        drag_target_ = DragTarget::none;
        return;
    }
    update_from_mouse(zgui::core::input->mouse_position());
}

bool zgui::ColorPicker::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    const zgui::Point absolute = get_absolute_position();
    if (inside_rect(position, zgui::Point(absolute.x + 10.f, absolute.y + 28.f),
                    zgui::Point(160.f, 128.f))) {
        drag_target_ = DragTarget::saturation_value;
    } else if (inside_rect(position, zgui::Point(absolute.x + 178.f, absolute.y + 28.f),
                           zgui::Point(14.f, 128.f))) {
        drag_target_ = DragTarget::hue;
    } else if (inside_rect(position, zgui::Point(absolute.x + 10.f, absolute.y + 166.f),
                           zgui::Point(160.f, 10.f))) {
        drag_target_ = DragTarget::alpha;
    } else {
        drag_target_ = DragTarget::none;
    }
    update_from_mouse(position);
    return true;
}

zgui::ColorSwatch::ColorSwatch(std::string label, zgui::Color* value,
                               std::function<void()> on_change)
    : value_(value != nullptr ? value : fallback_value_.get()), on_change_(std::move(on_change)) {
    assert(value != nullptr && "color swatch value must not be null");
    if (value != nullptr)
        owned_value_.reset();
    set_title(std::move(label));
}

zgui::ColorSwatch::ColorSwatch(zgui::Color* value, std::function<void()> on_change)
    : ColorSwatch({}, value, std::move(on_change)) {}

zgui::ColorSwatch::ColorSwatch(std::string label, const zgui::Bound<zgui::Color>& value,
                               std::function<void()> on_change)
    : ColorSwatch(std::move(label), value.get(), std::move(on_change)) {
    if (value)
        owned_value_ = value.owner();
}

zgui::ColorSwatch::ColorSwatch(const zgui::Bound<zgui::Color>& value,
                               std::function<void()> on_change)
    : ColorSwatch({}, value, std::move(on_change)) {}

void zgui::ColorSwatch::set_on_change(std::function<void()> on_change) {
    on_change_ = std::move(on_change);
}

void zgui::ColorSwatch::set_vertical_layout(bool vertical) {
    vertical_layout_ = vertical;
}

bool zgui::ColorSwatch::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    const zgui::Point absolute = get_absolute_position();
    const bool in_pick_button = vertical_layout_
                                    ? position.y >= absolute.y + std::max(0.f, size_.y - 16.f)
                                    : position.x >= absolute.x + std::max(0.f, size_.x - 42.f);
    if (!in_pick_button)
        return true;
    if (!picker_.expired())
        return true;
    if (zgui::core::window_manager == nullptr) {
        assert(zgui::core::window_manager != nullptr &&
               "ColorSwatch requires an active WindowManager");
        return true;
    }

    auto picker = owned_value_ ? std::make_shared<zgui::ColorPicker>(
                                     zgui::Bound<zgui::Color>(owned_value_), on_change_)
                               : std::make_shared<zgui::ColorPicker>(value_, on_change_);
    picker->set_position(zgui::Point(absolute.x, absolute.y + size_.y + 2.f));
    picker_ = picker;
    zgui::core::window_manager->show_overlay(picker);
    return true;
}

void zgui::ColorSwatch::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    if (vertical_layout_) {
        const float label_width = graphics.measure_text_width(title_);
        graphics.draw_text(title_,
                           zgui::Point(position.x + (size.x - label_width) / 2.f, position.y + 1.f),
                           theme.text_primary);
        const zgui::Point bar_position(position.x + 2.f, position.y + 15.f);
        const zgui::Point bar_size(std::max(0.f, size.x - 4.f), 8.f);
        graphics.draw_frame(bar_position, bar_size, theme.border_outer);
        graphics.fill_rect(
            zgui::Point(bar_position.x + 2.f, bar_position.y + 2.f),
            zgui::Point(std::max(0.f, bar_size.x - 4.f), std::max(0.f, bar_size.y - 4.f)), *value_);
        const float button_width = std::min(42.f, std::max(0.f, size.x - 4.f));
        const float button_x = position.x + (size.x - button_width) / 2.f;
        const float button_y = position.y + std::max(0.f, size.y - 16.f);
        graphics.draw_frame(zgui::Point(button_x, button_y), zgui::Point(button_width, 14.f),
                            theme.control_bg);
        const float text_width = graphics.measure_text_width("Pick");
        graphics.draw_text(
            "Pick", zgui::Point(button_x + (button_width - text_width) / 2.f, button_y + 1.f),
            theme.text_primary);
        return;
    }

    const float label_width = title_.empty() ? 0.f : graphics.measure_text_width(title_) + 6.f;
    if (!title_.empty())
        graphics.draw_text(title_, position, theme.text_primary);

    const float button_x = position.x + std::max(0.f, size.x - 42.f);
    const float preview_x = position.x + label_width;
    const float preview_width = std::max(0.f, button_x - preview_x - 4.f);
    graphics.draw_frame(zgui::Point(preview_x, position.y), zgui::Point(preview_width, size.y),
                        theme.border_outer);
    graphics.fill_rect(zgui::Point(preview_x + 2.f, position.y + 2.f),
                       zgui::Point(std::max(0.f, preview_width - 4.f), std::max(0.f, size.y - 4.f)),
                       *value_);
    graphics.draw_frame(zgui::Point(button_x, position.y), zgui::Point(42.f, size.y),
                        theme.control_bg);
    const float text_width = graphics.measure_text_width("Pick");
    graphics.draw_text(
        "Pick",
        zgui::Point(button_x + (42.f - text_width) / 2.f, position.y + (size.y - 14.f) / 2.f),
        theme.text_primary);
}

zgui::ColorButton::ColorButton(std::string label, zgui::Color* value,
                               std::function<void()> on_change)
    : value_(value != nullptr ? value : fallback_value_.get()), on_change_(std::move(on_change)) {
    assert(value != nullptr && "color button value must not be null");
    if (value != nullptr)
        owned_value_.reset();
    set_title(std::move(label));
}

zgui::ColorButton::ColorButton(std::string label, const zgui::Bound<zgui::Color>& value,
                               std::function<void()> on_change)
    : ColorButton(std::move(label), value.get(), std::move(on_change)) {
    if (value)
        owned_value_ = value.owner();
}

bool zgui::ColorButton::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    const zgui::Point absolute = get_absolute_position();
    const float button_y = absolute.y + 14.f;
    if (position.y < button_y)
        return true;
    pressed_ = true;
    if (!picker_.expired())
        return true;
    if (zgui::core::window_manager == nullptr) {
        assert(zgui::core::window_manager != nullptr &&
               "ColorButton requires an active WindowManager");
        return true;
    }

    auto picker = owned_value_ ? std::make_shared<zgui::ColorPicker>(
                                     zgui::Bound<zgui::Color>(owned_value_), on_change_)
                               : std::make_shared<zgui::ColorPicker>(value_, on_change_);
    const zgui::Point screen = zgui::core::graphics->viewport_size();
    zgui::Point popup_position(absolute.x, button_y + std::max(0.f, size_.y - 14.f) + 2.f);
    if (screen.x > 0.f && screen.y > 0.f) {
        popup_position.x =
            std::clamp(popup_position.x, 0.f, std::max(0.f, screen.x - picker->get_size().x));
        popup_position.y =
            std::clamp(popup_position.y, 0.f, std::max(0.f, screen.y - picker->get_size().y));
    }
    picker->set_position(popup_position);
    picker_ = picker;
    zgui::core::window_manager->show_overlay(picker);
    return true;
}

void zgui::ColorButton::update() {
    if (pressed_ && !zgui::core::input->mouse_button_pressed())
        pressed_ = false;
}

void zgui::ColorButton::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    const std::string fitted_title = graphics.fit_text(title_, size.x);
    graphics.draw_text(fitted_title, zgui::Point(position.x, position.y), theme.text_primary);
    const zgui::Point button_position(position.x, position.y + 14.f);
    const zgui::Point button_size(size.x, std::max(18.f, size.y - 14.f));
    const bool hovered = contains_point(zgui::core::input->mouse_position());
    graphics.draw_frame(button_position, button_size,
                        pressed_ || hovered ? theme.control_hover : theme.control_bg);
    const std::string label = graphics.fit_text("Pick", button_size.x - 8.f);
    const float label_width = graphics.measure_text_width(label);
    graphics.draw_text(label,
                       zgui::Point(button_position.x + (button_size.x - label_width) * 0.5f,
                                   button_position.y + 2.f),
                       theme.text_primary);
    constexpr float bar_height = 4.f;
    const float bar_width = std::max(0.f, button_size.x - 6.f);
    const float bar_x = button_position.x + 3.f;
    const float bar_y = button_position.y + button_size.y - bar_height - 3.f;
    for (int checker = 0; checker < static_cast<int>(bar_width); checker += 4) {
        graphics.fill_rect(
            zgui::Point(bar_x + static_cast<float>(checker), bar_y),
            zgui::Point(std::min(4.f, bar_width - static_cast<float>(checker)), bar_height),
            ((checker / 4) % 2 == 0) ? theme.text_primary : theme.border_inner);
    }
    graphics.fill_rect(zgui::Point(bar_x, bar_y), zgui::Point(bar_width, bar_height), *value_);
    graphics.draw_rect(zgui::Point(bar_x, bar_y), zgui::Point(bar_width, bar_height),
                       theme.border_outer);
}

float zgui::TableView::content_height() const {
    const float available_height = std::max(0.f, size_.y - 24.f);
    return std::floor(available_height / row_height_) * row_height_;
}

std::vector<float> zgui::TableView::effective_column_widths(float available_width) const {
    std::vector<float> widths(columns_.size(), 0.f);
    float fixed_total = 0.f;
    float total_weight = 0.f;
    std::optional<std::size_t> last_visible;
    for (std::size_t index = 0; index < columns_.size(); ++index) {
        if (!columns_[index].visible)
            continue;
        const bool user_fixed = index < user_fixed_columns_.size() && user_fixed_columns_[index];
        if (columns_[index].weight > 0.f && !user_fixed) {
            total_weight += columns_[index].weight;
        } else {
            widths[index] = columns_[index].width;
            fixed_total += widths[index];
        }
        last_visible = index;
    }
    const float flexible_width = std::max(0.f, available_width - fixed_total);
    if (total_weight > 0.f) {
        for (std::size_t index = 0; index < columns_.size(); ++index) {
            if (columns_[index].visible && columns_[index].weight > 0.f &&
                (index >= user_fixed_columns_.size() || !user_fixed_columns_[index])) {
                widths[index] =
                    std::max(24.f, flexible_width * columns_[index].weight / total_weight);
            }
        }
    } else if (last_visible && fixed_total < available_width) {
        widths[*last_visible] += available_width - fixed_total;
    }
    return widths;
}

float zgui::TableView::maximum_scroll() const {
    const float total_height = row_height_ * static_cast<float>(rows_.size());
    return std::max(0.f, total_height - content_height());
}

void zgui::TableView::scroll_to_mouse(float mouse_y) {
    const zgui::Point absolute = get_absolute_position();
    const float track_top = absolute.y + 22.f;
    const float track_height = content_height();
    if (track_height <= 0.f)
        return;
    const float ratio = std::clamp((mouse_y - track_top) / track_height, 0.f, 1.f);
    const float target = ratio * maximum_scroll();
    scroll_offset_ = std::min(maximum_scroll(), std::round(target / row_height_) * row_height_);
}

void zgui::TableView::set_columns(std::vector<Column> columns) {
    for (Column& column : columns) {
        column.width = std::isfinite(column.width) ? std::max(24.f, column.width) : 24.f;
        column.weight = std::isfinite(column.weight) ? std::max(0.f, column.weight) : 0.f;
    }
    columns_ = std::move(columns);
    user_fixed_columns_.assign(columns_.size(), false);
    if (resizing_column_ && *resizing_column_ >= columns_.size())
        resizing_column_.reset();
}

void zgui::TableView::set_size(const zgui::Point& size) {
    const bool changed = size.x != size_.x || size.y != size_.y;
    UIElement::set_size(size);
    if (changed)
        std::fill(user_fixed_columns_.begin(), user_fixed_columns_.end(), false);
    scroll_offset_ = std::clamp(scroll_offset_, 0.f, maximum_scroll());
}

void zgui::TableView::set_zebra_stripes(bool enabled) {
    zebra_stripes_ = enabled;
}

void zgui::TableView::add_row(Row row) {
    rows_.push_back(std::move(row));
    row_backgrounds_.push_back(std::nullopt);
}

void zgui::TableView::add_row(Row row, zgui::Color background) {
    rows_.push_back(std::move(row));
    row_backgrounds_.push_back(background);
}

void zgui::TableView::add_row(const std::vector<std::string>& cells) {
    Row row;
    row.reserve(cells.size());
    for (const std::string& text : cells)
        row.push_back({text, std::nullopt});
    add_row(std::move(row));
}

void zgui::TableView::clear_rows() {
    rows_.clear();
    row_backgrounds_.clear();
    selected_row_.reset();
    scroll_offset_ = 0.f;
}

void zgui::TableView::set_column_visible(std::size_t index, bool visible) {
    if (index >= columns_.size())
        return;
    columns_[index].visible = visible;
}

void zgui::TableView::set_on_selection_changed(std::function<void(std::size_t)> callback) {
    on_selection_changed_ = std::move(callback);
}

bool zgui::TableView::select_row(std::size_t index) {
    if (index >= rows_.size())
        return false;
    const bool changed = selected_row_ != index;
    selected_row_ = index;
    if (changed && on_selection_changed_)
        on_selection_changed_(index);
    return true;
}

bool zgui::TableView::set_cell(std::size_t row, std::size_t column, Cell cell) {
    if (row >= rows_.size() || column >= columns_.size())
        return false;
    if (rows_[row].size() <= column)
        rows_[row].resize(column + 1);
    rows_[row][column] = std::move(cell);
    return true;
}

std::optional<std::size_t> zgui::TableView::selected_row() const {
    return selected_row_;
}

std::size_t zgui::TableView::row_count() const {
    return rows_.size();
}

void zgui::TableView::set_row_height(float height) {
    if (!std::isfinite(height) || height <= 0.f) {
        assert(std::isfinite(height) && height > 0.f &&
               "table row height must be finite and positive");
        return;
    }
    row_height_ = height;
    scroll_offset_ =
        std::min(maximum_scroll(), std::floor(scroll_offset_ / row_height_) * row_height_);
}

void zgui::TableView::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    constexpr float header_height = 20.f;
    constexpr float scrollbar_width = 10.f;
    const zgui::Point header_position(position.x + 2.f, position.y + 2.f);
    const float inner_width = std::max(0.f, size.x - 4.f);
    graphics.draw_frame(position, size, theme.groupbox_bg);
    graphics.fill_rect(header_position, zgui::Point(inner_width, header_height),
                       theme.window_header);
    graphics.flush();
    const float content_width = std::max(0.f, inner_width - scrollbar_width);
    const std::vector<float> column_widths = effective_column_widths(content_width);

    {
        ClipScope header_clip(graphics, static_cast<int>(header_position.x),
                              static_cast<int>(header_position.y), static_cast<int>(content_width),
                              static_cast<int>(header_height));
        float column_x = header_position.x;
        for (std::size_t index = 0; index < columns_.size(); ++index) {
            if (column_widths[index] <= 0.f)
                continue;
            const float width = std::min(
                column_widths[index], std::max(0.f, header_position.x + content_width - column_x));
            graphics.draw_text(truncate_with_ellipsis(graphics, columns_[index].title, width - 8.f),
                               zgui::Point(column_x + 4.f, header_position.y + 3.f),
                               theme.text_primary);
            column_x += column_widths[index];
        }
    }

    const float content_y = position.y + 22.f;
    const float rows_height = content_height();
    const float scroll = std::min(scroll_offset_, maximum_scroll());
    {
        ClipScope rows_clip(graphics, static_cast<int>(position.x + 2.f),
                            static_cast<int>(content_y), static_cast<int>(content_width),
                            static_cast<int>(rows_height));
        std::size_t row_index = static_cast<std::size_t>(scroll / row_height_);
        float row_y = content_y + static_cast<float>(row_index) * row_height_ - scroll;
        for (; row_index < rows_.size() && row_y < content_y + rows_height;
             ++row_index, row_y += row_height_) {
            const zgui::Point row_position(position.x + 2.f, row_y);
            const zgui::Point row_size(content_width, row_height_);
            const bool selected = selected_row_ && *selected_row_ == row_index;
            if (selected) {
                graphics.fill_rect(row_position, row_size, theme.accent);
            } else if (row_index < row_backgrounds_.size() &&
                       row_backgrounds_[row_index].has_value()) {
                graphics.fill_rect(row_position, row_size, *row_backgrounds_[row_index]);
            } else if (zebra_stripes_ && (row_index % 2) != 0) {
                graphics.fill_rect(row_position, row_size, theme.window_bg);
            }

            float column_x = row_position.x;
            const Row& row = rows_[row_index];
            for (std::size_t column_index = 0;
                 column_index < columns_.size() && column_x < row_position.x + content_width;
                 ++column_index) {
                const float cell_width = std::min(column_widths[column_index],
                                                  row_position.x + content_width - column_x);
                if (cell_width <= 0.f)
                    continue;
                if (column_index < row.size()) {
                    const Cell& cell = row[column_index];
                    ClipScope cell_clip(graphics, static_cast<int>(column_x),
                                        static_cast<int>(row_y), static_cast<int>(cell_width),
                                        static_cast<int>(row_height_));
                    graphics.draw_text(truncate_with_ellipsis(graphics, cell.text, cell_width - 8.f,
                                                              !cell.user_data),
                                       zgui::Point(column_x + 4.f, row_y + 2.f),
                                       selected ? theme.text_primary
                                       : row_index < row_backgrounds_.size() &&
                                               row_backgrounds_[row_index].has_value()
                                           ? theme.text_primary
                                           : cell.color.value_or(theme.text_primary));
                }
                column_x += column_widths[column_index];
            }
        }
    }

    const float scrollbar_x = position.x + size.x - scrollbar_width - 2.f;
    const float scrollbar_y = content_y;
    const float scrollbar_height = rows_height;
    graphics.fill_rect(zgui::Point(scrollbar_x, scrollbar_y),
                       zgui::Point(scrollbar_width, scrollbar_height), theme.window_bg);
    const float total_height = row_height_ * static_cast<float>(rows_.size());
    const float thumb_height =
        total_height <= 0.f
            ? scrollbar_height
            : std::min(scrollbar_height,
                       std::max(14.f, scrollbar_height * scrollbar_height / total_height));
    const float scroll_ratio = maximum_scroll() <= 0.f ? 0.f : scroll / maximum_scroll();
    const float thumb_y =
        scrollbar_y + std::max(0.f, scrollbar_height - thumb_height) * scroll_ratio;
    graphics.draw_frame(zgui::Point(scrollbar_x, thumb_y),
                        zgui::Point(scrollbar_width, thumb_height), theme.control_hover);

    const zgui::Point mouse = zgui::core::input->mouse_position();
    const bool in_header =
        mouse.y >= header_position.y && mouse.y < header_position.y + header_height;
    float divider_x = header_position.x;
    for (std::size_t index = 0; index + 1 < column_widths.size(); ++index) {
        divider_x += column_widths[index];
        if (divider_x >= header_position.x + content_width)
            break;
        if (column_widths[index] <= 0.f)
            continue;
        const bool hovered = in_header && std::abs(mouse.x - divider_x) <= 4.f;
        graphics.draw_line(zgui::Point(divider_x, header_position.y),
                           zgui::Point(divider_x, header_position.y + header_height),
                           hovered ? theme.accent : theme.border_outer);
    }
}

void zgui::TableView::update() {
    const zgui::Point mouse = zgui::core::input->mouse_position();
    if (resizing_column_) {
        if (!zgui::core::input->mouse_button_pressed()) {
            resizing_column_.reset();
        } else {
            Column& column = columns_[*resizing_column_];
            column.width = std::max(24.f, resize_start_width_ + mouse.x - resize_start_x_);
        }
    } else if (dragging_scrollbar_) {
        if (!zgui::core::input->mouse_button_pressed()) {
            dragging_scrollbar_ = false;
        } else {
            scroll_to_mouse(mouse.y);
        }
    } else if (contains_point(mouse) && zgui::core::input->mouse_wheel_delta() != 0) {
        scroll_offset_ =
            std::clamp(scroll_offset_ - static_cast<float>(zgui::core::input->mouse_wheel_delta()) *
                                            row_height_ * 3.f,
                       0.f, maximum_scroll());
    }
}

bool zgui::TableView::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    const zgui::Point absolute = get_absolute_position();
    if (position.y < absolute.y + 22.f) {
        float divider_x = absolute.x + 2.f;
        const float available_width = std::max(0.f, get_size().x - 14.f);
        const std::vector<float> widths = effective_column_widths(available_width);
        for (std::size_t index = 0; index + 1 < widths.size(); ++index) {
            divider_x += widths[index];
            if (widths[index] <= 0.f)
                continue;
            if (divider_x < absolute.x + get_size().x - 12.f &&
                std::abs(position.x - divider_x) <= 4.f) {
                resizing_column_ = index;
                resize_start_x_ = position.x;
                resize_start_width_ = widths[index];
                user_fixed_columns_[index] = true;
                return true;
            }
        }
        return true;
    }
    if (position.x >= absolute.x + get_size().x - 12.f) {
        dragging_scrollbar_ = true;
        scroll_to_mouse(position.y);
        return true;
    }

    if (position.y >= absolute.y + 22.f + content_height())
        return true;
    const float row_position = position.y - (absolute.y + 22.f) + scroll_offset_;
    if (row_position >= 0.f) {
        const auto row = static_cast<std::size_t>(row_position / row_height_);
        if (row < rows_.size())
            select_row(row);
    }
    return true;
}

bool zgui::TableView::handles_mouse_wheel(zgui::Point position) const {
    if (!contains_point(position))
        return false;
    const float maximum = maximum_scroll();
    const int wheel = zgui::core::input->mouse_wheel_delta();
    if (wheel == 0)
        return maximum > 0.f;
    if (wheel > 0)
        return scroll_offset_ > 0.f;
    return scroll_offset_ < maximum;
}

std::size_t zgui::TableView::widget_count() const {
    std::size_t count = UIElement::widget_count() + rows_.size();
    for (const Row& row : rows_)
        count += row.size();
    return count;
}

std::size_t zgui::TableView::visible_widget_count() const {
    const std::size_t first_row = static_cast<std::size_t>(scroll_offset_ / row_height_);
    const std::size_t visible_rows =
        row_height_ > 0.f ? static_cast<std::size_t>(content_height() / row_height_) : 0;
    const std::size_t end_row = std::min(rows_.size(), first_row + visible_rows);
    std::size_t count = 1;
    for (std::size_t row = first_row; row < end_row; ++row)
        count += 1 + rows_[row].size();
    return count;
}

void zgui::Viewport::set_on_render(std::function<void(int, int, int, int)> on_render) {
    on_render_ = std::move(on_render);
}

void zgui::Viewport::set_on_input(std::function<void(const zgui::ViewportInput&)> on_input) {
    on_input_ = std::move(on_input);
}

void zgui::Viewport::draw() {
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    auto& graphics = *zgui::core::graphics;
    graphics.draw_frame(position, size, zgui::core::active_theme.groupbox_bg);
    graphics.flush();

    const int inner_x = static_cast<int>(position.x + 2.f);
    const int inner_y = static_cast<int>(position.y + 2.f);
    const int inner_width = std::max(0, static_cast<int>(size.x - 4.f));
    const int inner_height = std::max(0, static_cast<int>(size.y - 4.f));
    ClipScope viewport_clip(graphics, inner_x, inner_y, inner_width, inner_height);
    if (on_render_)
        on_render_(inner_x, inner_y, inner_width, inner_height);
    UIElement::draw();
}

void zgui::Viewport::update() {
    if (!on_input_)
        return;
    const bool popup_open =
        zgui::core::window_manager != nullptr && zgui::core::window_manager->has_overlays();
    if (popup_open) {
        pointer_active_ = false;
        return;
    }
    const zgui::Point mouse = zgui::core::input->mouse_position();
    const bool inside = contains_point(mouse);
    const bool left = zgui::core::input->mouse_button_down(Input::MouseButton::left);
    const bool middle = zgui::core::input->mouse_button_down(Input::MouseButton::middle);
    const bool right = zgui::core::input->mouse_button_down(Input::MouseButton::right);
    const bool rotating = pointer_active_ && left;
    const bool panning = pointer_active_ && (middle || right);
    const int wheel = inside ? zgui::core::input->mouse_wheel_delta() : 0;
    const bool double_clicked = inside && zgui::core::input->mouse_double_clicked();
    if (!left && !middle && !right)
        pointer_active_ = false;
    if ((inside && (wheel != 0 || double_clicked)) || rotating || panning) {
        on_input_({zgui::core::input->mouse_delta(), wheel, rotating, panning, double_clicked});
    }
}

bool zgui::Viewport::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    pointer_active_ = true;
    return true;
}

zgui::Rect zgui::Viewport::child_clip_rect() const {
    const zgui::Point absolute = get_absolute_position();
    return {zgui::Point(absolute.x + 2.f, absolute.y + 2.f),
            zgui::Point(std::max(0.f, size_.x - 4.f), std::max(0.f, size_.y - 4.f))};
}

zgui::TabBar::TabBar(float tab_height) : tab_height_(tab_height) {}

zgui::TabBar::TabLayout zgui::TabBar::measure_tab_layout(float available_width) const {
    TabLayout layout;
    layout.widths.resize(tabs_.size(), 0.f);
    if (tabs_.empty())
        return layout;

    available_width = std::max(0.f, available_width);
    float text_width = 0.f;
    for (const Tab& tab : tabs_)
        text_width += zgui::core::graphics->measure_text_width(tab.title);

    // Give up side padding before giving up text: labels are only truncated once the
    // padding is already at its minimum.
    const float tab_count = static_cast<float>(tabs_.size());
    const float minimum_padding = std::min(layout.padding, spacing::kTabLabelMinimumPadding);
    if (text_width + 2.f * layout.padding * tab_count > available_width) {
        layout.padding = std::clamp((available_width - text_width) / (2.f * tab_count),
                                    minimum_padding, layout.padding);
    }

    const float padding_width = 2.f * layout.padding * tab_count;
    if (available_width < padding_width) {
        layout.padding = available_width / (2.f * tab_count);
        layout.text_scale = 0.f;
    } else if (text_width > 0.f) {
        layout.text_scale = std::min(1.f, (available_width - padding_width) / text_width);
    }

    for (std::size_t index = 0; index < tabs_.size(); ++index) {
        const float label_width = zgui::core::graphics->measure_text_width(tabs_[index].title);
        layout.widths[index] = 2.f * layout.padding + label_width * layout.text_scale;
    }
    return layout;
}

std::shared_ptr<zgui::UIElement> zgui::TabBar::add_tab(std::string title) {
    auto page = std::make_shared<zgui::UIElement>();
    page->set_parent(this);
    tabs_.push_back({std::move(title), page});
    layout_tabs();
    return page;
}

void zgui::TabBar::set_size(const zgui::Point& size) {
    UIElement::set_size(size);
    layout_tabs();
}

bool zgui::TabBar::set_active_tab(std::size_t index) {
    if (index >= tabs_.size())
        return false;
    active_tab_ = index;
    page_scroll_offset_ = 0.f;
    return true;
}

std::size_t zgui::TabBar::get_active_tab() const {
    return active_tab_;
}

std::size_t zgui::TabBar::tab_count() const {
    return tabs_.size();
}

const std::string& zgui::TabBar::tab_title(std::size_t index) const {
    static const std::string empty;
    return index < tabs_.size() ? tabs_[index].title : empty;
}

std::size_t zgui::TabBar::tab_widget_count(std::size_t index) const {
    return index < tabs_.size() ? tabs_[index].page->widget_count() : 0;
}

std::size_t zgui::TabBar::tab_visible_widget_count(std::size_t index) const {
    return index < tabs_.size() && index == active_tab_ ? tabs_[index].page->visible_widget_count()
                                                        : 0;
}

std::size_t zgui::TabBar::widget_count() const {
    std::size_t count = 1;
    for (const Tab& tab : tabs_)
        count += tab.page->widget_count();
    for (const UIElement* child : children_)
        count += child->widget_count();
    return count;
}

std::size_t zgui::TabBar::visible_widget_count() const {
    if (tabs_.empty())
        return UIElement::visible_widget_count();
    return 1 + tabs_[active_tab_].page->visible_widget_count();
}

void zgui::TabBar::layout_tabs() {
    const zgui::Point size = get_size();
    const float viewport_height = std::max(0.f, size.y - tab_height_);
    float active_page_height = viewport_height;
    if (!tabs_.empty())
        active_page_height = std::max(viewport_height, tabs_[active_tab_].page->content_bottom());
    page_scroll_offset_ =
        std::clamp(page_scroll_offset_, 0.f, std::max(0.f, active_page_height - viewport_height));
    for (std::size_t index = 0; index < tabs_.size(); ++index) {
        auto& page = tabs_[index].page;
        const float page_height = std::max(viewport_height, page->content_bottom());
        page->set_position(zgui::Point(0.f, 0.f));
        page->set_size(zgui::Point(std::max(0.f, size.x), page_height));
    }
}

void zgui::TabBar::draw() {
    layout_tabs();
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    graphics.fill_rect(position, zgui::Point(size.x, tab_height_), theme.window_header);

    if (!tabs_.empty()) {
        const TabLayout tab_layout = measure_tab_layout(size.x);
        float tab_x = position.x;
        {
            ClipScope header_clip(graphics, static_cast<int>(position.x),
                                  static_cast<int>(position.y), static_cast<int>(size.x),
                                  static_cast<int>(tab_height_));
            for (std::size_t i = 0; i < tabs_.size(); ++i) {
                const float tab_width = tab_layout.widths[i];
                const float x = tab_x;
                const zgui::Point tab_position(x, position.y);
                graphics.fill_rect(tab_position, zgui::Point(tab_width, tab_height_),
                                   i == active_tab_ ? theme.control_bg : theme.window_header);
                if (i == active_tab_) {
                    graphics.fill_rect(zgui::Point(x, position.y + tab_height_ - 2.f),
                                       zgui::Point(tab_width, 2.f), theme.accent);
                }
                const std::string text = truncate_with_ellipsis(
                    graphics, tabs_[i].title, std::max(0.f, tab_width - 2.f * tab_layout.padding));
                const float text_x = x + (tab_width - graphics.measure_text_width(text)) / 2.f;
                graphics.draw_text(text,
                                   zgui::Point(text_x, position.y + (tab_height_ - 12.f) / 2.f),
                                   i == active_tab_ ? theme.text_primary : theme.text_dim);
                tab_x += tab_width;
            }
        }

        const zgui::Rect clip_rect = child_clip_rect();
        ClipScope clip(graphics, static_cast<int>(clip_rect.position.x),
                       static_cast<int>(clip_rect.position.y), static_cast<int>(clip_rect.size.x),
                       static_cast<int>(clip_rect.size.y));
        tabs_[active_tab_].page->draw();
    } else {
        UIElement::draw();
    }
}

void zgui::TabBar::update() {
    if (tabs_.empty()) {
        UIElement::update();
        return;
    }
    layout_tabs();
    const zgui::Point mouse = zgui::core::input->mouse_position();
    const zgui::Rect page_clip = child_clip_rect();
    if (page_clip.contains(mouse) && zgui::core::input->mouse_wheel_delta() != 0 &&
        !tabs_[active_tab_].page->handles_mouse_wheel(mouse)) {
        const float direction = static_cast<float>(zgui::core::input->mouse_wheel_delta());
        page_scroll_offset_ =
            std::clamp(page_scroll_offset_ - direction * 24.f, 0.f,
                       std::max(0.f, tabs_[active_tab_].page->get_size().y - page_clip.size.y));
        layout_tabs();
    }
    tabs_[active_tab_].page->update();
}

bool zgui::TabBar::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;

    const zgui::Point absolute = get_absolute_position();
    if (position.y < absolute.y + tab_height_) {
        if (tabs_.empty())
            return true;
        const float available_width = get_size().x;
        const TabLayout tab_layout = measure_tab_layout(available_width);
        float tab_x = absolute.x;
        for (std::size_t index = 0; index < tabs_.size(); ++index) {
            const float tab_width = tab_layout.widths[index];
            if (position.x >= tab_x && position.x < tab_x + tab_width) {
                set_active_tab(index);
                break;
            }
            tab_x += tab_width;
        }
        return true;
    }

    if (!tabs_.empty())
        return tabs_[active_tab_].page->handle_mouse_press(position);
    return UIElement::handle_mouse_press(position);
}

bool zgui::TabBar::handles_mouse_wheel(zgui::Point position) const {
    if (!contains_point(position))
        return false;
    if (tabs_.empty())
        return UIElement::handles_mouse_wheel(position);
    if (!child_clip_rect().contains(position))
        return false;
    const auto& page = tabs_[active_tab_].page;
    if (page->handles_mouse_wheel(position))
        return true;
    const float maximum = std::max(0.f, page->get_size().y - child_clip_rect().size.y);
    const int wheel = zgui::core::input->mouse_wheel_delta();
    if (wheel == 0)
        return maximum > 0.f;
    if (wheel > 0)
        return page_scroll_offset_ > 0.f;
    return page_scroll_offset_ < maximum;
}

float zgui::TabBar::content_bottom() const {
    if (tabs_.empty())
        return UIElement::content_bottom();

    float bottom = 0.f;
    for (const Tab& tab : tabs_)
        bottom = std::max(bottom, tab_height_ + tab.page->content_bottom());
    return bottom;
}

zgui::Rect zgui::TabBar::child_clip_rect() const {
    const zgui::Point absolute = get_absolute_position();
    return {zgui::Point(absolute.x, absolute.y + tab_height_),
            zgui::Point(size_.x, std::max(0.f, size_.y - tab_height_))};
}

zgui::Point zgui::TabBar::child_offset() const {
    return zgui::Point(0.f, tab_height_ - page_scroll_offset_);
}

float zgui::TabBar::content_right() const {
    if (tabs_.empty())
        return UIElement::content_right();
    float right = 0.f;
    for (const Tab& tab : tabs_)
        right = std::max(right, tab.page->content_right());
    return right;
}

void zgui::ScrollArea::scrollbar_visibility(bool& vertical, bool& horizontal) const {
    vertical = false;
    horizontal = false;
    for (int attempt = 0; attempt < 3; ++attempt) {
        const bool next_horizontal =
            horizontal_scrolling_ &&
            content_right() > std::max(0.f, size_.x - 2.f - (vertical ? scrollbar_width_ : 0.f));
        const bool next_vertical =
            content_bottom() > std::max(0.f, size_.y - 2.f - (horizontal ? scrollbar_width_ : 0.f));
        if (next_vertical == vertical && next_horizontal == horizontal)
            break;
        vertical = next_vertical;
        horizontal = next_horizontal;
    }
}

zgui::Point zgui::ScrollArea::viewport_size() const {
    bool vertical = false;
    bool horizontal = false;
    scrollbar_visibility(vertical, horizontal);
    return zgui::Point(std::max(0.f, size_.x - 2.f - (vertical ? scrollbar_width_ : 0.f)),
                       std::max(0.f, size_.y - 2.f - (horizontal ? scrollbar_width_ : 0.f)));
}

float zgui::ScrollArea::maximum_vertical_scroll() const {
    return std::max(0.f, content_bottom() - viewport_size().y);
}

float zgui::ScrollArea::maximum_horizontal_scroll() const {
    return horizontal_scrolling_ ? std::max(0.f, content_right() - viewport_size().x) : 0.f;
}

zgui::Rect zgui::ScrollArea::vertical_scrollbar_track() const {
    bool vertical = false;
    bool horizontal = false;
    scrollbar_visibility(vertical, horizontal);
    if (!vertical)
        return {};
    const zgui::Point position = get_absolute_position();
    return {zgui::Point(position.x + size_.x - 1.f - scrollbar_width_, position.y + 1.f),
            zgui::Point(scrollbar_width_,
                        std::max(0.f, size_.y - 2.f - (horizontal ? scrollbar_width_ : 0.f)))};
}

zgui::Rect zgui::ScrollArea::horizontal_scrollbar_track() const {
    bool vertical = false;
    bool horizontal = false;
    scrollbar_visibility(vertical, horizontal);
    if (!horizontal)
        return {};
    const zgui::Point position = get_absolute_position();
    return {zgui::Point(position.x + 1.f, position.y + size_.y - 1.f - scrollbar_width_),
            zgui::Point(std::max(0.f, size_.x - 2.f - (vertical ? scrollbar_width_ : 0.f)),
                        scrollbar_width_)};
}

float zgui::ScrollArea::vertical_thumb_size() const {
    const zgui::Rect track = vertical_scrollbar_track();
    if (track.size.y <= 0.f)
        return 0.f;
    const float content_height = content_bottom();
    if (content_height <= 0.f)
        return track.size.y;
    return std::min(track.size.y,
                    std::max(14.f, track.size.y * viewport_size().y / content_height));
}

float zgui::ScrollArea::horizontal_thumb_size() const {
    const zgui::Rect track = horizontal_scrollbar_track();
    if (track.size.x <= 0.f)
        return 0.f;
    const float content_width = content_right();
    if (content_width <= 0.f)
        return track.size.x;
    return std::min(track.size.x, std::max(14.f, track.size.x * viewport_size().x / content_width));
}

void zgui::ScrollArea::scroll_vertical_to(float mouse_y) {
    const zgui::Rect track = vertical_scrollbar_track();
    const float thumb_size = vertical_thumb_size();
    const float travel = track.size.y - thumb_size;
    if (travel <= 0.f) {
        scroll_offset_.y = 0.f;
        return;
    }
    const float ratio =
        std::clamp((mouse_y - track.position.y - thumb_size / 2.f) / travel, 0.f, 1.f);
    scroll_offset_.y = ratio * maximum_vertical_scroll();
}

void zgui::ScrollArea::scroll_horizontal_to(float mouse_x) {
    const zgui::Rect track = horizontal_scrollbar_track();
    const float thumb_size = horizontal_thumb_size();
    const float travel = track.size.x - thumb_size;
    if (travel <= 0.f) {
        scroll_offset_.x = 0.f;
        return;
    }
    const float ratio =
        std::clamp((mouse_x - track.position.x - thumb_size / 2.f) / travel, 0.f, 1.f);
    scroll_offset_.x = ratio * maximum_horizontal_scroll();
}

void zgui::ScrollArea::set_horizontal_scrolling(bool enabled) {
    horizontal_scrolling_ = enabled;
    set_scroll_offset(scroll_offset_);
}

bool zgui::ScrollArea::horizontal_scrolling() const {
    return horizontal_scrolling_;
}

void zgui::ScrollArea::set_scroll_offset(zgui::Point offset) {
    scroll_offset_.x = std::clamp(offset.x, 0.f, maximum_horizontal_scroll());
    scroll_offset_.y = std::clamp(offset.y, 0.f, maximum_vertical_scroll());
}

zgui::Point zgui::ScrollArea::scroll_offset() const {
    return scroll_offset_;
}

void zgui::ScrollArea::set_size(const zgui::Point& size) {
    UIElement::set_size(size);
    set_scroll_offset(scroll_offset_);
}

void zgui::ScrollArea::draw() {
    if (!zgui::core::require_initialized())
        return;
    const zgui::Point position = get_absolute_position();
    const zgui::Theme& theme = zgui::core::active_theme;
    auto& graphics = *zgui::core::graphics;
    graphics.draw_frame(position, size_, theme.window_bg);
    UIElement::draw();

    const zgui::Rect vertical_track = vertical_scrollbar_track();
    if (vertical_track.size.y > 0.f) {
        graphics.fill_rect(vertical_track.position, vertical_track.size, theme.window_bg);
        const float thumb_size = vertical_thumb_size();
        const float travel = vertical_track.size.y - thumb_size;
        const float ratio =
            maximum_vertical_scroll() <= 0.f ? 0.f : scroll_offset_.y / maximum_vertical_scroll();
        graphics.draw_frame(zgui::Point(vertical_track.position.x,
                                        vertical_track.position.y + std::max(0.f, travel) * ratio),
                            zgui::Point(vertical_track.size.x, thumb_size), theme.control_hover);
    }

    const zgui::Rect horizontal_track = horizontal_scrollbar_track();
    if (horizontal_track.size.x > 0.f) {
        graphics.fill_rect(horizontal_track.position, horizontal_track.size, theme.window_bg);
        const float thumb_size = horizontal_thumb_size();
        const float travel = horizontal_track.size.x - thumb_size;
        const float ratio = maximum_horizontal_scroll() <= 0.f
                                ? 0.f
                                : scroll_offset_.x / maximum_horizontal_scroll();
        graphics.draw_frame(zgui::Point(horizontal_track.position.x + std::max(0.f, travel) * ratio,
                                        horizontal_track.position.y),
                            zgui::Point(thumb_size, horizontal_track.size.y), theme.control_hover);
    }
}

void zgui::ScrollArea::update() {
    if (!zgui::core::require_initialized())
        return;
    const zgui::Point mouse = zgui::core::input->mouse_position();
    const int wheel = zgui::core::input->mouse_wheel_delta();
    const bool child_handles_wheel = wheel != 0 && UIElement::handles_mouse_wheel(mouse);
    UIElement::update();

    if (dragging_vertical_scrollbar_) {
        if (!zgui::core::input->mouse_button_pressed())
            dragging_vertical_scrollbar_ = false;
        else
            scroll_vertical_to(mouse.y);
    } else if (dragging_horizontal_scrollbar_) {
        if (!zgui::core::input->mouse_button_pressed())
            dragging_horizontal_scrollbar_ = false;
        else
            scroll_horizontal_to(mouse.x);
    } else if (wheel != 0 && contains_point(mouse) && !child_handles_wheel) {
        scroll_offset_.y -= static_cast<float>(wheel) * 24.f;
    }
    set_scroll_offset(scroll_offset_);
}

bool zgui::ScrollArea::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    const zgui::Rect vertical_track = vertical_scrollbar_track();
    if (vertical_track.contains(position)) {
        dragging_vertical_scrollbar_ = true;
        dragging_horizontal_scrollbar_ = false;
        scroll_vertical_to(position.y);
        return true;
    }
    const zgui::Rect horizontal_track = horizontal_scrollbar_track();
    if (horizontal_track.contains(position)) {
        dragging_horizontal_scrollbar_ = true;
        dragging_vertical_scrollbar_ = false;
        scroll_horizontal_to(position.x);
        return true;
    }
    UIElement::handle_mouse_press(position);
    return true;
}

bool zgui::ScrollArea::handles_mouse_wheel(zgui::Point position) const {
    if (!contains_point(position))
        return false;
    if (UIElement::handles_mouse_wheel(position))
        return true;

    const int wheel = zgui::core::input->mouse_wheel_delta();
    if (wheel == 0)
        return maximum_vertical_scroll() > 0.f;
    if (wheel > 0)
        return scroll_offset_.y > 0.f;
    return scroll_offset_.y < maximum_vertical_scroll();
}

zgui::Rect zgui::ScrollArea::child_clip_rect() const {
    const zgui::Point position = get_absolute_position();
    return {zgui::Point(position.x + 1.f, position.y + 1.f), viewport_size()};
}

zgui::Point zgui::ScrollArea::child_offset() const {
    return zgui::Point(1.f - scroll_offset_.x, 1.f - scroll_offset_.y);
}

zgui::SubTabBar::SubTabBar(float tab_height) : TabBar(tab_height) {}

zgui::GroupBox::GroupBox(std::string title) {
    set_title(std::move(title));
}

void zgui::GroupBox::set_sub_tabs(const std::shared_ptr<zgui::SubTabBar>& tabs) {
    sub_tabs_ = tabs;
    if (sub_tabs_)
        sub_tabs_->set_parent(this);
    layout_sub_tabs();
}

std::shared_ptr<zgui::SubTabBar> zgui::GroupBox::get_sub_tabs() const {
    return sub_tabs_;
}

void zgui::GroupBox::set_fit_content(bool fit_content) {
    fit_content_ = fit_content;
}

void zgui::GroupBox::set_size(const zgui::Point& size) {
    UIElement::set_size(size);
    layout_sub_tabs();
}

void zgui::GroupBox::fit_to_content(float bottom_padding) {
    if (!fit_content_ || (anchor_top_ && anchor_bottom_))
        return;
    bottom_padding = std::max(0.f, bottom_padding);
    const float bottom =
        sub_tabs_ ? sub_tabs_->get_position().y + sub_tabs_->content_bottom() : content_bottom();
    const float minimum_height =
        sub_tabs_ ? spacing::kGroupSubTabTop + spacing::kSubTabBarHeight +
                        spacing::kMinimumSubTabPageHeight + spacing::kSubTabBarBottomGap
                  : spacing::kGroupTitleHeight + spacing::kGroupTitleGap + bottom_padding;
    const float reserved_bottom =
        sub_tabs_ ? std::max(bottom_padding, spacing::kSubTabBarBottomGap) : bottom_padding;
    set_size(zgui::Point(size_.x, std::max(minimum_height, bottom + reserved_bottom)));
}

void zgui::GroupBox::layout_sub_tabs() {
    if (!sub_tabs_)
        return;
    const zgui::Point size = get_size();
    sub_tabs_->set_position(zgui::Point(spacing::kSubTabStripInset, spacing::kGroupSubTabTop));
    sub_tabs_->set_size(
        zgui::Point(std::max(0.f, size.x - spacing::kSubTabStripInset * 2.f),
                    std::max(spacing::kSubTabBarHeight,
                             size.y - spacing::kGroupSubTabTop - spacing::kSubTabBarBottomGap)));
}

void zgui::GroupBox::draw() {
    if (fit_content_ && !(anchor_top_ && anchor_bottom_))
        fit_to_content();
    else
        layout_sub_tabs();
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    auto& graphics = *zgui::core::graphics;
    const zgui::Theme& theme = zgui::core::active_theme;
    const float border_y = position.y + spacing::kGroupBorderTopInset;
    const float right = position.x + size.x;
    const float bottom = position.y + size.y;
    graphics.fill_rect(
        zgui::Point(position.x + 1.f, border_y + 1.f),
        zgui::Point(std::max(0.f, size.x - 2.f), std::max(0.f, bottom - border_y - 1.f)),
        theme.groupbox_bg);

    const float title_x = position.x + spacing::kGroupInnerPadding + 2.f;
    const std::string title =
        truncate_with_ellipsis(graphics, title_, std::max(0.f, size.x - 20.f));
    const float title_width = graphics.measure_text_width(title);
    const float notch_start = title_.empty() ? title_x : title_x - 4.f;
    const float notch_end = title_.empty() ? notch_start : title_x + title_width + 4.f;
    if (!title_.empty()) {
        graphics.draw_line(zgui::Point(position.x, border_y), zgui::Point(notch_start, border_y),
                           theme.border_outer);
        graphics.draw_line(zgui::Point(notch_end, border_y), zgui::Point(right, border_y),
                           theme.border_outer);
    } else {
        graphics.draw_line(zgui::Point(position.x, border_y), zgui::Point(right, border_y),
                           theme.border_outer);
    }
    graphics.draw_line(zgui::Point(position.x, border_y), zgui::Point(position.x, bottom),
                       theme.border_outer);
    graphics.draw_line(zgui::Point(right, border_y), zgui::Point(right, bottom),
                       theme.border_outer);
    graphics.draw_line(zgui::Point(position.x, bottom), zgui::Point(right, bottom),
                       theme.border_outer);
    if (!title.empty())
        graphics.draw_text(title, zgui::Point(title_x, position.y), theme.text_primary);

    const zgui::Rect clip_rect = child_clip_rect();
    ClipScope clip(graphics, static_cast<int>(clip_rect.position.x),
                   static_cast<int>(clip_rect.position.y), static_cast<int>(clip_rect.size.x),
                   static_cast<int>(clip_rect.size.y));
    if (sub_tabs_)
        sub_tabs_->draw();
    else
        UIElement::draw();
}

void zgui::GroupBox::update() {
    if (sub_tabs_)
        sub_tabs_->update();
    else
        UIElement::update();
}

bool zgui::GroupBox::handle_mouse_press(zgui::Point position) {
    if (!contains_point(position))
        return false;
    if (sub_tabs_)
        return sub_tabs_->handle_mouse_press(position);
    return UIElement::handle_mouse_press(position);
}

bool zgui::GroupBox::handles_mouse_wheel(zgui::Point position) const {
    if (!contains_point(position))
        return false;
    if (sub_tabs_)
        return sub_tabs_->handles_mouse_wheel(position);
    return UIElement::handles_mouse_wheel(position);
}

zgui::Rect zgui::GroupBox::child_clip_rect() const {
    const zgui::Point absolute = get_absolute_position();
    return {zgui::Point(absolute.x + spacing::kGroupInnerPadding,
                        absolute.y + spacing::kGroupTitleHeight),
            zgui::Point(
                std::max(0.f, size_.x - spacing::kGroupInnerPadding * 2.f),
                std::max(0.f, size_.y - spacing::kGroupTitleHeight - spacing::kGroupBottomPad))};
}

float zgui::GroupBox::content_right() const {
    const float children_right = UIElement::content_right();
    if (!sub_tabs_)
        return children_right;
    return std::max(children_right, sub_tabs_->get_position().x + sub_tabs_->content_right());
}

std::size_t zgui::GroupBox::widget_count() const {
    return UIElement::widget_count() + (sub_tabs_ ? sub_tabs_->widget_count() : 0);
}

std::size_t zgui::GroupBox::visible_widget_count() const {
    return UIElement::visible_widget_count() + (sub_tabs_ ? sub_tabs_->visible_widget_count() : 0);
}

zgui::StackLayout::StackLayout(zgui::UIElement& parent, float start_y, float padding, float gap)
    : parent_(parent), group_(nullptr), cursor_y_(start_y), padding_(std::max(0.f, padding)),
      gap_(std::max(0.f, gap)) {}

zgui::StackLayout::StackLayout(zgui::GroupBox& group, float start_y, float padding, float gap)
    : parent_(group), group_(&group), cursor_y_(start_y), padding_(std::max(0.f, padding)),
      gap_(std::max(0.f, gap)) {}

zgui::StackLayout zgui::StackLayout::page(zgui::UIElement& page_element, float start_y,
                                          float padding, float gap) {
    return StackLayout(page_element, start_y, padding, gap);
}

float zgui::StackLayout::cursor_y() const {
    return cursor_y_;
}

void zgui::StackLayout::add_row(zgui::UIElement& element, float height) {
    const float width = std::max(0.f, parent_.get_size().x - padding_ * 2.f);
    element.set_position(zgui::Point(padding_, cursor_y_));
    element.set_size(zgui::Point(width, std::max(0.f, height)));
    parent_.add_child(&element);
    element.set_horizontal_constraints(0.f, 1.f, padding_, -padding_);
    cursor_y_ += std::max(0.f, height) + gap_;
    if (group_)
        group_->fit_to_content();
}

void zgui::StackLayout::add_row(const std::shared_ptr<zgui::UIElement>& element, float height) {
    if (!element) {
        assert(element && "layout element must not be null");
        return;
    }
    const float width = std::max(0.f, parent_.get_size().x - padding_ * 2.f);
    element->set_position(zgui::Point(padding_, cursor_y_));
    element->set_size(zgui::Point(width, std::max(0.f, height)));
    parent_.add_child(element);
    element->set_horizontal_constraints(0.f, 1.f, padding_, -padding_);
    cursor_y_ += std::max(0.f, height) + gap_;
    if (group_)
        group_->fit_to_content();
}

void zgui::StackLayout::add_two_column_row(const std::shared_ptr<zgui::CheckBox>& check_box,
                                           const std::shared_ptr<zgui::ComboBox>& combo_box,
                                           float left_fraction, float height) {
    if (!check_box || !combo_box) {
        assert(check_box && combo_box && "two-column layout elements must not be null");
        return;
    }
    const float width = std::max(0.f, parent_.get_size().x - padding_ * 2.f - gap_);
    const float left_width = width * std::clamp(left_fraction, 0.f, 1.f);
    const float right_width = width - left_width;
    const float row_height = std::max(0.f, height);
    check_box->set_position(zgui::Point(padding_, cursor_y_));
    check_box->set_size(zgui::Point(left_width, row_height));
    combo_box->set_position(zgui::Point(padding_ + left_width + gap_, cursor_y_));
    combo_box->set_size(zgui::Point(right_width, std::min(15.f, row_height)));
    parent_.add_child(check_box);
    parent_.add_child(combo_box);
    const float fraction = std::clamp(left_fraction, 0.f, 1.f);
    const float left_right_offset = padding_ - 2.f * fraction * padding_ - fraction * gap_;
    check_box->set_horizontal_constraints(0.f, fraction, padding_, left_right_offset);
    combo_box->set_horizontal_constraints(
        fraction, 1.f, padding_ - 2.f * fraction * padding_ + (1.f - fraction) * gap_, -padding_);
    cursor_y_ += row_height + gap_;
    if (group_)
        group_->fit_to_content();
}

void zgui::StackLayout::add_three_column_row(const std::shared_ptr<zgui::ColorButton>& first,
                                             const std::shared_ptr<zgui::ColorButton>& second,
                                             const std::shared_ptr<zgui::ColorButton>& third,
                                             float height) {
    if (!first || !second || !third) {
        assert(first && second && third && "three-column layout elements must not be null");
        return;
    }
    const float width = std::max(0.f, parent_.get_size().x - padding_ * 2.f - gap_ * 2.f);
    const float column_width = width / 3.f;
    const float row_height = std::max(0.f, height);
    const std::shared_ptr<zgui::ColorButton> swatches[] = {first, second, third};
    for (std::size_t i = 0; i < 3; ++i) {
        swatches[i]->set_position(
            zgui::Point(padding_ + static_cast<float>(i) * (column_width + gap_), cursor_y_));
        swatches[i]->set_size(zgui::Point(column_width, row_height));
        parent_.add_child(swatches[i]);
        const float fraction = static_cast<float>(i) / 3.f;
        const float next_fraction = static_cast<float>(i + 1) / 3.f;
        swatches[i]->set_horizontal_constraints(
            fraction, next_fraction,
            padding_ * (1.f - 2.f * fraction) + static_cast<float>(i) * gap_ / 3.f,
            padding_ * (1.f - 2.f * next_fraction) + (static_cast<float>(i) - 2.f) * gap_ / 3.f);
    }
    cursor_y_ += row_height + gap_;
    if (group_)
        group_->fit_to_content();
}

zgui::Window::Window(std::string title) {
    set_title(std::move(title));
}

void zgui::Window::set_minimum_size(float width, float height) {
    min_width_ = std::max(1.f, width);
    min_height_ = std::max(1.f, height);
    set_size(get_size());
}

void zgui::Window::set_minimum_size_from_content() {
    float width = children_.empty() ? 160.f : 1.f;
    float height = children_.empty() ? 100.f : 1.f;
    for (const UIElement* child : children_) {
        const auto* tabs = dynamic_cast<const TabBar*>(child);
        const float child_width = tabs != nullptr && child->content_right() > 0.f
                                      ? child->content_right()
                                      : std::max(child->get_size().x, child->content_right());
        const float child_height = tabs != nullptr
                                       ? child->content_bottom()
                                       : std::max(child->get_size().y, child->content_bottom());
        width = std::max(width, child->get_position().x + child_width + 8.f);
        height = std::max(height, child->get_position().y + child_height +
                                      (status_bar_visible_ ? 14.f : 2.f));
    }
    set_minimum_size(width, height);
}

zgui::Point zgui::Window::get_minimum_size() const {
    return zgui::Point(min_width_, min_height_);
}

void zgui::Window::set_resizable(bool resizable) {
    resizable_ = resizable;
    if (!resizable_)
        resizing_ = false;
}

bool zgui::Window::is_resizable() const {
    return resizable_;
}

void zgui::Window::set_frameless(bool frameless) {
    frame_visible_ = !frameless;
    title_bar_visible_ = !frameless;
    if (frameless) {
        set_resizable(false);
        set_minimum_size(1.f, 1.f);
    }
}

void zgui::Window::set_title_bar_visible(bool visible) {
    title_bar_visible_ = visible;
}

void zgui::Window::set_always_on_top(bool always_on_top) {
    always_on_top_ = always_on_top;
}

bool zgui::Window::is_always_on_top() const {
    return always_on_top_;
}

void zgui::Window::set_visible(bool visible) {
    visible_ = visible;
}

bool zgui::Window::is_visible() const {
    return visible_;
}

void zgui::Window::set_size(const zgui::Point& size) {
    UIElement::set_size(zgui::Point(std::max(min_width_, size.x), std::max(min_height_, size.y)));
}

void zgui::Window::set_status_bar_visible(bool visible) {
    status_bar_visible_ = visible;
}

void zgui::Window::set_status_checkbox(std::string label, bool* value) {
    owned_status_checkbox_.reset();
    status_checkbox_label_ = std::move(label);
    status_checkbox_ = value;
}

void zgui::Window::set_status_checkbox(std::string label, const zgui::Bound<bool>& value) {
    set_status_checkbox(std::move(label), value.get());
    owned_status_checkbox_ = value.owner();
}

void zgui::Window::set_status_indicators(std::vector<zgui::Color> indicators) {
    status_indicators_ = std::move(indicators);
}

bool zgui::Window::handle_mouse_press(zgui::Point position) {
    if (!visible_ || !contains_point(position))
        return false;

    const zgui::Point absolute = get_absolute_position();
    const zgui::Point size = get_size();
    const float status_height = status_bar_visible_ ? 14.f : 0.f;
    if (status_bar_visible_ && status_checkbox_ != nullptr &&
        position.y >= absolute.y + size.y - status_height && position.x >= absolute.x + 8.f &&
        position.x < absolute.x + 20.f) {
        *status_checkbox_ = !*status_checkbox_;
        return true;
    }
    if (resizable_ && position.x >= absolute.x + size.x - 14.f &&
        position.y >= absolute.y + size.y - 14.f) {
        resizing_ = true;
        resize_start_mouse_ = position;
        resize_start_size_ = size;
        return true;
    }
    if (position.y < absolute.y + 20.f) {
        dragging_ = true;
        drag_offset_ = zgui::Point(position.x - absolute.x, position.y - absolute.y);
        return true;
    }
    return UIElement::handle_mouse_press(position);
}

void zgui::Window::draw() {
    if (!visible_)
        return;
    const zgui::Point position = get_absolute_position();
    const zgui::Point size = get_size();
    const float status_height = status_bar_visible_ ? 14.f : 0.f;
    auto& graphics = *zgui::core::graphics;
    const zgui::Theme& theme = zgui::core::active_theme;

    if (frame_visible_)
        graphics.draw_frame(position, size, theme.window_bg);
    if (title_bar_visible_) {
        graphics.fill_rect(position, zgui::Point(size.x, 20.f), theme.window_header);
        const std::string title =
            truncate_with_ellipsis(graphics, title_, std::max(0.f, size.x - 16.f));
        const float title_x = position.x + (size.x - graphics.measure_text_width(title)) / 2.f;
        graphics.draw_text(title, zgui::Point(title_x, position.y + 3.f), theme.text_primary);
    }

    UIElement::draw();

    if (status_bar_visible_) {
        const float status_y = position.y + size.y - status_height;
        graphics.fill_rect(zgui::Point(position.x + 1.f, status_y),
                           zgui::Point(std::max(0.f, size.x - 2.f), status_height),
                           theme.window_header);
        constexpr float indicator_size = 8.f;
        constexpr float indicator_gap = 3.f;
        float indicator_x = position.x + size.x - 5.f - indicator_size - (resizable_ ? 14.f : 0.f);
        for (auto indicator = status_indicators_.rbegin(); indicator != status_indicators_.rend();
             ++indicator) {
            graphics.fill_rect(zgui::Point(indicator_x, status_y + 3.f),
                               zgui::Point(indicator_size, indicator_size), *indicator);
            indicator_x -= indicator_size + indicator_gap;
        }
        if (status_checkbox_ != nullptr) {
            graphics.draw_frame(zgui::Point(position.x + 8.f, status_y + 2.f),
                                zgui::Point(10.f, 10.f), theme.control_bg);
            if (*status_checkbox_)
                graphics.fill_rect(zgui::Point(position.x + 10.f, status_y + 4.f),
                                   zgui::Point(6.f, 6.f), theme.accent);
            const std::string label = truncate_with_ellipsis(
                graphics, status_checkbox_label_, std::max(0.f, indicator_x - position.x - 24.f));
            graphics.draw_text(label, zgui::Point(position.x + 22.f, status_y + 2.f),
                               theme.text_primary);
        }
    }

    if (resizable_) {
        const zgui::Point mouse = zgui::core::input->mouse_position();
        const bool hovered =
            inside_rect(mouse, zgui::Point(position.x + size.x - 14.f, position.y + size.y - 14.f),
                        zgui::Point(14.f, 14.f));
        const zgui::Color grip = hovered || resizing_ ? theme.text_primary : theme.border_inner;
        for (int step = 0; step < 3; ++step) {
            const float offset = 3.f + static_cast<float>(step) * 3.f;
            graphics.draw_line(
                zgui::Point(position.x + size.x - offset - 1.f, position.y + size.y - 2.f),
                zgui::Point(position.x + size.x - 2.f, position.y + size.y - offset - 1.f), grip);
        }
    }
}

void zgui::Window::update() {
    if (!visible_)
        return;
    if (resizing_) {
        if (!zgui::core::input->mouse_button_pressed()) {
            resizing_ = false;
        } else {
            const zgui::Point mouse = zgui::core::input->mouse_position();
            set_size(zgui::Point(resize_start_size_.x + mouse.x - resize_start_mouse_.x,
                                 resize_start_size_.y + mouse.y - resize_start_mouse_.y));
        }
    } else if (dragging_) {
        if (!zgui::core::input->mouse_button_pressed()) {
            dragging_ = false;
        } else {
            const zgui::Point mouse = zgui::core::input->mouse_position();
            set_position(zgui::Point(mouse.x - drag_offset_.x, mouse.y - drag_offset_.y));
        }
    }
    UIElement::update();
}

zgui::Rect zgui::Window::child_clip_rect() const {
    const zgui::Point absolute = get_absolute_position();
    const float status_height = status_bar_visible_ ? 14.f : 0.f;
    const float horizontal_inset = frame_visible_ ? 2.f : 0.f;
    const float title_height = title_bar_visible_ ? 20.f : 0.f;
    return {zgui::Point(absolute.x + horizontal_inset, absolute.y + title_height),
            zgui::Point(std::max(0.f, size_.x - horizontal_inset * 2.f),
                        std::max(0.f, size_.y - title_height - status_height))};
}

zgui::WindowManager::~WindowManager() {
    if (zgui::core::window_manager == this) {
        zgui::core::window_manager = nullptr;
        zgui::core::focused_text_input = nullptr;
    }
}

void zgui::WindowManager::add_window(const std::shared_ptr<zgui::Window>& window) {
    if (!window) {
        assert(window && "window must not be null");
        return;
    }
    const auto topmost = std::find_if(windows_.begin(), windows_.end(), [](const auto& current) {
        return current->is_always_on_top();
    });
    if (window->is_always_on_top())
        windows_.push_back(window);
    else
        windows_.insert(topmost, window);
}

void zgui::WindowManager::remove_window(const std::shared_ptr<zgui::Window>& window) {
    windows_.erase(std::remove(windows_.begin(), windows_.end(), window), windows_.end());
}

void zgui::WindowManager::bring_to_front(const std::shared_ptr<zgui::Window>& window) {
    const auto found = std::find(windows_.begin(), windows_.end(), window);
    if (found == windows_.end())
        return;
    if (std::next(found) == windows_.end())
        return;
    const auto front = *found;
    windows_.erase(found);
    if (front->is_always_on_top()) {
        windows_.push_back(front);
    } else {
        const auto topmost =
            std::find_if(windows_.begin(), windows_.end(),
                         [](const auto& current) { return current->is_always_on_top(); });
        windows_.insert(topmost, front);
    }
}

const std::vector<std::shared_ptr<zgui::Window>>& zgui::WindowManager::windows() const {
    return windows_;
}

void zgui::WindowManager::show_overlay(const std::shared_ptr<zgui::UIElement>& overlay) {
    if (!overlay) {
        assert(overlay && "overlay must not be null");
        return;
    }
    if (std::find(overlays_.begin(), overlays_.end(), overlay) == overlays_.end()) {
        overlay->set_parent(nullptr);
        overlays_.push_back(overlay);
    }
}

void zgui::WindowManager::dismiss_overlay(const zgui::UIElement* overlay) {
    if (const auto tooltip = tooltip_overlay_.lock(); tooltip.get() == overlay) {
        tooltip_overlay_.reset();
        tooltip_owner_ = nullptr;
    }
    overlays_.erase(std::remove_if(overlays_.begin(), overlays_.end(),
                                   [overlay](const auto& item) { return item.get() == overlay; }),
                    overlays_.end());
}

bool zgui::WindowManager::show_tooltip(const zgui::UIElement* owner,
                                       const std::shared_ptr<zgui::UIElement>& tooltip) {
    if (owner == nullptr || !tooltip) {
        assert(owner != nullptr && tooltip && "tooltip owner and overlay must not be null");
        return false;
    }
    if (tooltip_owner_ != nullptr && tooltip_owner_ != owner)
        return false;
    tooltip_owner_ = owner;
    tooltip_overlay_ = tooltip;
    show_overlay(tooltip);
    return true;
}

void zgui::WindowManager::dismiss_tooltip(const zgui::UIElement* owner) {
    if (owner == nullptr || tooltip_owner_ != owner)
        return;
    const auto tooltip = tooltip_overlay_.lock();
    tooltip_owner_ = nullptr;
    tooltip_overlay_.reset();
    if (tooltip)
        dismiss_overlay(tooltip.get());
}

bool zgui::WindowManager::has_tooltip(const zgui::UIElement* owner) const {
    return owner != nullptr && tooltip_owner_ == owner && !tooltip_overlay_.expired();
}

bool zgui::WindowManager::has_overlays() const {
    return !overlays_.empty();
}

bool zgui::WindowManager::validate_layout(std::string* error) const {
    for (const auto& window : windows_) {
        if (!validate_layout(*window, error))
            return false;
    }
    return true;
}

bool zgui::WindowManager::validate_layout(const zgui::Window& window, std::string* error) const {
    const auto fail = [error](const std::string& message) {
        if (error != nullptr)
            *error = message;
        return false;
    };
    const auto contains_rect = [](const zgui::Rect& outer, const zgui::Rect& inner) {
        constexpr float epsilon = 0.01f;
        return inner.position.x >= outer.position.x - epsilon &&
               inner.position.y >= outer.position.y - epsilon &&
               inner.position.x + inner.size.x <= outer.position.x + outer.size.x + epsilon &&
               inner.position.y + inner.size.y <= outer.position.y + outer.size.y + epsilon;
    };
    const auto overlaps = [](const zgui::Rect& left, const zgui::Rect& right) {
        return left.position.x < right.position.x + right.size.x &&
               right.position.x < left.position.x + left.size.x &&
               left.position.y < right.position.y + right.size.y &&
               right.position.y < left.position.y + left.size.y;
    };

    std::function<bool(const UIElement&)> visit;
    visit = [&](const UIElement& parent) {
        std::vector<const UIElement*> children;
        children.reserve(parent.children_.size());
        for (const UIElement* child : parent.children_)
            children.push_back(child);
        const auto* tabs = dynamic_cast<const TabBar*>(&parent);
        if (tabs != nullptr && !tabs->tabs_.empty()) {
            children.clear();
            for (const TabBar::Tab& tab : tabs->tabs_) {
                const Rect viewport = tabs->child_clip_rect();
                if (viewport.size.x <= 0.f || viewport.size.y <= 0.f ||
                    tab.page->children_.empty()) {
                    assert(viewport.size.x > 0.f && viewport.size.y > 0.f &&
                           !tab.page->children_.empty());
                    return fail("tab '" + tab.title + "' has an empty content page");
                }
                if (!visit(*tab.page))
                    return false;
            }
        }
        if (const auto* group = dynamic_cast<const GroupBox*>(&parent)) {
            if (group->sub_tabs_) {
                const zgui::Rect sub_tabs_rect{group->sub_tabs_->get_absolute_position(),
                                               group->sub_tabs_->get_size()};
                if (!contains_rect(group->child_clip_rect(), sub_tabs_rect))
                    return fail("subtab strip exceeds group '" + group->get_title() +
                                "' content rect");
                if (!visit(*group->sub_tabs_))
                    return false;
            }
        }

        const zgui::Rect parent_clip = parent.child_clip_rect();
        const bool scroll_container = dynamic_cast<const ScrollArea*>(&parent) != nullptr;
        for (std::size_t index = 0; index < children.size(); ++index) {
            const UIElement& child = *children[index];
            const zgui::Rect child_rect{child.get_absolute_position(), child.get_size()};
            if (!std::isfinite(child_rect.size.x) || !std::isfinite(child_rect.size.y) ||
                child_rect.size.x < 0.f || child_rect.size.y < 0.f) {
                return fail("child '" + child.get_title() + "' has an invalid size in parent '" +
                            parent.get_title() + "'");
            }
            if (!scroll_container && !contains_rect(parent_clip, child_rect)) {
                return fail(
                    "child '" + child.get_title() + "' exceeds parent '" + parent.get_title() +
                    "' content rect [child=" + std::to_string(child_rect.position.x) + "," +
                    std::to_string(child_rect.position.y) + " " +
                    std::to_string(child_rect.size.x) + "x" + std::to_string(child_rect.size.y) +
                    "; clip=" + std::to_string(parent_clip.position.x) + "," +
                    std::to_string(parent_clip.position.y) + " " +
                    std::to_string(parent_clip.size.x) + "x" + std::to_string(parent_clip.size.y) +
                    "]");
            }
            for (std::size_t other = index + 1; other < children.size(); ++other) {
                const zgui::Rect other_rect{children[other]->get_absolute_position(),
                                            children[other]->get_size()};
                if (overlaps(child_rect, other_rect)) {
                    return fail("siblings '" + child.get_title() + "' and '" +
                                children[other]->get_title() + "' overlap in '" +
                                parent.get_title() + "' [" + std::to_string(child_rect.position.x) +
                                "," + std::to_string(child_rect.position.y) + " " +
                                std::to_string(child_rect.size.x) + "x" +
                                std::to_string(child_rect.size.y) + " vs " +
                                std::to_string(other_rect.position.x) + "," +
                                std::to_string(other_rect.position.y) + " " +
                                std::to_string(other_rect.size.x) + "x" +
                                std::to_string(other_rect.size.y) + "]");
                }
            }
            if (!visit(child))
                return false;
        }
        return true;
    };

    if (!visit(window))
        return false;
    if (error != nullptr)
        error->clear();
    return true;
}

void zgui::WindowManager::draw() {
    if (!zgui::core::require_initialized())
        return;
    zgui::core::window_manager = this;
    std::stable_partition(windows_.begin(), windows_.end(),
                          [](const auto& window) { return !window->is_always_on_top(); });
    const auto windows = windows_;
    const auto overlays = overlays_;
    for (const auto& window : windows)
        if (window->is_visible())
            window->draw();
    for (const auto& overlay : overlays)
        overlay->draw();
    assert(zgui::core::graphics->clip_depth() == 0 &&
           "Graphics clip stack must be balanced at the end of a frame");
}

void zgui::WindowManager::update() {
    if (!zgui::core::require_initialized())
        return;
    zgui::core::window_manager = this;
    std::stable_partition(windows_.begin(), windows_.end(),
                          [](const auto& window) { return !window->is_always_on_top(); });
    const bool mouse_pressed = zgui::core::input->mouse_button_pressed();
    const zgui::Point mouse = zgui::core::input->mouse_position();
    if (zgui::core::input->escape_pressed() && !overlays_.empty()) {
        dismiss_overlay(overlays_.back().get());
        zgui::core::focused_text_input = nullptr;
    } else if (mouse_pressed && !mouse_was_pressed_) {
        if (!overlays_.empty()) {
            zgui::core::focused_text_input = nullptr;
            const auto top_overlay = overlays_.back();
            if (top_overlay->contains_point(mouse)) {
                top_overlay->handle_mouse_press(mouse);
            } else {
                dismiss_overlay(top_overlay.get());
            }
        } else {
            zgui::core::focused_text_input = nullptr;
            const auto windows = windows_;
            for (auto window = windows.rbegin(); window != windows.rend(); ++window) {
                if ((*window)->is_visible() && (*window)->contains_point(mouse)) {
                    const auto target = *window;
                    bring_to_front(target);
                    target->handle_mouse_press(mouse);
                    break;
                }
            }
        }
    }

    const auto windows = windows_;
    const auto overlays = overlays_;
    for (const auto& window : windows)
        if (window->is_visible())
            window->update();
    for (const auto& overlay : overlays)
        overlay->update();
    mouse_was_pressed_ = mouse_pressed;
}
