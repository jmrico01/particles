#pragma once

#include "km_lib.h"
#include "km_math.h"
#include "main_platform.h"

#define MAX_TRIANGLES 500000

struct Triangle
{
    Vec3 v[3];
    Vec2 uv[3];
    Vec3 n[3];
    float32 area;
};

struct Mesh
{
    DynamicArray<Triangle> triangles;
};

struct MeshGL
{
    GLuint vertexArray;
    GLuint vertexBuffer;
    GLuint normalBuffer;
    GLuint programID;
    int vertexCount;
};

Mesh LoadMeshFromObj(const ThreadContext* thread,
    const char* fileName,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory);
void FreeMesh(Mesh* mesh);

MeshGL LoadMeshGL(const ThreadContext* thread, const Mesh& mesh,
    DEBUGPlatformReadFileFunc DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc DEBUGPlatformFreeFileMemory);
void DrawMeshGL(const MeshGL& meshGL, Mat4 proj, Mat4 view, Vec4 color);
void FreeMeshGL(MeshGL* meshGL);
