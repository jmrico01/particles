#include "particles.h"

#include <stdlib.h>

#include "km_debug.h"
#include "ogl_base.h"
#include "opengl_funcs.h"

#define PARTICLE_EPS 0.0001f
#define BOUNCE_MARGIN 0.001f

ParticleSystemGL InitParticleSystemGL(const ThreadContext* thread,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory)
{
    ParticleSystemGL psGL;
    const GLfloat vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f
    };
    const GLfloat uvs[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    glGenVertexArrays(1, &psGL.vertexArray);
    glBindVertexArray(psGL.vertexArray);

    glGenBuffers(1, &psGL.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, psGL.vertexBuffer);
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
    glVertexAttribDivisor(0, 0);

    glGenBuffers(1, &psGL.uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, psGL.uvBuffer);
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
    glVertexAttribDivisor(1, 0);

    glGenBuffers(1, &psGL.posBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, psGL.posBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(Vec3), NULL,
        GL_STREAM_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, // match shader layout location
        3, // size (vec3)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
    );
    glVertexAttribDivisor(2, 1);

    glGenBuffers(1, &psGL.colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, psGL.colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(Vec4), NULL,
        GL_STREAM_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, // match shader layout location
        4, // size (vec4)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
    );
    glVertexAttribDivisor(3, 1);

    glGenBuffers(1, &psGL.sizeBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, psGL.sizeBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(Vec2), NULL,
        GL_STREAM_DRAW);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4, // match shader layout location
        2, // size (vec2)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
    );
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);

    psGL.programID = LoadShaders(thread,
        "shaders/particle.vert", "shaders/particle.frag",
        DEBUGPlatformReadFile, DEBUGPlatformFreeFileMemory);
    
    return psGL;
}

void CreateParticleSystem(ParticleSystem* ps, int maxParticles,
    int particlesPerSec, float32 maxLife, Vec3 gravity,
    float32 linearDamp, float32 quadraticDamp,
    Attractor* attractors, int numAttractors,
    PlaneCollider* planeColliders, int numPlaneColliders,
    AxisBoxCollider* boxColliders, int numBoxColliders,
    SphereCollider* sphereColliders, int numSphereColliders,
    InitParticleFunction initParticleFunc, GLuint texture,
    Mesh* mesh, MeshGL* meshGL)
{
    DEBUG_ASSERT(0 <= maxParticles && maxParticles <= MAX_PARTICLES);

    ps->spawnCounter = 0.0f;
    ps->active = 0;

    ps->maxParticles = maxParticles;
    ps->particlesPerSec = particlesPerSec;
    ps->maxLife = maxLife;
    ps->gravity = gravity;
    ps->linearDamp = linearDamp;
    ps->quadraticDamp = quadraticDamp;

    DEBUG_ASSERT(0 <= numAttractors && numAttractors < MAX_ATTRACTORS);
    for (int i = 0; i < numAttractors; i++) {
        ps->attractors[i] = attractors[i];
    }
    ps->numAttractors = numAttractors;

    DEBUG_ASSERT(0 <= numPlaneColliders
        && numPlaneColliders < MAX_COLLIDERS);
    for (int i = 0; i < numPlaneColliders; i++) {
        ps->planeColliders[i] = planeColliders[i];
    }
    ps->numPlaneColliders = numPlaneColliders;
    DEBUG_ASSERT(0 <= numBoxColliders
        && numBoxColliders < MAX_COLLIDERS);
    for (int i = 0; i < numBoxColliders; i++) {
        ps->boxColliders[i] = boxColliders[i];
    }
    ps->numBoxColliders = numBoxColliders;
    DEBUG_ASSERT(0 <= numSphereColliders
        && numSphereColliders < MAX_COLLIDERS);
    for (int i = 0; i < numSphereColliders; i++) {
        ps->sphereColliders[i] = sphereColliders[i];
    }
    ps->numSphereColliders = numSphereColliders;

    ps->initParticleFunc = initParticleFunc;

    ps->texture = texture;

    ps->mesh = mesh;
    ps->meshGL = meshGL;

    ps->width = 0;
    ps->height = 0;
    ps->hookeStrength = 0.0f;
}

