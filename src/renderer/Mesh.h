#pragma once
/**
 * @file Mesh.h
 * @brief GPU mesh data — VAO/VBO/EBO wrapper for vertex data upload and drawing.
 *
 * Vertex layout: position (3f) + normal (3f) + color (3f) + uv (2f) = 11 floats.
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

    // ── Primitive Generators ────────────────────────────────────────────

    /** Unit cube with per-face normals, colors, and UV coordinates. */
    static Mesh create_cube() {
        Mesh mesh;
        std::vector<Vertex> vertices = {
            // Front (Z+) — Blue
            {{-0.5f,-0.5f, 0.5f}, {0,0,1}, {0.2f,0.4f,0.9f}, {0,0}},
            {{ 0.5f,-0.5f, 0.5f}, {0,0,1}, {0.2f,0.4f,0.9f}, {1,0}},
            {{ 0.5f, 0.5f, 0.5f}, {0,0,1}, {0.3f,0.5f,1.0f}, {1,1}},
            {{-0.5f, 0.5f, 0.5f}, {0,0,1}, {0.3f,0.5f,1.0f}, {0,1}},
            // Back (Z-) — Teal
            {{ 0.5f,-0.5f,-0.5f}, {0,0,-1}, {0.0f,0.7f,0.7f}, {0,0}},
            {{-0.5f,-0.5f,-0.5f}, {0,0,-1}, {0.0f,0.7f,0.7f}, {1,0}},
            {{-0.5f, 0.5f,-0.5f}, {0,0,-1}, {0.1f,0.8f,0.8f}, {1,1}},
            {{ 0.5f, 0.5f,-0.5f}, {0,0,-1}, {0.1f,0.8f,0.8f}, {0,1}},
            // Top (Y+) — Green
            {{-0.5f, 0.5f, 0.5f}, {0,1,0}, {0.2f,0.9f,0.3f}, {0,0}},
            {{ 0.5f, 0.5f, 0.5f}, {0,1,0}, {0.2f,0.9f,0.3f}, {1,0}},
            {{ 0.5f, 0.5f,-0.5f}, {0,1,0}, {0.3f,1.0f,0.4f}, {1,1}},
            {{-0.5f, 0.5f,-0.5f}, {0,1,0}, {0.3f,1.0f,0.4f}, {0,1}},
            // Bottom (Y-) — Orange
            {{-0.5f,-0.5f,-0.5f}, {0,-1,0}, {0.9f,0.5f,0.1f}, {0,0}},
            {{ 0.5f,-0.5f,-0.5f}, {0,-1,0}, {0.9f,0.5f,0.1f}, {1,0}},
            {{ 0.5f,-0.5f, 0.5f}, {0,-1,0}, {1.0f,0.6f,0.2f}, {1,1}},
            {{-0.5f,-0.5f, 0.5f}, {0,-1,0}, {1.0f,0.6f,0.2f}, {0,1}},
            // Right (X+) — Red
            {{ 0.5f,-0.5f, 0.5f}, {1,0,0}, {0.9f,0.2f,0.2f}, {0,0}},
            {{ 0.5f,-0.5f,-0.5f}, {1,0,0}, {0.9f,0.2f,0.2f}, {1,0}},
            {{ 0.5f, 0.5f,-0.5f}, {1,0,0}, {1.0f,0.3f,0.3f}, {1,1}},
            {{ 0.5f, 0.5f, 0.5f}, {1,0,0}, {1.0f,0.3f,0.3f}, {0,1}},
            // Left (X-) — Purple
            {{-0.5f,-0.5f,-0.5f}, {-1,0,0}, {0.6f,0.2f,0.9f}, {0,0}},
            {{-0.5f,-0.5f, 0.5f}, {-1,0,0}, {0.6f,0.2f,0.9f}, {1,0}},
            {{-0.5f, 0.5f, 0.5f}, {-1,0,0}, {0.7f,0.3f,1.0f}, {1,1}},
            {{-0.5f, 0.5f,-0.5f}, {-1,0,0}, {0.7f,0.3f,1.0f}, {0,1}},
        };

        std::vector<unsigned int> indices;
        for (unsigned int f = 0; f < 6; ++f) {
            unsigned int b = f * 4;
            indices.push_back(b+0); indices.push_back(b+1); indices.push_back(b+2);
            indices.push_back(b+0); indices.push_back(b+2); indices.push_back(b+3);
        }

        mesh.upload(vertices, indices);
        return mesh;
    }

    /** Large textured floor plane on XZ. UVs tile the texture. */
    static Mesh create_floor_plane(float half_size = 20.0f, float uv_scale = 4.0f) {
        Mesh mesh;
        float s = half_size;
        float u = uv_scale;

        std::vector<Vertex> vertices = {
            {{-s, 0, -s}, {0,1,0}, {1,1,1}, {0, 0}},
            {{ s, 0, -s}, {0,1,0}, {1,1,1}, {u, 0}},
            {{ s, 0,  s}, {0,1,0}, {1,1,1}, {u, u}},
            {{-s, 0,  s}, {0,1,0}, {1,1,1}, {0, u}},
        };

        std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};
        mesh.upload(vertices, indices);
        return mesh;
    }

    /** Low-poly sphere (icosphere with subdivisions). */
    static Mesh create_sphere(int subdivisions = 2, float r = 0.5f,
                               float cr = 0.8f, float cg = 0.6f, float cb = 0.3f) {
        Mesh mesh;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Start with icosahedron
        const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
        std::vector<float> pos = {
            -1, t,0,  1, t,0,  -1,-t,0,  1,-t,0,
             0,-1,t,  0, 1,t,   0,-1,-t, 0, 1,-t,
             t,0,-1,  t,0, 1,  -t,0,-1, -t,0, 1,
        };
        // Normalize all to unit sphere
        for (size_t i = 0; i < pos.size(); i += 3) {
            float len = std::sqrt(pos[i]*pos[i]+pos[i+1]*pos[i+1]+pos[i+2]*pos[i+2]);
            pos[i] /= len; pos[i+1] /= len; pos[i+2] /= len;
        }

        std::vector<unsigned int> idx = {
            0,11,5, 0,5,1, 0,1,7, 0,7,10, 0,10,11,
            1,5,9, 5,11,4, 11,10,2, 10,7,6, 7,1,8,
            3,9,4, 3,4,2, 3,2,6, 3,6,8, 3,8,9,
            4,9,5, 2,4,11, 6,2,10, 8,6,7, 9,8,1,
        };

        // Subdivide
        for (int s = 0; s < subdivisions; ++s) {
            std::vector<unsigned int> new_idx;
            for (size_t i = 0; i < idx.size(); i += 3) {
                unsigned int a = idx[i], b = idx[i+1], c = idx[i+2];
                // Midpoints
                auto midpoint = [&](unsigned int i0, unsigned int i1) -> unsigned int {
                    float mx = (pos[i0*3]+pos[i1*3])*0.5f;
                    float my = (pos[i0*3+1]+pos[i1*3+1])*0.5f;
                    float mz = (pos[i0*3+2]+pos[i1*3+2])*0.5f;
                    float len = std::sqrt(mx*mx+my*my+mz*mz);
                    mx/=len; my/=len; mz/=len;
                    unsigned int ni = static_cast<unsigned int>(pos.size()/3);
                    pos.push_back(mx); pos.push_back(my); pos.push_back(mz);
                    return ni;
                };
                unsigned int ab = midpoint(a, b);
                unsigned int bc = midpoint(b, c);
                unsigned int ca = midpoint(c, a);
                new_idx.push_back(a);  new_idx.push_back(ab); new_idx.push_back(ca);
                new_idx.push_back(b);  new_idx.push_back(bc); new_idx.push_back(ab);
                new_idx.push_back(c);  new_idx.push_back(ca); new_idx.push_back(bc);
                new_idx.push_back(ab); new_idx.push_back(bc); new_idx.push_back(ca);
            }
            idx = new_idx;
        }

        // Build vertex data
        for (size_t i = 0; i < pos.size(); i += 3) {
            Vertex v{};
            v.position[0] = pos[i]*r; v.position[1] = pos[i+1]*r; v.position[2] = pos[i+2]*r;
            v.normal[0] = pos[i]; v.normal[1] = pos[i+1]; v.normal[2] = pos[i+2];
            v.color[0] = cr; v.color[1] = cg; v.color[2] = cb;
            // Spherical UV
            v.uv[0] = 0.5f + std::atan2(pos[i+2], pos[i]) / (2.0f * 3.14159265f);
            v.uv[1] = 0.5f - std::asin(pos[i+1]) / 3.14159265f;
            vertices.push_back(v);
        }

        mesh.upload(vertices, idx);
        return mesh;
    }

    /** Unit cylinder along Y axis with per-face normals. */
    static Mesh create_cylinder(int segments = 16, float radius = 0.5f, float height = 1.0f,
                                 float cr = 0.6f, float cg = 0.6f, float cb = 0.6f) {
        Mesh mesh;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        const float pi = 3.14159265f;
        float half_h = height * 0.5f;

        // Side vertices: two rings (bottom and top)
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
            float nx = std::cos(angle);
            float nz = std::sin(angle);
            float u = static_cast<float>(i) / static_cast<float>(segments);

            // Bottom ring
            Vertex vb{};
            vb.position[0] = nx * radius; vb.position[1] = -half_h; vb.position[2] = nz * radius;
            vb.normal[0] = nx; vb.normal[1] = 0; vb.normal[2] = nz;
            vb.color[0] = cr; vb.color[1] = cg; vb.color[2] = cb;
            vb.uv[0] = u; vb.uv[1] = 0;
            vertices.push_back(vb);

            // Top ring
            Vertex vt{};
            vt.position[0] = nx * radius; vt.position[1] = half_h; vt.position[2] = nz * radius;
            vt.normal[0] = nx; vt.normal[1] = 0; vt.normal[2] = nz;
            vt.color[0] = cr * 1.1f; vt.color[1] = cg * 1.1f; vt.color[2] = cb * 1.1f;
            vt.uv[0] = u; vt.uv[1] = 1;
            vertices.push_back(vt);
        }

        // Side indices
        for (int i = 0; i < segments; ++i) {
            unsigned int bl = static_cast<unsigned int>(i * 2);
            unsigned int br = static_cast<unsigned int>((i + 1) * 2);
            unsigned int tl = bl + 1;
            unsigned int tr = br + 1;
            indices.push_back(bl); indices.push_back(br); indices.push_back(tr);
            indices.push_back(bl); indices.push_back(tr); indices.push_back(tl);
        }

        // Cap centers
        unsigned int bot_center = static_cast<unsigned int>(vertices.size());
        Vertex bc{};
        bc.position[0] = 0; bc.position[1] = -half_h; bc.position[2] = 0;
        bc.normal[0] = 0; bc.normal[1] = -1; bc.normal[2] = 0;
        bc.color[0] = cr * 0.8f; bc.color[1] = cg * 0.8f; bc.color[2] = cb * 0.8f;
        bc.uv[0] = 0.5f; bc.uv[1] = 0.5f;
        vertices.push_back(bc);

        unsigned int top_center = static_cast<unsigned int>(vertices.size());
        Vertex tc{};
        tc.position[0] = 0; tc.position[1] = half_h; tc.position[2] = 0;
        tc.normal[0] = 0; tc.normal[1] = 1; tc.normal[2] = 0;
        tc.color[0] = cr * 1.2f; tc.color[1] = cg * 1.2f; tc.color[2] = cb * 1.2f;
        tc.uv[0] = 0.5f; tc.uv[1] = 0.5f;
        vertices.push_back(tc);

        // Cap ring vertices (separate for correct normals)
        unsigned int bot_ring_start = static_cast<unsigned int>(vertices.size());
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
            float nx = std::cos(angle);
            float nz = std::sin(angle);
            Vertex v{};
            v.position[0] = nx * radius; v.position[1] = -half_h; v.position[2] = nz * radius;
            v.normal[0] = 0; v.normal[1] = -1; v.normal[2] = 0;
            v.color[0] = cr * 0.8f; v.color[1] = cg * 0.8f; v.color[2] = cb * 0.8f;
            v.uv[0] = nx * 0.5f + 0.5f; v.uv[1] = nz * 0.5f + 0.5f;
            vertices.push_back(v);
        }
        unsigned int top_ring_start = static_cast<unsigned int>(vertices.size());
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
            float nx = std::cos(angle);
            float nz = std::sin(angle);
            Vertex v{};
            v.position[0] = nx * radius; v.position[1] = half_h; v.position[2] = nz * radius;
            v.normal[0] = 0; v.normal[1] = 1; v.normal[2] = 0;
            v.color[0] = cr * 1.2f; v.color[1] = cg * 1.2f; v.color[2] = cb * 1.2f;
            v.uv[0] = nx * 0.5f + 0.5f; v.uv[1] = nz * 0.5f + 0.5f;
            vertices.push_back(v);
        }

        // Bottom cap (winding: CW from below = CCW from outside looking down)
        for (int i = 0; i < segments; ++i) {
            indices.push_back(bot_center);
            indices.push_back(bot_ring_start + static_cast<unsigned int>(i + 1));
            indices.push_back(bot_ring_start + static_cast<unsigned int>(i));
        }
        // Top cap
        for (int i = 0; i < segments; ++i) {
            indices.push_back(top_center);
            indices.push_back(top_ring_start + static_cast<unsigned int>(i));
            indices.push_back(top_ring_start + static_cast<unsigned int>(i + 1));
        }

        mesh.upload(vertices, indices);
        return mesh;
    }

    /** Cone along Y axis (apex at top). */
    static Mesh create_cone(int segments = 16, float radius = 0.5f, float height = 1.0f,
                             float cr = 0.7f, float cg = 0.5f, float cb = 0.3f) {
        Mesh mesh;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        const float pi = 3.14159265f;
        float half_h = height * 0.5f;
        float slope = radius / height;

        // Apex vertex (shared by all side triangles with averaged normal)
        unsigned int apex_start = static_cast<unsigned int>(vertices.size());

        // Side triangles: each segment gets its own apex + base pair for correct normals
        for (int i = 0; i < segments; ++i) {
            float a0 = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
            float a1 = 2.0f * pi * static_cast<float>(i + 1) / static_cast<float>(segments);
            float amid = (a0 + a1) * 0.5f;

            float nx = std::cos(amid);
            float nz = std::sin(amid);
            float ny = slope;
            float nlen = std::sqrt(nx*nx + ny*ny + nz*nz);
            nx /= nlen; ny /= nlen; nz /= nlen;

            // Apex
            Vertex va{};
            va.position[0] = 0; va.position[1] = half_h; va.position[2] = 0;
            va.normal[0] = nx; va.normal[1] = ny; va.normal[2] = nz;
            va.color[0] = cr * 1.2f; va.color[1] = cg * 1.2f; va.color[2] = cb * 1.2f;
            va.uv[0] = 0.5f; va.uv[1] = 1.0f;
            vertices.push_back(va);

            // Base left
            Vertex vl{};
            vl.position[0] = std::cos(a0) * radius; vl.position[1] = -half_h;
            vl.position[2] = std::sin(a0) * radius;
            vl.normal[0] = nx; vl.normal[1] = ny; vl.normal[2] = nz;
            vl.color[0] = cr; vl.color[1] = cg; vl.color[2] = cb;
            vl.uv[0] = static_cast<float>(i) / segments; vl.uv[1] = 0;
            vertices.push_back(vl);

            // Base right
            Vertex vr{};
            vr.position[0] = std::cos(a1) * radius; vr.position[1] = -half_h;
            vr.position[2] = std::sin(a1) * radius;
            vr.normal[0] = nx; vr.normal[1] = ny; vr.normal[2] = nz;
            vr.color[0] = cr; vr.color[1] = cg; vr.color[2] = cb;
            vr.uv[0] = static_cast<float>(i + 1) / segments; vr.uv[1] = 0;
            vertices.push_back(vr);

            unsigned int base = apex_start + static_cast<unsigned int>(i * 3);
            indices.push_back(base);     // apex
            indices.push_back(base + 2); // base right
            indices.push_back(base + 1); // base left
        }

        // Bottom cap
        unsigned int bot_center = static_cast<unsigned int>(vertices.size());
        Vertex bc{};
        bc.position[0] = 0; bc.position[1] = -half_h; bc.position[2] = 0;
        bc.normal[0] = 0; bc.normal[1] = -1; bc.normal[2] = 0;
        bc.color[0] = cr * 0.7f; bc.color[1] = cg * 0.7f; bc.color[2] = cb * 0.7f;
        bc.uv[0] = 0.5f; bc.uv[1] = 0.5f;
        vertices.push_back(bc);

        unsigned int cap_start = static_cast<unsigned int>(vertices.size());
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
            float cx = std::cos(angle);
            float cz = std::sin(angle);
            Vertex v{};
            v.position[0] = cx * radius; v.position[1] = -half_h; v.position[2] = cz * radius;
            v.normal[0] = 0; v.normal[1] = -1; v.normal[2] = 0;
            v.color[0] = cr * 0.7f; v.color[1] = cg * 0.7f; v.color[2] = cb * 0.7f;
            v.uv[0] = cx * 0.5f + 0.5f; v.uv[1] = cz * 0.5f + 0.5f;
            vertices.push_back(v);
        }
        for (int i = 0; i < segments; ++i) {
            indices.push_back(bot_center);
            indices.push_back(cap_start + static_cast<unsigned int>(i + 1));
            indices.push_back(cap_start + static_cast<unsigned int>(i));
        }

        mesh.upload(vertices, indices);
        return mesh;
    }

    /** Capsule (cylinder with hemisphere caps) along Y axis. */
    static Mesh create_capsule(int segments = 16, int rings = 8,
                                float radius = 0.3f, float height = 1.0f,
                                float cr = 0.5f, float cg = 0.7f, float cb = 0.9f) {
        Mesh mesh;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        const float pi = 3.14159265f;
        float half_h = height * 0.5f; // half of cylinder body

        // Build capsule as latitude/longitude sphere stretched along Y
        int half_rings = rings / 2;

        auto add_ring = [&](float y_center, float lat_start, float lat_end, int ring_count, bool flip) {
            unsigned int base = static_cast<unsigned int>(vertices.size());

            for (int r = 0; r <= ring_count; ++r) {
                float t = static_cast<float>(r) / static_cast<float>(ring_count);
                float lat = lat_start + t * (lat_end - lat_start);
                float cos_lat = std::cos(lat);
                float sin_lat = std::sin(lat);

                for (int s = 0; s <= segments; ++s) {
                    float lon = 2.0f * pi * static_cast<float>(s) / static_cast<float>(segments);
                    float cos_lon = std::cos(lon);
                    float sin_lon = std::sin(lon);

                    Vertex v{};
                    v.position[0] = cos_lat * cos_lon * radius;
                    v.position[1] = y_center + sin_lat * radius;
                    v.position[2] = cos_lat * sin_lon * radius;
                    v.normal[0] = cos_lat * cos_lon;
                    v.normal[1] = sin_lat;
                    v.normal[2] = cos_lat * sin_lon;
                    v.color[0] = cr; v.color[1] = cg; v.color[2] = cb;
                    v.uv[0] = static_cast<float>(s) / segments;
                    v.uv[1] = flip ? (1.0f - t * 0.25f) : (t * 0.25f);
                    vertices.push_back(v);
                }
            }

            // Indices
            int stride = segments + 1;
            for (int r = 0; r < ring_count; ++r) {
                for (int s = 0; s < segments; ++s) {
                    unsigned int a = base + static_cast<unsigned int>(r * stride + s);
                    unsigned int b = a + 1;
                    unsigned int c = a + static_cast<unsigned int>(stride);
                    unsigned int d = c + 1;
                    indices.push_back(a); indices.push_back(c); indices.push_back(d);
                    indices.push_back(a); indices.push_back(d); indices.push_back(b);
                }
            }
        };

        // Bottom hemisphere (lat: -pi/2 to 0)
        add_ring(-half_h, -pi * 0.5f, 0.0f, half_rings, false);

        // Cylinder body (just connect bottom hemisphere equator to top hemisphere equator)
        {
            unsigned int base = static_cast<unsigned int>(vertices.size());
            // Bottom ring
            for (int s = 0; s <= segments; ++s) {
                float lon = 2.0f * pi * static_cast<float>(s) / static_cast<float>(segments);
                Vertex v{};
                v.position[0] = std::cos(lon) * radius;
                v.position[1] = -half_h;
                v.position[2] = std::sin(lon) * radius;
                v.normal[0] = std::cos(lon); v.normal[1] = 0; v.normal[2] = std::sin(lon);
                v.color[0] = cr; v.color[1] = cg; v.color[2] = cb;
                v.uv[0] = static_cast<float>(s) / segments; v.uv[1] = 0.25f;
                vertices.push_back(v);
            }
            // Top ring
            for (int s = 0; s <= segments; ++s) {
                float lon = 2.0f * pi * static_cast<float>(s) / static_cast<float>(segments);
                Vertex v{};
                v.position[0] = std::cos(lon) * radius;
                v.position[1] = half_h;
                v.position[2] = std::sin(lon) * radius;
                v.normal[0] = std::cos(lon); v.normal[1] = 0; v.normal[2] = std::sin(lon);
                v.color[0] = cr; v.color[1] = cg; v.color[2] = cb;
                v.uv[0] = static_cast<float>(s) / segments; v.uv[1] = 0.75f;
                vertices.push_back(v);
            }

            int stride = segments + 1;
            for (int s = 0; s < segments; ++s) {
                unsigned int a = base + static_cast<unsigned int>(s);
                unsigned int b = a + 1;
                unsigned int c = a + static_cast<unsigned int>(stride);
                unsigned int d = c + 1;
                indices.push_back(a); indices.push_back(c); indices.push_back(d);
                indices.push_back(a); indices.push_back(d); indices.push_back(b);
            }
        }

        // Top hemisphere (lat: 0 to pi/2)
        add_ring(half_h, 0.0f, pi * 0.5f, half_rings, true);

        mesh.upload(vertices, indices);
        return mesh;
    }

    /** Wedge/ramp shape — triangular prism along Z axis. */
    static Mesh create_wedge(float cr = 0.5f, float cg = 0.4f, float cb = 0.3f) {
        Mesh mesh;

        // Wedge: base on XZ plane, slopes from Z- bottom to Z+ top
        // 6 unique positions forming a triangular prism along X
        // Bottom-front-left, bottom-front-right, bottom-back-left, bottom-back-right,
        // top-back-left, top-back-right
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Face normals computed for each face
        // Bottom face (Y-): 4 verts
        unsigned int base = 0;
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, {0,-1,0}, {cr*0.8f,cg*0.8f,cb*0.8f}, {0,0}});
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, {0,-1,0}, {cr*0.8f,cg*0.8f,cb*0.8f}, {1,0}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, {0,-1,0}, {cr*0.8f,cg*0.8f,cb*0.8f}, {1,1}});
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, {0,-1,0}, {cr*0.8f,cg*0.8f,cb*0.8f}, {0,1}});
        indices.push_back(base); indices.push_back(base+2); indices.push_back(base+1);
        indices.push_back(base); indices.push_back(base+3); indices.push_back(base+2);

        // Back face (Z+): quad from bottom-back to top-back
        base = static_cast<unsigned int>(vertices.size());
        vertices.push_back({{-0.5f, -0.5f, 0.5f}, {0,0,1}, {cr,cg,cb}, {0,0}});
        vertices.push_back({{ 0.5f, -0.5f, 0.5f}, {0,0,1}, {cr,cg,cb}, {1,0}});
        vertices.push_back({{ 0.5f,  0.5f, 0.5f}, {0,0,1}, {cr*1.1f,cg*1.1f,cb*1.1f}, {1,1}});
        vertices.push_back({{-0.5f,  0.5f, 0.5f}, {0,0,1}, {cr*1.1f,cg*1.1f,cb*1.1f}, {0,1}});
        indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);
        indices.push_back(base); indices.push_back(base+2); indices.push_back(base+3);

        // Slope face (front-top diagonal): from front-bottom to back-top
        // Normal points outward from slope: (0, 1, 1) normalized
        float sn = 1.0f / std::sqrt(2.0f);
        base = static_cast<unsigned int>(vertices.size());
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, {0,sn,-sn}, {cr*1.1f,cg*1.1f,cb*1.1f}, {0,0}});
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, {0,sn,-sn}, {cr*1.1f,cg*1.1f,cb*1.1f}, {1,0}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, {0,sn,-sn}, {cr*1.2f,cg*1.2f,cb*1.2f}, {1,1}});
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, {0,sn,-sn}, {cr*1.2f,cg*1.2f,cb*1.2f}, {0,1}});
        indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);
        indices.push_back(base); indices.push_back(base+2); indices.push_back(base+3);

        // Left face (X-): triangle
        base = static_cast<unsigned int>(vertices.size());
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, {-1,0,0}, {cr*0.9f,cg*0.9f,cb*0.9f}, {0,0}});
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, {-1,0,0}, {cr*0.9f,cg*0.9f,cb*0.9f}, {1,1}});
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, {-1,0,0}, {cr*0.9f,cg*0.9f,cb*0.9f}, {0,1}});
        indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);

        // Right face (X+): triangle
        base = static_cast<unsigned int>(vertices.size());
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, {1,0,0}, {cr*0.9f,cg*0.9f,cb*0.9f}, {1,0}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, {1,0,0}, {cr*0.9f,cg*0.9f,cb*0.9f}, {1,1}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, {1,0,0}, {cr*0.9f,cg*0.9f,cb*0.9f}, {0,1}});
        indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);

        mesh.upload(vertices, indices);
        return mesh;
    }

    /** Pyramid with square base, apex at Y+. */
    static Mesh create_pyramid(float cr = 0.7f, float cg = 0.6f, float cb = 0.3f) {
        Mesh mesh;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        float h = 0.5f; // half height
        float b = 0.5f; // half base

        // 4 triangular side faces + 1 quad base = 4*3 + 4 = 16 vertices
        // Each face has its own vertices for correct normals

        // Front face (Z+)
        {
            float nx = 0, nz = 1.0f, ny = b / h;
            float nlen = std::sqrt(nx*nx + ny*ny + nz*nz);
            nx /= nlen; ny /= nlen; nz /= nlen;
            unsigned int base_idx = static_cast<unsigned int>(vertices.size());
            vertices.push_back({{-b, -h, b}, {nx,ny,nz}, {cr,cg,cb}, {0,0}});
            vertices.push_back({{ b, -h, b}, {nx,ny,nz}, {cr,cg,cb}, {1,0}});
            vertices.push_back({{ 0,  h, 0}, {nx,ny,nz}, {cr*1.3f,cg*1.3f,cb*1.3f}, {0.5f,1}});
            indices.push_back(base_idx); indices.push_back(base_idx+1); indices.push_back(base_idx+2);
        }
        // Back face (Z-)
        {
            float nx = 0, nz = -1.0f, ny = b / h;
            float nlen = std::sqrt(nx*nx + ny*ny + nz*nz);
            nx /= nlen; ny /= nlen; nz /= nlen;
            unsigned int base_idx = static_cast<unsigned int>(vertices.size());
            vertices.push_back({{ b, -h, -b}, {nx,ny,nz}, {cr,cg,cb}, {0,0}});
            vertices.push_back({{-b, -h, -b}, {nx,ny,nz}, {cr,cg,cb}, {1,0}});
            vertices.push_back({{ 0,  h,  0}, {nx,ny,nz}, {cr*1.3f,cg*1.3f,cb*1.3f}, {0.5f,1}});
            indices.push_back(base_idx); indices.push_back(base_idx+1); indices.push_back(base_idx+2);
        }
        // Right face (X+)
        {
            float nx = 1.0f, nz = 0, ny = b / h;
            float nlen = std::sqrt(nx*nx + ny*ny + nz*nz);
            nx /= nlen; ny /= nlen; nz /= nlen;
            unsigned int base_idx = static_cast<unsigned int>(vertices.size());
            vertices.push_back({{ b, -h,  b}, {nx,ny,nz}, {cr*0.9f,cg*0.9f,cb*0.9f}, {0,0}});
            vertices.push_back({{ b, -h, -b}, {nx,ny,nz}, {cr*0.9f,cg*0.9f,cb*0.9f}, {1,0}});
            vertices.push_back({{ 0,  h,  0}, {nx,ny,nz}, {cr*1.3f,cg*1.3f,cb*1.3f}, {0.5f,1}});
            indices.push_back(base_idx); indices.push_back(base_idx+1); indices.push_back(base_idx+2);
        }
        // Left face (X-)
        {
            float nx = -1.0f, nz = 0, ny = b / h;
            float nlen = std::sqrt(nx*nx + ny*ny + nz*nz);
            nx /= nlen; ny /= nlen; nz /= nlen;
            unsigned int base_idx = static_cast<unsigned int>(vertices.size());
            vertices.push_back({{-b, -h, -b}, {nx,ny,nz}, {cr*0.9f,cg*0.9f,cb*0.9f}, {0,0}});
            vertices.push_back({{-b, -h,  b}, {nx,ny,nz}, {cr*0.9f,cg*0.9f,cb*0.9f}, {1,0}});
            vertices.push_back({{ 0,  h,  0}, {nx,ny,nz}, {cr*1.3f,cg*1.3f,cb*1.3f}, {0.5f,1}});
            indices.push_back(base_idx); indices.push_back(base_idx+1); indices.push_back(base_idx+2);
        }
        // Base (Y-)
        {
            unsigned int base_idx = static_cast<unsigned int>(vertices.size());
            vertices.push_back({{-b, -h, -b}, {0,-1,0}, {cr*0.7f,cg*0.7f,cb*0.7f}, {0,0}});
            vertices.push_back({{ b, -h, -b}, {0,-1,0}, {cr*0.7f,cg*0.7f,cb*0.7f}, {1,0}});
            vertices.push_back({{ b, -h,  b}, {0,-1,0}, {cr*0.7f,cg*0.7f,cb*0.7f}, {1,1}});
            vertices.push_back({{-b, -h,  b}, {0,-1,0}, {cr*0.7f,cg*0.7f,cb*0.7f}, {0,1}});
            indices.push_back(base_idx); indices.push_back(base_idx+2); indices.push_back(base_idx+1);
            indices.push_back(base_idx); indices.push_back(base_idx+3); indices.push_back(base_idx+2);
        }

        mesh.upload(vertices, indices);
        return mesh;
    }

    /** Grid lines on XZ plane (for spatial reference, drawn with GL_LINES). */
    static Mesh create_grid(int half_size = 10, float spacing = 1.0f) {
        Mesh mesh;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        unsigned int idx = 0;
        float extent = half_size * spacing;

        for (int i = -half_size; i <= half_size; ++i) {
            float pos = i * spacing;
            float brightness = (i == 0) ? 0.6f : 0.25f;

            vertices.push_back({{pos, 0.0f, -extent}, {0,1,0},
                                {brightness,brightness,brightness}, {0,0}});
            vertices.push_back({{pos, 0.0f,  extent}, {0,1,0},
                                {brightness,brightness,brightness}, {1,0}});
            indices.push_back(idx++); indices.push_back(idx++);

            vertices.push_back({{-extent, 0.0f, pos}, {0,1,0},
                                {brightness,brightness,brightness}, {0,0}});
            vertices.push_back({{ extent, 0.0f, pos}, {0,1,0},
                                {brightness,brightness,brightness}, {1,0}});
            indices.push_back(idx++); indices.push_back(idx++);
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

        gl::glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex,position)));
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex,normal)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex,color)));
        gl::glEnableVertexAttribArray(2);
        gl::glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex,uv)));
        gl::glEnableVertexAttribArray(3);

        gl::glBindVertexArray(0);
        return mesh;
    }
};

} // namespace renderer
} // namespace qe
