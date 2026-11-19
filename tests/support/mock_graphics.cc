// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "mock_graphics.hh"

#include <algorithm>

void zgui_test::MockGraphics::draw() {
    GraphicsCommand command;
    command.type = GraphicsCommandType::Draw;
    command.clip_depth = zgui::Graphics::clip_depth();
    commands_.push_back(std::move(command));
}

void zgui_test::MockGraphics::flush() {
    draw();
}

void zgui_test::MockGraphics::reset() {
    GraphicsCommand command;
    command.type = GraphicsCommandType::Reset;
    commands_.push_back(std::move(command));
}

void zgui_test::MockGraphics::set_clip_rect(int x, int y, int width, int height) {
    GraphicsCommand command;
    command.type = GraphicsCommandType::SetClip;
    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;
    command.clip_depth = zgui::Graphics::clip_depth();
    commands_.push_back(command);
    last_clip_ = command;
}

void zgui_test::MockGraphics::reset_clip_rect() {
    GraphicsCommand command;
    command.type = GraphicsCommandType::ResetClip;
    commands_.push_back(std::move(command));
    last_clip_.reset();
}

void zgui_test::MockGraphics::draw_text(const std::string& text, zgui::Point position,
                                        zgui::Color color) {
    GraphicsCommand command;
    command.type = GraphicsCommandType::Text;
    command.text = text;
    command.position = position;
    command.color = color;
    command.clip_depth = zgui::Graphics::clip_depth();
    commands_.push_back(std::move(command));
}

float zgui_test::MockGraphics::measure_text_width(const std::string& text) const {
    return static_cast<float>(text.size()) * character_width_;
}

zgui::Point zgui_test::MockGraphics::viewport_size() const {
    return viewport_;
}

void zgui_test::MockGraphics::set_viewport_size(zgui::Point size) {
    viewport_ = size;
}

void zgui_test::MockGraphics::set_character_width(float width) {
    character_width_ = width;
}

void zgui_test::MockGraphics::clear_commands() {
    commands_.clear();
}

const std::vector<zgui_test::GraphicsCommand>& zgui_test::MockGraphics::commands() const {
    return commands_;
}

std::vector<std::string> zgui_test::MockGraphics::texts() const {
    std::vector<std::string> result;
    for (const GraphicsCommand& command : commands_) {
        if (command.type == GraphicsCommandType::Text)
            result.push_back(command.text);
    }
    return result;
}

std::size_t zgui_test::MockGraphics::count_fill_rects() const {
    return static_cast<std::size_t>(
        std::count_if(commands_.begin(), commands_.end(), [](const GraphicsCommand& command) {
            return command.type == GraphicsCommandType::Geometry &&
                   command.topology == zgui::VertexTopology::TRIANGLE_LIST &&
                   command.vertices.size() == 6;
        }));
}

std::vector<zgui_test::GraphicsCommand> zgui_test::MockGraphics::commands_between_clips() const {
    std::vector<GraphicsCommand> result;
    for (const GraphicsCommand& command : commands_) {
        if (command.type != GraphicsCommandType::SetClip &&
            command.type != GraphicsCommandType::ResetClip && command.clip_depth != 0) {
            result.push_back(command);
        }
    }
    return result;
}

std::optional<zgui_test::GraphicsCommand> zgui_test::MockGraphics::last_clip() const {
    return last_clip_;
}

std::size_t zgui_test::MockGraphics::clip_depth() const {
    return zgui::Graphics::clip_depth();
}

void zgui_test::MockGraphics::append_geometry(const std::vector<zgui::Vertex>& vertices,
                                              std::size_t count) {
    GraphicsCommand command;
    command.type = GraphicsCommandType::Geometry;
    command.topology = vertex_topology_;
    command.clip_depth = zgui::Graphics::clip_depth();
    const std::size_t end = std::min(count, vertices.size());
    command.vertices.assign(vertices.begin(), vertices.begin() + static_cast<std::ptrdiff_t>(end));
    commands_.push_back(std::move(command));
}

void zgui_test::MockGraphics::set_topology(zgui::VertexTopology topology) {
    vertex_topology_ = topology;
}
