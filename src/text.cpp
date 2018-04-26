#include "text.h"

#include <GL/glew.h>

#include "main.h"
#include "ogl_base.h"

#define ATLAS_DIM_MIN 512
#define ATLAS_DIM_MAX 2048

TextGL CreateTextGL()
{
    TextGL textGL;
    // TODO probably use indexing for this
    const GLfloat vertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f
    };
    const GLfloat uvs[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f
    };

    glGenVertexArrays(1, &textGL.vertexArray);
    glBindVertexArray(textGL.vertexArray);

    glGenBuffers(1, &textGL.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, textGL.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, // match shader layout location
        3, // size (vec3)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
    );

    glGenBuffers(1, &textGL.uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, textGL.uvBuffer);
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

    glBindVertexArray(0);

    textGL.programID = LoadShaders(
        "shaders/text.vert",
        "shaders/text.frag"
    );

    return textGL;
}

FontFace LoadFontFace(
    FT_Library library,
    const char* path, unsigned int height)
{
    FontFace face = {};
    face.height = height;

    // Load font face using FreeType.
    FT_Face ftFace;
    char fullPath[PATH_MAX_LENGTH];
    CatStrings(GetAppPath(), path, fullPath, PATH_MAX_LENGTH);
    FT_Error error = FT_New_Face(library, fullPath, 0, &ftFace);
    if (error == FT_Err_Unknown_File_Format) {
        printf("Unsupported file format for %s\n", fullPath);
        return face;
    }
    else if (error) {
        printf("Font file couldn't be read: %s\n", fullPath);
        return face;
    }

    error = FT_Set_Pixel_Sizes(ftFace, 0, height);
    if (error) {
        printf("Failed to set font pixel size\n");
        //return 1;
    }

    // Fill in the non-UV parameters of GlyphInfo struct array.
    for (int ch = 0; ch < MAX_GLYPHS; ch++) {
        error = FT_Load_Char(ftFace, ch, FT_LOAD_RENDER);
        if (error) {
            face.glyphInfo[ch].width = 0;
            face.glyphInfo[ch].height = 0;
            face.glyphInfo[ch].offsetX = 0;
            face.glyphInfo[ch].offsetY = 0;
            face.glyphInfo[ch].advanceX = 0;
            face.glyphInfo[ch].advanceY = 0;
            continue;
        }
        FT_GlyphSlot glyph = ftFace->glyph;

        face.glyphInfo[ch].width = glyph->bitmap.width;
        face.glyphInfo[ch].height = glyph->bitmap.rows;
        face.glyphInfo[ch].offsetX = glyph->bitmap_left;
        face.glyphInfo[ch].offsetY =
            glyph->bitmap_top - (int)glyph->bitmap.rows;
        face.glyphInfo[ch].advanceX = glyph->advance.x;
        face.glyphInfo[ch].advanceY = glyph->advance.y;
    }

    const unsigned int pad = 2;
    // Find the lowest dimension atlas that fits all characters to be loaded.
    // Atlas dimension is always square and power-of-two.
    unsigned int atlasWidth = 0;
    unsigned int atlasHeight = 0;
    for (unsigned int dim = ATLAS_DIM_MIN; dim <= ATLAS_DIM_MAX; dim *= 2) {
        unsigned int originI = pad;
        unsigned int originJ = pad;
        bool fits = true;
        for (int ch = 0; ch < MAX_GLYPHS; ch++) {
            unsigned int glyphWidth = face.glyphInfo[ch].width;
            if (originI + glyphWidth + pad >= dim) {
                originI = pad;
                originJ += height + pad;
            }
            originI += glyphWidth + pad;

            if (originJ + pad >= dim) {
                fits = false;
                break;
            }
        }
        if (fits) {
            atlasWidth = dim;
            atlasHeight = dim;
            break;
        }
    }

    if (atlasWidth == 0 || atlasHeight == 0) {
        printf("PANIC! Atlas not big enough\n"); // TODO error handling
        return face;
    }
    //printf("atlasSize: %u x %u\n", atlasWidth, atlasHeight);

    // Allocate and initialize atlas texture data.
    uint8* atlasData = (uint8*)malloc(atlasWidth * atlasHeight);
    for (unsigned int j = 0; j < atlasHeight; j++) {
        for (unsigned int i = 0; i < atlasWidth; i++) {
            atlasData[j * atlasWidth + i] = 0;
        }
    }

    unsigned int originI = pad;
    unsigned int originJ = pad;
    for (int ch = 0; ch < MAX_GLYPHS; ch++) {
        error = FT_Load_Char(ftFace, ch, FT_LOAD_RENDER);
        if (error) {
            continue;
        }
        FT_GlyphSlot glyph = ftFace->glyph;

        unsigned int glyphWidth = glyph->bitmap.width;
        unsigned int glyphHeight = glyph->bitmap.rows;
        if (originI + glyphWidth + pad >= atlasWidth) {
            originI = pad;
            originJ += height + pad;
        }

        // Write glyph bitmap into atlas.
        for (unsigned int j = 0; j < glyphHeight; j++) {
            for (unsigned int i = 0; i < glyphWidth; i++) {
                int indAtlas = (originJ + j) * atlasWidth + originI + i;
                int indBuffer = (glyphHeight - 1 - j) * glyphWidth + i;
                atlasData[indAtlas] = glyph->bitmap.buffer[indBuffer];
            }
        }
        // Save UV coordinate data.
        face.glyphInfo[ch].uvOrigin = {
            (float)originI / atlasWidth,
            (float)originJ / atlasHeight
        };
        face.glyphInfo[ch].uvSize = {
            (float)glyphWidth / atlasWidth,
            (float)glyphHeight / atlasHeight
        };

        originI += glyphWidth + pad;
    }

    glGenTextures(1, &face.atlasTexture);
    glBindTexture(GL_TEXTURE_2D, face.atlasTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        atlasWidth,
        atlasHeight,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        atlasData
    );
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    free(atlasData);

    return face;
}

