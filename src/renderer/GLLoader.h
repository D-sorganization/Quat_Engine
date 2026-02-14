#pragma once
/**
 * @file GLLoader.h
 * @brief Minimal OpenGL 3.3 Core Profile function loader.
 *
 * Loads GL function pointers via SDL_GL_GetProcAddress.
 * Covers the subset of GL 3.3 needed for basic mesh rendering:
 *   - Shaders (compile, link, uniforms)
 *   - Buffers (VAO, VBO, EBO)
 *   - Drawing (drawElements, drawArrays)
 *   - State (viewport, clear, enable, blend)
 *
 * Usage:
 *   // After creating SDL GL context:
 *   if (!qe::renderer::gl::load()) { // handle error }
 */

#include <SDL.h>

// ── OpenGL Types ────────────────────────────────────────────────────────────
using GLboolean  = unsigned char;
using GLchar     = char;
using GLint      = int;
using GLuint     = unsigned int;
using GLsizei    = int;
using GLenum     = unsigned int;
using GLfloat    = float;
using GLbitfield = unsigned int;
using GLsizeiptr = ptrdiff_t;
using GLvoid     = void;

// ── OpenGL Constants ────────────────────────────────────────────────────────

// Boolean
constexpr GLboolean GL_TRUE  = 1;
constexpr GLboolean GL_FALSE = 0;

// Buffer bits
constexpr GLbitfield GL_COLOR_BUFFER_BIT   = 0x00004000;
constexpr GLbitfield GL_DEPTH_BUFFER_BIT   = 0x00000100;
constexpr GLbitfield GL_STENCIL_BUFFER_BIT = 0x00000400;

// Capabilities
constexpr GLenum GL_DEPTH_TEST    = 0x0B71;
constexpr GLenum GL_CULL_FACE     = 0x0B44;
constexpr GLenum GL_BLEND         = 0x0BE2;
constexpr GLenum GL_BACK          = 0x0405;
constexpr GLenum GL_FRONT         = 0x0404;

// Blend funcs
constexpr GLenum GL_SRC_ALPHA           = 0x0302;
constexpr GLenum GL_ONE_MINUS_SRC_ALPHA = 0x0303;

// Primitive types
constexpr GLenum GL_TRIANGLES = 0x0004;
constexpr GLenum GL_LINES     = 0x0001;

// Data types
constexpr GLenum GL_FLOAT        = 0x1406;
constexpr GLenum GL_UNSIGNED_INT = 0x1405;
constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;

// Shader types
constexpr GLenum GL_VERTEX_SHADER   = 0x8B31;
constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;
constexpr GLenum GL_COMPILE_STATUS  = 0x8B81;
constexpr GLenum GL_LINK_STATUS     = 0x8B82;
constexpr GLenum GL_INFO_LOG_LENGTH = 0x8B84;

// Buffer targets
constexpr GLenum GL_ARRAY_BUFFER         = 0x8892;
constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;

// Buffer usage
constexpr GLenum GL_STATIC_DRAW  = 0x88E4;
constexpr GLenum GL_DYNAMIC_DRAW = 0x88E8;

// Texture
constexpr GLenum GL_TEXTURE_2D = 0x0DE1;
constexpr GLenum GL_TEXTURE0   = 0x84C0;
constexpr GLenum GL_TEXTURE_WRAP_S     = 0x2802;
constexpr GLenum GL_TEXTURE_WRAP_T     = 0x2803;
constexpr GLenum GL_TEXTURE_MIN_FILTER = 0x2801;
constexpr GLenum GL_TEXTURE_MAG_FILTER = 0x2800;
constexpr GLenum GL_REPEAT             = 0x2901;
constexpr GLenum GL_CLAMP_TO_EDGE      = 0x812F;
constexpr GLenum GL_LINEAR             = 0x2601;
constexpr GLenum GL_NEAREST            = 0x2600;
constexpr GLenum GL_LINEAR_MIPMAP_LINEAR = 0x2703;
constexpr GLenum GL_RGB  = 0x1907;
constexpr GLenum GL_RGBA = 0x1908;

// Polygon mode
constexpr GLenum GL_FILL = 0x1B02;
constexpr GLenum GL_LINE = 0x1B01;
constexpr GLenum GL_FRONT_AND_BACK = 0x0408;

// Error
constexpr GLenum GL_NO_ERROR = 0;

