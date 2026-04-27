// Copyright (c) 2026 D-Sorganization. All rights reserved.
/**
 * @file test_objloader.cpp
 * @brief Tests for OBJLoader — malformed input handling and valid parse paths.
 *
 * Uses parse_content() / parse_stream() which exercise the parser without
 * requiring an OpenGL context or filesystem access.  The GPU-upload path
 * (OBJLoader::load) is not tested here because it requires a live GL context.
 *
 * Issue #105: OBJLoader malformed-input tests and error handling.
 *
 * Validates:
 *   - Valid triangle (position-only face tokens)
 *   - Valid quad auto-triangulated into 2 triangles
 *   - Missing normals produce fallback geometry (no face_verts dropped)
 *   - Out-of-range position index is flagged as an error and face is skipped
 *   - Malformed face token (non-numeric) is flagged as an error
 *   - Incomplete vertex record ('v' with only 2 components) is skipped
 *   - Comment-only file parses as empty but ok
 *   - Empty string parses as empty but ok
 *   - Face with fewer than 3 vertices is skipped with an error count
 *   - v/vt format (position + texcoord, no normal) is parsed correctly
 *   - v//vn format (position + normal, no texcoord) is parsed correctly
 *   - v/vt/vn format parsed correctly
 *   - Zero-index face token (OBJ is 1-based) is flagged as out-of-range
 */

#include "test_framework.h"

// Include OBJParser.h — the pure-C++ parsing layer with no SDL or OpenGL
// dependencies.  This allows the test to run in headless environments where
// no GL context exists (i.e. the normal CI build with QE_BUILD_DEMO=OFF).

#include "../src/renderer/OBJParser.h"

#include <iostream>
#include <string>

using namespace qe::renderer;

// ── Helper: single-triangle OBJ ─────────────────────────────────────────────

static const std::string TRIANGLE_OBJ =
    "# Simple triangle\n"
    "v 0.0 0.0 0.0\n"
    "v 1.0 0.0 0.0\n"
    "v 0.5 1.0 0.0\n"
    "f 1 2 3\n";

static const std::string QUAD_OBJ =
    "v 0.0 0.0 0.0\n"
    "v 1.0 0.0 0.0\n"
    "v 1.0 1.0 0.0\n"
    "v 0.0 1.0 0.0\n"
    "f 1 2 3 4\n";

static const std::string TRIANGLE_WITH_NORMALS_OBJ =
    "v 0.0 0.0 0.0\n"
    "v 1.0 0.0 0.0\n"
    "v 0.5 1.0 0.0\n"
    "vn 0.0 0.0 1.0\n"
    "vn 0.0 0.0 1.0\n"
    "vn 0.0 0.0 1.0\n"
    "f 1//1 2//2 3//3\n";

static const std::string TRIANGLE_WITH_TEX_OBJ =
    "v 0.0 0.0 0.0\n"
    "v 1.0 0.0 0.0\n"
    "v 0.5 1.0 0.0\n"
    "vt 0.0 0.0\n"
    "vt 1.0 0.0\n"
    "vt 0.5 1.0\n"
    "f 1/1 2/2 3/3\n";

static const std::string TRIANGLE_WITH_FULL_OBJ =
    "v 0.0 0.0 0.0\n"
    "v 1.0 0.0 0.0\n"
    "v 0.5 1.0 0.0\n"
    "vt 0.0 0.0\n"
    "vt 1.0 0.0\n"
    "vt 0.5 1.0\n"
    "vn 0.0 0.0 1.0\n"
    "vn 0.0 0.0 1.0\n"
    "vn 0.0 0.0 1.0\n"
    "f 1/1/1 2/2/2 3/3/3\n";

// ── Valid input tests ────────────────────────────────────────────────────────

void test_valid_triangle_parses_ok() {
    auto r = OBJParser::parse_content(TRIANGLE_OBJ);
    ASSERT_TRUE(r.ok);
    ASSERT_TRUE(r.error_count == 0);
    // 3 vertices
    ASSERT_TRUE(r.mesh.positions.size() == 9);  // 3 verts * 3 floats
    // 1 triangle = 3 face_verts
    ASSERT_TRUE(r.mesh.face_verts.size() == 3);
}

void test_valid_quad_triangulates_to_2_tris() {
    auto r = OBJParser::parse_content(QUAD_OBJ);
    ASSERT_TRUE(r.ok);
    ASSERT_TRUE(r.error_count == 0);
    // Fan triangulation: (0,1,2) and (0,2,3) = 6 face_verts
    ASSERT_TRUE(r.mesh.face_verts.size() == 6);
}

