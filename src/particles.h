#pragma once

#include "km_math.h"
#include "opengl.h"
#include "main_platform.h"

#define MAX_PARTICLES 10000

struct ParticleSystemGL
{
    GLuint vertexArray;
    GLuint vertexBuffer;
    GLuint uvBuffer;
    GLuint posBuffer;
    GLuint programID;
};

struct Particle
{
    Vec3 pos;
    Vec3 vel;
};
struct ParticleDrawInfo
{
    Vec3 pos[MAX_PARTICLES];
};

struct ParticleSystem
{
    uint32 maxParticles;
    Particle particles[MAX_PARTICLES];
    bool active[MAX_PARTICLES];
    Vec3 gravity;
    // attractors

    ParticleDrawInfo drawInfo;
};

ParticleSystemGL InitParticleSystemGL(const ThreadContext* thread,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory);

void CreateParticleSystem(ParticleSystem* ps, uint32 maxParticles);
void UpdateParticleSystem(ParticleSystem* ps, float32 deltaTime);
void DrawParticleSystem(ParticleSystemGL psGL, ParticleSystem* ps,
    GLuint texture, Vec3 camRight, Vec3 camUp, Mat4 vp);