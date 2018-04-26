#include "ogl_base.h"

//#include <ft2build.h>
//#include FT_FREETYPE_H
#include <stdlib.h>

#include "km_debug.h"
#include "km_defines.h"
#include "km_math.h"

#if 0

// TODO get this out of here
global_var Mat4 pixelToClip_;

void InitOpenGL()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
}

void ResizeGL(int width, int height)
{
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    Vec3 pixelScale = {
        2.0f / (float)width,
        2.0f / (float)height,
        1.0f
    };
    Vec3 view = { -1.0f, -1.0f, 0.0f };
    pixelToClip_ = Translate(view) * Scale(pixelScale);
}
#endif

internal bool CompileAndCheckShader(GLuint shaderID,
    DEBUGReadFileResult shaderFile)
{
    // Compile shader.
    GLint shaderFileSize = (GLint)shaderFile.size;
    glShaderSource(shaderID, 1, (const GLchar* const*)&shaderFile.data,
        &shaderFileSize);
    glCompileShader(shaderID);

    // Check shader.
    GLint result;
    int infoLogLength;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (result == GL_FALSE) {
        // TODO get rid of this malloc
        char* infoLog = (char*)malloc((size_t)(infoLogLength + 1));
        glGetShaderInfoLog(shaderID, infoLogLength, NULL, infoLog);
        infoLog[infoLogLength] = 0;
        DEBUG_PRINT("Shader compilation log:\n");
        DEBUG_PRINT("%s\n", infoLog);
        free(infoLog);

        return false;
    }

    return true;
}

GLuint LoadShaders(
	const ThreadContext* thread,
	DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
	DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory,
    const char* vertFilePath, const char* fragFilePath)
{
    // Create GL shaders.
    GLuint vertShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read shader code from files.
    DEBUGReadFileResult vertFile = DEBUGPlatformReadFile(thread, vertFilePath);
    if (vertFile.size == 0) {
        DEBUG_PRINT("Failed to read vertex shader file.\n");
        return 0; // TODO what to return
    }
    DEBUGReadFileResult fragFile = DEBUGPlatformReadFile(thread, fragFilePath);
    if (fragFile.size == 0) {
        DEBUG_PRINT("Failed to read fragment shader file.\n");
        return 0; // TODO what to return
    }

    // Compile and check shader code.
    // TODO error checking
    if (!CompileAndCheckShader(vertShaderID, vertFile)) {
        DEBUG_PRINT("Vertex shader compilation failed (%s)\n", vertFilePath);
        glDeleteShader(vertShaderID);
        glDeleteShader(fragShaderID);

        return 0; // TODO what to return
    }
    if (!CompileAndCheckShader(fragShaderID, fragFile)) {
        DEBUG_PRINT("Fragment shader compilation failed (%s)\n", fragFilePath);
        glDeleteShader(vertShaderID);
        glDeleteShader(fragShaderID);

        return 0; // TODO what to return
    }

    // Link the shader program.
    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertShaderID);
    glAttachShader(programID, fragShaderID);
    glLinkProgram(programID);

    // Check the shader program.
    GLint result;
    int infoLogLength;
    glGetProgramiv(programID, GL_LINK_STATUS, &result);
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (result == GL_FALSE) {
        // TODO get rid of this malloc
        char* infoLog = (char*)malloc((size_t)(infoLogLength + 1));
        glGetProgramInfoLog(programID, infoLogLength, NULL, infoLog);
        infoLog[infoLogLength] = 0;
        DEBUG_PRINT("Program linking failed:\n");
        DEBUG_PRINT("%s\n", infoLog);
        free(infoLog);

        return 0; // TODO what to return
    }

    glDetachShader(programID, vertShaderID);
    glDetachShader(programID, fragShaderID);
    glDeleteShader(vertShaderID);
    glDeleteShader(fragShaderID);

    DEBUGPlatformFreeFileMemory(thread, &vertFile);
    DEBUGPlatformFreeFileMemory(thread, &fragFile);

    return programID;
}

#if 0
RectGL CreateRectGL()
{
    RectGL rectGL;
    const GLfloat vertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f
    };

    glGenVertexArrays(1, &rectGL.vertexArray);
    glBindVertexArray(rectGL.vertexArray);

    glGenBuffers(1, &rectGL.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, rectGL.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, // match shader layout location
        3, // size (vec3)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
    );

    glBindVertexArray(0);

    rectGL.programID = LoadShaders(
        "shaders/rect.vert",
        "shaders/rect.frag");
    
    return rectGL;
}