int GetTextWidth(const FontFace& face, const char* text)
{
    float x = 0.0f;
    float y = 0.0f;
    for (const char* p = text; *p != 0; p++) {
        GlyphInfo glyphInfo = face.glyphInfo[*p];
        x += (float)glyphInfo.advanceX / 64.0f;
        y += (float)glyphInfo.advanceY / 64.0f;
    }

    return (int)x;
}

void DrawText(
    TextGL textGL, const FontFace& face,
    const char* text, Vec3 pos, Vec4 color)
{
    GLint loc;
    glUseProgram(textGL.programID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, face.atlasTexture);
    loc = glGetUniformLocation(textGL.programID, "textureSampler");
    glUniform1i(loc, 0);

    loc = glGetUniformLocation(textGL.programID, "color");
    glUniform4fv(loc, 1, &color.e[0]);
    loc = glGetUniformLocation(textGL.programID, "pixelToClip");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &pixelToClip_.e[0][0]);

    glBindVertexArray(textGL.vertexArray);

    float x = 0.0f;
    float y = 0.0f;
    for (const char* p = text; *p != 0; p++) {
        GlyphInfo glyphInfo = face.glyphInfo[*p];
        Vec3 glyphPos = pos;
        glyphPos.x += x + (float)glyphInfo.offsetX;
        glyphPos.y += y + (float)glyphInfo.offsetY;
        loc = glGetUniformLocation(textGL.programID, "posBottomLeft");
        glUniform3fv(loc, 1, &glyphPos.e[0]);
        Vec2 glyphSize = {
            (float)glyphInfo.width, (float)glyphInfo.height
        };
        loc = glGetUniformLocation(textGL.programID, "size");
        glUniform2fv(loc, 1, &glyphSize.e[0]);
        loc = glGetUniformLocation(textGL.programID, "uvOrigin");
        glUniform2fv(loc, 1, &glyphInfo.uvOrigin.e[0]);
        loc = glGetUniformLocation(textGL.programID, "uvSize");
        glUniform2fv(loc, 1, &glyphInfo.uvSize.e[0]);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (float)glyphInfo.advanceX / 64.0f;
        y += (float)glyphInfo.advanceY / 64.0f;
    }

    glBindVertexArray(0);
}

// Anchor is in range (0-1, 0-1).
void DrawText(
    TextGL textGL, const FontFace& face,
    const char* text, Vec3 pos, Vec2 anchor, Vec4 color)
{
    int textWidth = GetTextWidth(face, text);
    pos.x -= anchor.x * (float)textWidth;
    pos.y -= anchor.y * face.height; // TODO get face height.

    DrawText(textGL, face, text, pos, color);
}