#pragma once

#include "main_platform.h"
#include "opengl.h"
#include "km_math.h"

struct RectGL
{
    GLuint vertexArray;
    GLuint vertexBuffer;
    GLuint programID;
};

struct TexturedRectGL
{
    GLuint vertexArray;
    GLuint vertexBuffer;
    GLuint uvBuffer;
    GLuint programID;
};

struct LineGL
{
    GLuint vertexArray;
    GLuint vertexBuffer;
    GLuint programID;
};

GLuint LoadShaders(const ThreadContext* thread,
	DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
	DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory,
    const char* vertFilePath, const char* fragFilePath);

RectGL InitRectGL(const ThreadContext* thread,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory);
TexturedRectGL InitTexturedRectGL(const ThreadContext* thread,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory);
LineGL InitLineGL(const ThreadContext* thread,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory);

void DrawRect(RectGL rectGL, ScreenInfo screenInfo,
    Vec2Int pos, Vec2 anchor, Vec2Int size, Vec4 color);
void DrawTexturedRect(TexturedRectGL texturedRectGL, ScreenInfo screenInfo,
    Vec2Int pos, Vec2 anchor, Vec2Int size, GLuint texture);
void DrawLine(LineGL lineGL,
    Mat4 proj, Mat4 view, Vec3 v1, Vec3 v2, Vec4 color);