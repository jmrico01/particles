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

const char* defaultMesh_ = "bunny.obj";

const Vec4 defaultIdleColor = { 0.2f, 0.2f, 0.2f, 1.0f };
const Vec4 defaultHoverColor = { 0.55f, 0.55f, 0.45f, 1.0f };
const Vec4 defaultPressColor = { 0.8f, 0.8f, 0.65f, 1.0f };
const Vec4 defaultTextColor = { 0.9f, 0.9f, 0.9f, 1.0f };

const Vec4 interestIdleColor = { 0.2f, 0.4f, 0.4f, 1.0f };
const Vec4 interestHoverColor = { 0.4f, 0.6f, 0.6f, 1.0f };
const Vec4 interestPressColor = { 0.6f, 0.8f, 0.8f, 1.0f };
const Vec4 interestTextColor = { 0.7f, 0.9f, 0.9f, 1.0f };

struct ChangeMeshData
{
    GameState* gameState;
    const ThreadContext* thread;
    DEBUGPlatformReadFileFunc* DEBUGPlatformReadFile;
    DEBUGPlatformFreeFileMemoryFunc* DEBUGPlatformFreeFileMemory;
};

internal inline float32 RandFloat()
{
    return (float32)rand() / RAND_MAX;
}
internal inline float32 RandFloat(float32 min, float32 max)
{
    DEBUG_ASSERT(max > min);
    return RandFloat() * (max - min) + min;
}

internal void InitParticleRandom(ParticleSystem* ps, Particle* particle,
    void* data)
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

internal void InitParticleFountain(ParticleSystem* ps, Particle* particle,
    void* data)
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
    particle->bounceMult = RandFloat(0.6f, 1.0f);
}

internal void InitParticleSphere(ParticleSystem* ps, Particle* particle,
    void* data)
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

internal Vec3 RandomPointInTriangle(Vec3 v0, Vec3 v1, Vec3 v2, Vec3 normal)
{
    // Picks random point in parallelogram (v0, v1, v2, v1+v2)
    // Source: http://mathworld.wolfram.com/TrianglePointPicking.html
    Vec3 v0v1 = v1 - v0;
    Vec3 v0v2 = v2 - v0;
    float32 a1 = RandFloat();
    float32 a2 = RandFloat();
    Vec3 randPt = a1 * v0v1 + a2 * v0v2 + v0;

    // "Folds" the outside points into the triangle (my code)
    Vec3 out = Normalize(Cross(v2 - v1, normal));
    float32 dotOut = Dot(randPt - v1, out);
    if (dotOut > 0.0f) {
        randPt -= dotOut * out;
        return Vec3::zero;
    }

    return randPt;
}

internal void InitParticleMesh(ParticleSystem* ps, Particle* particle,
    void* data)
{
    Mesh* mesh = ps->mesh;
    // First, pick a random face, weighted by its area
    float32 totalArea = 0.0f;
    for (int i = 0; i < (int)mesh->triangles.size; i++) {
        totalArea += mesh->triangles[i].area;
    }
    float32 randFace = RandFloat(0.0f, totalArea);
    totalArea = 0.0f;
    int face = -1;
    for (int i = 0; i < (int)mesh->triangles.size; i++) {
        totalArea += mesh->triangles[i].area;
        if (totalArea >= randFace) {
            face = i;
            break;
        }
    }
    DEBUG_ASSERT(face != -1);

    Triangle triangle = mesh->triangles[face];
    Vec3 normal = (triangle.n[0] + triangle.n[1] + triangle.n[2]) / 3.0f;
    particle->life = 0.0f;
    particle->pos = RandomPointInTriangle(
        triangle.v[0], triangle.v[1], triangle.v[2], normal);
    // Minimal velocity
    float32 speed = RandFloat(-0.01f, 0.01f);
    particle->vel = speed * normal;
    particle->color = { 
        RandFloat(),
        RandFloat(),
        RandFloat(),
        1.0f
    };
    float randSize = RandFloat() * 0.04f + 0.02f;
    particle->size = { randSize, randSize };
    particle->bounceMult = 1.0f;
}

