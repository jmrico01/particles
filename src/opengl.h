#pragma once

#include "km_defines.h"

#if defined(GAME_LINUX) && defined(GAME_PLATFORM_CODE)
#include <GL/gl.h>
#endif

#if !defined(GAME_PLATFORM_CODE) || defined(GAME_WIN32)

#define GL_FALSE					0
#define GL_TRUE						1

#define GL_BYTE						0x1400
#define GL_UNSIGNED_BYTE			0x1401
#define GL_SHORT					0x1402
#define GL_UNSIGNED_SHORT			0x1403
#define GL_INT						0x1404
#define GL_UNSIGNED_INT				0x1405
#define GL_FLOAT					0x1406
#define GL_DOUBLE					0x140A

#define GL_LINES					0x0001
#define GL_LINE_LOOP				0x0002
#define GL_LINE_STRIP				0x0003
#define GL_TRIANGLES				0x0004

#define GL_DEPTH_BUFFER_BIT			0x00000100
#define GL_STENCIL_BUFFER_BIT		0x00000400
#define GL_COLOR_BUFFER_BIT			0x00004000

#define GL_VENDOR					0x1F00
#define GL_RENDERER					0x1F01
#define GL_VERSION					0x1F02
#define GL_EXTENSIONS				0x1F03
#define GL_SHADING_LANGUAGE_VERSION	0x8B8C

#define GL_FRAGMENT_SHADER			0x8B30
#define GL_VERTEX_SHADER			0x8B31
#define GL_COMPILE_STATUS			0x8B81
#define GL_LINK_STATUS				0x8B82
#define GL_INFO_LOG_LENGTH			0x8B84

#define GL_ARRAY_BUFFER				0x8892
#define GL_ELEMENT_ARRAY_BUFFER		0x8893
#define GL_STREAM_DRAW				0x88E0
#define GL_STREAM_READ				0x88E1
#define GL_STREAM_COPY				0x88E2
#define GL_STATIC_DRAW				0x88E4
#define GL_STATIC_READ				0x88E5
#define GL_STATIC_COPY				0x88E6
#define GL_DYNAMIC_DRAW				0x88E8
#define GL_DYNAMIC_READ				0x88E9
#define GL_DYNAMIC_COPY				0x88EA

#define GL_BLEND					0x0BE2
#define GL_CULL_FACE				0x0B44
#define GL_CULL_FACE_MODE			0x0B45
#define GL_FRONT_FACE				0x0B46
#define GL_DEPTH_TEST				0x0B71

#define GL_NEVER					0x0200
#define GL_LESS						0x0201
#define GL_EQUAL					0x0202
#define GL_LEQUAL					0x0203
#define GL_GREATER					0x0204
#define GL_NOTEQUAL					0x0205
#define GL_GEQUAL					0x0206
#define GL_ALWAYS					0x0207
#define GL_ZERO						0
#define GL_ONE						1
#define GL_SRC_COLOR				0x0300
#define GL_ONE_MINUS_SRC_COLOR		0x0301
#define GL_SRC_ALPHA				0x0302
#define GL_ONE_MINUS_SRC_ALPHA		0x0303
#define GL_DST_ALPHA				0x0304
#define GL_ONE_MINUS_DST_ALPHA		0x0305
#define GL_DST_COLOR				0x0306
#define GL_ONE_MINUS_DST_COLOR		0x0307

typedef void	GLvoid;

typedef bool	GLboolean;

typedef char	GLchar;
typedef int8	GLbyte;
typedef int16	GLshort;
typedef int32	GLint;
typedef int64	GLint64;

typedef uint8	GLubyte;
typedef uint16	GLushort;
typedef uint32	GLuint;
typedef uint64	GLuint64;

typedef uint32	GLenum;
typedef uint32	GLbitfield;

typedef float32	GLfloat;
typedef float32	GLclampf;
typedef float64	GLdouble;
typedef float64	GLclampd;

typedef uint32	GLsizei;
typedef size_t	GLsizeiptr;

#endif

// X Macro trickery for declaring required OpenGL functions
// The general FUNC macro has the form
//		FUNC( return type, function name, arg1, arg2, arg3, arg4, ... )
// This macro will be used for:
//	- Declaring the functions
//	- Declaring pointers to the functions in struct OpenGLFunctions
//	- Loading the functions in platform layers
//	- More stuff, probably, as time goes on
#define GL_FUNCTIONS_BASE \
	FUNC(void,				glViewport,		GLint x, GLint y, \
                                            GLsizei width, GLsizei height) \
	FUNC(const GLubyte*,	glGetString,	GLenum name) \
	FUNC(void,				glClear,		GLbitfield mask) \
	FUNC(void,				glClearColor,	GLclampf r, GLclampf g, \
                                            GLclampf b, GLclampf a) \
	FUNC(void,				glClearDepth,	GLdouble depth)

