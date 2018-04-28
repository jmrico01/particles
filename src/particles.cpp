#include "particles.h"

#include <cstdlib>

#include "km_debug.h"
#include "ogl_base.h"
#include "opengl_funcs.h"

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

    glBindVertexArray(0);

    psGL.programID = LoadShaders(thread,
        "shaders/particle.vert", "shaders/particle.frag",
        DEBUGPlatformReadFile, DEBUGPlatformFreeFileMemory);
    
    return psGL;
}

void CreateParticleSystem(ParticleSystem* ps, uint32 maxParticles)
{
    if (maxParticles > MAX_PARTICLES) {
        DEBUG_PRINT("Maximum supported number of particles: %d\n",
            MAX_PARTICLES);
        return;
    }
    ps->maxParticles = maxParticles;
    for (uint32 i = 0; i < maxParticles; i++) {
        ps->particles[i].pos = Vec3::zero;
        ps->particles[i].vel = Vec3::zero;
        ps->active[i] = false;
    }

    for (uint32 i = 0; i < 1000; i++) {
        ps->particles[i].vel = {
            (float32)rand() / RAND_MAX - 0.5f,
            (float32)rand() / RAND_MAX - 0.5f,
            (float32)rand() / RAND_MAX - 0.5f
        };
        ps->active[i] = true;
    }

    ps->gravity = Vec3::zero;
}

void UpdateParticleSystem(ParticleSystem* ps, float32 deltaTime)
{
    int maxParticles = (int)ps->maxParticles;
    for (int i = 0; i < maxParticles; i++) {
        if (ps->active[i]) {
            ps->particles[i].pos += ps->particles[i].vel * deltaTime;
            ps->particles[i].vel += ps->gravity * deltaTime;
        }
    }
}

void DrawParticleSystem(ParticleSystemGL psGL, ParticleSystem* ps,
    GLuint texture, Vec3 camRight, Vec3 camUp, Mat4 vp)
{
    int count = 0;
    int maxParticles = (int)ps->maxParticles;
    for (int i = 0; i < maxParticles; i++) {
        if (ps->active[i]) {
            ps->drawInfo.pos[count++] = ps->particles[i].pos;
        }
    }

    GLint loc;
    glUseProgram(psGL.programID);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
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
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(Vec3),
        ps->drawInfo.pos);

    glBindVertexArray(psGL.vertexArray);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count);
    glBindVertexArray(0);
}