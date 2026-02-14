#pragma once
/**
 * @file Shader.h
 * @brief OpenGL shader program compilation and uniform management.
 */

#include "GLLoader.h"
#include "../math/Mat4.h"
#include "../math/Vec3.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace qe {
namespace renderer {

class Shader {
public:
    GLuint program_id = 0;

    Shader() = default;

    /** Compile and link from source strings. */
    bool compile(const std::string& vertex_src, const std::string& fragment_src) {
        GLuint vert = compile_stage(GL_VERTEX_SHADER, vertex_src);
        if (!vert) return false;

        GLuint frag = compile_stage(GL_FRAGMENT_SHADER, fragment_src);
        if (!frag) {
            gl::glDeleteShader(vert);
            return false;
        }

        program_id = gl::glCreateProgram();
        gl::glAttachShader(program_id, vert);
        gl::glAttachShader(program_id, frag);
        gl::glLinkProgram(program_id);

        GLint success = 0;
        gl::glGetProgramiv(program_id, GL_LINK_STATUS, &success);
        if (!success) {
            char log[512];
            gl::glGetProgramInfoLog(program_id, 512, nullptr, log);
            std::cerr << "[Shader] Link error: " << log << std::endl;
            gl::glDeleteProgram(program_id);
            program_id = 0;
        }

        gl::glDeleteShader(vert);
        gl::glDeleteShader(frag);
        return program_id != 0;
    }

    /** Compile from shader files. */
    bool load_from_files(const std::string& vert_path, const std::string& frag_path) {
        std::string vert_src = read_file(vert_path);
        std::string frag_src = read_file(frag_path);

        if (vert_src.empty() || frag_src.empty()) {
            std::cerr << "[Shader] Failed to read shader files" << std::endl;
            return false;
        }
        return compile(vert_src, frag_src);
    }

    void use() const {
        gl::glUseProgram(program_id);
    }

    void destroy() {
        if (program_id) {
            gl::glDeleteProgram(program_id);
            program_id = 0;
        }
    }

    // --- Uniform Setters ---

    void set_float(const std::string& name, float value) const {
        gl::glUniform1f(gl::glGetUniformLocation(program_id, name.c_str()), value);
    }

    void set_vec3(const std::string& name, const math::Vec3& v) const {
        gl::glUniform3f(gl::glGetUniformLocation(program_id, name.c_str()),
                        v.x, v.y, v.z);
    }

    void set_int(const std::string& name, int value) const {
        gl::glUniform1i(gl::glGetUniformLocation(program_id, name.c_str()), value);
    }

    void set_mat4(const std::string& name, const math::Mat4& m) const {
        gl::glUniformMatrix4fv(
            gl::glGetUniformLocation(program_id, name.c_str()),
            1, GL_FALSE, m.data()
        );
    }

private:
    static GLuint compile_stage(GLenum type, const std::string& source) {
        GLuint shader = gl::glCreateShader(type);
        const char* src = source.c_str();
        gl::glShaderSource(shader, 1, &src, nullptr);
        gl::glCompileShader(shader);

        GLint success = 0;
        gl::glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[512];
            gl::glGetShaderInfoLog(shader, 512, nullptr, log);
            const char* type_name = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
            std::cerr << "[Shader] " << type_name << " compile error: " << log << std::endl;
            gl::glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    static std::string read_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[Shader] Cannot open: " << path << std::endl;
            return "";
        }
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
};

} // namespace renderer
} // namespace qe