// String queries
constexpr GLenum GL_VERSION  = 0x1F02;
constexpr GLenum GL_RENDERER = 0x1F01;
constexpr GLenum GL_VENDOR   = 0x1F00;

// ── Function Pointer Typedefs ───────────────────────────────────────────────
// Using C calling convention (APIENTRY on Windows = __stdcall)
#ifdef _WIN32
#define QE_APIENTRY __stdcall
#else
#define QE_APIENTRY
#endif

// Core GL 1.x (may be directly available, but we load anyway)
using PFNGLCLEARPROC          = void(QE_APIENTRY*)(GLbitfield);
using PFNGLCLEARCOLORPROC     = void(QE_APIENTRY*)(GLfloat, GLfloat, GLfloat, GLfloat);
using PFNGLVIEWPORTPROC       = void(QE_APIENTRY*)(GLint, GLint, GLsizei, GLsizei);
using PFNGLENABLEPROC         = void(QE_APIENTRY*)(GLenum);
using PFNGLDISABLEPROC        = void(QE_APIENTRY*)(GLenum);
using PFNGLBLENDFUNCPROC      = void(QE_APIENTRY*)(GLenum, GLenum);
using PFNGLCULLFACEPROC       = void(QE_APIENTRY*)(GLenum);
using PFNGLPOLYGONMODEPROC    = void(QE_APIENTRY*)(GLenum, GLenum);
using PFNGLDRAWARRAYSPROC     = void(QE_APIENTRY*)(GLenum, GLint, GLsizei);
using PFNGLDRAWELEMENTSPROC   = void(QE_APIENTRY*)(GLenum, GLsizei, GLenum, const void*);
using PFNGLGETSTRINGPROC      = const GLchar*(QE_APIENTRY*)(GLenum);
using PFNGLGETERRORPROC       = GLenum(QE_APIENTRY*)();
using PFNGLDEPTHMASKPROC      = void(QE_APIENTRY*)(GLboolean);
using PFNGLLINEWIDTHPROC      = void(QE_APIENTRY*)(GLfloat);

// Shader functions
using PFNGLCREATESHADERPROC      = GLuint(QE_APIENTRY*)(GLenum);
using PFNGLSHADERSOURCEPROC      = void(QE_APIENTRY*)(GLuint, GLsizei, const GLchar**, const GLint*);
using PFNGLCOMPILESHADERPROC     = void(QE_APIENTRY*)(GLuint);
using PFNGLGETSHADERIVPROC       = void(QE_APIENTRY*)(GLuint, GLenum, GLint*);
using PFNGLGETSHADERINFOLOGPROC  = void(QE_APIENTRY*)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFNGLDELETESHADERPROC      = void(QE_APIENTRY*)(GLuint);
using PFNGLCREATEPROGRAMPROC     = GLuint(QE_APIENTRY*)();
using PFNGLATTACHSHADERPROC      = void(QE_APIENTRY*)(GLuint, GLuint);
using PFNGLLINKPROGRAMPROC       = void(QE_APIENTRY*)(GLuint);
using PFNGLGETPROGRAMIVPROC      = void(QE_APIENTRY*)(GLuint, GLenum, GLint*);
using PFNGLGETPROGRAMINFOLOGPROC = void(QE_APIENTRY*)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFNGLUSEPROGRAMPROC        = void(QE_APIENTRY*)(GLuint);
using PFNGLDELETEPROGRAMPROC     = void(QE_APIENTRY*)(GLuint);

