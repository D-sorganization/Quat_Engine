#pragma once
/**
 * @file HUD.h
 * @brief Heads-Up Display rendering: crosshair, health bars, ammo, combo text.
 *
 * Design by Contract:
 *   - Precondition: OpenGL context must be active
 *   - Invariant: all screen positions are in NDC [-1, 1]
 *   - Postcondition: draw methods restore GL state they modify
 *
 * Uses simple line/quad primitives — no font rendering required.
 * All HUD elements are built from colored vertices.
 */

#include "GLLoader.h"
#include "Mesh.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace qe {
namespace renderer {

/**
 * Bar display element (health bar, ammo bar, cooldown indicator).
 */
struct HUDBar {
    float x, y;          // NDC position (bottom-left of bar)
    float width, height;  // NDC dimensions
    float fill;           // 0.0 to 1.0
    float r, g, b;        // Bar fill color
    float bg_r = 0.1f, bg_g = 0.1f, bg_b = 0.1f;  // Background color
};

/**
 * Indicator element (combo flash, wave number, score popup).
 */
struct HUDIndicator {
    float x, y;
    float size;
    float r, g, b;
    float alpha;
    float timer;          // Counts down, used for fade-out
};

class HUD {
public:
    HUD() = default;

    /**
     * Create the crosshair mesh (called once at init).
     * Enhanced crosshair with dot + gap + lines.
     */
    void init_crosshair() {
        std::vector<Vertex> v;
        std::vector<unsigned int> idx;
        unsigned int i = 0;

        float s = 0.025f;   // Line length
        float g = 0.006f;   // Gap from center
        float d = 0.003f;   // Dot half-size
        float c = 0.9f;

        // Center dot (small quad)
        v.push_back({{-d, -d, 0}, {0,0,1}, {1, 1, 1}, {0,0}});
        v.push_back({{ d, -d, 0}, {0,0,1}, {1, 1, 1}, {0,0}});
        v.push_back({{ d,  d, 0}, {0,0,1}, {1, 1, 1}, {0,0}});
        v.push_back({{-d,  d, 0}, {0,0,1}, {1, 1, 1}, {0,0}});
        idx.push_back(i); idx.push_back(i+1); idx.push_back(i+2);
        idx.push_back(i); idx.push_back(i+2); idx.push_back(i+3);
        i += 4;

        // Crosshair lines (4 lines with gap)
        auto add_line = [&](float x1, float y1, float x2, float y2) {
            v.push_back({{x1, y1, 0}, {0,0,1}, {c, 1, c}, {0,0}});
            v.push_back({{x2, y2, 0}, {0,0,1}, {c, 1, c}, {0,0}});
            idx.push_back(i++);
            idx.push_back(i++);
        };

        add_line(-s, 0, -g, 0);   // Left
        add_line( g, 0,  s, 0);   // Right
        add_line(0,  g, 0,  s);   // Top
        add_line(0, -s, 0, -g);   // Bottom

        crosshair_.upload(v, idx);
        crosshair_tri_count_ = 6;  // 2 triangles for dot
        crosshair_line_start_ = 12; // 12 indices for dot (wait, 6 for dot)
        // Actually: 6 indices for dot triangles, then 8 for lines
        crosshair_line_count_ = 8;
    }

    /**
     * Draw the crosshair. Uses GL_TRIANGLES for dot, GL_LINES for arms.
     */
    void draw_crosshair() const {
        gl::glBindVertexArray(crosshair_.vao);
        // Draw dot as triangles
        gl::glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        // Draw lines (starting at index 6)
        gl::glDrawElements(GL_LINES, 8, GL_UNSIGNED_INT,
                          reinterpret_cast<void*>(6 * sizeof(unsigned int)));
        gl::glBindVertexArray(0);
    }