void test_missing_normals_parses_ok() {
    // Triangle without vn lines; normals will be computed from geometry
    auto r = OBJParser::parse_content(TRIANGLE_OBJ);
    ASSERT_TRUE(r.ok);
    ASSERT_TRUE(r.error_count == 0);
    ASSERT_TRUE(r.mesh.normals.empty());
    ASSERT_TRUE(r.mesh.face_verts.size() == 3);
}

void test_v_slash_vn_format_parses_ok() {
    auto r = OBJParser::parse_content(TRIANGLE_WITH_NORMALS_OBJ);
    ASSERT_TRUE(r.ok);
    ASSERT_TRUE(r.error_count == 0);
    ASSERT_TRUE(r.mesh.normals.size() == 9);  // 3 normals * 3 floats
    ASSERT_TRUE(r.mesh.face_verts.size() == 3);
    // Check norm_idx is set
    ASSERT_TRUE(r.mesh.face_verts[0].norm_idx == 0);
    ASSERT_TRUE(r.mesh.face_verts[1].norm_idx == 1);
    ASSERT_TRUE(r.mesh.face_verts[2].norm_idx == 2);
}

void test_v_slash_vt_format_parses_ok() {
    auto r = OBJParser::parse_content(TRIANGLE_WITH_TEX_OBJ);
    ASSERT_TRUE(r.ok);
    ASSERT_TRUE(r.error_count == 0);
    ASSERT_TRUE(r.mesh.texcoords.size() == 6);  // 3 texcoords * 2 floats
    ASSERT_TRUE(r.mesh.face_verts.size() == 3);
    ASSERT_TRUE(r.mesh.face_verts[0].tex_idx == 0);
}

void test_v_slash_vt_slash_vn_format_parses_ok() {
    auto r = OBJParser::parse_content(TRIANGLE_WITH_FULL_OBJ);
    ASSERT_TRUE(r.ok);
    ASSERT_TRUE(r.error_count == 0);
    ASSERT_TRUE(r.mesh.face_verts.size() == 3);
    ASSERT_TRUE(r.mesh.face_verts[0].pos_idx  == 0);
    ASSERT_TRUE(r.mesh.face_verts[0].tex_idx  == 0);
    ASSERT_TRUE(r.mesh.face_verts[0].norm_idx == 0);
}

void test_comment_only_file_parses_empty_ok() {
    auto r = OBJParser::parse_content("# just a comment\n# another comment\n");
    ASSERT_TRUE(r.ok);
    ASSERT_TRUE(r.error_count == 0);
    ASSERT_TRUE(r.mesh.positions.empty());
    ASSERT_TRUE(r.mesh.face_verts.empty());
}

void test_empty_string_parses_empty_ok() {
    auto r = OBJParser::parse_content("");
    ASSERT_TRUE(r.ok);
    ASSERT_TRUE(r.error_count == 0);
    ASSERT_TRUE(r.mesh.positions.empty());
}

// ── Malformed input tests ────────────────────────────────────────────────────

void test_malformed_face_token_is_skipped_with_error() {
    // "abc" is not a valid integer face token
    std::string obj =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.5 1.0 0.0\n"
        "f abc 2 3\n";
    auto r = OBJParser::parse_content(obj);
    ASSERT_TRUE(r.error_count > 0);
    // Bad face should be skipped entirely
    ASSERT_TRUE(r.mesh.face_verts.empty());
}

void test_malformed_face_token_empty_pos_part_skipped() {
    // "/1/1" has no position index
    std::string obj =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.5 1.0 0.0\n"
        "vn 0.0 0.0 1.0\n"
        "f /1/1 /1/1 /1/1\n";
    auto r = OBJParser::parse_content(obj);
    ASSERT_TRUE(r.error_count > 0);
    ASSERT_TRUE(r.mesh.face_verts.empty());
}

void test_out_of_range_face_index_skipped_with_error() {
    // Only 3 vertices defined; face references vertex 99
    std::string obj =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.5 1.0 0.0\n"
        "f 1 2 99\n";
    auto r = OBJParser::parse_content(obj);
    ASSERT_TRUE(r.error_count > 0);
    ASSERT_TRUE(r.mesh.face_verts.empty());
}

