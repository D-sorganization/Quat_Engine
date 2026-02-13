#pragma once
/**
 * @file Mesh.h
 * @brief GPU mesh data — VAO/VBO/EBO wrapper for vertex data upload and drawing.
 *
 * Vertex layout: position (3f) + normal (3f) + color (3f) = 9 floats per vertex.
 */

#include "GLLoader.h"

#include <vector>

namespace qe {
namespace renderer {

/** Per-vertex data sent to the GPU. */
struct Vertex {
    float position[3];
    float normal[3];
    float color[3];
};

class Mesh {
public:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei index_count = 0;

    Mesh() = default;

    /** Upload vertex + index data to the GPU. */
    void upload(const std::vector<Vertex>& vertices,
                const std::vector<unsigned int>& indices) {
        index_count = static_cast<GLsizei>(indices.size());

        gl::glGenVertexArrays(1, &vao);
        gl::glGenBuffers(1, &vbo);
        gl::glGenBuffers(1, &ebo);

        gl::glBindVertexArray(vao);

        // Vertex buffer
        gl::glBindBuffer(GL_ARRAY_BUFFER, vbo);
        gl::glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                         vertices.data(), GL_STATIC_DRAW);

        // Index buffer
        gl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        gl::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                         indices.data(), GL_STATIC_DRAW);

        // Position attribute (location = 0)
        gl::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                                  sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, position)));
        gl::glEnableVertexAttribArray(0);

        // Normal attribute (location = 1)
        gl::glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                                  sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, normal)));
        gl::glEnableVertexAttribArray(1);

        // Color attribute (location = 2)
        gl::glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                                  sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, color)));
        gl::glEnableVertexAttribArray(2);

        gl::glBindVertexArray(0);
    }

    /** Draw the mesh using its index buffer. */
    void draw() const {
        gl::glBindVertexArray(vao);
        gl::glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);
        gl::glBindVertexArray(0);
    }

    /** Release GPU resources. */
    void destroy() {
        if (ebo) { gl::glDeleteBuffers(1, &ebo); ebo = 0; }
        if (vbo) { gl::glDeleteBuffers(1, &vbo); vbo = 0; }
        if (vao) { gl::glDeleteVertexArrays(1, &vao); vao = 0; }
    }

    // --- Primitive Generators ---

    /** Create a unit cube centered at origin with per-face normals and colors. */
    static Mesh create_cube() {
        Mesh mesh;

        // 6 faces × 4 vertices = 24 vertices
        // Each face has a unique normal and color
        std::vector<Vertex> vertices = {
            // Front face (Z+) — Blue
            {{-0.5f, -0.5f,  0.5f}, { 0, 0, 1}, {0.2f, 0.4f, 0.9f}},
            {{ 0.5f, -0.5f,  0.5f}, { 0, 0, 1}, {0.2f, 0.4f, 0.9f}},
            {{ 0.5f,  0.5f,  0.5f}, { 0, 0, 1}, {0.3f, 0.5f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, { 0, 0, 1}, {0.3f, 0.5f, 1.0f}},

            // Back face (Z-) — Teal
            {{ 0.5f, -0.5f, -0.5f}, { 0, 0,-1}, {0.0f, 0.7f, 0.7f}},
            {{-0.5f, -0.5f, -0.5f}, { 0, 0,-1}, {0.0f, 0.7f, 0.7f}},
            {{-0.5f,  0.5f, -0.5f}, { 0, 0,-1}, {0.1f, 0.8f, 0.8f}},
            {{ 0.5f,  0.5f, -0.5f}, { 0, 0,-1}, {0.1f, 0.8f, 0.8f}},

            // Top face (Y+) — Green
            {{-0.5f,  0.5f,  0.5f}, { 0, 1, 0}, {0.2f, 0.9f, 0.3f}},
            {{ 0.5f,  0.5f,  0.5f}, { 0, 1, 0}, {0.2f, 0.9f, 0.3f}},
            {{ 0.5f,  0.5f, -0.5f}, { 0, 1, 0}, {0.3f, 1.0f, 0.4f}},
            {{-0.5f,  0.5f, -0.5f}, { 0, 1, 0}, {0.3f, 1.0f, 0.4f}},

            // Bottom face (Y-) — Orange
            {{-0.5f, -0.5f, -0.5f}, { 0,-1, 0}, {0.9f, 0.5f, 0.1f}},
            {{ 0.5f, -0.5f, -0.5f}, { 0,-1, 0}, {0.9f, 0.5f, 0.1f}},
            {{ 0.5f, -0.5f,  0.5f}, { 0,-1, 0}, {1.0f, 0.6f, 0.2f}},
            {{-0.5f, -0.5f,  0.5f}, { 0,-1, 0}, {1.0f, 0.6f, 0.2f}},

            // Right face (X+) — Red
            {{ 0.5f, -0.5f,  0.5f}, { 1, 0, 0}, {0.9f, 0.2f, 0.2f}},
            {{ 0.5f, -0.5f, -0.5f}, { 1, 0, 0}, {0.9f, 0.2f, 0.2f}},
            {{ 0.5f,  0.5f, -0.5f}, { 1, 0, 0}, {1.0f, 0.3f, 0.3f}},
            {{ 0.5f,  0.5f,  0.5f}, { 1, 0, 0}, {1.0f, 0.3f, 0.3f}},

            // Left face (X-) — Purple
            {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0.6f, 0.2f, 0.9f}},
            {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {0.6f, 0.2f, 0.9f}},
            {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {0.7f, 0.3f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0.7f, 0.3f, 1.0f}},
        };

        std::vector<unsigned int> indices;
        for (unsigned int face = 0; face < 6; ++face) {
            unsigned int base = face * 4;
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        }

        mesh.upload(vertices, indices);
        return mesh;
    }

    /** Create a grid floor on the XZ plane. */
    static Mesh create_grid(int half_size = 10, float spacing = 1.0f) {
        Mesh mesh;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        unsigned int idx = 0;

        float extent = half_size * spacing;

        for (int i = -half_size; i <= half_size; ++i) {
            float pos = i * spacing;
            float brightness = (i == 0) ? 0.6f : 0.25f;

            // Line along X
            vertices.push_back({{pos, 0.0f, -extent}, {0, 1, 0},
                                {brightness, brightness, brightness}});
            vertices.push_back({{pos, 0.0f,  extent}, {0, 1, 0},
                                {brightness, brightness, brightness}});
            indices.push_back(idx++);
            indices.push_back(idx++);

            // Line along Z
            vertices.push_back({{-extent, 0.0f, pos}, {0, 1, 0},
                                {brightness, brightness, brightness}});
            vertices.push_back({{ extent, 0.0f, pos}, {0, 1, 0},
                                {brightness, brightness, brightness}});
            indices.push_back(idx++);
            indices.push_back(idx++);
        }

        mesh.index_count = static_cast<GLsizei>(indices.size());

        gl::glGenVertexArrays(1, &mesh.vao);
        gl::glGenBuffers(1, &mesh.vbo);
        gl::glGenBuffers(1, &mesh.ebo);

        gl::glBindVertexArray(mesh.vao);

        gl::glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
        gl::glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                         vertices.data(), GL_STATIC_DRAW);

        gl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
        gl::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                         indices.data(), GL_STATIC_DRAW);

        gl::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, position)));
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, normal)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, color)));
        gl::glEnableVertexAttribArray(2);

        gl::glBindVertexArray(0);
        return mesh;
    }
};

} // namespace renderer
} // namespace qe
