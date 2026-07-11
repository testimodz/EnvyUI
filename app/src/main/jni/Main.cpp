#include "EGL/egl.h"
#include "GLES3/gl3.h"
#include <android/input.h>
#include <android/native_window.h>
#include <time.h>
#include "ImGui/imgui.h"
#include "ImGui/backends/imgui_impl_android.h"
#include "ImGui/backends/imgui_impl_opengl3.h"
#include "ImGui/misc/fonts/Roboto-Regular.h"
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "KittyMemory/MemoryPatch.h"
#include "And64InlineHook/And64InlineHook.hpp"
#include "Substrate/SubstrateHook.h"
#include "Substrate/CydiaSubstrate.h"
#include "Tools/Call_Tools.h"
#include "Tools/MonoString.h"
#include "Il2cpp/il2cpp-class.h"
#include "Il2cpp/Il2cpp.h"
#include "Includes/Macros.h"
#include "Hook.h"
#include "Includes/dex_data.h"
#include "Includes/JNIStuff.h"


//example int hook
bool myToggle = false; // this is our toggle 
int Coinsss = 999;
// Int Method Offset Hooking Example:
int (*og_tesing)(void *instance);
int new_testing(void *instance) {
	if (instance != NULL) {
		if (myToggle) {
			return Coinsss; // add your custom value
		}
	}
	return og_tesing(instance);
}


static void HookCoins() { //Safe Hooking (Auto Update)
    //Namespace: User
	//public class Profile
    Il2CppClass* currencyClass = Il2cpp::FindClass("User.Profile"); // NameSpace & Class
    if (!currencyClass) {
        LOGE("Class Not Found");
        return;
    }
	//public int get_HardCurrency() { }
    MethodInfo* get_Currency = currencyClass->getMethod("get_HardCurrency", 0);
    if (!get_Currency) {
        LOGE("Method Not Found");
        return;
    }
	
    void* get_CurrencyAddr = get_Currency->methodPointer;
    if (!get_CurrencyAddr) {
        LOGE("Pointer is NULL");
        return;
    }
    DobbyHook((void*)get_CurrencyAddr, (void*)new_testing, (void**)&og_tesing);
}

static char inputText[128] = "";

static void BeginDraw() { // Menu
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(868, 562), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("IMGUI", 0, ImGuiWindowFlags_NoSavedSettings)) {
  
    ImGui::InputText("Enter Text", inputText, sizeof(inputText)); // keyboard test
    
    ImGui::Checkbox("Test", &myToggle); // example Hack
    ImGui::InputInt("Add Coins Value", &Coinsss);
  }
  ImGui::End();
}

EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
EGLBoolean _eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
  eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
  eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);
  if (glWidth <= 0 || glHeight <= 0) {
    return eglSwapBuffers(dpy, surface);
  }

  if (!setup) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsLight();
    ImGui::GetStyle().WindowTitleAlign = ImVec2(0.5f, 0.5f);
    ImGui_ImplOpenGL3_Init("#version 300 es");
    ImFontConfig font_cfg;
    io.Fonts->AddFontFromMemoryTTF(&Roboto_Regular, sizeof(Roboto_Regular), 40.0, &font_cfg, io.Fonts->GetGlyphRangesCyrillic());
    ImGui::GetStyle().ScaleAllSizes(3.0f);
    setup = true;
  }
  ImGuiIO &io = ImGui::GetIO();
  static bool s_lastWantTextInput = false;
  bool wantTextInput = ImGui::GetIO().WantTextInput;
  if (wantTextInput != s_lastWantTextInput) {
    s_lastWantTextInput = wantTextInput;
    ShowKeyboard(wantTextInput);
  }
  static MethodInfo *TouchCountMethod = nullptr;
  static MethodInfo *GetTouchMethodInfo = nullptr;

  if (!TouchCountMethod) {
    TouchCountMethod = Il2cpp::FindMethod(OBFUSCATE("UnityEngine.Input"), OBFUSCATE("get_touchCount"), 0);
  }

  if (!GetTouchMethodInfo) {
    GetTouchMethodInfo = Il2cpp::FindMethod(OBFUSCATE("UnityEngine.Input"), OBFUSCATE("GetTouch"), 1);
  }

  if (TouchCountMethod && GetTouchMethodInfo) {
    int touchCount = TouchCountMethod->invoke_static < int > ();
    if (touchCount > 0) {
      auto touch = GetTouchMethodInfo->invoke_static < UnityEngine_Touch_Fields > (0);
      float reverseY = io.DisplaySize.y - touch.m_Position.fields.y;

      switch (touch.m_Phase) {
        case TouchPhase::Began:
        case TouchPhase::Stationary:
        io.MousePos = ImVec2(touch.m_Position.fields.x, reverseY);
        io.MouseDown[0] = true;
        break;
        case TouchPhase::Ended:
        case TouchPhase::Canceled:
        io.MouseDown[0] = false;
        should_clear_mouse_pos = true;
        break;
        case TouchPhase::Moved:
        io.MousePos = ImVec2(touch.m_Position.fields.x, reverseY);
        break;
        default:
        break;
      }
    } else {
      io.MouseDown[0] = false;
    }
  }
  ImGui_ImplOpenGL3_NewFrame();
  io.DisplaySize = ImVec2((float)glWidth, (float)glHeight);
  ImGui::NewFrame();
  BeginDraw();
  ImGui::EndFrame();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  return orig_eglSwapBuffers(dpy, surface);
}

void *Init(void *) {
  ProcMap il2cppMap;
  while (!il2cppMap.isValid()) {
    il2cppMap = KittyMemory::getLibraryBaseMap("libil2cpp.so");
    sleep(1);
  }
  Il2cpp::Init();
  
  //@Call Hooks
  Tools::Hook((void *) DobbySymbolResolver("/system/lib/libandroid.so", "ANativeWindow_getWidth"), (void *) _ANativeWindow_getWidth, (void **) &orig_ANativeWindow_getWidth);Tools::Hook((void *) DobbySymbolResolver("/system/lib/libinput.so", "_ZN7android13InputConsumer21initializeMotionEventEPNS_11MotionEventEPKNS_12InputMessageE"), (void *) myInput, (void **) &origInput);
  auto swapBuffers = ((uintptr_t)DobbySymbolResolver(OBFUSCATE("libEGL.so"), OBFUSCATE("eglSwapBuffers")));
  DobbyHook((void *)swapBuffers, (void *)_eglSwapBuffers, (void **)&orig_eglSwapBuffers);
  
  //@Call Hooks
  HookCoins();
  
  return nullptr;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
  jvm = vm;
  JNIEnv* env;
  LoadDex(imgui_dex, imgui_dex_len);
  vm->GetEnv((void**)&env, JNI_VERSION_1_6);
  pthread_t myThread;
  pthread_create(&myThread, nullptr, Init, nullptr);
  return JNI_VERSION_1_6;
}