internal void InitParticleSphereJet(ParticleSystem* ps, Particle* particle,
    void* data)
{
    particle->life = 0.0f;
    particle->pos = Vec3::unitZ * 3.0f;
    float32 spread = RandFloat(0.0f, 0.5f);
    Vec2 circleVel = {
        RandFloat() - 0.5f,
        RandFloat() - 0.5f
    };
    circleVel = Normalize(circleVel) * spread;
    particle->vel = {
        circleVel.x,
        circleVel.y,
        -RandFloat(2.0f, 4.0f)
    };
    particle->color = {
        RandFloat(0.6f, 1.0f),
        RandFloat(0.2f, 1.0f),
        RandFloat(0.0f, 1.0f),
        1.0f
    };
    float randSize = RandFloat() * 0.1f + 0.05f;
    particle->size = { randSize, randSize };
    particle->bounceMult = RandFloat(0.6f, 1.0f);
}

internal void PresetChange(Button* button, void* data)
{
    GameState* gameState = (GameState*)data;

    Preset preset = PRESET_LAST;
    for (int i = 0; i < PRESET_LAST; i++) {
        gameState->presetButtons[i].box.color = defaultIdleColor;
        gameState->presetButtons[i].box.hoverColor = defaultHoverColor;
        gameState->presetButtons[i].box.pressColor = defaultPressColor;
        if (button == &gameState->presetButtons[i]) {
            gameState->presetButtons[i].box.color = interestIdleColor;
            gameState->presetButtons[i].box.hoverColor = interestHoverColor;
            gameState->presetButtons[i].box.pressColor = interestPressColor;
            preset = (Preset)i;
        }
    }

    gameState->activePreset = preset;
    switch (preset) {
        case PRESET_SPHERE: {
            CreateParticleSystem(&gameState->ps,
                MAX_PARTICLES, 500, 5.0f, Vec3 { 0.0f, 0.0f, 0.0f },
                0.0f, 0.0f,
                nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0,
                InitParticleSphere, gameState->pTexSpark,
                nullptr, nullptr);
        } break;
        case PRESET_FOUNTAIN_SINK: {
            PlaneCollider groundPlane;
            groundPlane.type = COLLIDER_SINK;
            groundPlane.normal = Vec3::unitY;
            groundPlane.point = Vec3::zero;
            CreateParticleSystem(&gameState->ps,
                10000, 1000, 10.0f, Vec3 { 0.0f, -1.0f, 0.0f },
                0.1f, 0.05f,
                nullptr, 0, &groundPlane, 1, nullptr, 0, nullptr, 0,
                InitParticleFountain, gameState->pTexBase,
                nullptr, nullptr);
        } break;
        case PRESET_FOUNTAIN_BOUNCE: {
            PlaneCollider groundPlane;
            groundPlane.type = COLLIDER_BOUNCE;
            groundPlane.normal = Vec3::unitY;
            groundPlane.point = Vec3::zero;
            CreateParticleSystem(&gameState->ps,
                1500, 1500, 10.0f, Vec3 { 0.0f, -1.0f, 0.0f },
                0.1f, 0.05f,
                nullptr, 0, &groundPlane, 1, nullptr, 0, nullptr, 0,
                InitParticleFountain, gameState->pTexBase,
                nullptr, nullptr);
        } break;
        case PRESET_BOX_COLLIDERS: {
        } break;
        case PRESET_SPHERE_COLLIDERS: {
            SphereCollider spheres[3];
            spheres[0].type = COLLIDER_BOUNCE;
            spheres[0].center = { 0.2f, 0.2f, -2.0f };
            spheres[0].radius = 1.1f;
            spheres[1].type = COLLIDER_BOUNCE;
            spheres[1].center = { -4.0f, -4.0f, 1.0f };
            spheres[1].radius = 2.0f;
            spheres[2].type = COLLIDER_SINK;
            spheres[2].center = { 0.2f, 0.2f, 0.0f };
            spheres[2].radius = 0.25f;
            CreateParticleSystem(&gameState->ps,
                MAX_PARTICLES, 500, 6.0f, Vec3 { 0.0f, 0.0f, 0.0f },
                0.1f, 0.05f,
                nullptr, 0, nullptr, 0, nullptr, 0, spheres, 3,
                InitParticleSphereJet, gameState->pTexFire,
                nullptr, nullptr);
        } break;
        case PRESET_ATTRACTORS: {
            Attractor a[4];
            a[0].pos = Vec3 { 2.0f, 4.0f, 0.0f };
            a[0].strength = 2.5f;
            a[1].pos = Vec3 { 1.0f, 0.0f, 1.0f };
            a[1].strength = 1.0f;
            a[2].pos = Vec3 { -5.0f, -1.0f, 0.0f };
            a[2].strength = 4.0f;
            a[3].pos = Vec3 { -5.0f, 5.0, -5.0f };
            a[3].strength = 2.0f;
            CreateParticleSystem(&gameState->ps,
                10000, 1000, 10.0f, Vec3 { 0.0f, 0.0f, 0.0f },
                0.2f, 0.2f,
                a, 4, nullptr, 0, nullptr, 0, nullptr, 0,
                InitParticleRandom, gameState->pTexBase,
                nullptr, nullptr);
        } break;
        case PRESET_MESH: {
            CreateParticleSystem(&gameState->ps,
                MAX_PARTICLES, 1000, 2.0f, Vec3 { 0.0f, 0.0f, 0.0f },
                0.1f, 0.05f,
                nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0,
                InitParticleMesh, gameState->pTexBase,
                &gameState->loadedMesh, &gameState->loadedMeshGL);
        } break;

        case PRESET_LAST: {
        } break;
    }
}