// Uniform functions
using PFNGLGETUNIFORMLOCATIONPROC = GLint(QE_APIENTRY*)(GLuint, const GLchar*);
using PFNGLUNIFORM1FPROC          = void(QE_APIENTRY*)(GLint, GLfloat);
using PFNGLUNIFORM3FPROC          = void(QE_APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat);
using PFNGLUNIFORM4FPROC          = void(QE_APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
using PFNGLUNIFORMMATRIX4FVPROC   = void(QE_APIENTRY*)(GLint, GLsizei, GLboolean, const GLfloat*);

// VAO functions
using PFNGLGENVERTEXARRAYSPROC    = void(QE_APIENTRY*)(GLsizei, GLuint*);
using PFNGLBINDVERTEXARRAYPROC    = void(QE_APIENTRY*)(GLuint);
using PFNGLDELETEVERTEXARRAYSPROC = void(QE_APIENTRY*)(GLsizei, const GLuint*);

// VBO functions
using PFNGLGENBUFFERSPROC             = void(QE_APIENTRY*)(GLsizei, GLuint*);
using PFNGLBINDBUFFERPROC             = void(QE_APIENTRY*)(GLenum, GLuint);
using PFNGLBUFFERDATAPROC             = void(QE_APIENTRY*)(GLenum, GLsizeiptr, const void*, GLenum);
using PFNGLDELETEBUFFERSPROC          = void(QE_APIENTRY*)(GLsizei, const GLuint*);
using PFNGLENABLEVERTEXATTRIBARRAYPROC = void(QE_APIENTRY*)(GLuint);
using PFNGLVERTEXATTRIBPOINTERPROC    = void(QE_APIENTRY*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);

// Texture functions
using PFNGLGENTEXTURESPROC     = void(QE_APIENTRY*)(GLsizei, GLuint*);
using PFNGLBINDTEXTUREPROC     = void(QE_APIENTRY*)(GLenum, GLuint);
using PFNGLTEXIMAGE2DPROC      = void(QE_APIENTRY*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
using PFNGLTEXPARAMETERIPROC   = void(QE_APIENTRY*)(GLenum, GLenum, GLint);
using PFNGLACTIVETEXTUREPROC   = void(QE_APIENTRY*)(GLenum);
using PFNGLGENERATEMIPMAPPROC  = void(QE_APIENTRY*)(GLenum);
using PFNGLDELETETEXTURESPROC  = void(QE_APIENTRY*)(GLsizei, const GLuint*);
using PFNGLUNIFORM1IPROC       = void(QE_APIENTRY*)(GLint, GLint);

// ── Global Function Pointers ────────────────────────────────────────────────
namespace qe {
namespace renderer {
namespace gl {

// Core
inline PFNGLCLEARPROC         glClear         = nullptr;
inline PFNGLCLEARCOLORPROC    glClearColor    = nullptr;
inline PFNGLVIEWPORTPROC      glViewport      = nullptr;
inline PFNGLENABLEPROC        glEnable        = nullptr;
inline PFNGLDISABLEPROC       glDisable       = nullptr;
inline PFNGLBLENDFUNCPROC     glBlendFunc     = nullptr;
inline PFNGLCULLFACEPROC      glCullFace      = nullptr;
inline PFNGLPOLYGONMODEPROC   glPolygonMode   = nullptr;
inline PFNGLDRAWARRAYSPROC    glDrawArrays    = nullptr;
inline PFNGLDRAWELEMENTSPROC  glDrawElements  = nullptr;
inline PFNGLGETSTRINGPROC     glGetString     = nullptr;
inline PFNGLGETERRORPROC      glGetError      = nullptr;
inline PFNGLDEPTHMASKPROC     glDepthMask     = nullptr;
inline PFNGLLINEWIDTHPROC     glLineWidth     = nullptr;

// Shaders
inline PFNGLCREATESHADERPROC      glCreateShader      = nullptr;
inline PFNGLSHADERSOURCEPROC      glShaderSource      = nullptr;
inline PFNGLCOMPILESHADERPROC     glCompileShader     = nullptr;
inline PFNGLGETSHADERIVPROC       glGetShaderiv       = nullptr;
inline PFNGLGETSHADERINFOLOGPROC  glGetShaderInfoLog  = nullptr;
inline PFNGLDELETESHADERPROC      glDeleteShader      = nullptr;
inline PFNGLCREATEPROGRAMPROC     glCreateProgram     = nullptr;
inline PFNGLATTACHSHADERPROC      glAttachShader      = nullptr;
inline PFNGLLINKPROGRAMPROC       glLinkProgram       = nullptr;
inline PFNGLGETPROGRAMIVPROC      glGetProgramiv      = nullptr;
inline PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
inline PFNGLUSEPROGRAMPROC        glUseProgram        = nullptr;
inline PFNGLDELETEPROGRAMPROC     glDeleteProgram     = nullptr;

// Uniforms
inline PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
inline PFNGLUNIFORM1FPROC          glUniform1f          = nullptr;
inline PFNGLUNIFORM3FPROC          glUniform3f          = nullptr;
inline PFNGLUNIFORM4FPROC          glUniform4f          = nullptr;
inline PFNGLUNIFORMMATRIX4FVPROC   glUniformMatrix4fv   = nullptr;

// VAO
inline PFNGLGENVERTEXARRAYSPROC    glGenVertexArrays    = nullptr;
inline PFNGLBINDVERTEXARRAYPROC    glBindVertexArray    = nullptr;
inline PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;

// VBO
inline PFNGLGENBUFFERSPROC              glGenBuffers              = nullptr;
inline PFNGLBINDBUFFERPROC              glBindBuffer              = nullptr;
inline PFNGLBUFFERDATAPROC              glBufferData              = nullptr;
inline PFNGLDELETEBUFFERSPROC           glDeleteBuffers           = nullptr;
inline PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
inline PFNGLVERTEXATTRIBPOINTERPROC    glVertexAttribPointer     = nullptr;

// Textures
inline PFNGLGENTEXTURESPROC     glGenTextures     = nullptr;
inline PFNGLBINDTEXTUREPROC     glBindTexture     = nullptr;
inline PFNGLTEXIMAGE2DPROC      glTexImage2D      = nullptr;
inline PFNGLTEXPARAMETERIPROC   glTexParameteri   = nullptr;
inline PFNGLACTIVETEXTUREPROC   glActiveTexture   = nullptr;
inline PFNGLGENERATEMIPMAPPROC  glGenerateMipmap  = nullptr;
inline PFNGLDELETETEXTURESPROC  glDeleteTextures  = nullptr;
inline PFNGLUNIFORM1IPROC       glUniform1i       = nullptr;

// ── Loader Function ─────────────────────────────────────────────────────────

/**
 * Load all OpenGL 3.3 core function pointers via SDL.
 * Must be called AFTER SDL_GL_CreateContext().
 * @return true if all required functions were loaded.
 */
inline bool load() {
    #define QE_LOAD_GL(name) \
        name = reinterpret_cast<decltype(name)>(SDL_GL_GetProcAddress(#name)); \
        if (!name) return false

    // Core
    QE_LOAD_GL(glClear);
    QE_LOAD_GL(glClearColor);
    QE_LOAD_GL(glViewport);
    QE_LOAD_GL(glEnable);
    QE_LOAD_GL(glDisable);
    QE_LOAD_GL(glBlendFunc);
    QE_LOAD_GL(glCullFace);
    QE_LOAD_GL(glPolygonMode);
    QE_LOAD_GL(glDrawArrays);
    QE_LOAD_GL(glDrawElements);
    QE_LOAD_GL(glGetString);
    QE_LOAD_GL(glGetError);
    QE_LOAD_GL(glDepthMask);
    QE_LOAD_GL(glLineWidth);

    // Shaders
    QE_LOAD_GL(glCreateShader);
    QE_LOAD_GL(glShaderSource);
    QE_LOAD_GL(glCompileShader);
    QE_LOAD_GL(glGetShaderiv);
    QE_LOAD_GL(glGetShaderInfoLog);
    QE_LOAD_GL(glDeleteShader);
    QE_LOAD_GL(glCreateProgram);
    QE_LOAD_GL(glAttachShader);
    QE_LOAD_GL(glLinkProgram);
    QE_LOAD_GL(glGetProgramiv);
    QE_LOAD_GL(glGetProgramInfoLog);
    QE_LOAD_GL(glUseProgram);
    QE_LOAD_GL(glDeleteProgram);

    // Uniforms
    QE_LOAD_GL(glGetUniformLocation);
    QE_LOAD_GL(glUniform1f);
    QE_LOAD_GL(glUniform3f);
    QE_LOAD_GL(glUniform4f);
    QE_LOAD_GL(glUniformMatrix4fv);

    // VAO
    QE_LOAD_GL(glGenVertexArrays);
    QE_LOAD_GL(glBindVertexArray);
    QE_LOAD_GL(glDeleteVertexArrays);

    // VBO
    QE_LOAD_GL(glGenBuffers);
    QE_LOAD_GL(glBindBuffer);
    QE_LOAD_GL(glBufferData);
    QE_LOAD_GL(glDeleteBuffers);
    QE_LOAD_GL(glEnableVertexAttribArray);
    QE_LOAD_GL(glVertexAttribPointer);

    // Textures
    QE_LOAD_GL(glGenTextures);
    QE_LOAD_GL(glBindTexture);
    QE_LOAD_GL(glTexImage2D);
    QE_LOAD_GL(glTexParameteri);
    QE_LOAD_GL(glActiveTexture);
    QE_LOAD_GL(glGenerateMipmap);
    QE_LOAD_GL(glDeleteTextures);
    QE_LOAD_GL(glUniform1i);

    #undef QE_LOAD_GL
    return true;
}

} // namespace gl
} // namespace renderer
} // namespace qe