#define GL_FUNCTIONS_ALL \
	FUNC(void,	glEnable, GLenum cap) \
	FUNC(void,	glDisable, GLenum cap) \
	FUNC(void,	glBlendFunc, GLenum sfactor, GLenum dfactor) \
	FUNC(void,	glDepthFunc, GLenum func) \
	FUNC(void,	glDepthRange, GLdouble near, GLdouble far) \
\
	FUNC(GLuint, glCreateShader, GLenum type) \
	FUNC(GLuint, glCreateProgram) \
	FUNC(void,	glShaderSource, GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) \
	FUNC(void,	glCompileShader, GLuint shader) \
	FUNC(void,	glGetShaderiv, GLuint shader, GLenum pname, GLint* params) \
	FUNC(void,	glGetShaderInfoLog, GLuint shader, GLsizei bufSize, GLsizei *length, GLchar* infoLog) \
	FUNC(void,	glGetProgramInfoLog, GLuint program, GLsizei bufSize, GLsizei *length, GLchar* infoLog) \
	FUNC(void,	glAttachShader, GLuint program, GLuint shader) \
	FUNC(void,	glLinkProgram, GLuint program) \
	FUNC(void,	glGetProgramiv, GLuint program, GLenum pname, GLint *params) \
	FUNC(void,	glDetachShader, GLuint program, GLuint shader) \
	FUNC(void,	glDeleteProgram, GLuint program) \
	FUNC(void,	glDeleteShader, GLuint shader) \
\
	FUNC(void,	glGenBuffers, GLsizei n, GLuint* buffers) \
	FUNC(void,	glBindBuffer, GLenum target, GLuint buffer) \
	FUNC(void,	glBufferData, GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) \
	FUNC(void,	glBindVertexArray, GLuint array) \
	FUNC(void,	glDeleteVertexArrays, GLsizei n, const GLuint* arrays) \
	FUNC(void,	glGenVertexArrays, GLsizei n, GLuint* arrays) \
	FUNC(void,	glEnableVertexAttribArray, GLuint index) \
	FUNC(void,	glVertexAttribPointer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer) \
\
	FUNC(void,	glUseProgram, GLuint program) \
	FUNC(GLint,	glGetUniformLocation, GLuint program, const GLchar* name) \
	FUNC(void,	glUniform1f, GLint location, GLfloat v0) \
	FUNC(void,	glUniform2f, GLint location, GLfloat v0, GLfloat v1) \
	FUNC(void,	glUniform3f, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) \
	FUNC(void,	glUniform4f, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) \
	FUNC(void,	glUniform1i, GLint location, GLint v0) \
	FUNC(void,	glUniform2i, GLint location, GLint v0, GLint v1) \
	FUNC(void,	glUniform3i, GLint location, GLint v0, GLint v1, GLint v2) \
	FUNC(void,	glUniform4i, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) \
	FUNC(void,	glUniform1fv, GLint location, GLsizei count, const GLfloat *value) \
	FUNC(void,	glUniform2fv, GLint location, GLsizei count, const GLfloat *value) \
	FUNC(void,	glUniform3fv, GLint location, GLsizei count, const GLfloat *value) \
	FUNC(void,	glUniform4fv, GLint location, GLsizei count, const GLfloat *value) \
	FUNC(void,	glUniform1iv, GLint location, GLsizei count, const GLint *value) \
	FUNC(void,	glUniform2iv, GLint location, GLsizei count, const GLint *value) \
	FUNC(void,	glUniform3iv, GLint location, GLsizei count, const GLint *value) \
	FUNC(void,	glUniform4iv, GLint location, GLsizei count, const GLint *value) \
	FUNC(void,	glUniformMatrix2fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
	FUNC(void,	glUniformMatrix3fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
	FUNC(void,	glUniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
	FUNC(void,	glDrawArrays, GLenum mode, GLint first, GLsizei count) \
	FUNC(void,	glDrawElements, GLenum mode, GLsizei count, GLenum type, const void *indices)

// Generate function declarations
#define FUNC(returntype, name, ...) \
    typedef returntype name##Func ( __VA_ARGS__ );
GL_FUNCTIONS_BASE
GL_FUNCTIONS_ALL
#undef FUNC

struct OpenGLFunctions
{
	// Generate function pointers
#define FUNC(returntype, name, ...) name##Func* name;
	GL_FUNCTIONS_BASE
	GL_FUNCTIONS_ALL
#undef FUNC
};