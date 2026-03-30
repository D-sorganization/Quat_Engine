#pragma once
/**
 * @file PostProcess.h
 * @brief Full-screen post-processing pipeline (CRT scanlines, chromatic
 *        aberration, vignette, film grain).
 *
 * Renders the scene to an off-screen framebuffer, then draws a full-screen
 * quad with the scene texture and applies screen-space effects via a
 * fragment shader.
 *
 * Uses the engine's own GL loader (GLLoader.h) — no external glad required.
 */

#include "GLLoader.h"
#include "Shader.h"

#include <iostream>
#include <memory>

// Additional FBO / renderbuffer constants not yet in GLLoader.h
#ifndef GL_FRAMEBUFFER
constexpr GLenum GL_FRAMEBUFFER              = 0x8D40;
constexpr GLenum GL_RENDERBUFFER             = 0x8D41;
constexpr GLenum GL_COLOR_ATTACHMENT0        = 0x8CE0;
constexpr GLenum GL_DEPTH_STENCIL_ATTACHMENT = 0x821A;
constexpr GLenum GL_DEPTH24_STENCIL8         = 0x88F0;
constexpr GLenum GL_FRAMEBUFFER_COMPLETE     = 0x8CD5;
#endif

// FBO function pointer types
using PFNGLGENFRAMEBUFFERSPROC         = void(QE_APIENTRY*)(GLsizei, GLuint*);
using PFNGLDELETEFRAMEBUFFERSPROC      = void(QE_APIENTRY*)(GLsizei, const GLuint*);
using PFNGLBINDFRAMEBUFFERPROC         = void(QE_APIENTRY*)(GLenum, GLuint);
using PFNGLFRAMEBUFFERTEXTURE2DPROC    = void(QE_APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);
using PFNGLCHECKFRAMEBUFFERSTATUSPROC  = GLenum(QE_APIENTRY*)(GLenum);
using PFNGLGENRENDERBUFFERSPROC        = void(QE_APIENTRY*)(GLsizei, GLuint*);
using PFNGLDELETERENDERBUFFERSPROC     = void(QE_APIENTRY*)(GLsizei, const GLuint*);
using PFNGLBINDRENDERBUFFERPROC        = void(QE_APIENTRY*)(GLenum, GLuint);
using PFNGLRENDERBUFFERSTORAGEPROC     = void(QE_APIENTRY*)(GLenum, GLenum, GLsizei, GLsizei);
using PFNGLFRAMEBUFFERRENDERBUFFERPROC = void(QE_APIENTRY*)(GLenum, GLenum, GLenum, GLuint);

namespace qe {
namespace renderer {

// Lazy-loaded FBO function pointers (loaded once on first use).
namespace detail {
    inline PFNGLGENFRAMEBUFFERSPROC         glGenFramebuffers         = nullptr;
    inline PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers      = nullptr;
    inline PFNGLBINDFRAMEBUFFERPROC         glBindFramebuffer         = nullptr;
    inline PFNGLFRAMEBUFFERTEXTURE2DPROC    glFramebufferTexture2D    = nullptr;
    inline PFNGLCHECKFRAMEBUFFERSTATUSPROC  glCheckFramebufferStatus  = nullptr;
    inline PFNGLGENRENDERBUFFERSPROC        glGenRenderbuffers        = nullptr;
    inline PFNGLDELETERENDERBUFFERSPROC     glDeleteRenderbuffers     = nullptr;
    inline PFNGLBINDRENDERBUFFERPROC        glBindRenderbuffer        = nullptr;
    inline PFNGLRENDERBUFFERSTORAGEPROC     glRenderbufferStorage     = nullptr;
    inline PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;