TexturedRectGL CreateTexturedRectGL()
{
    TexturedRectGL texturedRectGL;
    // TODO probably use indexing for this
    const GLfloat vertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f
    };
    const GLfloat uvs[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f
    };

    glGenVertexArrays(1, &texturedRectGL.vertexArray);
    glBindVertexArray(texturedRectGL.vertexArray);

    glGenBuffers(1, &texturedRectGL.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texturedRectGL.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, // match shader layout location
        3, // size (vec3)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
    );

    glGenBuffers(1, &texturedRectGL.uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texturedRectGL.uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, // match shader layout location
        2, // size (vec2)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
    );

    glBindVertexArray(0);

    texturedRectGL.programID = LoadShaders(
        "shaders/texturedRect.vert",
        "shaders/texturedRect.frag");
    
    return texturedRectGL;
}

LineGL CreateLineGL()
{
    LineGL lineGL;
    const GLfloat vertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f
    };

    glGenVertexArrays(1, &lineGL.vertexArray);
    glBindVertexArray(lineGL.vertexArray);

    glGenBuffers(1, &lineGL.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, lineGL.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, // match shader layout location
        3, // size (vec3)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
    );

    glBindVertexArray(0);

    lineGL.programID = LoadShaders(
        "shaders/line.vert",
        "shaders/line.frag");
    
    return lineGL;
}

void DrawRect(
    RectGL rectGL,
    Vec3 pos, Vec2 anchor, Vec2 size, Vec4 color)
{
    GLint loc;
    glUseProgram(rectGL.programID);
    pos.x -= anchor.x * size.x;
    pos.y -= anchor.y * size.y;
    loc = glGetUniformLocation(rectGL.programID, "posBottomLeft");
    glUniform3fv(loc, 1, &pos.e[0]);
    loc = glGetUniformLocation(rectGL.programID, "size");
    glUniform2fv(loc, 1, &size.e[0]);
    loc = glGetUniformLocation(rectGL.programID, "color");
    glUniform4fv(loc, 1, &color.e[0]);
    loc = glGetUniformLocation(rectGL.programID, "pixelToClip");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &pixelToClip_.e[0][0]);

    glBindVertexArray(rectGL.vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void DrawTexturedRect(
    TexturedRectGL texturedRectGL,
    Vec3 pos, Vec2 anchor, Vec2 size, GLuint texture)
{
    GLint loc;
    glUseProgram(texturedRectGL.programID);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    loc = glGetUniformLocation(texturedRectGL.programID, "textureSampler");
    glUniform1i(loc, 0);

    pos.x -= anchor.x * size.x;
    pos.y -= anchor.y * size.y;
    loc = glGetUniformLocation(texturedRectGL.programID, "posBottomLeft");
    glUniform3fv(loc, 1, &pos.e[0]);
    loc = glGetUniformLocation(texturedRectGL.programID, "size");
    glUniform2fv(loc, 1, &size.e[0]);
    loc = glGetUniformLocation(texturedRectGL.programID, "pixelToClip");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &pixelToClip_.e[0][0]);

    glBindVertexArray(texturedRectGL.vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void DrawLine(
    LineGL lineGL, Mat4 proj, Mat4 view,
    Vec3 v1, Vec3 v2, Vec4 color)
{
    GLint loc;
    glUseProgram(lineGL.programID);

    Vec3 unitX = { 1.0f, 0.0f, 0.0f };
    Quat rot = QuatRotBetweenVectors(unitX, v2 - v1);
    float scale = Mag(v2 - v1);
    Mat4 model = Translate(v1) * UnitQuatToMat4(rot) * Scale(scale);
    Mat4 mvp = proj * view * model;
    //Mat4 mvp = Mat4::one;
    loc = glGetUniformLocation(lineGL.programID, "mvp");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &mvp.e[0][0]);
    loc = glGetUniformLocation(lineGL.programID, "color");
    glUniform4fv(loc, 1, &color.e[0]);

    glBindVertexArray(lineGL.vertexArray);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}
#endif