#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file Mesh.h
 * @brief GPU mesh data — VAO/VBO/EBO wrapper for vertex data upload and drawing.
 *
 * Vertex layout: position (3f) + normal (3f) + color (3f) + uv (2f) = 11 floats.
 *
 * Procedural shape generators (create_cube, create_sphere, etc.) have been
 * extracted to MeshPrimitives.h (issue #117 — split oversized modules).
 * Include MeshPrimitives.h when you need those factory functions.
 */

#include "GLLoader.h"

#include <cmath>
#include <vector>

namespace qe {
namespace renderer {

/** Per-vertex data sent to the GPU. */
struct Vertex {
    float position[3] = {0, 0, 0};
    float normal[3]   = {0, 1, 0};
    float color[3]    = {1, 1, 1};
    float uv[2]       = {0, 0};
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

        gl::glBindBuffer(GL_ARRAY_BUFFER, vbo);
        gl::glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                         vertices.data(), GL_STATIC_DRAW);

        gl::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        gl::glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                         indices.data(), GL_STATIC_DRAW);

        // Position (location = 0)
        gl::glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, position)));
        gl::glEnableVertexAttribArray(0);

        // Normal (location = 1)
        gl::glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, normal)));
        gl::glEnableVertexAttribArray(1);

        // Color (location = 2)
        gl::glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, color)));
        gl::glEnableVertexAttribArray(2);

        // UV (location = 3)
        gl::glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, uv)));
        gl::glEnableVertexAttribArray(3);

        gl::glBindVertexArray(0);
    }

    void draw() const {
        gl::glBindVertexArray(vao);
        gl::glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);
        gl::glBindVertexArray(0);
    }

    void destroy() {
        if (ebo) { gl::glDeleteBuffers(1, &ebo); ebo = 0; }
        if (vbo) { gl::glDeleteBuffers(1, &vbo); vbo = 0; }
        if (vao) { gl::glDeleteVertexArrays(1, &vao); vao = 0; }
    }
};

} // namespace renderer
} // namespace qe