internal int IndTo1D(int x, int y, ParticleSystem* ps)
{
    return y * ps->width + x;
}
internal int IndTo1D(Vec2Int i, ParticleSystem* ps)
{
    return IndTo1D(i.x, i.y, ps);
}
internal Vec2Int IndTo2D(int i, ParticleSystem* ps)
{
    return { i % ps->width, i / ps->width};
}

void CreateParticleSystem(ParticleSystem* ps,
    int width, int height, Vec3 origin, Vec3 strideX, Vec3 strideY,
    Vec3 gravity, float32 hookeStrength, float32 hookeEqDist,
    float32 linearDamp, float32 quadraticDamp,
    Attractor* attractors, int numAttractors,
    PlaneCollider* planeColliders, int numPlaneColliders,
    AxisBoxCollider* boxColliders, int numBoxColliders,
    SphereCollider* sphereColliders, int numSphereColliders,
    GLuint texture)
{
    DEBUG_ASSERT(width > 0 && height > 0);
    int numParticles = width * height;
    DEBUG_ASSERT(0 <= numParticles && numParticles <= MAX_PARTICLES);
    ps->width = width;
    ps->height = height;
    ps->hookeStrength = hookeStrength;
    ps->hookeEqDist = hookeEqDist;

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int i = IndTo1D(x, y, ps);
            ps->particles[i].life = 0.0f;
            ps->particles[i].pos = origin
                + strideX * (float32)x + strideY * (float32)y;
            ps->particles[i].vel = Vec3::zero;
            ps->particles[i].color = Vec4::one;
            ps->particles[i].size = { 0.05f, 0.05f };
            ps->particles[i].bounceMult = 0.0f;
            ps->particles[i].frictionMult = 0.25f;
        }
    }

    ps->spawnCounter = 0.0f;
    ps->active = numParticles;

    ps->maxParticles = numParticles;
    ps->particlesPerSec = 0;
    ps->maxLife = 0.0f;
    ps->gravity = gravity;
    ps->linearDamp = linearDamp;
    ps->quadraticDamp = quadraticDamp;

    DEBUG_ASSERT(0 <= numAttractors && numAttractors < MAX_ATTRACTORS);
    for (int i = 0; i < numAttractors; i++) {
        ps->attractors[i] = attractors[i];
    }
    ps->numAttractors = numAttractors;

    DEBUG_ASSERT(0 <= numPlaneColliders
        && numPlaneColliders < MAX_COLLIDERS);
    for (int i = 0; i < numPlaneColliders; i++) {
        ps->planeColliders[i] = planeColliders[i];
    }
    ps->numPlaneColliders = numPlaneColliders;
    DEBUG_ASSERT(0 <= numBoxColliders
        && numBoxColliders < MAX_COLLIDERS);
    for (int i = 0; i < numBoxColliders; i++) {
        ps->boxColliders[i] = boxColliders[i];
    }
    ps->numBoxColliders = numBoxColliders;
    DEBUG_ASSERT(0 <= numSphereColliders
        && numSphereColliders < MAX_COLLIDERS);
    for (int i = 0; i < numSphereColliders; i++) {
        ps->sphereColliders[i] = sphereColliders[i];
    }
    ps->numSphereColliders = numSphereColliders;

    ps->initParticleFunc = nullptr;

    ps->texture = texture;

    ps->mesh = nullptr;
    ps->meshGL = nullptr;
}

internal Vec3 CalculateHookeForce(ParticleSystem* ps, int i, int neighbor)
{
    Vec3 distVec = ps->particles[neighbor].pos - ps->particles[i].pos;
    float32 dist = Mag(distVec);
    distVec /= dist;
    float32 deltaX = dist - ps->hookeEqDist;
    return ps->hookeStrength * deltaX * distVec;
}

internal void HandleBounceCollision(Particle* p,
    Vec3 intersect, Vec3 normal, float32 deltaTime, float32 offset)
{
    Vec3 velNormal = Dot(normal, p->vel) * normal;
    Vec3 velTangent = p->vel - velNormal;
    p->vel = velTangent * p->frictionMult - velNormal * p->bounceMult;
    float32 off = MinFloat32(Mag(p->vel) * deltaTime, offset);
    p->pos = intersect + normal * offset;
}