internal void UpdatePresetLayout(GameState* gameState, ScreenInfo screenInfo)
{
    Vec2Int size = {
        150, (int)gameState->fontFaceMedium.height + UI_SPACING
    };
    Vec2Int origin = {
        screenInfo.size.x - UI_MARGIN - size.x,
        UI_MARGIN
    };
    Vec2Int stride = {
        0, size.y + UI_SPACING
    };
    for (int i = 0; i < PRESET_LAST; i++) {
        gameState->presetButtons[i].box.origin = origin + stride * i;
        gameState->presetButtons[i].box.size = size;
        gameState->presetButtons[i].box.color = defaultIdleColor;
        gameState->presetButtons[i].box.hoverColor = defaultHoverColor;
        gameState->presetButtons[i].box.pressColor = defaultPressColor;
    }
    Preset activePreset = gameState->activePreset;
    if (activePreset != PRESET_LAST) {
        gameState->presetButtons[activePreset].box.color
            = interestIdleColor;
        gameState->presetButtons[activePreset].box.hoverColor
            = interestHoverColor;
        gameState->presetButtons[activePreset].box.pressColor
            = interestPressColor;
    }
}

internal void ToggleDrawColliders(Button* button, void* data)
{
    GameState* gameState = (GameState*)data;
    gameState->drawColliders = !gameState->drawColliders;
}

internal bool32 IsFile(const ThreadContext* thread, const char* path,
    DEBUGPlatformReadFileFunc DEBUGPlatformReadFile,
    DEBUGPlatformFreeFileMemoryFunc DEBUGPlatformFreeFileMemory)
{
    DEBUGReadFileResult objFile = DEBUGPlatformReadFile(thread, path);
    if (!objFile.data) {
        return false;
    }
    DEBUGPlatformFreeFileMemory(thread, &objFile);
    return true;
}

