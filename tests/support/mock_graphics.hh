// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#ifndef ZGUI_MOCK_GRAPHICS_HH
#define ZGUI_MOCK_GRAPHICS_HH

#include "../../zgui.hh"

#include <optional>
#include <string>
#include <vector>

namespace zgui_test {

    enum class GraphicsCommandType { Geometry, Text, SetClip, ResetClip, Draw, Reset };

    struct GraphicsCommand {
        GraphicsCommandType type = GraphicsCommandType::Draw;
        zgui::VertexTopology topology = zgui::VertexTopology::TRIANGLE_LIST;
        std::vector<zgui::Vertex> vertices;
        std::string text;
        zgui::Point position{};
        zgui::Color color{};
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        std::size_t clip_depth = 0;
    };

    class MockGraphics final : public zgui::Graphics {
    public:
        void draw() override;
        void flush() override;
        void reset() override;
        void set_clip_rect(int x, int y, int width, int height) override;
        void reset_clip_rect() override;
        void draw_text(const std::string& text, zgui::Point position, zgui::Color color) override;
        float measure_text_width(const std::string& text) const override;
        zgui::Point viewport_size() const override;

        void set_viewport_size(zgui::Point size);
        void set_character_width(float width);
        void clear_commands();
        const std::vector<GraphicsCommand>& commands() const;
        std::vector<std::string> texts() const;
        std::size_t count_fill_rects() const;
        std::vector<GraphicsCommand> commands_between_clips() const;
        std::optional<GraphicsCommand> last_clip() const;
        std::size_t clip_depth() const;

    protected:
        void append_geometry(const std::vector<zgui::Vertex>& vertices, std::size_t count) override;
        void set_topology(zgui::VertexTopology topology) override;

    private:
        std::vector<GraphicsCommand> commands_;
        std::optional<GraphicsCommand> last_clip_;
        zgui::Point viewport_{800.f, 600.f};
        float character_width_ = 8.f;
    };

} // namespace zgui_test

#endif // ZGUI_MOCK_GRAPHICS_HH
