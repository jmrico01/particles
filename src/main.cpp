#include "main.h"

#include "main_platform.h"
#include "km_debug.h"
#include "km_defines.h"
#include "km_input.h"
#include "km_math.h"
#include "opengl.h"
#include "opengl_funcs.h"
#include "load_png.h"

#define DEFAULT_CAM_Z 3.0f
#define CAM_ZOOM_STEP 0.999f
#define CAM_MOVE_STEP 0.25f

#define UI_MARGIN 20
#define UI_SPACING 6

const Vec4 defaultIdleColor = { 0.2f, 0.2f, 0.2f, 1.0f };
const Vec4 defaultHoverColor = { 0.55f, 0.55f, 0.45f, 1.0f };
const Vec4 defaultPressColor = { 0.8f, 0.8f, 0.65f, 1.0f };
const Vec4 defaultTextColor = { 0.9f, 0.9f, 0.9f, 1.0f };

const Vec4 interestIdleColor = { 0.2f, 0.4f, 0.4f, 1.0f };
const Vec4 interestHoverColor = { 0.4f, 0.6f, 0.6f, 1.0f };
const Vec4 interestPressColor = { 0.6f, 0.8f, 0.8f, 1.0f };
const Vec4 interestTextColor = { 0.7f, 0.9f, 0.9f, 1.0f };

internal inline float32 RandFloat()
{
    return (float32)rand() / RAND_MAX;
}
internal inline float32 RandFloat(float32 min, float32 max)
{
    DEBUG_ASSERT(max > min);
    return RandFloat() * (max - min) + min;
}

internal void InputFieldCallbackTest(InputField* inputField, void* data)
{
    DEBUG_PRINT("hello");
}
internal void ButtonCallbackTest(Button* inputField, void* data)
{
    DEBUG_PRINT("sailor");
}

internal void InitParticleRandom(ParticleSystem* ps, Particle* particle)
{
    particle->life = 0.0f;
    particle->pos = Vec3::zero;
    Vec3 randDir = {
        RandFloat() - 0.5f,
        RandFloat() - 0.5f,
        RandFloat() - 0.5f
    };
    float32 speed = RandFloat(0.5f, 1.5f);
    particle->vel = speed * Normalize(randDir);
    particle->color = {
        RandFloat(),
        RandFloat(),
        RandFloat(),
        1.0f
    };
    float randSize = RandFloat() * 0.1f + 0.05f;
    particle->size = { randSize, randSize };
    particle->bounceMult = 1.0f;
}

internal void InitParticleFountain(ParticleSystem* ps, Particle* particle)
{
    particle->life = 0.0f;
    particle->pos = Vec3::unitY * 0.5f;
    float32 spread = RandFloat(0.0f, 0.5f);
    Vec2 circleVel = {
        RandFloat() - 0.5f,
        RandFloat() - 0.5f
    };
    circleVel = Normalize(circleVel) * spread;
    particle->vel = {
        circleVel.x,
        RandFloat(1.5f, 3.0f),
        circleVel.y
    };
    particle->color = {
        RandFloat(),
        RandFloat(),
        RandFloat(),
        1.0f
    };
    float randSize = RandFloat() * 0.1f + 0.05f;
    particle->size = { randSize, randSize };
    particle->bounceMult = RandFloat(0.5f, 1.0f);
}

