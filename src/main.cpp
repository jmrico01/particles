#include "main_platform.h"
#include "km_debug.h"
#include "km_defines.h"
#include "km_input.h"
#include "km_math.h"
#include "opengl.h"
#include "opengl_global.h"
#include "ogl_base.h"

struct Tiles
{
#define TILES_X		100
#define TILES_Y		100
#define TILE_SIZE	0.05f
	float32 height[TILES_X][TILES_Y];
	Vec4 color[TILES_X][TILES_Y];

	GLuint vao;
	GLuint quadIndBuffer;
	GLuint lineIndBuffer;

	GLuint shader;
};

struct GameState
{
	Tiles tiles;

	Vec3 pos;
	float32 angle;

	GLuint rectShader;
	GLuint rectVAO;
};

internal GLint LoadRectVAO(const ThreadContext* thread)
{
	GLfloat vertData[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f
	};
	GLuint vertBuffer;
	glGenBuffers(1, &vertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertData), vertData, GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	return vao;
}

internal void DrawRect(
	const ThreadContext* thread,
	GLuint rectShader, GLuint rectVAO,
	Mat4 mvp, float32 r, float32 g, float32 b, float32 a)
{
	glUseProgram(rectShader);
	glUniformMatrix4fv(
		glGetUniformLocation(rectShader, "mvp"),
        1, GL_FALSE, &mvp.e[0][0]);
	glUniform4f(
		glGetUniformLocation(rectShader, "color"),
        r, g, b, a);

	glBindVertexArray(rectVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

internal void InitTiles(
	const ThreadContext* thread,
	DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile,
	DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory,
	Tiles* tiles)
{
	for (int i = 0; i < TILES_X; i++)
	{
		for (int j = 0; j < TILES_Y; j++)
		{
			tiles->height[i][j] = 0.0f;
			tiles->color[i][j] = Vec4{ 0.3f, 0.3f, 0.3f, 1.0f };
		}
	}
	
	// Initialize shader
	tiles->shader = LoadShaders(thread,
		DEBUGPlatformReadFile,
		DEBUGPlatformFreeFileMemory,
		"shaders/rectAlt.vert", "shaders/rectAlt.frag");

	// Initialize VAO
	glGenVertexArrays(1, &tiles->vao);
	glBindVertexArray(tiles->vao);

	// Vertices
	GLfloat vertexData[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};
	GLuint vertBuffer;
	glGenBuffers(1, &vertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
	glBufferData(GL_ARRAY_BUFFER,
        sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Indices for quad rendering
	GLuint quadIndices[] = {
		0, 1, 2,
		2, 3, 0
	};
	glGenBuffers(1, &tiles->quadIndBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tiles->quadIndBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

	// Indices for line (quad outline) rendering
	GLuint lineIndices[] = {
		0, 1,
		1, 2,
		2, 3,
		3, 0
	};
	glGenBuffers(1, &tiles->lineIndBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tiles->lineIndBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        sizeof(lineIndices), lineIndices, GL_STATIC_DRAW);

	if (!tiles->shader) {
		DEBUG_ASSERT(0);
    }
	if (!tiles->vao) {
		DEBUG_ASSERT(0);
    }
	if (!tiles->quadIndBuffer) {
		DEBUG_ASSERT(0);
    }
	if (!tiles->lineIndBuffer) {
		DEBUG_ASSERT(0);
    }
}

internal void DrawTiles(
	const ThreadContext* thread,
	Tiles* tiles, Mat4 vp)
{
	glUseProgram(tiles->shader);
	glBindVertexArray(tiles->vao);

	Vec3 offset{ -(float32)TILES_X * TILE_SIZE / 2.0f, -(float32)TILES_Y * TILE_SIZE / 2.0f, 0.0f };

	for (int i = 0; i < TILES_X; i++)
	{
		for (int j = 0; j < TILES_Y; j++)
		{
			Vec3 pos;
			pos.x = (float32)i * TILE_SIZE;
			pos.y = (float32)j * TILE_SIZE;
			pos.z = 0.0f;

			Mat4 model = Translate(pos + offset) * Scale(Vec3{ TILE_SIZE, TILE_SIZE, 1.0f });
			glUniformMatrix4fv(
				glGetUniformLocation(tiles->shader, "mvp"),
                1, GL_FALSE, &(vp * model).e[0][0]);
			glUniform4fv(
				glGetUniformLocation(tiles->shader, "color"),
                1, &tiles->color[i][j].e[0]);

			// TODO the 6 is hard-coded
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                tiles->quadIndBuffer);
			glDrawElements(GL_TRIANGLES,
                6, GL_UNSIGNED_INT, (void*)0);

			pos.z = 0.001f;
			model = Translate(pos + offset) * Scale(Vec3{ TILE_SIZE, TILE_SIZE, 1.0f });
			glUniformMatrix4fv(
				glGetUniformLocation(tiles->shader, "mvp"),
                1, GL_FALSE, &(vp * model).e[0][0]);
			Vec4 color = { 0.1f, 0.1f, 0.1f, 1.0f };
			glUniform4fv(
				glGetUniformLocation(tiles->shader, "color"),
                1, &color.e[0]);

			// TODO the 8 is hard-coded
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                tiles->lineIndBuffer);
			glDrawElements(GL_LINES,
                8, GL_UNSIGNED_INT, (void*)0);
		}
	}

	glBindVertexArray(0);
}

internal void TintTiles(
	const ThreadContext* thread,
	Tiles* tiles, Vec3 pos)
{
	Vec3 offset = {
        -(float32)TILES_X * TILE_SIZE / 2.0f,
        -(float32)TILES_Y * TILE_SIZE / 2.0f,
        0.0f
    };

	Vec3 indices = (pos - offset) / TILE_SIZE;
	int i = (int)indices.x;
	int j = (int)indices.y;
	tiles->color[i][j] = Vec4 { 0.8f, 0.3f, 0.3f, 1.0f };
}

extern "C" GAME_UPDATE_AND_RENDER_FUNC(GameUpdateAndRender)
{
    // NOTE: for clarity
    // A call to this function means the following has happened:
    //  - A frame has been displayed to the user
    //  - The latest user input has been processed by the platform layer
    //
    // This function is expected to update the state of the game
    // and draw the frame that will be displayed, ideally, some constant
    // amount of time in the future.
	DEBUG_ASSERT(sizeof(GameState) <= memory->permanentStorageSize);

	GameState *gameState = (GameState*)memory->permanentStorage;
    if (memory->DEBUGShouldInitGlobalFuncs) {
	    // Initialize global function names
#if GAME_SLOW
        debugPrint_ = platformFuncs->DEBUGPlatformPrint;
#endif
        #define FUNC(returntype, name, ...) name = \
        platformFuncs->glFunctions.name;
            GL_FUNCTIONS_BASE
            GL_FUNCTIONS_ALL
        #undef FUNC

        memory->DEBUGShouldInitGlobalFuncs = false;
    }
	if (!memory->isInitialized) {
# if 0
		// TODO round tripping, not final
		char* filename = __FILE__;
		DEBUGReadFileResult readResult =
            memory->DEBUGPlatformReadFile(thread, filename);
		if (readResult.data)
		{
			memory->DEBUGPlatformWriteFile(thread, "test.out",
				(uint32)readResult.size, readResult.data);
			memory->DEBUGPlatformFreeFileMemory(thread, readResult.data);
		}
#endif
		
		InitTiles(thread,
			platformFuncs->DEBUGPlatformReadFile,
			platformFuncs->DEBUGPlatformFreeFileMemory,
			&gameState->tiles);

		gameState->pos.x = 0.0f;
		gameState->pos.y = 0.0f;
		gameState->pos.z = 0.0f;

		// Isometric angle to which completely upward-facing
		// 90 degree angles get transformed
		gameState->angle = 2.0f * PI_F / 3.0f;

		gameState->rectShader = LoadShaders(thread,
			platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory,
			"shaders/rect.vert", "shaders/rect.frag");
		/*GLfloat rectPosData[] = {
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,
			0.0f, 0.0f
		};*/
		gameState->rectVAO = LoadRectVAO(thread);
		if (!gameState->rectShader) {
			// TODO logging / EXIT GAME! no way to do this yet...
			DEBUG_PANIC("LoadShaders failed");
		}
		if (!gameState->rectVAO) {
			// TODO logging / exit
			DEBUG_PANIC("LoadRectVAO failed");
		}

		glClearColor(0.0f, 0.0f, 0.1f, 0.0f);
		// Very explicit depth testing setup (DEFAULT VALUES)
		// NDC is left-handed with this setup
		// (very subtle left-handedness definition:
		//	front objects have z = -1, far objects have z = 1)
		glEnable(GL_DEPTH_TEST);
		// Nearer objects have less z than farther objects
		glDepthFunc(GL_LESS);
		// Depth buffer clears to farthest z-value (1)
		glClearDepth(1.0);
		// Depth buffer transforms -1 to 1 range to 0 to 1 range
		glDepthRange(0.0, 1.0);

		glDisable(GL_BLEND);

		// TODO this may be more appropriate to do in the platform layer
		memory->isInitialized = true;
	}

	const GameControllerInput* input0 = &input->controllers[0];

	float32 speed = 0.01f;
	Vec3 playerRight = Normalize(Vec3{ 1.0f, 1.0f, 0.0f });
	Vec3 playerForward = Normalize(Vec3{ -1.0f, 1.0f, 0.0f });
	Vec3 vel = Vec3::zero;
	if (input0->isConnected) {
		vel = (input0->leftEnd.x * playerRight
        + input0->leftEnd.y * playerForward) * speed;
	}
	if (input->keyboard[KM_KEY_D].isDown) {
		vel += playerRight * speed;
	}
	if (input->keyboard[KM_KEY_A].isDown) {
		vel -= playerRight * speed;
	}
	if (input->keyboard[KM_KEY_W].isDown) {
		vel += playerForward * speed;
	}
	if (input->keyboard[KM_KEY_S].isDown) {
		vel -= playerForward * speed;
	}
	gameState->pos += vel;

	//screenInfo.width;
	//screenInfo.height;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*if (input0->x.isDown) {
		gameState->angle -= 0.005f;
    }
	if (input0->b.isDown) {
		gameState->angle += 0.005f;
    }*/

	float isoZ = PI_F / 4.0f;
	float isoX = acosf(tanf((PI_F - gameState->angle) / 2.0f));
 	Quat rotationZ = QuatFromAngleUnitAxis(-isoZ, Vec3{ 0.0f, 0.0f, 1.0f });
	Quat rotationX = QuatFromAngleUnitAxis(-isoX, Vec3{ 1.0f, 0.0f, 0.0f });

	float zHalfRange = 10.0f;
	Mat4 proj = Scale(Vec3 {
        (float32)screenInfo.size.y / (float32)screenInfo.size.x,
		1.0f,
        1.0f / zHalfRange
    });
	Mat4 view = //Scale(Vec3{ 1.0f, 1.0f, 1.0f / zRange }) *
		UnitQuatToMat4(rotationX) *
		UnitQuatToMat4(rotationZ) *
		Translate(-gameState->pos) *
		Scale(Vec3 { 1.0f, 1.0f, -1.0f });

	TintTiles(thread, &gameState->tiles, gameState->pos);
	DrawTiles(thread, &gameState->tiles, proj * view);

	float32 boxSize = 0.1f;
	float32 boxGray = 0.5f;

	if (input->controllers[0].a.isDown) {
		boxGray = 1.0f;
    }

	Vec3 centeredPos = {
        gameState->pos.x - boxSize / 2.0f,
		gameState->pos.y - boxSize / 2.0f,
		0.01f
    };
	Mat4 playerMat = Translate(centeredPos)
        * Scale(Vec3{ boxSize, boxSize, 1.0f });
	DrawRect(thread, gameState->rectShader, gameState->rectVAO,
		proj * view * playerMat, boxGray, boxGray, boxGray, 1.0f);
}

#include "ogl_base.cpp"
#include "km_input.cpp"