    inline bool load_fbo_functions() {
        #define QE_LOAD_FBO(name) \
            name = reinterpret_cast<decltype(name)>(SDL_GL_GetProcAddress(#name)); \
            if (!name) return false
        QE_LOAD_FBO(glGenFramebuffers);
        QE_LOAD_FBO(glDeleteFramebuffers);
        QE_LOAD_FBO(glBindFramebuffer);
        QE_LOAD_FBO(glFramebufferTexture2D);
        QE_LOAD_FBO(glCheckFramebufferStatus);
        QE_LOAD_FBO(glGenRenderbuffers);
        QE_LOAD_FBO(glDeleteRenderbuffers);
        QE_LOAD_FBO(glBindRenderbuffer);
        QE_LOAD_FBO(glRenderbufferStorage);
        QE_LOAD_FBO(glFramebufferRenderbuffer);
        #undef QE_LOAD_FBO
        return true;
    }
} // namespace detail

class PostProcess {
public:
    int crtEnabled        = 0;
    int aberrationEnabled = 0;
    int vignetteEnabled   = 0;
    int grainEnabled      = 0;
    float aberrationOffset = 0.005f;

    PostProcess(int init_width, int init_height)
        : width_(init_width), height_(init_height) {
        if (!detail::glGenFramebuffers) {
            detail::load_fbo_functions();
        }
        init_fbo();
        init_quad();
    }

    ~PostProcess() {
        using namespace detail;
        if (glDeleteFramebuffers)  glDeleteFramebuffers(1, &fbo_);
        if (glDeleteRenderbuffers) glDeleteRenderbuffers(1, &rbo_);
        gl::glDeleteTextures(1, &color_tex_);
        gl::glDeleteVertexArrays(1, &quad_vao_);
        gl::glDeleteBuffers(1, &quad_vbo_);
        post_shader_.destroy();
    }

    // Non-copyable, non-movable (owns GPU resources)
    PostProcess(const PostProcess&) = delete;
    PostProcess& operator=(const PostProcess&) = delete;

    /** Load the post-processing shader from files. */
    void init(const char* vert_path, const char* frag_path) {
        post_shader_.load_from_files(vert_path, frag_path);
    }

    /** Bind the off-screen framebuffer for scene rendering. */
    void bind() {
        detail::glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        gl::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        gl::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::glEnable(GL_DEPTH_TEST);
    }

    /** Unbind FBO and prepare for full-screen quad draw. */
    void unbind() {
        detail::glBindFramebuffer(GL_FRAMEBUFFER, 0);
        gl::glDisable(GL_DEPTH_TEST);
        gl::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        gl::glClear(GL_COLOR_BUFFER_BIT);
    }

    /** Resize internal buffers when window size changes. */
    void updateResolution(int w, int h) {
        if (w == width_ && h == height_) return;
        width_ = w;
        height_ = h;
        gl::glBindTexture(GL_TEXTURE_2D, color_tex_);
        gl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width_, height_,
                         0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        detail::glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
        detail::glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                                      width_, height_);
        detail::glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    /** Draw the full-screen quad with post-processing effects. */
    void render(float time) {
        if (post_shader_.program_id == 0) return;
        post_shader_.use();
        post_shader_.set_int("screenTexture", 0);
        post_shader_.set_float("uTime", time);
        post_shader_.set_int("uAberrationEnabled", aberrationEnabled);
        post_shader_.set_float("uAberrationOffset", aberrationOffset);
        post_shader_.set_int("uCRTEnabled", crtEnabled);
        post_shader_.set_int("uVignetteEnabled", vignetteEnabled);
        post_shader_.set_int("uGrainEnabled", grainEnabled);

        gl::glBindVertexArray(quad_vao_);
        gl::glBindTexture(GL_TEXTURE_2D, color_tex_);
        gl::glDrawArrays(GL_TRIANGLES, 0, 6);
        gl::glBindVertexArray(0);
        gl::glEnable(GL_DEPTH_TEST);
    }

private:
    GLuint fbo_       = 0;
    GLuint rbo_       = 0;
    GLuint color_tex_ = 0;
    GLuint quad_vao_  = 0;
    GLuint quad_vbo_  = 0;
    int    width_     = 0;
    int    height_    = 0;
    Shader post_shader_;

    void init_fbo() {
        using namespace detail;
        glGenFramebuffers(1, &fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

        gl::glGenTextures(1, &color_tex_);
        gl::glBindTexture(GL_TEXTURE_2D, color_tex_);
        gl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width_, height_,
                         0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            static_cast<GLint>(GL_LINEAR));
        gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                            static_cast<GLint>(GL_LINEAR));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, color_tex_, 0);

        glGenRenderbuffers(1, &rbo_);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                              width_, height_);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, rbo_);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            QE_LOG_ERROR("PostProcess") << "Framebuffer is not complete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void init_quad() {
        float quad_vertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        constexpr GLsizei STRIDE = 4 * static_cast<GLsizei>(sizeof(float));

        gl::glGenVertexArrays(1, &quad_vao_);
        gl::glGenBuffers(1, &quad_vbo_);
        gl::glBindVertexArray(quad_vao_);
        gl::glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
        gl::glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(sizeof(quad_vertices)),
                         &quad_vertices, GL_STATIC_DRAW);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, STRIDE, nullptr);
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, STRIDE,
                                  reinterpret_cast<void*>(2 * sizeof(float)));
        gl::glBindVertexArray(0);
    }
};

} // namespace renderer
} // namespace qe