internal void ChangeMesh(InputField* field, void* data)
{
    ChangeMeshData* cmData = (ChangeMeshData*)data;
    GameState* gameState = cmData->gameState;

    char meshPath[256];
    sprintf(meshPath, "data/models/%s", field->text);
    if (IsFile(cmData->thread, meshPath,
    cmData->DEBUGPlatformReadFile, cmData->DEBUGPlatformFreeFileMemory)) {
        FreeMesh(&gameState->loadedMesh);
        FreeMeshGL(&gameState->loadedMeshGL);
        gameState->loadedMesh = LoadMeshFromObj(cmData->thread,
            meshPath,
            cmData->DEBUGPlatformReadFile,
            cmData->DEBUGPlatformFreeFileMemory);
        gameState->loadedMeshGL = LoadMeshGL(cmData->thread,
            gameState->loadedMesh,
            cmData->DEBUGPlatformReadFile,
            cmData->DEBUGPlatformFreeFileMemory);
    }
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

		glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);

        gameState->cameraPos = { 0.0f, 0.0f, DEFAULT_CAM_Z };
        gameState->modelRot = QuatFromAngleUnitAxis(PI_F / 6.0f, Vec3::unitX)
            * QuatFromAngleUnitAxis(-PI_F / 4.0f, Vec3::unitY);

        gameState->drawColliders = false;

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
        gameState->planeGL = InitPlaneGL(thread,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->boxGL = InitBoxGL(thread,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->psGL = InitParticleSystemGL(thread,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->sphereMesh = LoadMeshFromObj(thread,
            "data/models/sphere-2res.obj",
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->sphereMeshGL = LoadMeshGL(thread,
            gameState->sphereMesh,
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

        gameState->activePreset = PRESET_SPHERE;
        for (int i = 0; i < PRESET_LAST; i++) {
            gameState->presetButtons[i] = CreateButton(
                Vec2Int::zero, Vec2Int::zero,
                presetNames_[i], PresetChange,
                defaultIdleColor,
                defaultHoverColor,
                defaultPressColor,
                defaultTextColor
            );
        }
        UpdatePresetLayout(gameState, screenInfo);

        Vec2Int drawCollidersOrigin = { UI_MARGIN, UI_MARGIN };
        Vec2Int drawCollidersSize = {
            150,
            (int)gameState->fontFaceMedium.height + UI_SPACING
        };
        gameState->drawCollidersButton = CreateButton(
            drawCollidersOrigin, drawCollidersSize,
            "Draw Objects", ToggleDrawColliders,
            interestIdleColor, interestHoverColor, interestPressColor,
            defaultTextColor
        );

        Vec2Int modelFieldOrigin = {
            drawCollidersOrigin.x,
            drawCollidersOrigin.y + drawCollidersSize.y + UI_SPACING * 2
        };
        Vec2Int modelFieldSize = drawCollidersSize;
        gameState->modelField = CreateInputField(
            modelFieldOrigin, modelFieldSize,
            defaultMesh_, ChangeMesh,
            defaultIdleColor, defaultHoverColor, defaultPressColor,
            defaultTextColor);

        ChangeMeshData cmData;
        cmData.gameState = gameState;
        cmData.thread = thread;
        cmData.DEBUGPlatformReadFile = platformFuncs->DEBUGPlatformReadFile;
        cmData.DEBUGPlatformFreeFileMemory =
            platformFuncs->DEBUGPlatformFreeFileMemory;
        ChangeMesh(&gameState->modelField, (void*)&cmData);
        /*char meshPath[256];
        sprintf(meshPath, "data/models/%s", defaultMesh_);
        gameState->loadedMesh = LoadMeshFromObj(thread,
            meshPath,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);
        gameState->loadedMeshGL = LoadMeshGL(thread, gameState->loadedMesh,
            platformFuncs->DEBUGPlatformReadFile,
            platformFuncs->DEBUGPlatformFreeFileMemory);*/

        // Initializes particle system
        PresetChange(&gameState->presetButtons[gameState->activePreset],
            (void*)gameState);

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

    Vec3 forward = -Vec3::unitZ;
    Vec3 right = Vec3::unitX;
    if (input->keyboard[KM_KEY_ARROW_UP].isDown
    || input->keyboard[KM_KEY_W].isDown) {
        gameState->cameraPos += forward * deltaTime;
    }
    if (input->keyboard[KM_KEY_ARROW_DOWN].isDown
    || input->keyboard[KM_KEY_S].isDown) {
        gameState->cameraPos -= forward * deltaTime;
    }
    if (input->keyboard[KM_KEY_ARROW_RIGHT].isDown
    || input->keyboard[KM_KEY_D].isDown) {
        gameState->cameraPos += right * deltaTime;
    }
    if (input->keyboard[KM_KEY_ARROW_LEFT].isDown
    || input->keyboard[KM_KEY_A].isDown) {
        gameState->cameraPos -= right * deltaTime;
    }

    UpdatePresetLayout(gameState, screenInfo);
    UpdateButtons(gameState->presetButtons, PRESET_LAST,
        input, (void*)gameState);

    gameState->drawCollidersButton.box.color = defaultIdleColor;
    gameState->drawCollidersButton.box.hoverColor = defaultHoverColor;
    gameState->drawCollidersButton.box.pressColor = defaultPressColor;
    if (gameState->drawColliders) {
        gameState->drawCollidersButton.box.color = interestIdleColor;
        gameState->drawCollidersButton.box.hoverColor = interestHoverColor;
        gameState->drawCollidersButton.box.pressColor = interestPressColor;
    }
    UpdateButtons(&gameState->drawCollidersButton, 1,
        input, (void*)gameState);
    ChangeMeshData cmData;
    cmData.gameState = gameState;
    cmData.thread = thread;
    cmData.DEBUGPlatformReadFile = platformFuncs->DEBUGPlatformReadFile;
    cmData.DEBUGPlatformFreeFileMemory =
        platformFuncs->DEBUGPlatformFreeFileMemory;
    UpdateInputFields(&gameState->modelField, 1,
        input, (void*)&cmData);

    UpdateParticleSystem(&gameState->ps, deltaTime, nullptr);

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
    if (gameState->drawColliders) {
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
    }
    // NOTE this one draw call doesn't work...
    // DrawLine(gameState->lineGL, proj, view,
    //     Vec3::zero, -Vec3::unitX * DEBUG_AXES_HALF_LENGTH,
    //     Vec4 { 0.5f, 0.0f, 0.0f, 1.0f });
    // but this one does?! investigate, maybe
    // DrawLine(gameState->lineGL, proj, view,
    //     -Vec3::unitX * DEBUG_AXES_HALF_LENGTH, Vec3::zero,
    //     Vec4 { 0.5f, 0.0f, 0.0f, 1.0f });

    DEBUG_ASSERT(sizeof(ParticleSystemDataGL) <= memory->transientStorageSize);
    ParticleSystemDataGL* dataGL = (ParticleSystemDataGL*)
        memory->transientStorage;
    // Get camera right and up vectors for billboard draw
    Vec3 camRight = { view.e[0][0], view.e[1][0], view.e[2][0] };
    Vec3 camUp = { view.e[0][1], view.e[1][1], view.e[2][1] };
    Vec3 camOut = { view.e[0][2], view.e[1][2], view.e[2][2] };
    DrawParticleSystem(gameState->psGL,
        gameState->planeGL, gameState->boxGL, gameState->sphereMeshGL,
        &gameState->ps,
        camRight, camUp, gameState->cameraPos, proj, view,
        dataGL,
        gameState->drawColliders);

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

    DrawButtons(gameState->presetButtons, PRESET_LAST,
        gameState->rectGL, gameState->textGL,
        gameState->fontFaceMedium, screenInfo);
    DrawButtons(&gameState->drawCollidersButton, 1,
        gameState->rectGL, gameState->textGL,
        gameState->fontFaceMedium, screenInfo);
    Vec2Int modelFieldTextPos = gameState->modelField.box.origin;
    modelFieldTextPos.y += gameState->modelField.box.size.y + UI_SPACING;
    DrawText(gameState->textGL, gameState->fontFaceMedium, screenInfo,
        "Loaded Model:", modelFieldTextPos, defaultTextColor);
    DrawInputFields(&gameState->modelField, 1,
        gameState->rectGL, gameState->textGL,
        gameState->fontFaceMedium, screenInfo);
}

#include "km_input.cpp"
#include "km_lib.cpp"
#include "ogl_base.cpp"
#include "text.cpp"
#include "gui.cpp"
#include "load_png.cpp"
#include "particles.cpp"
#include "mesh.cpp"