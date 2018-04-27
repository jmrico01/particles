#pragma once

#include "km_math.h"
#include "ogl_base.h"

struct GameState
{
    Vec3 cameraPos;
    Quat modelRot;

    RectGL rectGL;
    LineGL lineGL;
};
