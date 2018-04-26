#pragma once

#include "km_math.h"
#include "ogl_base.h"
#include "main.h"
#include "text.h"

#define INPUT_BUFFER_SIZE 2048

enum ClickStateFlags {
    CLICKSTATE_NONE = 0,
    CLICKSTATE_LEFT_PRESS = 1 << 0,
    CLICKSTATE_RIGHT_PRESS = 1 << 1
};

struct Button;

typedef void (*ButtonCallback)(Button*, void*);

struct ClickableBox
{
    Vec2 origin;
    Vec2 size;

    bool hovered;
    bool pressed;

    Vec4 color;
    Vec4 hoverColor;
    Vec4 pressColor;
};

struct Button
{
    ClickableBox box;
    char text[INPUT_BUFFER_SIZE];
    ButtonCallback callback;
    Vec4 textColor;
};

struct InputField
{
    ClickableBox box;
    char text[INPUT_BUFFER_SIZE];
    uint32 textLen;
    Vec4 textColor;
};

ClickableBox CreateClickableBox(Vec2 origin, Vec2 size,
    Vec4 color, Vec4 hoverColor, Vec4 pressColor);
Button CreateButton(Vec2 origin, Vec2 size,
    const char* text, ButtonCallback callback,
    Vec4 color, Vec4 hoverColor, Vec4 pressColor, Vec4 textColor);
InputField CreateInputField(Vec2 origin, Vec2 size, const char* text,
    Vec4 color, Vec4 hoverColor, Vec4 pressColor, Vec4 textColor);

void UpdateClickableBoxes(ClickableBox boxes[], uint32 n,
    Vec2 mousePos, int clickState);
void DrawClickableBoxes(ClickableBox boxes[], uint32 n, RectGL rectGL);

void UpdateButtons(Button buttons[], uint32 n,
    Vec2 mousePos, int clickState, void* data);
void DrawButtons(Button buttons[], uint32 n,
    RectGL rectGL, TextGL textGL, const FontFace& face);

void UpdateInputFields(InputField fields[], uint32 n,
    Vec2 mousePos, int clickState, KeyEvent* keyBuf, uint32 keyBufSize);
void DrawInputFields(InputField fields[], uint32 n,
    RectGL rectGL, TextGL textGL, const FontFace& face);