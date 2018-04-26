#include "km_input.h"
#include "main_platform.h"

inline bool32 WasKeyPressed(GameInput* input, KeyInputCode keyCode)
{
    return input->keyboard[keyCode].isDown
        && input->keyboard[keyCode].transitions > 0;
}

inline bool32 WasKeyReleased(GameInput* input, KeyInputCode keyCode)
{
    return !input->keyboard[keyCode].isDown
        && input->keyboard[keyCode].transitions > 0;
}

void ClearInput(GameInput* input, GameInput* inputPrev)
{
    for (int i = 0; i < 5; i++) {
        input->mouseButtons[i].isDown = inputPrev->mouseButtons[i].isDown;
        input->mouseButtons[i].transitions = 0;
    }
    input->mousePos = inputPrev->mousePos;
    input->mouseWheel = inputPrev->mouseWheel;

    for (int i = 0; i < KM_KEY_LAST; i++) {
        input->keyboard[i].isDown = inputPrev->keyboard[i].isDown;
        input->keyboard[i].transitions = 0;
    }
    input->keyboardString[0] = '\0';
    input->keyboardStringLen = 0;
}