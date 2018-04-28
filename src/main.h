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
    ParticleSystemGL psGL;

    FT_Library ftLibrary;
    FontFace fontFaceSmall;
    FontFace fontFaceMedium;
    FontFace fontFaceLarge;

    ParticleSystem ps;

    GLuint textureID;
    InputField inputField;
    Button button;
};
