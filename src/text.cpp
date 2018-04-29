#include "text.h"

#include "km_debug.h"
#include "main.h"
#include "ogl_base.h"

#define ATLAS_DIM_MIN 128
#define ATLAS_DIM_MAX 2048

TextGL InitTextGL(const ThreadContext* thread,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory)
{
    TextGL textGL;
    // TODO probably use indexing for this
    const GLfloat vertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f
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
        2, // size (vec2)
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

    textGL.programID = LoadShaders(thread,
        "shaders/text.vert", "shaders/text.frag",
        DEBUGPlatformReadFile, DEBUGPlatformFreeFileMemory);

    return textGL;
}

FontFace LoadFontFace(const ThreadContext* thread,
    FT_Library library,
    const char* path, uint32 height,
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory)
{
    FontFace face = {};
    face.height = height;

    // Load font face using FreeType.
    FT_Face ftFace;
    DEBUGReadFileResult fontFile = DEBUGPlatformReadFile(thread, path);
    FT_Open_Args openArgs = {};
    openArgs.flags = FT_OPEN_MEMORY;
    openArgs.memory_base = (const FT_Byte*)fontFile.data;
    openArgs.memory_size = (FT_Long)fontFile.size;
    FT_Error error = FT_Open_Face(library, &openArgs, 0, &ftFace);
    if (error == FT_Err_Unknown_File_Format) {
        DEBUG_PRINT("Unsupported file format for %s\n", path);
        return face;
    }
    else if (error) {
        DEBUG_PRINT("Font file couldn't be read: %s\n", path);
        return face;
    }

    error = FT_Set_Pixel_Sizes(ftFace, 0, height);
    if (error) {
        DEBUG_PRINT("Failed to set font pixel size\n");
        return face;
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

    const uint32 pad = 2;
    // Find the lowest dimension atlas that fits all characters to be loaded.
    // Atlas dimension is always square and power-of-two.
    uint32 atlasWidth = 0;
    uint32 atlasHeight = 0;
    for (uint32 dim = ATLAS_DIM_MIN; dim <= ATLAS_DIM_MAX; dim *= 2) {
        uint32 originI = pad;
        uint32 originJ = pad;
        bool fits = true;
        for (int ch = 0; ch < MAX_GLYPHS; ch++) {
            uint32 glyphWidth = face.glyphInfo[ch].width;
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
    // TODO get rid of this malloc
    uint8* atlasData = (uint8*)malloc(atlasWidth * atlasHeight);
    for (uint32 j = 0; j < atlasHeight; j++) {
        for (uint32 i = 0; i < atlasWidth; i++) {
            atlasData[j * atlasWidth + i] = 0;
        }
    }

    uint32 originI = pad;
    uint32 originJ = pad;
    for (int ch = 0; ch < MAX_GLYPHS; ch++) {
        error = FT_Load_Char(ftFace, ch, FT_LOAD_RENDER);
        if (error) {
            continue;
        }
        FT_GlyphSlot glyph = ftFace->glyph;

        uint32 glyphWidth = glyph->bitmap.width;
        uint32 glyphHeight = glyph->bitmap.rows;
        if (originI + glyphWidth + pad >= atlasWidth) {
            originI = pad;
            originJ += height + pad;
        }

        // Write glyph bitmap into atlas.
        for (uint32 j = 0; j < glyphHeight; j++) {
            for (uint32 i = 0; i < glyphWidth; i++) {
                int indAtlas = (originJ + j) * atlasWidth + originI + i;
                int indBuffer = (glyphHeight - 1 - j) * glyphWidth + i;
                atlasData[indAtlas] = glyph->bitmap.buffer[indBuffer];
            }
        }
        // Save UV coordinate data.
        face.glyphInfo[ch].uvOrigin = {
            (float32)originI / atlasWidth,
            (float32)originJ / atlasHeight
        };
        face.glyphInfo[ch].uvSize = {
            (float32)glyphWidth / atlasWidth,
            (float32)glyphHeight / atlasHeight
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

void DrawText(TextGL textGL, const FontFace& face, ScreenInfo screenInfo,
    const char* text,
    Vec2Int pos, Vec4 color)
{
    GLint loc;
    glUseProgram(textGL.programID);
    glActiveTexture(GL_TEXTURE0);
    // TODO should I be doing this here?
    // or should I do it globally for TEXTURE0 somewhere,
    // and designate TEXTURE0 as GL_TEXTURE_2D
    glBindTexture(GL_TEXTURE_2D, face.atlasTexture);
    loc = glGetUniformLocation(textGL.programID, "textureSampler");
    glUniform1i(loc, 0);

    loc = glGetUniformLocation(textGL.programID, "color");
    glUniform4fv(loc, 1, &color.e[0]);

    glBindVertexArray(textGL.vertexArray);

    int x = 0, y = 0;
    for (const char* p = text; *p != 0; p++) {
        GlyphInfo glyphInfo = face.glyphInfo[*p];
        Vec2Int glyphPos = pos;
        glyphPos.x += x + glyphInfo.offsetX;
        glyphPos.y += y + glyphInfo.offsetY;
        Vec2Int glyphSize = { (int)glyphInfo.width, (int)glyphInfo.height };
        RectCoordsNDC ndc = ToRectCoordsNDC(glyphPos, glyphSize, screenInfo);
        loc = glGetUniformLocation(textGL.programID, "posBottomLeft");
        glUniform3fv(loc, 1, &ndc.pos.e[0]);
        loc = glGetUniformLocation(textGL.programID, "size");
        glUniform2fv(loc, 1, &ndc.size.e[0]);
        loc = glGetUniformLocation(textGL.programID, "uvOrigin");
        glUniform2fv(loc, 1, &glyphInfo.uvOrigin.e[0]);
        loc = glGetUniformLocation(textGL.programID, "uvSize");
        glUniform2fv(loc, 1, &glyphInfo.uvSize.e[0]);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += glyphInfo.advanceX / 64;
        y += glyphInfo.advanceY / 64;
    }

    glBindVertexArray(0);
}

// Anchor is in range (0-1, 0-1).
void DrawText(TextGL textGL, const FontFace& face, ScreenInfo screenInfo,
    const char* text,
    Vec2Int pos, Vec2 anchor, Vec4 color)
{
    int textWidth = GetTextWidth(face, text);
    pos.x -= (int)(anchor.x * textWidth);
    pos.y -= (int)(anchor.y * face.height);

    DrawText(textGL, face, screenInfo, text, pos, color);
}