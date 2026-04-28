// SPDX-License-Identifier: MIT
#pragma once
// Copyright (c) 2026 D-Sorganization. All rights reserved.

/**
 * @file OBJParser.h
 * @brief Pure Wavefront .OBJ text parser — no GPU, no SDL, no OpenGL.
 *
 * Contains only RawMesh, ParseResult, and the parsing logic extracted from
 * OBJLoader.h.  This header can be included in headless unit tests without
 * requiring an OpenGL context or SDL2.
 *
 * OBJLoader.h wraps this with GPU upload (Mesh::upload) and file I/O.
 *
 * Error handling policy:
 *   - Incomplete vertex records ('v' / 'vn' / 'vt' with wrong component count)
 *     are logged into ParseResult::error_message and the line is skipped.
 *   - Malformed face tokens (non-numeric, empty position part, trailing garbage)
 *     cause the entire face line to be skipped with an error count increment.
 *   - Out-of-range position indices (including 0, which is invalid in OBJ)
 *     cause the face line to be skipped with an error count increment.
 *   - Faces with fewer than 3 valid vertices are skipped.
 *   - Empty or comment-only content is not an error (ok=true, zero geometry).
 *
 * Complexity: parse_stream is O(L + F) time and O(V + T + N + F) space, where
 * L is input text length, V/T/N are position/texcoord/normal records, and F is
 * the number of triangulated face vertices emitted. Each face token is parsed
 * once; n-gon faces are triangulated with O(k) work for k vertices.
 */

#include <sstream>
#include <string>
#include <vector>

namespace qe {
namespace renderer {

// ── Raw mesh data (pure CPU, no GL) ─────────────────────────────────────────

struct OBJRawMesh {
    std::vector<float> positions;   // x, y, z (groups of 3)
    std::vector<float> texcoords;   // u, v (groups of 2)
    std::vector<float> normals;     // x, y, z (groups of 3)

    struct FaceVertex {
        int pos_idx  = -1;
        int tex_idx  = -1;
        int norm_idx = -1;
    };
    std::vector<FaceVertex> face_verts;  // Triangulated face vertices
};

// ── Parse result ─────────────────────────────────────────────────────────────

struct OBJParseResult {
    OBJRawMesh mesh;
    bool       ok          = false;
    int        error_count = 0;
    std::string error_message;
};

// ── Parser implementation ────────────────────────────────────────────────────

class OBJParser {
public:
    /** Parse OBJ content from an already-open istream.
     *  Complexity: O(L + F) time, O(V + T + N + F) space.
     */
    static OBJParseResult parse_stream(std::istream& stream) {
        OBJParseResult result;
        result.ok = true;
        parse_stream_into(stream, result);
        return result;
    }

    /** Parse OBJ content from a string (convenience wrapper for tests).
     *  Complexity: O(L + F) time, O(V + T + N + F) space.
     */
    static OBJParseResult parse_content(const std::string& content) {
        std::istringstream ss(content);
        return parse_stream(ss);
    }

private:
    static void parse_stream_into(std::istream& stream, OBJParseResult& result) {
        OBJRawMesh& raw = result.mesh;
        int line_num = 0;

        std::string line;
        while (std::getline(stream, line)) {
            ++line_num;
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string prefix;
            if (!(iss >> prefix)) continue;

            if (prefix == "v") {
                float x = 0, y = 0, z = 0;
                if (!(iss >> x >> y >> z)) {
                    ++result.error_count;
                    result.error_message += "line " + std::to_string(line_num)
                        + ": incomplete 'v' record; ";
                    continue;
                }
                raw.positions.push_back(x);
                raw.positions.push_back(y);
                raw.positions.push_back(z);
            } else if (prefix == "vt") {
                float u = 0, v = 0;
                if (!(iss >> u >> v)) {
                    ++result.error_count;
                    result.error_message += "line " + std::to_string(line_num)
                        + ": incomplete 'vt' record; ";
                    continue;
                }
                raw.texcoords.push_back(u);
                raw.texcoords.push_back(v);
            } else if (prefix == "vn") {
                float x = 0, y = 0, z = 0;
                if (!(iss >> x >> y >> z)) {
                    ++result.error_count;
                    result.error_message += "line " + std::to_string(line_num)
                        + ": incomplete 'vn' record; ";
                    continue;
                }
                raw.normals.push_back(x);
                raw.normals.push_back(y);
                raw.normals.push_back(z);
            } else if (prefix == "f") {
                parse_face_line(iss, line_num, raw, result);
            }
        }
    }

