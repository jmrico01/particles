#pragma once

#include "km_math.h"
#include "opengl.h"
#include "ogl_base.h"
#include "main_platform.h"
#include "mesh.h"

#define MAX_PARTICLES 100000
#define MAX_SPAWN (MAX_PARTICLES / 10)
#define MAX_ATTRACTORS 50
#define MAX_COLLIDERS 20

enum ColliderType
{
    COLLIDER_SINK,
    COLLIDER_BOUNCE
};

struct Particle
{
    float32 life;
    Vec3 pos;
    Vec3 vel;
    Vec4 color;
    Vec2 size;
    float32 bounceMult;
    float32 frictionMult;

    float32 depth;
};

struct Attractor
{
    Vec3 pos;
    float32 strength;
};

struct PlaneCollider
{
    ColliderType type;

    Vec3 normal;
    Vec3 point;
};
struct AxisBoxCollider
{
    ColliderType type;

    Vec3 min;
    Vec3 max;
};
struct SphereCollider
{
    ColliderType type;

    Vec3 center;
    float32 radius;
};

struct ParticleSystem;
typedef void (*InitParticleFunction)(ParticleSystem*, Particle*, void* data);

struct ParticleSystem
{
    Particle particles[MAX_PARTICLES];
    float32 spawnCounter;
    int active;

    int maxParticles;
    int particlesPerSec;
    float32 maxLife;
    Vec3 gravity;
    float32 linearDamp;
    float32 quadraticDamp;

    Attractor attractors[MAX_ATTRACTORS];
    int numAttractors;

    PlaneCollider planeColliders[MAX_COLLIDERS];
    int numPlaneColliders;

    AxisBoxCollider boxColliders[MAX_COLLIDERS];
    int numBoxColliders;

    SphereCollider sphereColliders[MAX_COLLIDERS];
    int numSphereColliders;

    InitParticleFunction initParticleFunc;

    GLuint texture;

    Mesh* mesh;
    MeshGL* meshGL;

    // Grid mode
    int width, height;
    float32 hookeEqDist;
    float32 hookeStrength;
};

struct ParticleSystemGL
{
    GLuint vertexArray;
    GLuint vertexBuffer;
    GLuint uvBuffer;
    GLuint posBuffer;
    GLuint colorBuffer;
    GLuint sizeBuffer;
    GLuint programID;
};

struct ParticleSystemDataGL
{
    Vec3 pos[MAX_PARTICLES];
    Vec4 color[MAX_PARTICLES];
    Vec2 size[MAX_PARTICLES];
};

ParticleSystemGL InitParticleSystemGL(const ThreadContext* thread,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory);

void CreateParticleSystem(ParticleSystem* ps, int maxParticles,
    int particlesPerSec, float32 maxLife, Vec3 gravity,
    float32 linearDamp, float32 quadraticDamp,
    Attractor* attractors, int numAttractors,
    PlaneCollider* planeColliders, int numPlaneColliders,
    AxisBoxCollider* boxColliders, int numBoxColliders,
    SphereCollider* sphereColliders, int numSphereColliders,
    InitParticleFunction initParticleFunc, GLuint texture,
    Mesh* mesh, MeshGL* meshGL);
void CreateParticleSystem(ParticleSystem* ps,
    int width, int height, Vec3 origin, Vec3 strideX, Vec3 strideY,
    Vec3 gravity, float32 hookeStrength, float32 hookeEqDist,
    float32 linearDamp, float32 quadraticDamp,
    Attractor* attractors, int numAttractors,
    PlaneCollider* planeColliders, int numPlaneColliders,
    AxisBoxCollider* boxColliders, int numBoxColliders,
    SphereCollider* sphereColliders, int numSphereColliders,
    GLuint texture);
void UpdateParticleSystem(ParticleSystem* ps, float32 deltaTime, void* data);
void DrawParticleSystem(ParticleSystemGL psGL,
    PlaneGL planeGL, BoxGL boxGL, MeshGL sphereMeshGL,
    ParticleSystem* ps,
    Vec3 camRight, Vec3 camUp, Vec3 camPos, Mat4 proj, Mat4 view,
    ParticleSystemDataGL* dataGL,
    bool32 drawColliders);