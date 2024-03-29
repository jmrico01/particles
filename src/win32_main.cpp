#include "win32_main.h"

#include <stdio.h>
#include <stdarg.h>
#include <Xinput.h>
#include <intrin.h> // __rdtsc

#include "opengl.h"
#include "km_debug.h"
#include "km_input.h"

/*
    TODO

    - WINDOWS 7 VIRTUAL MACHINE

    - Saved game locations
    - Getting a handle to our own exe file
    - Asset loading path
    - Threading (launch a thread)
    - Sleep/timeBeginPeriod (don't melt the processor)
    - ClipCursor() (multimonitor support)
    - WM_SETCURSOR (control cursor visibility)
    - QueryCancelAutoplay
    - WM_ACTIVATEAPP (for when we are not the active app)
*/

/*
.clang_complete, can it make do without this?
-IC:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include
-IC:/Program Files (x86)/Windows Kits/10/Include/10.0.15063.0/ucrt
-IC:/Program Files (x86)/Windows Kits/10/Include/10.0.15063.0/um
-IC:/Program Files (x86)/Windows Kits/10/Include/10.0.15063.0/shared
*/

#define START_WIDTH 1280
#define START_HEIGHT 800

#define WIN32_MAX_KEYCODE 256

// TODO this is a global for now
global_var char pathToApp_[MAX_PATH];
global_var bool32 running_ = true;
global_var GameInput* input_ = nullptr;             // for WndProc WM_CHAR
global_var glViewportFunc* glViewport_ = nullptr;   // for WndProc WM_SIZE
global_var ScreenInfo* screenInfo_ = nullptr;       // for WndProc WM_SIZE
global_var KeyInputCode toKM_[WIN32_MAX_KEYCODE];

global_var bool32 DEBUGshowCursor_;
global_var WINDOWPLACEMENT DEBUGwpPrev = { sizeof(DEBUGwpPrev) };

// XInput functions
#define XINPUT_GET_STATE_FUNC(name) DWORD WINAPI name(DWORD dwUserIndex, \
    XINPUT_STATE *pState)
typedef XINPUT_GET_STATE_FUNC(XInputGetStateFunc);
XINPUT_GET_STATE_FUNC(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
internal XInputGetStateFunc *xInputGetState_ = XInputGetStateStub;
#define XInputGetState xInputGetState_

#define XINPUT_SET_STATE_FUNC(name) DWORD WINAPI name(DWORD dwUserIndex, \
    XINPUT_VIBRATION *pVibration)
typedef XINPUT_SET_STATE_FUNC(XInputSetStateFunc);
XINPUT_SET_STATE_FUNC(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
internal XInputSetStateFunc *xInputSetState_ = XInputSetStateStub;
#define XInputSetState xInputSetState_

// WGL functions
typedef BOOL WINAPI wglSwapIntervalEXTFunc(int interval);
global_var wglSwapIntervalEXTFunc* wglSwapInterval_ = NULL;
#define wglSwapInterval wglSwapInterval_

internal int StringLength(const char* string)
{
    int length = 0;
    while (*string++) {
        length++;
    }

    return length;
}
internal void CatStrings(
    size_t sourceACount, const char* sourceA,
    size_t sourceBCount, const char* sourceB,
    size_t destCount, char* dest)
{
    for (size_t i = 0; i < sourceACount; i++) {
        *dest++ = *sourceA++;
    }

    for (size_t i = 0; i < sourceBCount; i++) {
        *dest++ = *sourceB++;
    }

    *dest++ = '\0';
}

internal void Win32ToggleFullscreen(HWND hWnd, OpenGLFunctions* glFuncs)
{
    // This follows Raymond Chen's perscription for fullscreen toggling. See:
    // https://blogs.msdn.microsoft.com/oldnewthing/20100412-00/?p=14353

    DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
    if (dwStyle & WS_OVERLAPPEDWINDOW) {
        // Switch to fullscreen
        MONITORINFO monitorInfo = { sizeof(monitorInfo) };
        HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
        if (GetWindowPlacement(hWnd, &DEBUGwpPrev)
        && GetMonitorInfo(hMonitor, &monitorInfo)) {
            SetWindowLong(hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hWnd, HWND_TOP,
                monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            glFuncs->glViewport(
                (GLint)monitorInfo.rcMonitor.left,
                (GLint)monitorInfo.rcMonitor.top,
                (GLsizei)(monitorInfo.rcMonitor.right
                    - monitorInfo.rcMonitor.left),
                (GLsizei)(monitorInfo.rcMonitor.bottom
                    - monitorInfo.rcMonitor.top));
        }
    }
    else {
        // Switch to windowed
        SetWindowLong(hWnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hWnd, &DEBUGwpPrev);
        SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER
            | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        glFuncs->glViewport(
            (GLint)clientRect.left, (GLint)clientRect.top,
            (GLsizei)(clientRect.right - clientRect.left),
            (GLsizei)(clientRect.bottom - clientRect.top));
    }
}

internal void RemoveFileNameFromPath(
    const char* filePath, char* dest, uint64 destLength)
{
    unsigned int lastSlash = 0;
    // TODO confused... some cross-platform code inside a win32 file
    //  maybe I meant to pull this out sometime?
#ifdef _WIN32
    char pathSep = '\\';
#else
    char pathSep = '/';
#endif
    while (filePath[lastSlash] != '\0') {
        lastSlash++;
    }
    // TODO unsafe!
    while (filePath[lastSlash] != pathSep) {
        lastSlash--;
    }
    if (lastSlash + 2 > destLength) {
        return;
    }
    for (unsigned int i = 0; i < lastSlash + 1; i++) {
        dest[i] = filePath[i];
    }
    dest[lastSlash + 1] = '\0';
}

internal void Win32GetExeFilePath(Win32State* state)
{
    // NOTE
    // Never use MAX_PATH in code that is user-facing, because it is
    // dangerous and can lead to bad results
    DWORD size = GetModuleFileName(NULL,
        state->exeFilePath, sizeof(state->exeFilePath));
    state->exeOnePastLastSlash = state->exeFilePath;
    for (char* scan = state->exeFilePath; *scan; scan++) {
        if (*scan == '\\') {
            state->exeOnePastLastSlash = scan + 1;
        }
    }
}
internal void Win32BuildExePathFileName(Win32State* state,
    const char* fileName,
    int destCount, char* dest)
{
    CatStrings(state->exeOnePastLastSlash - state->exeFilePath,
        state->exeFilePath, StringLength(fileName), fileName,
        destCount, dest);
}

internal void Win32GetInputFileLocation(
    Win32State* state, bool32 inputStream,
    int slotIndex, int destCount, char* dest)
{
    char temp[64];
    wsprintf(temp, "recording_%d_%s.kmi",
        slotIndex, inputStream ? "input" : "state");
    Win32BuildExePathFileName(state, temp, destCount, dest);
}

inline FILETIME Win32GetLastWriteTime(char* fileName)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesEx(fileName, GetFileExInfoStandard, &data)) {
        // TODO log?
        FILETIME zero = {};
        return zero;
    }
    return data.ftLastWriteTime;
}

