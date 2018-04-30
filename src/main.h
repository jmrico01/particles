#pragma once

#include "km_math.h"
#include "ogl_base.h"
#include "text.h"
#include "gui.h"
#include "particles.h"
#include "mesh.h"

enum Preset
{
    PRESET_FIRE_SWIRL,
    PRESET_CLOTH_OFFSET,
    PRESET_CLOTH,
    PRESET_MESH,
    PRESET_ATTRACTORS,
    PRESET_SPHERE_COLLIDERS,
    PRESET_BOX_COLLIDERS,
    PRESET_FOUNTAIN_BOUNCE,
    PRESET_FOUNTAIN_SINK,
    PRESET_SPHERE,

    PRESET_LAST // keep at the end
};

global_var const char* presetNames_[PRESET_LAST] = {
    "Fire Swirl",
    "Cloth (Off-Center)",
    "Cloth",
    "Mesh Uniform",
    "Attractors",
    "Sphere Collider",
    "Axis Box Collider",
    "Fountain Bounce",
    "Fountain Sink",
    "Sphere Uniform"
};

struct GameState
{
    Vec3 cameraPos;
    Quat modelRot;

    bool32 drawColliders;

    RectGL rectGL;
    TexturedRectGL texturedRectGL;
    LineGL lineGL;
    TextGL textGL;
    PlaneGL planeGL;
    BoxGL boxGL;
    ParticleSystemGL psGL;
    Mesh sphereMesh;
    MeshGL sphereMeshGL;

    FT_Library ftLibrary;
    FontFace fontFaceSmall;
    FontFace fontFaceMedium;
    FontFace fontFaceLarge;

    GLuint pTexBase;
    GLuint pTexFire;
    GLuint pTexSmoke;
    GLuint pTexSpark;
    GLuint pTexSphere;

    Preset activePreset;
    Button presetButtons[PRESET_LAST];
    Button drawCollidersButton;
    InputField modelField;

    ParticleSystem ps;

    Mesh loadedMesh;
    MeshGL loadedMeshGL;
};