internal void InitParticleSphere(ParticleSystem* ps, Particle* particle)
{
    const float32 radius = 2.0f;

    particle->life = 0.0f;
    Vec3 sphereDir;
    float32 mag;
    do {
        sphereDir.x = RandFloat(-1.0f, 1.0f);
        sphereDir.y = RandFloat(-1.0f, 1.0f);
        sphereDir.z = RandFloat(-1.0f, 1.0f);
        mag = Mag(sphereDir);
    } while (mag > radius);
    sphereDir /= mag;
    particle->pos = sphereDir * radius;

    float32 speed = RandFloat(-0.05f, 0.05f);
    particle->vel = speed * sphereDir;
    float32 randColor = RandFloat(0.5f, 1.0f);
    particle->color = { randColor, randColor, randColor, 1.0f };
    float randSize = RandFloat() * 0.1f + 0.05f;
    particle->size = { randSize, randSize };
    particle->bounceMult = 1.0f;
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
		glClearColor(0.0f, 0.0f, 0.05f, 0.0f);
		// Very explicit depth testing setup (DEFAULT VALUES)
		// NDC is left-handed with this setup:
		//	front objects have z = -1, far objects have z = 1
		glEnable(GL_DEPTH_TEST);
		// Nearer objects have less z than farther objects
		glDepthFunc(GL_LEQUAL);
		// Depth buffer clears to farthest z-value (1)
		glClearDepth(1.0);
		// Depth buffer transforms -1 to 1 range to 0 to 1 range
		glDepthRange(0.0, 1.0);
        // NOTE my scene doesn't need this, as long as I make sure to
        // render everything in proper order
        glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);

        gameState->cameraPos = { 0.0f, 0.0f, DEFAULT_CAM_Z };
        gameState->modelRot = QuatFromAngleUnitAxis(PI_F / 6.0f, Vec3::unitX)
            * QuatFromAngleUnitAxis(-PI_F / 4.0f, Vec3::unitY);

        gameState->rectGL = InitRectGL(thread,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->texturedRectGL = InitTexturedRectGL(thread,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->lineGL = InitLineGL(thread,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->textGL = InitTextGL(thread,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->boxGL = InitBoxGL(thread,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->psGL = InitParticleSystemGL(thread,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);

        FT_Error error = FT_Init_FreeType(&gameState->ftLibrary);
        if (error) {
            DEBUG_PRINT("FreeType init error: %d\n", error);
        }
        gameState->fontFaceSmall = LoadFontFace(thread, gameState->ftLibrary,
            "data/fonts/computer-modern/serif.ttf", 14,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->fontFaceMedium = LoadFontFace(thread, gameState->ftLibrary,
            "data/fonts/computer-modern/serif.ttf", 18,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->fontFaceLarge = LoadFontFace(thread, gameState->ftLibrary,
            "data/fonts/computer-modern/serif.ttf", 24,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);

        gameState->pTexBase = LoadPNGOpenGL(thread,
            "data/textures/base.png",
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->pTexFire = LoadPNGOpenGL(thread,
            "data/textures/fire.png",
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->pTexSmoke = LoadPNGOpenGL(thread,
            "data/textures/smoke.png",
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->pTexSpark = LoadPNGOpenGL(thread,
            "data/textures/spark.png",
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->pTexSphere = LoadPNGOpenGL(thread,
            "data/textures/sphere.png",
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);

        gameState->inputField = CreateInputField(
            Vec2Int { UI_MARGIN, 500 }, Vec2Int { 200, 30 },
            "Huh?", InputFieldCallbackTest,
            defaultIdleColor,
            defaultHoverColor,
            defaultPressColor,
            defaultTextColor
        );
        gameState->button = CreateButton(
            Vec2Int { 500, UI_MARGIN }, Vec2Int { 200, 30 },
            "Squish This", ButtonCallbackTest,
            defaultIdleColor,
            defaultHoverColor,
            defaultPressColor,
            defaultTextColor
        );

        // Attractor demo
        // Attractor a[4];
        // a[0].pos = Vec3 { 2.0f, 4.0f, 0.0f };
        // a[0].strength = 2.5f;
        // a[1].pos = Vec3 { 1.0f, 0.0f, 1.0f };
        // a[1].strength = 1.0f;
        // a[2].pos = Vec3 { -5.0f, -1.0f, 0.0f };
        // a[2].strength = 4.0f;
        // a[3].pos = Vec3 { -5.0f, 5.0, -5.0f };
        // a[3].strength = 2.0f;
        // CreateParticleSystem(&gameState->ps,
        //     10000, 1000, 10.0f, Vec3 { 0.0f, 0.0f, 0.0f },
        //     0.2f, 0.2f,
        //     a, 4, nullptr, 0, nullptr, 0, nullptr, 0,
        //     InitParticleRandom, gameState->pTexBase);
        
        // Sphere demo
        // CreateParticleSystem(&gameState->ps,
        //     MAX_PARTICLES, 500, 5.0f, Vec3 { 0.0f, 0.0f, 0.0f },
        //     0.0f, 0.0f,
        //     nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0,
        //     InitParticleSphere, gameState->pTexSpark);
        
        // Fountain demo
        // PlaneCollider groundPlane;
        // groundPlane.type = COLLIDER_BOUNCE;
        // groundPlane.normal = Vec3::unitY;
        // groundPlane.point = Vec3::zero;
        // CreateParticleSystem(&gameState->ps,
        //     1500, 1500, 10.0f, Vec3 { 0.0f, -1.0f, 0.0f },
        //     0.1f, 0.05f,
        //     nullptr, 0, &groundPlane, 1, nullptr, 0, nullptr, 0,
        //     InitParticleFountain, gameState->pTexBase);

        PlaneCollider groundPlane;
        groundPlane.type = COLLIDER_BOUNCE;
        groundPlane.normal = Vec3::unitY;
        groundPlane.point = Vec3::zero;
        AxisBoxCollider box;
        box.type = COLLIDER_SINK;
        box.min = { 0.25f, 0.25f, 0.25f };
        box.max = { 1.25f, 1.25f, 1.25f };
        CreateParticleSystem(&gameState->ps,
            1500, 1500, 10.0f, Vec3 { 0.0f, -1.0f, 0.0f },
            0.1f, 0.05f,
            nullptr, 0, &groundPlane, 1, &box, 1, nullptr, 0,
            InitParticleFountain, gameState->pTexBase);

		memory->isInitialized = true;
	}

    // Camera control
    if (input->mouseButtons[0].isDown) {
        float speed = 0.01f;
        gameState->modelRot =
            QuatFromAngleUnitAxis(input->mouseDelta.x * speed, Vec3::unitY)
            * QuatFromAngleUnitAxis(-input->mouseDelta.y * speed, Vec3::unitX)
            * gameState->modelRot;
        gameState->modelRot = Normalize(gameState->modelRot);
    }
    gameState->cameraPos.z = DEFAULT_CAM_Z
        * powf(CAM_ZOOM_STEP, (float)input->mouseWheel);
    if (WasKeyPressed(input, KM_KEY_ARROW_UP)) {
        gameState->cameraPos.y += CAM_MOVE_STEP;
    }
    if (WasKeyPressed(input, KM_KEY_ARROW_DOWN)) {
        gameState->cameraPos.y -= CAM_MOVE_STEP;
    }
    if (WasKeyPressed(input, KM_KEY_ARROW_LEFT)) {
        gameState->cameraPos.x -= CAM_MOVE_STEP;
    }
    if (WasKeyPressed(input, KM_KEY_ARROW_RIGHT)) {
        gameState->cameraPos.x += CAM_MOVE_STEP;
    }

    UpdateInputFields(&gameState->inputField, 1, input, (void*)gameState);
    UpdateButtons(&gameState->button, 1, input, (void*)gameState);

    UpdateParticleSystem(&gameState->ps, deltaTime);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mat4 proj = Projection(110.0f,
        (float32)screenInfo.size.x / (float32)screenInfo.size.y,
        0.1f, 10.0f);
    Mat4 view = Translate(-gameState->cameraPos)
        * UnitQuatToMat4(gameState->modelRot);
    Mat4 vp = proj * view;

    const float DEBUG_AXES_HALF_LENGTH = 5.0f;
#if GAME_INTERNAL
    const float DEBUG_AXES_COLOR_MAG = 1.0f;
#else
    const float DEBUG_AXES_COLOR_MAG = 0.5f;
#endif
    for (int i = 0; i < 3; i++) {
        Vec3 endPoint = Vec3::zero;
        endPoint.e[i] = DEBUG_AXES_HALF_LENGTH;
        Vec4 axisColor = Vec4::zero;
        axisColor.a = 1.0f;
        axisColor.e[i] = DEBUG_AXES_COLOR_MAG;
        DrawLine(gameState->lineGL, proj, view,
            Vec3::zero, endPoint, axisColor);
        axisColor.e[i] *= 0.5f;
        DrawLine(gameState->lineGL, proj, view,
            -endPoint, Vec3::zero, axisColor);
    }
    // NOTE this one draw call doesn't work...
    // DrawLine(gameState->lineGL, proj, view,
    //     Vec3::zero, -Vec3::unitX * DEBUG_AXES_HALF_LENGTH,
    //     Vec4 { 0.5f, 0.0f, 0.0f, 1.0f });
    // but this one does?! investigate, maybe
    // DrawLine(gameState->lineGL, proj, view,
    //     -Vec3::unitX * DEBUG_AXES_HALF_LENGTH, Vec3::zero,
    //     Vec4 { 0.5f, 0.0f, 0.0f, 1.0f });

    // Get camera right and up vectors for billboard draw
    DEBUG_ASSERT(sizeof(ParticleSystemDataGL) <= memory->transientStorageSize);
    ParticleSystemDataGL* dataGL = (ParticleSystemDataGL*)
        memory->transientStorage;
    Vec3 camRight = { view.e[0][0], view.e[1][0], view.e[2][0] };
    Vec3 camUp = { view.e[0][1], view.e[1][1], view.e[2][1] };
    DrawParticleSystem(gameState->psGL, gameState->boxGL,
        &gameState->ps,
        camRight, camUp, gameState->cameraPos, vp,
        dataGL);

    // Assignment title & name
    DrawText(gameState->textGL, gameState->fontFaceLarge, screenInfo,
        "Particles & Animation",
        Vec2Int { UI_MARGIN, screenInfo.size.y - UI_MARGIN },
        Vec2 { 0.0f, 1.0f },
        interestTextColor
    );
    DrawText(gameState->textGL, gameState->fontFaceLarge, screenInfo,
        "Jose M Rico <jrico>",
        Vec2Int {
            UI_MARGIN,
            screenInfo.size.y - UI_MARGIN
                - (int)gameState->fontFaceLarge.height - UI_SPACING
        },
        Vec2 { 0.0f, 1.0f },
        interestTextColor
    );
    // FPS counter
    char str[512];
    sprintf(str, "FPS: %f", 1.0 / deltaTime);
    DrawText(gameState->textGL, gameState->fontFaceMedium, screenInfo,
        str,
        Vec2Int {
            screenInfo.size.x - UI_MARGIN,
            screenInfo.size.y - UI_MARGIN
        },
        Vec2 { 1.0f, 1.0f },
        interestTextColor
    );
    // Particle counter
    sprintf(str, "Particles: %d", gameState->ps.active);
    DrawText(gameState->textGL, gameState->fontFaceMedium, screenInfo,
        str,
        Vec2Int {
            screenInfo.size.x - UI_MARGIN,
            screenInfo.size.y - UI_MARGIN
                - (int)gameState->fontFaceMedium.height - UI_SPACING
        },
        Vec2 { 1.0f, 1.0f },
        interestTextColor
    );

    DrawInputFields(&gameState->inputField, 1,
        gameState->rectGL, gameState->textGL,
        gameState->fontFaceMedium, screenInfo);
    DrawButtons(&gameState->button, 1,
        gameState->rectGL, gameState->textGL,
        gameState->fontFaceMedium, screenInfo);
}

#include "km_input.cpp"
#include "ogl_base.cpp"
#include "text.cpp"
#include "gui.cpp"
#include "load_png.cpp"
#include "particles.cpp"