internal inline bool32 IsInsideBox(Vec3 p, Vec3 boxMin, Vec3 boxMax)
{
    return boxMin.x <= p.x && p.x <= boxMax.x 
        && boxMin.y <= p.y && p.y <= boxMax.y
        && boxMin.z <= p.z && p.z <= boxMax.z;
}

void UpdateParticleSystem(ParticleSystem* ps, float32 deltaTime, void* data)
{
    bool32 isGrid = ps->width != 0 && ps->height != 0;

    // Update all particle non-position data
    for (int i = 0; i < ps->active; i++) {
        ps->particles[i].life += deltaTime;

        // Damping
        float32 magVel = Mag(ps->particles[i].vel);
        Vec3 damp = (ps->linearDamp + ps->quadraticDamp * magVel)
            * ps->particles[i].vel;
        // Attractors
        Vec3 attract = Vec3::zero;
        for (int a = 0; a < ps->numAttractors; a++) {
            Vec3 toAttractor = ps->attractors[a].pos - ps->particles[i].pos;
            float32 distToAttractor = Mag(toAttractor);
            if (distToAttractor < PARTICLE_EPS) {
                continue;
            }
            toAttractor /= distToAttractor;
            float32 attractMag = ps->attractors[a].strength / distToAttractor;
            attract += attractMag * toAttractor;
        }
        // Hooke forces (grid)
        Vec3 hookeForce = Vec3::zero;
        if (isGrid) {
            Vec2Int coords = IndTo2D(i, ps);
            if (coords.x > 0) {
                int neighbor = IndTo1D(coords.x - 1, coords.y, ps);
                hookeForce += CalculateHookeForce(ps, i, neighbor);
            }
            if (coords.x < ps->width - 1) {
                int neighbor = IndTo1D(coords.x + 1, coords.y, ps);
                hookeForce += CalculateHookeForce(ps, i, neighbor);
            }
            if (coords.y > 0) {
                int neighbor = IndTo1D(coords.x, coords.y - 1, ps);
                hookeForce += CalculateHookeForce(ps, i, neighbor);
            }
            if (coords.y < ps->height - 1) {
                int neighbor = IndTo1D(coords.x, coords.y + 1, ps);
                hookeForce += CalculateHookeForce(ps, i, neighbor);
            }
        }
        // Velocity update
        ps->particles[i].vel += (ps->gravity + attract + hookeForce - damp)
            * deltaTime;

        if (!isGrid) {
            // Color update
            float32 alpha = 1.0f - ps->particles[i].life / ps->maxLife;
            alpha = sqrtf(alpha);
            ps->particles[i].color.a = alpha;
        }
    }
    // Update all particle position data
    for (int i = 0; i < ps->active; i++) {
        // Plane colliders
        for (int c = 0; c < ps->numPlaneColliders; c++) {
            Vec3 pos = ps->particles[i].pos;
            Vec3 dir = ps->particles[i].vel * deltaTime;
            Vec3 normal = ps->planeColliders[c].normal;
            Vec3 point = ps->planeColliders[c].point;

            float denom = Dot(normal, dir);
            if (abs(denom) < PARTICLE_EPS) {
                // Motion parallel to the plane
                continue;
            }

            float32 t = Dot(point - pos, normal) / denom;
            if (-PARTICLE_EPS <= t && t < 1.0f) {
                switch (ps->planeColliders[c].type) {
                    case COLLIDER_SINK: {
                        ps->particles[i].life = ps->maxLife + PARTICLE_EPS;
                    } break;
                    case COLLIDER_BOUNCE: {
                        Vec3 intersect = pos + t * dir;
                        HandleBounceCollision(&ps->particles[i],
                            intersect, normal, deltaTime, BOUNCE_MARGIN);
                    } break;
                }
            }
        }
        // Box colliders
        for (int c = 0; c < ps->numBoxColliders; c++) {
            Vec3 pos = ps->particles[i].pos;
            Vec3 dir = ps->particles[i].vel * deltaTime;
            //Vec3 newPos = pos + dir;
            Vec3 boxMin = ps->boxColliders[c].min;
            Vec3 boxMax = ps->boxColliders[c].max;
            bool32 found = false;
            float tIntMin = 1e6;
            Vec3 normal = Vec3::unitX;
            for (int e = 0; e < 3; e++) {
                float32 tInt1 = (boxMin.e[e] - pos.e[e])
                    / dir.e[e];
                Vec3 v1 = pos + tInt1 * dir;
                if (IsInsideBox(v1, boxMin, boxMax)
                && PARTICLE_EPS < tInt1 && tInt1 < tIntMin) {
                    tIntMin = tInt1;
                    Vec3 n = Vec3::zero;
                    n.e[e] = -1.0f;
                    normal = n;
                    found = true;
                }
                float32 tInt2 = (boxMax.e[e] - pos.e[e])
                    / dir.e[e];
                Vec3 v2 = pos + tInt2 * dir;
                if (IsInsideBox(v2, boxMin, boxMax)
                && PARTICLE_EPS < tInt2 && tInt2 < tIntMin) {
                    tIntMin = tInt2;
                    Vec3 n = Vec3::zero;
                    n.e[e] = 1.0f;
                    normal = n;
                    found = true;
                }
            }
            if (found && 0.0f <= tIntMin && tIntMin <= 1.0f) {
                switch (ps->boxColliders[c].type) {
                    case COLLIDER_SINK: {
                        ps->particles[i].life = ps->maxLife + PARTICLE_EPS;
                    } break;
                    case COLLIDER_BOUNCE: {
                        Vec3 intersect = pos + dir * tIntMin;
                        normal *= 1.1f;
                        HandleBounceCollision(&ps->particles[i],
                            intersect, normal, deltaTime, BOUNCE_MARGIN);
                    } break;
                }
            }
        }
        // Sphere colliders
        for (int c = 0; c < ps->numSphereColliders; c++) {
            // From Assignment 3, sphere + ray collision
            Vec3 pos = ps->particles[i].pos;
            Vec3 dir = ps->particles[i].vel * deltaTime;
            Vec3 center = ps->sphereColliders[c].center;
            float32 radius = ps->sphereColliders[c].radius;

            Vec3 toSphere = center - pos;
            float32 tClosest = Dot(toSphere, dir);
            Vec3 closest = pos + dir * tClosest;
            float32 dist = Mag(closest - center);
            if (dist > radius) {
                continue;
            }

            switch (ps->sphereColliders[c].type) {
                case COLLIDER_SINK: {
                    ps->particles[i].life = ps->maxLife + PARTICLE_EPS;
                } break;
                case COLLIDER_BOUNCE: {
                    float32 tOffset = sqrtf(radius * radius - dist * dist);
                    float32 tInt = tClosest - tOffset;
                    if (tInt < PARTICLE_EPS) {
                        tInt = tClosest + tOffset;
                        if (tInt < PARTICLE_EPS) {
                            continue;
                        }
                    }
                    Vec3 intersect = pos + dir * tInt;
                    Vec3 normal = Normalize(intersect - center);
                    HandleBounceCollision(&ps->particles[i],
                        intersect, normal, deltaTime, BOUNCE_MARGIN);
                } break;
            }
        }

        // Position update
        ps->particles[i].pos += ps->particles[i].vel * deltaTime;
    }

    if (isGrid) {
        return;
    }
    // Remove expired particles
    int p = 0;
    int active = ps->active;
    while (p < active) {
        if (ps->particles[p].life > ps->maxLife) {
            ps->particles[p] = ps->particles[active - 1];
            active--;
            continue;
        }
        p++;
    }
    ps->active = active;

    // Spawn new particles
    ps->spawnCounter += (float32)ps->particlesPerSec * deltaTime;
    int spawn = (int)ps->spawnCounter;
    if (spawn == 0) {
        // Nothing to spawn yet
        return;
    }
    if (spawn > MAX_SPAWN) {
        spawn = MAX_SPAWN;
    }
    ps->spawnCounter -= (float32)spawn;
    if (ps->active + spawn >= ps->maxParticles) {
        spawn = ps->maxParticles - ps->active - 1;
    }
    for (int i = 0; i < spawn; i++) {
        int ind = ps->active;
        ps->initParticleFunc(ps, &ps->particles[ind], data);
        ps->active++;
    }
}

