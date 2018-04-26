#pragma once
#define MAX_GLYPHS 128

#include <GL/glew.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "km_math.h"

struct TextGL
{
    GLuint vertexArray;
    GLuint vertexBuffer;
    GLuint uvBuffer;
    GLuint programID;
};
struct GlyphInfo
{
    unsigned int width;
    unsigned int height;
    int offsetX;
    int offsetY;
    int advanceX;
    int advanceY;
    Vec2 uvOrigin;
    Vec2 uvSize;
};
struct FontFace
{
    GLuint atlasTexture;
    unsigned int height;
    GlyphInfo glyphInfo[MAX_GLYPHS];
};

TextGL CreateTextGL();
FontFace LoadFontFace(
    FT_Library library,
    const char* path, unsigned int height);

int GetTextWidth(const FontFace& face, const char* text);
void DrawText(
    TextGL textGL, const FontFace& face,
    const char* text, Vec3 pos, Vec2 anchor, Vec4 color);
void DrawText(
    TextGL textGL, const FontFace& face,
    const char* text, Vec3 pos, Vec4 color);