internal Win32GameCode Win32LoadGameCode(
    char* gameCodeDLLPath, char* tempCodeDLLPath)
{
    Win32GameCode result = {};
    result.isValid = false;

    result.lastDLLWriteTime = Win32GetLastWriteTime(gameCodeDLLPath);
    CopyFile(gameCodeDLLPath, tempCodeDLLPath, FALSE);
    result.gameCodeDLL = LoadLibrary(tempCodeDLLPath);
    if (result.gameCodeDLL) {
        result.gameUpdateAndRender =
            (GameUpdateAndRenderFunc*)GetProcAddress(result.gameCodeDLL, "GameUpdateAndRender");

        result.isValid = result.gameUpdateAndRender != 0;
    }

    if (!result.isValid) {
        result.gameUpdateAndRender = 0;
    }

    return result;
}

internal void Win32UnloadGameCode(Win32GameCode* gameCode)
{
    if (gameCode->gameCodeDLL) {
        FreeLibrary(gameCode->gameCodeDLL);
    }

    gameCode->gameCodeDLL = NULL;
    gameCode->isValid = false;
    gameCode->gameUpdateAndRender = 0;
}

internal inline uint32 SafeTruncateUInt64(uint64 value)
{
    // TODO defines for maximum values
    DEBUG_ASSERT(value <= 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return result;
}

#if GAME_INTERNAL

DEBUG_PLATFORM_PRINT_FUNC(DEBUGPlatformPrint)
{
    const int MSG_MAX_LENGTH = 1024;
    const char* MSG_PREFIX = "";
    const char* MSG_SUFFIX = "";
    char msg[MSG_MAX_LENGTH];

    int cx1 = snprintf(msg, MSG_MAX_LENGTH, "%s", MSG_PREFIX);
    if (cx1 < 0) {
        return;
    }

    va_list args;
    va_start(args, format);
    int cx2 = vsnprintf(msg + cx1, MSG_MAX_LENGTH - cx1, format, args);
    va_end(args);
    if (cx2 < 0) {
        return;
    }

    int cx3 = snprintf(msg + cx1 + cx2, MSG_MAX_LENGTH - cx1 - cx2,
        "%s", MSG_SUFFIX);
    if (cx3 < 0) {
        return;
    }

    if ((cx1 + cx2 + cx3) >= MSG_MAX_LENGTH) {
        // error message too long. warn? ignore for now
    }

    OutputDebugString(msg);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY_FUNC(DEBUGPlatformFreeFileMemory)
{
    if (file->data) {
        VirtualFree(file->data, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_FILE_FUNC(DEBUGPlatformReadFile)
{
    DEBUGReadFileResult result = {};
    
    char fullPath[MAX_PATH];
    CatStrings(StringLength(pathToApp_), pathToApp_,
        StringLength(fileName), fileName, MAX_PATH, fullPath);

    HANDLE hFile = CreateFile(fullPath, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, NULL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        // TODO log
        return result;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        // TODO log
        return result;
    }

    uint32 fileSize32 = SafeTruncateUInt64(fileSize.QuadPart);
    result.data = VirtualAlloc(0, fileSize32,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!result.data) {
        // TODO log
        return result;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, result.data, fileSize32, &bytesRead, NULL) ||
    fileSize32 != bytesRead) {
        // TODO log
        DEBUGPlatformFreeFileMemory(thread, &result);
        return result;
    }

    result.size = fileSize32;
    CloseHandle(hFile);
    return result;
}

DEBUG_PLATFORM_WRITE_FILE_FUNC(DEBUGPlatformWriteFile)
{
    HANDLE hFile = CreateFile(fileName, GENERIC_WRITE, NULL,
        NULL, CREATE_ALWAYS, NULL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        // TODO log
        return false;
    }

    DWORD bytesWritten;
    if (!WriteFile(hFile, memory, memorySize, &bytesWritten, NULL)) {
        // TODO log
        return false;
    }

    CloseHandle(hFile);
    return bytesWritten == memorySize;
}

#endif

internal void Win32LoadXInput()
{
    HMODULE xInputLib = LoadLibrary("xinput1_4.dll");
    if (!xInputLib) {
        xInputLib = LoadLibrary("xinput1_3.dll");
    }
    if (!xInputLib) {
        // TODO warning message?
        xInputLib = LoadLibrary("xinput9_1_0.dll");
    }

    if (xInputLib) {
        XInputGetState = (XInputGetStateFunc*)GetProcAddress(xInputLib,
            "XInputGetState");
        XInputSetState = (XInputSetStateFunc*)GetProcAddress(xInputLib,
            "XInputSetState");
    }
}

LRESULT CALLBACK WndProc(
    HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message) {
        case WM_ACTIVATEAPP: {
            // TODO handle
        } break;
        case WM_CLOSE: {
            // TODO handle this with a message?
            running_ = false;
        } break;
        case WM_DESTROY: {
            // TODO handle this as an error?
            running_ = false;
        } break;

        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            if (glViewport_) {
                glViewport_(0, 0, width, height);
            }
            if (screenInfo_) {
                screenInfo_->size.x = width;
                screenInfo_->size.y = height;
            }
        } break;

        case WM_SETCURSOR: {
            if (DEBUGshowCursor_)
                result = DefWindowProc(hWnd, message, wParam, lParam);
            else
                SetCursor(0);
        } break;

        case WM_SYSKEYDOWN: {
            DEBUG_PANIC("WM_SYSKEYDOWN in WndProc");
        } break;
        case WM_SYSKEYUP: {
            DEBUG_PANIC("WM_SYSKEYUP in WndProc");
        } break;
        case WM_KEYDOWN: {
        } break;
        case WM_KEYUP: {
        } break;

        case WM_CHAR: {
            char c = (char)wParam;
            input_->keyboardString[input_->keyboardStringLen++] = c;
            input_->keyboardString[input_->keyboardStringLen] = '\0';
        } break;

        default: {
            result = DefWindowProc(hWnd, message, wParam, lParam);
        } break;
    }

    return result;
}

internal void Win32InitKeyCodeMap()
{
    for (int i = 0; i < WIN32_MAX_KEYCODE; i++) {
        toKM_[i] = KM_KEY_UNMAPPED;
    }

    // Numbers
    for (int i = 0x30; i < 0x3a; i++) {
        toKM_[i] = (KeyInputCode)(i - 0x30 + KM_KEY_0);
    }
    // Letters
    for (int i = 0x41; i < 0x5b; i++) {
        toKM_[i] = (KeyInputCode)(i - 0x41 + KM_KEY_A);
    }
    // Other text input
    toKM_[VK_SPACE] = KM_KEY_SPACE;
    toKM_[VK_BACK] = KM_KEY_BACKSPACE;
    // Arrow keys
    toKM_[VK_UP] = KM_KEY_ARROW_UP;
    toKM_[VK_DOWN] = KM_KEY_ARROW_DOWN;
    toKM_[VK_LEFT] = KM_KEY_ARROW_LEFT;
    toKM_[VK_RIGHT] = KM_KEY_ARROW_RIGHT;
    // Special keys
    toKM_[VK_ESCAPE] = KM_KEY_ESCAPE;
    toKM_[VK_SHIFT] = KM_KEY_SHIFT;
    toKM_[VK_CONTROL] = KM_KEY_CTRL;
    toKM_[VK_TAB] = KM_KEY_TAB;
    toKM_[VK_RETURN] = KM_KEY_ENTER;
    // Numpad
    for (int i = 0x60; i < 0x6a; i++) {
        toKM_[i] = (KeyInputCode)(i - 0x60 + KM_KEY_NUMPAD_0);
    }
}

internal inline KeyInputCode Win32KeyCodeToKM(int vkCode)
{
    return toKM_[vkCode];
}

internal void Win32ProcessMessages(
    HWND hWnd, GameInput* gameInput,
    OpenGLFunctions* glFuncs)
{
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        switch (msg.message) {
        case WM_QUIT: {
            running_ = false;
        } break;

        case WM_SYSKEYDOWN: {
            uint32 vkCode = (uint32)msg.wParam;
            bool32 altDown = (msg.lParam & (1 << 29));

            if (vkCode == VK_F4 && altDown) {
                running_ = false;
            }
        } break;
        case WM_SYSKEYUP: {
        } break;
        case WM_KEYDOWN: {
            uint32 vkCode = (uint32)msg.wParam;
            bool32 wasDown = ((msg.lParam & (1 << 30)) != 0);
            bool32 isDown = ((msg.lParam & (1 << 31)) == 0);
            int transitions = (wasDown != isDown) ? 1 : 0;
            DEBUG_ASSERT(isDown);

            int kmKeyCode = Win32KeyCodeToKM(vkCode);
            if (kmKeyCode != KM_KEY_UNMAPPED) {
                gameInput->keyboard[kmKeyCode].isDown = isDown;
                gameInput->keyboard[kmKeyCode].transitions = transitions;
            }

            if (vkCode == VK_ESCAPE) {
                // TODO eventually handle this in the game layer
                running_ = false;
            }
            if (vkCode == VK_F11) {
                Win32ToggleFullscreen(hWnd, glFuncs);
            }

            // Pass over to WndProc for WM_CHAR messages (string input)
            input_ = gameInput;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } break;
        case WM_KEYUP: {
            uint32 vkCode = (uint32)msg.wParam;
            bool32 wasDown = ((msg.lParam & (1 << 30)) != 0);
            bool32 isDown = ((msg.lParam & (1 << 31)) == 0);
            int transitions = (wasDown != isDown) ? 1 : 0;
            DEBUG_ASSERT(!isDown);

            int kmKeyCode = Win32KeyCodeToKM(vkCode);
            if (kmKeyCode != KM_KEY_UNMAPPED) {
                gameInput->keyboard[kmKeyCode].isDown = isDown;
                gameInput->keyboard[kmKeyCode].transitions = transitions;
            }
        } break;

        case WM_LBUTTONDOWN: {
            gameInput->mouseButtons[0].isDown = true;
            gameInput->mouseButtons[0].transitions = 1;
        } break;
        case WM_LBUTTONUP: {
            gameInput->mouseButtons[0].isDown = false;
            gameInput->mouseButtons[0].transitions = 1;
        } break;
        case WM_RBUTTONDOWN: {
            gameInput->mouseButtons[1].isDown = true;
            gameInput->mouseButtons[1].transitions = 1;
        } break;
        case WM_RBUTTONUP: {
            gameInput->mouseButtons[1].isDown = false;
            gameInput->mouseButtons[1].transitions = 1;
        } break;
        case WM_MBUTTONDOWN: {
            gameInput->mouseButtons[2].isDown = true;
            gameInput->mouseButtons[2].transitions = 1;
        } break;
        case WM_MBUTTONUP: {
            gameInput->mouseButtons[2].isDown = false;
            gameInput->mouseButtons[2].transitions = 1;
        } break;
        case WM_XBUTTONDOWN: {
            if ((msg.wParam & MK_XBUTTON1) != 0) {
                gameInput->mouseButtons[3].isDown = true;
                gameInput->mouseButtons[3].transitions = 1;
            }
            else if ((msg.wParam & MK_XBUTTON2) != 0) {
                gameInput->mouseButtons[4].isDown = true;
                gameInput->mouseButtons[4].transitions = 1;
            }
        } break;
        case WM_XBUTTONUP: {
            if ((msg.wParam & MK_XBUTTON1) != 0) {
                gameInput->mouseButtons[3].isDown = false;
                gameInput->mouseButtons[3].transitions = 1;
            }
            else if ((msg.wParam & MK_XBUTTON2) != 0) {
                gameInput->mouseButtons[4].isDown = false;
                gameInput->mouseButtons[4].transitions = 1;
            }
        } break;

        case WM_MOUSEWHEEL: {
            // TODO standardize these units
            gameInput->mouseWheel += GET_WHEEL_DELTA_WPARAM(msg.wParam);
        } break;

        default: {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } break;
        }
    }
}

internal void Win32ProcessXInputButton(
    DWORD xInputButtonState,
    GameButtonState *oldState,
    DWORD buttonBit,
    GameButtonState *newState)
{
    newState->isDown = ((xInputButtonState & buttonBit) == buttonBit);
    newState->transitions = (oldState->isDown != newState->isDown) ? 1 : 0;
}

internal float32 Win32ProcessXInputStickValue(SHORT value, SHORT deadZone)
{
    if (value < -deadZone) {
        return (float32)(value + deadZone) / (32768.0f - deadZone);
    }
    else if (value > deadZone) {
        return (float32)(value - deadZone) / (32767.0f - deadZone);
    }
    else {
        return 0.0f;
    }
}

internal void Win32RecordInputBegin(Win32State* state, int inputRecordingIndex)
{
    DEBUG_ASSERT(inputRecordingIndex < ARRAY_COUNT(state->replayBuffers));

    Win32ReplayBuffer* replayBuffer =
        &state->replayBuffers[inputRecordingIndex];
    if (replayBuffer->gameMemoryBlock) {
        state->inputRecordingIndex = inputRecordingIndex;

        char filePath[MAX_PATH];
        Win32GetInputFileLocation(state, true, inputRecordingIndex,
            sizeof(filePath), filePath);
        state->recordingHandle = CreateFile(filePath, GENERIC_WRITE,
            0, NULL, CREATE_ALWAYS, 0, NULL);

        CopyMemory(replayBuffer->gameMemoryBlock,
            state->gameMemoryBlock, state->gameMemorySize);
    }
}
internal void Win32RecordInputEnd(Win32State* state)
{
    state->inputRecordingIndex = 0;
    CloseHandle(state->recordingHandle);
}
internal void Win32RecordInput(Win32State* state, GameInput* input)
{
    DWORD bytesWritten;
    WriteFile(state->recordingHandle,
        input, sizeof(*input), &bytesWritten, NULL);
}

internal void Win32PlaybackBegin(Win32State* state, int inputPlayingIndex)
{
    DEBUG_ASSERT(inputPlayingIndex < ARRAY_COUNT(state->replayBuffers));
    Win32ReplayBuffer* replayBuffer = &state->replayBuffers[inputPlayingIndex];

    if (replayBuffer->gameMemoryBlock)
    {
        state->inputPlayingIndex = inputPlayingIndex;

        char filePath[MAX_PATH];
        Win32GetInputFileLocation(state, true, inputPlayingIndex,
            sizeof(filePath), filePath);
        state->playbackHandle = CreateFile(filePath, GENERIC_READ,
            0, NULL, OPEN_EXISTING, 0, NULL);

        CopyMemory(state->gameMemoryBlock, replayBuffer->gameMemoryBlock, state->gameMemorySize);
    }
}
internal void Win32PlaybackEnd(Win32State* state)
{
    state->inputPlayingIndex = 0;
    CloseHandle(state->playbackHandle);
}
internal void Win32PlayInput(Win32State* state, GameInput* input)
{
    DWORD bytesRead;
    if (ReadFile(state->playbackHandle,
    input, sizeof(*input), &bytesRead, NULL)) {
        if (bytesRead == 0) {
            int playingIndex = state->inputPlayingIndex;
            Win32PlaybackEnd(state);
            Win32PlaybackBegin(state, playingIndex);
        }
    }
}

#define LOAD_GL_FUNCTION(name) \
    glFuncs->name = (name##Func*)wglGetProcAddress(#name); \
    if (!glFuncs->name) { \
        glFuncs->name = (name##Func*)GetProcAddress(oglLib, #name); \
        if (!glFuncs->name) { \
            DEBUG_PRINT("OpenGL function load failed: %s", #name); \
        } \
    }

internal bool Win32LoadBaseGLFunctions(
    OpenGLFunctions* glFuncs, const HMODULE& oglLib)
{
    // Generate function loading code
#define FUNC(returntype, name, ...) LOAD_GL_FUNCTION(name);
    GL_FUNCTIONS_BASE
#undef FUNC

    return true;
}

internal bool Win32LoadAllGLFunctions(
    OpenGLFunctions* glFuncs, const HMODULE& oglLib)
{
    // Generate function loading code
#define FUNC(returntype, name, ...) LOAD_GL_FUNCTION(name);
    GL_FUNCTIONS_ALL
#undef FUNC

    return true;
}

internal bool Win32InitOpenGL(OpenGLFunctions* glFuncs,
    int width, int height)
{
    HMODULE oglLib = LoadLibrary("opengl32.dll");
    if (!oglLib) {
        DEBUG_PRINT("Failed to load opengl32.dll\n");
        return false;
    }

    if (!Win32LoadBaseGLFunctions(glFuncs, oglLib)) {
        // TODO logging (base GL loading failed, but context exists. weirdness)
        return false;
    }
    glViewport_ = glFuncs->glViewport;

    glFuncs->glViewport(0, 0, width, height);

    // Set v-sync
    // NOTE this isn't technically complete. we have to ask Windows
    // if the extensions are loaded, and THEN look for the
    // extension function name
    wglSwapInterval = (wglSwapIntervalEXTFunc*)
        wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapInterval) {
        wglSwapInterval(0);
    }
    else {
        // TODO no vsync. logging? just exit? just exit for now
        return false;
    }

    const GLubyte* vendorString = glFuncs->glGetString(GL_VENDOR);
    DEBUG_PRINT("GL_VENDOR: %s\n", vendorString);
    const GLubyte* rendererString = glFuncs->glGetString(GL_RENDERER);
    DEBUG_PRINT("GL_RENDERER: %s\n", rendererString);
    const GLubyte* versionString = glFuncs->glGetString(GL_VERSION);
    DEBUG_PRINT("GL_VERSION: %s\n", versionString);

    int32 majorVersion = versionString[0] - '0';
    int32 minorVersion = versionString[2] - '0';

    if (majorVersion < 3 || (majorVersion == 3 && minorVersion < 3)) {
        // TODO logging. opengl version is less than 3.3
        return false;
    }

    if (!Win32LoadAllGLFunctions(glFuncs, oglLib)) {
        // TODO logging (couldn't load all functions, but version is 3.3+)
        return false;
    }

    const GLubyte* glslString =
        glFuncs->glGetString(GL_SHADING_LANGUAGE_VERSION);
    DEBUG_PRINT("GL_SHADING_LANGUAGE_VERSION: %s\n", glslString);

    return true;
}

internal bool Win32CreateRC(HWND hWnd,
    BYTE colorBits, BYTE alphaBits, BYTE depthBits, BYTE stencilBits)
{
    // TODO these calls in total add about 40MB to program memory
    //      ...why?

    HDC hDC = GetDC(hWnd);
    if (!hDC) {
        // TODO log
        return false;
    }

    // Define and set pixel format
    PIXELFORMATDESCRIPTOR desiredPFD = { sizeof(desiredPFD) };
    desiredPFD.nVersion = 1;
    desiredPFD.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW
        | PFD_DOUBLEBUFFER;
    desiredPFD.iPixelType = PFD_TYPE_RGBA;
    desiredPFD.cColorBits = colorBits;
    desiredPFD.cAlphaBits = alphaBits;
    desiredPFD.cDepthBits = depthBits;
    desiredPFD.cStencilBits = stencilBits;
    desiredPFD.iLayerType = PFD_MAIN_PLANE;
    int pixelFormat = ChoosePixelFormat(hDC, &desiredPFD);
    if (!pixelFormat) {
        // TODO log
        DWORD error = GetLastError();
        return false;
    }

    PIXELFORMATDESCRIPTOR suggestedPFD = {};
    DescribePixelFormat(hDC, pixelFormat, sizeof(suggestedPFD), &suggestedPFD);
    if (!SetPixelFormat(hDC, pixelFormat, &suggestedPFD)) {
        // TODO log
        DWORD error = GetLastError();
        return false;
    }

    // Create and attach OpenGL rendering context to this thread
    HGLRC hGLRC = wglCreateContext(hDC);
    if (!hGLRC) {
        // TODO log
        DWORD error = GetLastError();
        return false;
    }
    if (!wglMakeCurrent(hDC, hGLRC)) {
        // TODO log
        DWORD error = GetLastError();
        return false;
    }

    ReleaseDC(hWnd, hDC);
    return true;
}

internal HWND Win32CreateWindow(
    HINSTANCE hInstance,
    const char* className, const char* windowName,
    int x, int y, int clientWidth, int clientHeight)
{
    WNDCLASSEX wndClass = { sizeof(wndClass) };
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = hInstance;
    //wndClass.hIcon = NULL;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.lpszClassName = className;

    if (!RegisterClassEx(&wndClass)) {
        // TODO log
        return NULL;
    }

    RECT windowRect     = {};
    windowRect.left     = x;
    windowRect.top      = y;
    windowRect.right    = x + clientWidth;
    windowRect.bottom   = y + clientHeight;

    if (!AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
    FALSE, 0)) {
        // TODO log
        GetLastError();
        return NULL;
    }

    HWND hWindow = CreateWindowEx(
        0,
        className,
        windowName,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        windowRect.left, windowRect.top,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
        0,
        0,
        hInstance,
        0);

    if (!hWindow) {
        // TODO log
        return NULL;
    }

    return hWindow;
}

int CALLBACK WinMain(
    HINSTANCE hInstance, HINSTANCE hPrevInst,
    LPSTR cmdline, int cmd_show)
{
    #if GAME_SLOW
    debugPrint_ = DEBUGPlatformPrint;
    #endif

    Win32State state = {};
    Win32GetExeFilePath(&state);

    RemoveFileNameFromPath(state.exeFilePath, pathToApp_, MAX_PATH);
    DEBUG_PRINT("Path to executable: %s\n", pathToApp_);

    Win32LoadXInput();

    // Create window
    HWND hWnd = Win32CreateWindow(hInstance,
        "OpenGLWindowClass", "particles",
        100, 100, START_WIDTH, START_HEIGHT);
    if (!hWnd) {
        return 1;
    }
    DEBUG_PRINT("Created Win32 window\n");

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    ScreenInfo screenInfo = {};
    screenInfo.size.x = clientRect.right - clientRect.left;
    screenInfo.size.y = clientRect.bottom - clientRect.top;
    // TODO is cColorBits ACTUALLY excluding alpha bits? Doesn't seem like it
    screenInfo.colorBits = 32;
    screenInfo.alphaBits = 8;
    screenInfo.depthBits = 24;
    screenInfo.stencilBits = 0;
    screenInfo_ = &screenInfo;

    // Create and attach rendering context for OpenGL
    if (!Win32CreateRC(hWnd, screenInfo.colorBits, screenInfo.alphaBits,
    screenInfo.depthBits, screenInfo.stencilBits)) {
        return 1;
    }
    DEBUG_PRINT("Created Win32 OpenGL rendering context\n");

    PlatformFunctions platformFuncs = {};
    platformFuncs.DEBUGPlatformPrint = DEBUGPlatformPrint;
    platformFuncs.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
    platformFuncs.DEBUGPlatformReadFile = DEBUGPlatformReadFile;
    platformFuncs.DEBUGPlatformWriteFile = DEBUGPlatformWriteFile;

    // Initialize OpenGL
    if (!Win32InitOpenGL(&platformFuncs.glFunctions,
    screenInfo.size.x, screenInfo.size.y)) {
        return 1;
    }
    DEBUG_PRINT("Initialized Win32 OpenGL\n");

    // Try to get monitor refresh rate
    // TODO make this more reliable
    int monitorRefreshHz = 60;
    {
        HDC hDC = GetDC(hWnd);
        int win32RefreshRate = GetDeviceCaps(hDC, VREFRESH);
        if (win32RefreshRate > 1)
            monitorRefreshHz = win32RefreshRate;
        ReleaseDC(hWnd, hDC);
    }

    // TODO probably remove this later
#if GAME_INTERNAL
    DEBUGshowCursor_ = true;
#endif

#if GAME_INTERNAL
    LPVOID baseAddress = (LPVOID)TERABYTES((uint64)2);;
#else
    LPVOID baseAddress = 0;
#endif

    GameMemory gameMemory = {};
    gameMemory.DEBUGShouldInitGlobalFuncs = true;

    gameMemory.permanentStorageSize = MEGABYTES(64);
    gameMemory.transientStorageSize = MEGABYTES(64);

    // TODO Look into using large virtual pages for this
    // potentially big allocation
    uint64 totalSize = gameMemory.permanentStorageSize
        + gameMemory.transientStorageSize;
    // TODO check allocation fail?
    gameMemory.permanentStorage = VirtualAlloc(baseAddress, (size_t)totalSize,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    gameMemory.transientStorage = ((uint8*)gameMemory.permanentStorage +
        gameMemory.permanentStorageSize);

    state.gameMemorySize = totalSize;
    state.gameMemoryBlock = gameMemory.permanentStorage;
    if (!gameMemory.permanentStorage || !gameMemory.transientStorage) {
        // TODO log
        return 1;
    }
    DEBUG_PRINT("Initialized game memory\n");

    for (int replayIndex = 0;
    replayIndex < ARRAY_COUNT(state.replayBuffers);
    replayIndex++) {
        Win32ReplayBuffer* replayBuffer = &state.replayBuffers[replayIndex];

        Win32GetInputFileLocation(&state, false, replayIndex,
            sizeof(replayBuffer->filePath), replayBuffer->filePath);

        replayBuffer->fileHandle = CreateFile(replayBuffer->filePath,
            GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, NULL);

        LARGE_INTEGER maxSize;
        maxSize.QuadPart = state.gameMemorySize;
        replayBuffer->memoryMap = CreateFileMapping(
            replayBuffer->fileHandle, 0, PAGE_READWRITE,
            maxSize.HighPart, maxSize.LowPart, NULL);
        replayBuffer->gameMemoryBlock = MapViewOfFile(
            replayBuffer->memoryMap, FILE_MAP_ALL_ACCESS, 0, 0,
            state.gameMemorySize);

        // TODO possibly check if this memory block "allocation" fails
        // Debug, so not really that important
    }
    DEBUG_PRINT("Initialized input replay system\n");

    char gameCodeDLLPath[MAX_PATH];
    Win32BuildExePathFileName(&state, "particles_game.dll",
        MAX_PATH, gameCodeDLLPath);
    char tempCodeDLLPath[MAX_PATH];
    Win32BuildExePathFileName(&state, "particles_game_temp.dll",
        MAX_PATH, tempCodeDLLPath);

    GameInput input[2] = {};
    GameInput *newInput = &input[0];
    GameInput *oldInput = &input[1];
    Win32InitKeyCodeMap();

    // Initialize timing information
    int64 timerFreq;
    {
        LARGE_INTEGER timerFreqResult;
        QueryPerformanceFrequency(&timerFreqResult);
        timerFreq = timerFreqResult.QuadPart;
    }

    LARGE_INTEGER timerLast;
    QueryPerformanceCounter(&timerLast);
    uint64 cyclesLast = __rdtsc();

    Win32GameCode gameCode =
        Win32LoadGameCode(gameCodeDLLPath, tempCodeDLLPath);

    running_ = true;
    while (running_) {
        // TODO this gets called twice very quickly in succession
        FILETIME newDLLWriteTime = Win32GetLastWriteTime(gameCodeDLLPath);
        if (CompareFileTime(&newDLLWriteTime, &gameCode.lastDLLWriteTime) > 0) {
            Win32UnloadGameCode(&gameCode);
            gameCode = Win32LoadGameCode(gameCodeDLLPath, tempCodeDLLPath);
            gameMemory.DEBUGShouldInitGlobalFuncs = true;
        }

        // Process keyboard input & other messages
        int mouseWheelPrev = newInput->mouseWheel;
        Win32ProcessMessages(hWnd, newInput, &platformFuncs.glFunctions);
        newInput->mouseWheelDelta = newInput->mouseWheel - mouseWheelPrev;

        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hWnd, &mousePos);
        Vec2Int mousePosPrev = newInput->mousePos;
        newInput->mousePos.x = mousePos.x;
        newInput->mousePos.y = screenInfo_->size.y - mousePos.y;
        newInput->mouseDelta = newInput->mousePos - mousePosPrev;
        if (mousePos.x < 0 || mousePos.x > screenInfo_->size.x
        || mousePos.y < 0 || mousePos.y > screenInfo_->size.y) {
            for (int i = 0; i < 5; i++) {
                int transitions = newInput->mouseButtons[i].isDown ? 1 : 0;
                newInput->mouseButtons[i].isDown = false;
                newInput->mouseButtons[i].transitions = transitions;
            }
        }

        DWORD maxControllerCount = XUSER_MAX_COUNT;
        if (maxControllerCount > ARRAY_COUNT(newInput->controllers)) {
            maxControllerCount = ARRAY_COUNT(newInput->controllers);
        }
        // TODO should we poll this more frequently?
        for (DWORD controllerInd = 0;
        controllerInd < maxControllerCount;
        controllerInd++) {
            GameControllerInput *oldController =
                &oldInput->controllers[controllerInd];
            GameControllerInput *newController =
                &newInput->controllers[controllerInd];

            XINPUT_STATE controllerState;
            // TODO the get state function has a really bad performance bug
            // which causes it to stall for a couple ms if a controller
            // isn't connected
            if (XInputGetState(controllerInd, &controllerState)
            == ERROR_SUCCESS) {
                newController->isConnected = true;

                // TODO check controller_state.dwPacketNumber
                XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

                Win32ProcessXInputButton(pad->wButtons,
                    &oldController->a, XINPUT_GAMEPAD_A,
                    &newController->a);
                Win32ProcessXInputButton(pad->wButtons,
                    &oldController->b, XINPUT_GAMEPAD_B,
                    &newController->b);
                Win32ProcessXInputButton(pad->wButtons,
                    &oldController->x, XINPUT_GAMEPAD_X,
                    &newController->x);
                Win32ProcessXInputButton(pad->wButtons,
                    &oldController->y, XINPUT_GAMEPAD_Y,
                    &newController->y);
                Win32ProcessXInputButton(pad->wButtons,
                    &oldController->lShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                    &newController->lShoulder);
                Win32ProcessXInputButton(pad->wButtons,
                    &oldController->rShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                    &newController->rShoulder);

                newController->leftStart = oldController->leftEnd;
                newController->rightStart = oldController->rightEnd;

                // TODO check if the deadzone is round
                newController->leftEnd.x = Win32ProcessXInputStickValue(
                    pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                newController->leftEnd.y = Win32ProcessXInputStickValue(
                    pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                newController->rightEnd.x = Win32ProcessXInputStickValue(
                    pad->sThumbRX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                newController->rightEnd.y = Win32ProcessXInputStickValue(
                    pad->sThumbRY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

#if 0
                XINPUT_VIBRATION vibration;
                vibration.wLeftMotorSpeed = 4000;
                vibration.wRightMotorSpeed = 10000;
                XInputSetState(controllerInd, &vibration);
#endif
            }
            else {
                newController->isConnected = false;
            }
        }

        // Recording game input
        if (newInput->controllers[0].lShoulder.isDown
        && newInput->controllers[0].lShoulder.transitions > 0
        && state.inputPlayingIndex == 0) {
            if (state.inputRecordingIndex == 0) {
                Win32RecordInputBegin(&state, 1);
                DEBUG_PRINT("RECORDING - START\n");
            }
            else {
                Win32RecordInputEnd(&state);
                DEBUG_PRINT("RECORDING - STOP\n");
            }
        }
        if (newInput->controllers[0].rShoulder.isDown
        && newInput->controllers[0].rShoulder.transitions > 0
        && state.inputRecordingIndex == 0) {
            if (state.inputPlayingIndex == 0) {
                Win32PlaybackBegin(&state, 1);
                DEBUG_PRINT("-> PLAYING - START\n");
            }
            else {
                Win32PlaybackEnd(&state);
                DEBUG_PRINT("-> PLAYING - STOP\n");
            }
        }

        if (state.inputRecordingIndex) {
            Win32RecordInput(&state, newInput);
        }
        if (state.inputPlayingIndex) {
            Win32PlayInput(&state, newInput);
        }

#if GAME_INTERNAL
        if (newInput->controllers[0].y.isDown
        && newInput->controllers[0].y.transitions > 0) {
            gameMemory.isInitialized = false;
        }
#endif
        
        LARGE_INTEGER timerEnd;
        QueryPerformanceCounter(&timerEnd);
        uint64 cyclesEnd = __rdtsc();
        int64 cyclesElapsed = cyclesEnd - cyclesLast;
        int64 timerElapsed = timerEnd.QuadPart - timerLast.QuadPart;
        float64 elapsed = (float64)timerElapsed / timerFreq;
        float64 fps = (float64)timerFreq / timerElapsed;
        int32 mCyclesPerFrame = (int32)(cyclesElapsed / (1000 * 1000));
        timerLast = timerEnd;
        cyclesLast = cyclesEnd;
        /*DEBUG_PRINT("%fs/f, %ff/s, %dMc/f\n",
            elapsed, fps, mCyclesPerFrame);*/

        if (gameCode.gameUpdateAndRender) {
            ThreadContext thread = {};
            gameCode.gameUpdateAndRender(&thread, &platformFuncs,
                newInput, screenInfo, (float32)elapsed,
                &gameMemory);
        }

        LARGE_INTEGER vsyncStart;
        QueryPerformanceCounter(&vsyncStart);
        // NOTE
        // SwapBuffers seems to effectively stall for vsync target time
        // It's probably more complicated than that under the hood,
        // but it does (I believe) effectively sleep your thread and yield
        // CPU time to other processes until the vsync target time is hit
        HDC hDC = GetDC(hWnd);
        SwapBuffers(hDC);
        ReleaseDC(hWnd, hDC);
        LARGE_INTEGER vsyncEnd;
        QueryPerformanceCounter(&vsyncEnd);

        int64 vsyncElapsed = vsyncEnd.QuadPart - vsyncStart.QuadPart;
        float32 vsyncElapsedMS = 1000.0f * vsyncElapsed / timerFreq;
        //DEBUG_PRINT("SwapBuffers took %f ms\n", vsyncElapsedMS);

        // LARGE_INTEGER timerEnd;
        // QueryPerformanceCounter(&timerEnd);
        // uint64 cyclesEnd = __rdtsc();

        // int64 cyclesElapsed = cyclesEnd - cyclesLast;
        // int64 timerElapsed = timerEnd.QuadPart - timerLast.QuadPart;
        // float32 elapsedMS = 1000.0f * timerElapsed / timerFreq;
        // float32 fps = (float32)timerFreq / timerElapsed;
        // int32 mCyclesPerFrame = (int32)(cyclesElapsed / (1000 * 1000));

        // /*DEBUG_PRINT("Rest of loop took %d ms\n",
        //     elapsedMS - vsyncElapsedMS);*/

        // /*DEBUG_PRINT("%fms/f, %ff/s, %dMc/f\n",
        //     elapsedMS, fps, mCyclesPerFrame);*/

        // timerLast = timerEnd;
        // cyclesLast = cyclesEnd;

        GameInput *temp = newInput;
        newInput = oldInput;
        oldInput = temp;
        ClearInput(newInput, oldInput);
    }

    return 0;
}

// TODO temporary! this is a bad idea! already compiled in main.cpp
#include "km_input.cpp"