internal int DepthComparator(const void* p, const void* q)
{
    float32 depthP = ((Particle*)p)->depth;
    float32 depthQ = ((Particle*)q)->depth;
    if (depthP > depthQ) {
        return -1;
    }
    else if (depthP < depthQ) {
        return 1;
    }
    else {
        return 0;
    }
}

void DrawParticleSystem(ParticleSystemGL psGL,
    PlaneGL planeGL, BoxGL boxGL, MeshGL sphereMeshGL,
    ParticleSystem* ps,
    Vec3 camRight, Vec3 camUp, Vec3 camPos, Mat4 proj, Mat4 view,
    ParticleSystemDataGL* dataGL,
    bool32 drawColliders)
{
    Mat4 vp = proj * view;

    int active = (int)ps->active;
    if (ps->width == 0 && ps->height == 0) {
        for (int i = 0; i < active; i++) {
            Vec4 transformed = vp * ToVec4(ps->particles[i].pos, 1.0f);
            ps->particles[i].depth = transformed.z;
        }
        qsort((void*)ps->particles, active, sizeof(Particle),
            DepthComparator);
    }
    for (int i = 0; i < active; i++) {
        dataGL->pos[i] = ps->particles[i].pos;
        dataGL->color[i] = ps->particles[i].color;
        dataGL->size[i] = ps->particles[i].size;
    }

    GLint loc;
    glUseProgram(psGL.programID);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ps->texture);
    loc = glGetUniformLocation(psGL.programID, "textureSampler");
    glUniform1i(loc, 0);

    Vec2 size = { 0.1f, 0.1f };
    loc = glGetUniformLocation(psGL.programID, "size");
    glUniform2fv(loc, 1, &size.e[0]);
    loc = glGetUniformLocation(psGL.programID, "camRight");
    glUniform3fv(loc, 1, &camRight.e[0]);
    loc = glGetUniformLocation(psGL.programID, "camUp");
    glUniform3fv(loc, 1, &camUp.e[0]);
    loc = glGetUniformLocation(psGL.programID, "vp");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &vp.e[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER, psGL.posBuffer);
    // Buffer orphaning, a common way to improve streaming perf.
    // See http://www.opengl.org/wiki/Buffer_Object_Streaming
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(Vec3), NULL,
        GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, active * sizeof(Vec3),
        dataGL->pos);
    glBindBuffer(GL_ARRAY_BUFFER, psGL.colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(Vec4), NULL,
        GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, active * sizeof(Vec4),
        dataGL->color);
    glBindBuffer(GL_ARRAY_BUFFER, psGL.sizeBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(Vec2), NULL,
        GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, active * sizeof(Vec2),
        dataGL->size);

    glBindVertexArray(psGL.vertexArray);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, active);
    glBindVertexArray(0);

    if (drawColliders) {
        glDisable(GL_DEPTH_TEST);
        Vec4 colliderColor = { 0.4f, 0.4f, 0.4f, 0.3f };
        for (int i = 0; i < ps->numPlaneColliders; i++) {
            DrawPlane(planeGL, vp,
                ps->planeColliders[i].point, ps->planeColliders[i].normal,
                colliderColor);
        }
        for (int i = 0; i < ps->numBoxColliders; i++) {
            DrawBox(boxGL, vp,
                ps->boxColliders[i].min, ps->boxColliders[i].max,
                colliderColor);
        }
        for (int i = 0; i < ps->numSphereColliders; i++) {
            Mat4 viewModel = view * Translate(ps->sphereColliders[i].center)
                * Scale(ps->sphereColliders[i].radius);
            DrawMeshGL(sphereMeshGL, proj, viewModel, colliderColor);
        }
        if (ps->meshGL != nullptr) {
            DrawMeshGL(*ps->meshGL, proj, view,
                Vec4 { 0.0f, 1.0f, 1.0f, 0.2f });
            glEnable(GL_DEPTH_TEST);
        }
    }
}