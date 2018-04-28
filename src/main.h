#pragma once

#include "km_math.h"
#include "ogl_base.h"
#include "text.h"

struct GameState
{
    Vec3 cameraPos;
    Quat modelRot;

    RectGL rectGL;
    LineGL lineGL;
    TextGL textGL;

    FT_Library ftLibrary;
    FontFace fontFaceSmall;
    FontFace fontFaceMedium;
    FontFace fontFaceLarge;
};
