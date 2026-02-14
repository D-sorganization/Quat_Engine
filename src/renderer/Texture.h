#pragma once
/**
 * @file Texture.h
 * @brief OpenGL texture wrapper with procedural texture generators.
 *
 * Supports:
 *   - Procedural generation: checkerboard, gradient, noise, brick
 *   - Raw pixel upload from memory
 *   - Automatic mipmapping
 */

#include "GLLoader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace qe {
namespace renderer {

class Texture {
public:
    GLuint id = 0;
    int width = 0;
    int height = 0;

    Texture() = default;

    /** Create texture from raw RGBA pixel data. */
    void upload(const std::vector<uint8_t>& pixels, int w, int h) {
        width = w;
        height = h;

        gl::glGenTextures(1, &id);
        gl::glBindTexture(GL_TEXTURE_2D, id);

        gl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
        gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        gl::glGenerateMipmap(GL_TEXTURE_2D);
        gl::glBindTexture(GL_TEXTURE_2D, 0);
    }

    /** Bind to a texture unit (0-based). */
    void bind(int unit = 0) const {
        gl::glActiveTexture(GL_TEXTURE0 + unit);
        gl::glBindTexture(GL_TEXTURE_2D, id);
    }

    void destroy() {
        if (id) { gl::glDeleteTextures(1, &id); id = 0; }
    }

    // ── Procedural Generators ──────────────────────────────────────────

    /** Checkerboard pattern. */
    static Texture create_checkerboard(int size = 256, int squares = 8,
                                        uint8_t r1 = 200, uint8_t g1 = 200,
                                        uint8_t b1 = 200,
                                        uint8_t r2 = 60, uint8_t g2 = 60,
                                        uint8_t b2 = 60) {
        Texture tex;
        std::vector<uint8_t> pixels(size * size * 4);
        int sq = size / squares;

        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                bool white = ((x / sq) + (y / sq)) % 2 == 0;
                int idx = (y * size + x) * 4;
                pixels[idx + 0] = white ? r1 : r2;
                pixels[idx + 1] = white ? g1 : g2;
                pixels[idx + 2] = white ? b1 : b2;
                pixels[idx + 3] = 255;
            }
        }

        tex.upload(pixels, size, size);
        return tex;
    }

    /** Brick wall pattern. */
    static Texture create_bricks(int size = 256) {
        Texture tex;
        std::vector<uint8_t> pixels(size * size * 4);

        int brick_w = size / 8;
        int brick_h = size / 16;
        int mortar = 2;

        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                int row = y / brick_h;
                int offset = (row % 2) * (brick_w / 2);
                int bx = (x + offset) % brick_w;
                int by = y % brick_h;

                bool is_mortar = (bx < mortar || by < mortar);

                int idx = (y * size + x) * 4;
                if (is_mortar) {
                    pixels[idx + 0] = 140;
                    pixels[idx + 1] = 135;
                    pixels[idx + 2] = 120;
                } else {
                    // Brick color with slight variation
                    float noise = pseudo_noise(x * 0.1f, y * 0.1f);
                    uint8_t base_r = static_cast<uint8_t>(150 + noise * 40);
                    uint8_t base_g = static_cast<uint8_t>(70 + noise * 20);
                    uint8_t base_b = static_cast<uint8_t>(50 + noise * 15);
                    pixels[idx + 0] = base_r;
                    pixels[idx + 1] = base_g;
                    pixels[idx + 2] = base_b;
                }
                pixels[idx + 3] = 255;
            }
        }

        tex.upload(pixels, size, size);
        return tex;
    }

    /** Metal/concrete floor pattern. */
    static Texture create_floor(int size = 256) {
        Texture tex;
        std::vector<uint8_t> pixels(size * size * 4);

        int tile = size / 4;
        int gap = 2;

        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                int tx = x % tile;
                int ty = y % tile;
                bool is_gap = (tx < gap || ty < gap);

                float noise = pseudo_noise(x * 0.05f, y * 0.05f);
                int idx = (y * size + x) * 4;

                if (is_gap) {
                    pixels[idx + 0] = 30;
                    pixels[idx + 1] = 30;
                    pixels[idx + 2] = 35;
                } else {
                    uint8_t base = static_cast<uint8_t>(80 + noise * 30);
                    pixels[idx + 0] = base;
                    pixels[idx + 1] = static_cast<uint8_t>(base + 5);
                    pixels[idx + 2] = static_cast<uint8_t>(base + 10);
                }
                pixels[idx + 3] = 255;
            }
        }

        tex.upload(pixels, size, size);
        return tex;
    }

    /** Gradient sky texture. */
    static Texture create_gradient(int size = 256,
                                    uint8_t r_top = 5, uint8_t g_top = 5,
                                    uint8_t b_top = 30,
                                    uint8_t r_bot = 40, uint8_t g_bot = 20,
                                    uint8_t b_bot = 60) {
        Texture tex;
        std::vector<uint8_t> pixels(size * size * 4);

        for (int y = 0; y < size; ++y) {
            float t = static_cast<float>(y) / (size - 1);
            uint8_t r = static_cast<uint8_t>(r_top + t * (r_bot - r_top));
            uint8_t g = static_cast<uint8_t>(g_top + t * (g_bot - g_top));
            uint8_t b = static_cast<uint8_t>(b_top + t * (b_bot - b_top));

            for (int x = 0; x < size; ++x) {
                int idx = (y * size + x) * 4;
                pixels[idx + 0] = r;
                pixels[idx + 1] = g;
                pixels[idx + 2] = b;
                pixels[idx + 3] = 255;
            }
        }

        tex.upload(pixels, size, size);
        return tex;
    }

    /** Solid color 1x1 texture. */
    static Texture create_solid(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        Texture tex;
        std::vector<uint8_t> pixels = {r, g, b, a};
        tex.upload(pixels, 1, 1);
        return tex;
    }

private:
    /** Simple pseudo-random noise for texture variation (0..1). */
    static float pseudo_noise(float x, float y) {
        float val = std::sin(x * 12.9898f + y * 78.233f) * 43758.5453f;
        return val - std::floor(val);
    }
};

} // namespace renderer
} // namespace qe
