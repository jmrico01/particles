#pragma once

#include "km_math.h"
#include "ogl_base.h"
#include "text.h"
#include "gui.h"
#include "particles.h"

struct GameState
{
    Vec3 cameraPos;
    Quat modelRot;

    RectGL rectGL;
    TexturedRectGL texturedRectGL;
    LineGL lineGL;
    TextGL textGL;
    BoxGL boxGL;
    ParticleSystemGL psGL;

    FT_Library ftLibrary;
    FontFace fontFaceSmall;
    FontFace fontFaceMedium;
    FontFace fontFaceLarge;

    GLuint pTexBase;
    GLuint pTexFire;
    GLuint pTexSmoke;
    GLuint pTexSpark;
    GLuint pTexSphere;
    InputField inputField;
    Button button;

    ParticleSystem ps;
};
