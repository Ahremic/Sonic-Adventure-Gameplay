#include "Input.h"
using namespace GlobalInput;

const uint16_t* Input::heldButtons     = reinterpret_cast<uint16_t*>(0x01E77B54);
const uint16_t* Input::pressedButtons  = reinterpret_cast<uint16_t*>(0x01E77B5C);
const uint16_t* Input::releasedButtons = reinterpret_cast<uint16_t*>(0x01E77B60);

float* const Input::leftStickX = reinterpret_cast<float*>(0x01E77B68);
float* const Input::leftStickY = reinterpret_cast<float*>(0x01E77B6C);
const float* Input::rightStickX = reinterpret_cast<float*>(0x01E77B74);
const float* Input::rightStickY = reinterpret_cast<float*>(0x01E77B78);

const float* Input::leftTrigger  = reinterpret_cast<float*>(0x01E77B80);
const float* Input::rightTrigger = reinterpret_cast<float*>(0x01E77B84);