void test_zero_index_in_face_is_out_of_range() {
    // OBJ indices are 1-based; 0 is not a valid index
    std::string obj =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.5 1.0 0.0\n"
        "f 0 1 2\n";
    auto r = OBJParser::parse_content(obj);
    ASSERT_TRUE(r.error_count > 0);
    ASSERT_TRUE(r.mesh.face_verts.empty());
}

void test_incomplete_vertex_record_v_is_skipped() {
    // "v 1.0 2.0" only has 2 components instead of 3
    std::string obj =
        "v 1.0 2.0\n"      // incomplete — should be skipped
        "v 0.0 0.0 0.0\n"  // valid
        "v 1.0 0.0 0.0\n"
        "v 0.5 1.0 0.0\n"
        "f 1 2 3\n";
    auto r = OBJParser::parse_content(obj);
    ASSERT_TRUE(r.error_count > 0);
    // Only 3 valid vertices (first line skipped)
    ASSERT_TRUE(r.mesh.positions.size() == 9);
    // Face references 1-based indices relative to the 3 valid verts
    ASSERT_TRUE(r.mesh.face_verts.size() == 3);
}

void test_incomplete_vn_record_is_skipped() {
    std::string obj =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.5 1.0 0.0\n"
        "vn 0.0 1.0\n"    // only 2 components — skip
        "f 1 2 3\n";
    auto r = OBJParser::parse_content(obj);
    ASSERT_TRUE(r.error_count > 0);
    ASSERT_TRUE(r.mesh.normals.empty());
    // Face is still valid (no norm indices referenced)
    ASSERT_TRUE(r.mesh.face_verts.size() == 3);
}

void test_face_with_only_2_vertices_is_skipped() {
    std::string obj =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "f 1 2\n";
    auto r = OBJParser::parse_content(obj);
    ASSERT_TRUE(r.error_count > 0);
    ASSERT_TRUE(r.mesh.face_verts.empty());
}

void test_face_before_any_vertices_is_out_of_range() {
    std::string obj = "f 1 2 3\n";
    auto r = OBJParser::parse_content(obj);
    ASSERT_TRUE(r.error_count > 0);
    ASSERT_TRUE(r.mesh.face_verts.empty());
}

void test_valid_face_after_invalid_face_is_parsed() {
    // Bad face first, then a good face — good face should be captured
    std::string obj =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.5 1.0 0.0\n"
        "f 1 2 99\n"   // bad (out of range)
        "f 1 2 3\n";   // good
    auto r = OBJParser::parse_content(obj);
    ASSERT_TRUE(r.error_count > 0);
    ASSERT_TRUE(r.mesh.face_verts.size() == 3);  // only the good face
}

void test_face_token_with_trailing_garbage_is_invalid() {
    // "1abc" — trailing non-numeric chars in face token
    std::string obj =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.5 1.0 0.0\n"
        "f 1abc 2 3\n";
    auto r = OBJParser::parse_content(obj);
    ASSERT_TRUE(r.error_count > 0);
    ASSERT_TRUE(r.mesh.face_verts.empty());
}

// ── Main ─────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== OBJLoader Tests ===" << std::endl;

    std::cout << "\n--- Valid Input ---" << std::endl;
    RUN_TEST(test_valid_triangle_parses_ok);
    RUN_TEST(test_valid_quad_triangulates_to_2_tris);
    RUN_TEST(test_missing_normals_parses_ok);
    RUN_TEST(test_v_slash_vn_format_parses_ok);
    RUN_TEST(test_v_slash_vt_format_parses_ok);
    RUN_TEST(test_v_slash_vt_slash_vn_format_parses_ok);
    RUN_TEST(test_comment_only_file_parses_empty_ok);
    RUN_TEST(test_empty_string_parses_empty_ok);

    std::cout << "\n--- Malformed Input (Issue #105) ---" << std::endl;
    RUN_TEST(test_malformed_face_token_is_skipped_with_error);
    RUN_TEST(test_malformed_face_token_empty_pos_part_skipped);
    RUN_TEST(test_out_of_range_face_index_skipped_with_error);
    RUN_TEST(test_zero_index_in_face_is_out_of_range);
    RUN_TEST(test_incomplete_vertex_record_v_is_skipped);
    RUN_TEST(test_incomplete_vn_record_is_skipped);
    RUN_TEST(test_face_with_only_2_vertices_is_skipped);
    RUN_TEST(test_face_before_any_vertices_is_out_of_range);
    RUN_TEST(test_valid_face_after_invalid_face_is_parsed);
    RUN_TEST(test_face_token_with_trailing_garbage_is_invalid);

    return TEST_REPORT();
}