    /**
     * Build and draw a filled bar (health, ammo, etc.).
     * Creates geometry on the fly (simple quad — cheap at HUD scale).
     */
    void draw_bar(const HUDBar& bar) const {
        std::vector<Vertex> v;
        std::vector<unsigned int> idx;

        // Background quad
        float x = bar.x, y = bar.y;
        float w = bar.width, h = bar.height;
        v.push_back({{x,   y,   0}, {0,0,1}, {bar.bg_r, bar.bg_g, bar.bg_b}, {0,0}});
        v.push_back({{x+w, y,   0}, {0,0,1}, {bar.bg_r, bar.bg_g, bar.bg_b}, {0,0}});
        v.push_back({{x+w, y+h, 0}, {0,0,1}, {bar.bg_r, bar.bg_g, bar.bg_b}, {0,0}});
        v.push_back({{x,   y+h, 0}, {0,0,1}, {bar.bg_r, bar.bg_g, bar.bg_b}, {0,0}});
        idx = {0, 1, 2, 0, 2, 3};

        // Fill quad
        float fw = w * std::clamp(bar.fill, 0.0f, 1.0f);
        if (fw > 0.001f) {
            v.push_back({{x,    y,   0}, {0,0,1}, {bar.r, bar.g, bar.b}, {0,0}});
            v.push_back({{x+fw, y,   0}, {0,0,1}, {bar.r, bar.g, bar.b}, {0,0}});
            v.push_back({{x+fw, y+h, 0}, {0,0,1}, {bar.r, bar.g, bar.b}, {0,0}});
            v.push_back({{x,    y+h, 0}, {0,0,1}, {bar.r, bar.g, bar.b}, {0,0}});
            idx.push_back(4); idx.push_back(5); idx.push_back(6);
            idx.push_back(4); idx.push_back(6); idx.push_back(7);
        }

        // Upload and draw
        Mesh m;
        m.upload(v, idx);
        m.draw();
        m.destroy();
    }

    /**
     * Draw a simple square indicator at NDC position.
     */
    void draw_indicator(float x, float y, float size,
                        float r, float g, float b) const {
        float hs = size * 0.5f;
        std::vector<Vertex> v = {
            {{x-hs, y-hs, 0}, {0,0,1}, {r, g, b}, {0,0}},
            {{x+hs, y-hs, 0}, {0,0,1}, {r, g, b}, {0,0}},
            {{x+hs, y+hs, 0}, {0,0,1}, {r, g, b}, {0,0}},
            {{x-hs, y+hs, 0}, {0,0,1}, {r, g, b}, {0,0}},
        };
        std::vector<unsigned int> idx = {0, 1, 2, 0, 2, 3};

        Mesh m;
        m.upload(v, idx);
        m.draw();
        m.destroy();
    }

    /**
     * Draw weapon indicators (dots showing which weapon is selected).
     */
    void draw_weapon_slots(int current, int total) const {
        float start_x = -0.15f;
        float spacing = 0.06f;
        float y = -0.92f;
        float size_active = 0.025f;
        float size_inactive = 0.015f;

        for (int i = 0; i < total && i < 5; ++i) {
            float x = start_x + i * spacing;
            if (i == current) {
                draw_indicator(x, y, size_active, 1.0f, 0.9f, 0.3f);
            } else {
                draw_indicator(x, y, size_inactive, 0.4f, 0.4f, 0.5f);
            }
        }
    }

    /**
     * Draw combo indicator (flashing squares for streak).
     */
    void draw_combo_indicator(int streak, float time) const {
        if (streak < 2) return;

        float pulse = 0.5f + 0.5f * std::sin(time * 6.0f);
        float x = 0.7f;
        float y = 0.3f;
        int bars = std::min(streak, 20);

        // Draw streak bars
        for (int i = 0; i < bars; ++i) {
            float bx = x;
            float by = y - i * 0.03f;
            float intensity = 0.5f + pulse * 0.5f;

            float r = (streak >= 20) ? 1.0f : (streak >= 10) ? 1.0f : (streak >= 5) ? 1.0f : 0.8f;
            float g = (streak >= 20) ? 0.2f : (streak >= 10) ? 0.6f : (streak >= 5) ? 0.9f : 0.8f;
            float b = (streak >= 20) ? 0.2f : (streak >= 10) ? 0.2f : (streak >= 5) ? 0.2f : 0.2f;

            draw_indicator(bx, by, 0.02f, r * intensity, g * intensity, b * intensity);
        }
    }

    void destroy() {
        crosshair_.destroy();
    }

private:
    Mesh crosshair_;
    int crosshair_tri_count_ = 0;
    int crosshair_line_start_ = 0;
    int crosshair_line_count_ = 0;
};

} // namespace renderer
} // namespace qe