    static void parse_face_line(std::istringstream& iss, int line_num,
                                  OBJRawMesh& raw, OBJParseResult& result) {
        std::vector<OBJRawMesh::FaceVertex> face;
        std::string token;
        while (iss >> token) {
            bool token_ok = false;
            OBJRawMesh::FaceVertex fv = try_parse_face_vertex(token, token_ok);
            if (!token_ok) {
                ++result.error_count;
                result.error_message += "line " + std::to_string(line_num)
                    + ": malformed face token '" + token + "'; ";
                return;  // skip the whole face
            }
            int pos_count = static_cast<int>(raw.positions.size()) / 3;
            if (fv.pos_idx < 0 || fv.pos_idx >= pos_count) {
                ++result.error_count;
                result.error_message += "line " + std::to_string(line_num)
                    + ": position index " + std::to_string(fv.pos_idx + 1)
                    + " out of range (have " + std::to_string(pos_count) + "); ";
                return;  // skip the whole face
            }
            face.push_back(fv);
        }
        if (face.size() < 3) {
            ++result.error_count;
            result.error_message += "line " + std::to_string(line_num)
                + ": face has fewer than 3 vertices; ";
            return;
        }
        // Triangulate (fan from first vertex)
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            raw.face_verts.push_back(face[0]);
            raw.face_verts.push_back(face[i]);
            raw.face_verts.push_back(face[i + 1]);
        }
    }

    /** Parse a single face-vertex token.  Sets ok=false on any parse error. */
    static OBJRawMesh::FaceVertex try_parse_face_vertex(const std::string& token,
                                                          bool& ok) {
        OBJRawMesh::FaceVertex fv;
        ok = false;

        if (token.empty()) return fv;

        size_t s1 = token.find('/');
        if (s1 == std::string::npos) {
            // Position-only token
            try {
                size_t consumed = 0;
                fv.pos_idx = std::stoi(token, &consumed) - 1;
                if (consumed != token.size()) return fv;  // trailing garbage
            } catch (...) {
                return fv;
            }
            ok = true;
            return fv;
        }

        size_t s2 = token.find('/', s1 + 1);

        // Position part
        const std::string pos_str = token.substr(0, s1);
        if (pos_str.empty()) return fv;
        try {
            size_t consumed = 0;
            fv.pos_idx = std::stoi(pos_str, &consumed) - 1;
            if (consumed != pos_str.size()) return fv;
        } catch (...) {
            return fv;
        }

        if (s2 != std::string::npos) {
            // v/vt/vn or v//vn format
            const std::string tc_str = token.substr(s1 + 1, s2 - s1 - 1);
            if (!tc_str.empty()) {
                try {
                    size_t consumed = 0;
                    fv.tex_idx = std::stoi(tc_str, &consumed) - 1;
                    if (consumed != tc_str.size()) return fv;
                } catch (...) {
                    return fv;
                }
            }
            const std::string n_str = token.substr(s2 + 1);
            if (!n_str.empty()) {
                try {
                    size_t consumed = 0;
                    fv.norm_idx = std::stoi(n_str, &consumed) - 1;
                    if (consumed != n_str.size()) return fv;
                } catch (...) {
                    return fv;
                }
            }
        } else {
            // v/vt format (no second slash)
            const std::string tc_str = token.substr(s1 + 1);
            if (tc_str.empty()) return fv;
            try {
                size_t consumed = 0;
                fv.tex_idx = std::stoi(tc_str, &consumed) - 1;
                if (consumed != tc_str.size()) return fv;
            } catch (...) {
                return fv;
            }
        }

        ok = true;
        return fv;
    }
};

} // namespace renderer
} // namespace qe
