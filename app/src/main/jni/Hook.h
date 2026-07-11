#pragma once
static int screenWidth = -1, glWidth, screenHeight = -1, glHeight;
static float density = -1;

#define HOOK(ret, func, ...) \
ret (*orig##func)(__VA_ARGS__); \
ret my##func(__VA_ARGS__)

HOOK(void, Input, void *thiz, void *ex_ab, void *ex_ac) {
  origInput(thiz, ex_ab, ex_ac);
  ImGui_ImplAndroid_HandleInputEvent((AInputEvent *)thiz);
  return;
}

int32_t (*orig_ANativeWindow_getWidth)(ANativeWindow* window);
int32_t _ANativeWindow_getWidth(ANativeWindow* window) {
  screenWidth = orig_ANativeWindow_getWidth(window);
  return orig_ANativeWindow_getWidth(window);
}

int32_t (*orig_ANativeWindow_getHeight)(ANativeWindow* window);
int32_t _ANativeWindow_getHeight(ANativeWindow* window) {
  screenHeight = orig_ANativeWindow_getHeight(window);
  return orig_ANativeWindow_getHeight(window);
}

bool setup = false;
static bool should_clear_mouse_pos = false;

struct UnityEngine_Vector2_Fields {
  float x;
  float y;
};
struct UnityEngine_Vector2_o {
  UnityEngine_Vector2_Fields fields;
};
enum TouchPhase {
  Began = 0,
  Moved = 1,
  Stationary = 2,
  Ended = 3,
  Canceled = 4
};
struct UnityEngine_Touch_Fields {
  int32_t m_FingerId;
  struct UnityEngine_Vector2_o m_Position;
  struct UnityEngine_Vector2_o m_RawPosition;
  struct UnityEngine_Vector2_o m_PositionDelta;
  float m_TimeDelta;
  int32_t m_TapCount;
  int32_t m_Phase;
  int32_t m_Type;
  float m_Pressure;
  float m_maximumPossiblePressure;
  float m_Radius;
  float m_fRadiusVariance;
  float m_AltitudeAngle;
  float m_AzimuthAngle;
};

bool setupimg;
struct TouchData {
  bool isPressed = false;
  float x = 0.0f;
  float y = 0.0f;
  bool hasPendingTouch = false;
  int action = -1;
} touchData;

/* Game MainActicity (OnCreate) Code.

const-string v0, "MEOW"

invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V

*/