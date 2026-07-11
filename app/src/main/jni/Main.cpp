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
#include "Offsets.h"

using namespace Toram::Offsets;

// ============================================================
//  Toggles & Values
// ============================================================

// Combat
bool b_godmode        = false;
bool b_crit1          = false;
bool b_crit2          = false;
bool b_crit3          = false;
bool b_penetrate      = false;
bool b_magpenet       = false;
bool b_phys_crit      = false;
bool b_mag_crit       = false;
bool b_ailment        = false;
bool b_mobeva         = false;
bool b_mobatk         = false;

// Defense pierce (Boss)
bool b_p_boss_n       = false;
bool b_p_boss_s       = false;
bool b_p_boss_m       = false;

// Defense pierce (Mob)
bool b_p_mob_n        = false;
bool b_p_mob_s        = false;
bool b_p_mob_m        = false;

// Speed
bool b_aspd           = false;
bool b_motion         = false;
bool b_fast_run       = false;
bool b_anim1          = false;
bool b_anim2          = false;

// Misc
bool b_longrange      = false;
bool b_stab           = false;
bool b_hit            = false;
bool b_element_setting = false;
bool b_quick_aura     = false;
bool b_brave_dmg      = false;

// Skip / QoL
bool b_fskip1         = false;
bool b_fskip2         = false;
bool b_skipbattle     = false;
bool b_skipmq         = false;
bool b_nodrop         = false;
bool b_mqitem         = false;
bool b_eqitem         = false;
bool b_mqmob          = false;
bool b_eqmob          = false;
bool b_unlockmap      = false;

// CrossFire
bool b_cf_qloader1    = false;
bool b_cf_qloader2    = false;

// ============================================================
//  KittyMemory Patches (NOP / return true style)
// ============================================================

static uintptr_t il2cppBase = 0;

static void ApplyPatch(bool enable, uintptr_t offset, const std::vector<uint8_t>& onBytes, const std::vector<uint8_t>& offBytes) {
    if (il2cppBase == 0) return;
    uintptr_t addr = il2cppBase + offset;
    if (enable) {
        KittyMemory::memWrite((void*)addr, onBytes.data(), onBytes.size());
    } else {
        KittyMemory::memWrite((void*)addr, offBytes.data(), offBytes.size());
    }
}

// ARM64: MOV W0, #1 = 20 00 80 52  |  RET = C0 03 5F D6
// ARM64: MOV W0, #0 = 00 00 80 52
static const std::vector<uint8_t> RET_TRUE  = {0x20, 0x00, 0x80, 0x52, 0xC0, 0x03, 0x5F, 0xD6};
static const std::vector<uint8_t> RET_FALSE = {0x00, 0x00, 0x80, 0x52, 0xC0, 0x03, 0x5F, 0xD6};
static const std::vector<uint8_t> NOP4      = {0x1F, 0x20, 0x03, 0xD5};
static const std::vector<uint8_t> NOP8      = {0x1F, 0x20, 0x03, 0xD5, 0x1F, 0x20, 0x03, 0xD5};

static void ApplyAllPatches() {
    // Combat
    ApplyPatch(b_godmode,    godmode,    RET_TRUE,  RET_FALSE);
    ApplyPatch(b_crit1,      crit1,      RET_TRUE,  RET_FALSE);
    ApplyPatch(b_crit2,      crit2,      RET_TRUE,  RET_FALSE);
    ApplyPatch(b_crit3,      crit3,      RET_TRUE,  RET_FALSE);
    ApplyPatch(b_penetrate,  penetrate,  NOP4,      NOP4);
    ApplyPatch(b_magpenet,   magpenet,   NOP4,      NOP4);
    ApplyPatch(b_ailment,    ailment,    RET_TRUE,  RET_FALSE);
    ApplyPatch(b_mobeva,     mobeva,     RET_FALSE, RET_FALSE);
    ApplyPatch(b_mobatk,     mobatk,     NOP4,      NOP4);

    // Critical Damage
    ApplyPatch(b_phys_crit,  phys_crit,  NOP4, NOP4);
    ApplyPatch(b_mag_crit,   mag_crit,   NOP4, NOP4);

    // Boss Defense
    ApplyPatch(b_p_boss_n,   p_boss_n,   RET_FALSE, RET_FALSE);
    ApplyPatch(b_p_boss_s,   p_boss_s,   RET_FALSE, RET_FALSE);
    ApplyPatch(b_p_boss_m,   p_boss_m,   RET_FALSE, RET_FALSE);

    // Mob Defense
    ApplyPatch(b_p_mob_n,    p_mob_n,    RET_FALSE, RET_FALSE);
    ApplyPatch(b_p_mob_s,    p_mob_s,    RET_FALSE, RET_FALSE);
    ApplyPatch(b_p_mob_m,    p_mob_m,    RET_FALSE, RET_FALSE);

    // Speed
    ApplyPatch(b_aspd,       aspd,       NOP4, NOP4);
    ApplyPatch(b_motion,     motion,     NOP4, NOP4);
    ApplyPatch(b_fast_run,   fast_run,   NOP4, NOP4);
    ApplyPatch(b_anim1,      anim1,      NOP4, NOP4);
    ApplyPatch(b_anim2,      anim2,      NOP4, NOP4);

    // Misc
    ApplyPatch(b_longrange,  longrange,  NOP4, NOP4);
    ApplyPatch(b_quick_aura, quick_aura, NOP4, NOP4);
    ApplyPatch(b_brave_dmg,  brave_dmg,  NOP4, NOP4);

    // Skip / QoL
    ApplyPatch(b_fskip1,     fskip1,     NOP4, NOP4);
    ApplyPatch(b_fskip2,     fskip2,     NOP4, NOP4);
    ApplyPatch(b_skipbattle, skipbattle, RET_TRUE, RET_FALSE);
    ApplyPatch(b_skipmq,     skipmq,     NOP4, NOP4);
    ApplyPatch(b_nodrop,     nodrop,     RET_TRUE, RET_FALSE);
    ApplyPatch(b_mqitem,     mqitem,     RET_TRUE, RET_FALSE);
    ApplyPatch(b_eqitem,     eqitem,     RET_TRUE, RET_FALSE);
    ApplyPatch(b_mqmob,      mqmob,      RET_TRUE, RET_FALSE);
    ApplyPatch(b_eqmob,      eqmob,      RET_TRUE, RET_FALSE);
    ApplyPatch(b_unlockmap,  unlockmap,  NOP8, NOP8);
    ApplyPatch(b_unlockmap,  unlockmap1, NOP8, NOP8);

    // CrossFire
    ApplyPatch(b_cf_qloader1, cf_qloader1, NOP4, NOP4);
    ApplyPatch(b_cf_qloader2, cf_qloader2, NOP4, NOP4);
}

// ============================================================
//  ImGui Menu
// ============================================================

static void BeginDraw() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("EnvyUI - Toram Online", 0, ImGuiWindowFlags_NoSavedSettings)) {

        // --- COMBAT ---
        if (ImGui::CollapsingHeader("[Combat]")) {
            if (ImGui::Checkbox("God Mode",        &b_godmode))    ApplyAllPatches();
            if (ImGui::Checkbox("Crit Always (1)", &b_crit1))      ApplyAllPatches();
            if (ImGui::Checkbox("Crit Always (2)", &b_crit2))      ApplyAllPatches();
            if (ImGui::Checkbox("Crit Always (3)", &b_crit3))      ApplyAllPatches();
            if (ImGui::Checkbox("Phys Penetrate",  &b_penetrate))  ApplyAllPatches();
            if (ImGui::Checkbox("Mag Penetrate",   &b_magpenet))   ApplyAllPatches();
            if (ImGui::Checkbox("Phys Crit Dmg",   &b_phys_crit))  ApplyAllPatches();
            if (ImGui::Checkbox("Mag Crit Dmg",    &b_mag_crit))   ApplyAllPatches();
            if (ImGui::Checkbox("Ailment Always",  &b_ailment))    ApplyAllPatches();
            if (ImGui::Checkbox("Mob No Eva",      &b_mobeva))     ApplyAllPatches();
            if (ImGui::Checkbox("Mob No Attack",   &b_mobatk))     ApplyAllPatches();
        }

        // --- DEFENSE PIERCE ---
        if (ImGui::CollapsingHeader("[Defense Pierce]")) {
            ImGui::Text("Boss");
            if (ImGui::Checkbox("Boss Def Normal",  &b_p_boss_n))  ApplyAllPatches();
            if (ImGui::Checkbox("Boss Def Skill",   &b_p_boss_s))  ApplyAllPatches();
            if (ImGui::Checkbox("Boss Def Magic",   &b_p_boss_m))  ApplyAllPatches();
            ImGui::Separator();
            ImGui::Text("Mob");
            if (ImGui::Checkbox("Mob Def Normal",   &b_p_mob_n))   ApplyAllPatches();
            if (ImGui::Checkbox("Mob Def Skill",    &b_p_mob_s))   ApplyAllPatches();
            if (ImGui::Checkbox("Mob Def Magic",    &b_p_mob_m))   ApplyAllPatches();
        }

        // --- SPEED ---
        if (ImGui::CollapsingHeader("[Speed]")) {
            if (ImGui::Checkbox("ASPD Boost",       &b_aspd))      ApplyAllPatches();
            if (ImGui::Checkbox("Motion Speed",     &b_motion))    ApplyAllPatches();
            if (ImGui::Checkbox("Fast Run",         &b_fast_run))  ApplyAllPatches();
            if (ImGui::Checkbox("Anim Skip (1)",    &b_anim1))     ApplyAllPatches();
            if (ImGui::Checkbox("Anim Skip (2)",    &b_anim2))     ApplyAllPatches();
        }

        // --- MISC ---
        if (ImGui::CollapsingHeader("[Misc]")) {
            if (ImGui::Checkbox("Long Range",       &b_longrange))  ApplyAllPatches();
            if (ImGui::Checkbox("Stability",        &b_stab))       ApplyAllPatches();
            if (ImGui::Checkbox("Hit Always",       &b_hit))        ApplyAllPatches();
            if (ImGui::Checkbox("Quick Aura",       &b_quick_aura)) ApplyAllPatches();
            if (ImGui::Checkbox("Brave Dmg Boost",  &b_brave_dmg))  ApplyAllPatches();
            if (ImGui::Checkbox("Element Override", &b_element_setting)) ApplyAllPatches();
        }

        // --- SKIP / QoL ---
        if (ImGui::CollapsingHeader("[Skip / QoL]")) {
            if (ImGui::Checkbox("Field Skip (1)",   &b_fskip1))    ApplyAllPatches();
            if (ImGui::Checkbox("Field Skip (2)",   &b_fskip2))    ApplyAllPatches();
            if (ImGui::Checkbox("Skip Battle",      &b_skipbattle)) ApplyAllPatches();
            if (ImGui::Checkbox("Skip Main Quest",  &b_skipmq))    ApplyAllPatches();
            if (ImGui::Checkbox("No Drop Anim",     &b_nodrop))    ApplyAllPatches();
            if (ImGui::Checkbox("Unlock Map",       &b_unlockmap)) ApplyAllPatches();
        }

        // --- QUEST ITEMS ---
        if (ImGui::CollapsingHeader("[Quest Items]")) {
            if (ImGui::Checkbox("MQ Item Always Done",  &b_mqitem)) ApplyAllPatches();
            if (ImGui::Checkbox("EQ Item Always Done",  &b_eqitem)) ApplyAllPatches();
            if (ImGui::Checkbox("MQ Mob Always Done",   &b_mqmob))  ApplyAllPatches();
            if (ImGui::Checkbox("EQ Mob Always Done",   &b_eqmob))  ApplyAllPatches();
        }

        // --- CROSSFIRE ---
        if (ImGui::CollapsingHeader("[CrossFire]")) {
            if (ImGui::Checkbox("CF QLoader (1)",   &b_cf_qloader1)) ApplyAllPatches();
            if (ImGui::Checkbox("CF QLoader (2)",   &b_cf_qloader2)) ApplyAllPatches();
        }
    }
    ImGui::End();
}

// ============================================================
//  EGL Hook
// ============================================================

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
        ImGui::StyleColorsDark();
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

    if (!TouchCountMethod)
        TouchCountMethod = Il2cpp::FindMethod(OBFUSCATE("UnityEngine.Input"), OBFUSCATE("get_touchCount"), 0);
    if (!GetTouchMethodInfo)
        GetTouchMethodInfo = Il2cpp::FindMethod(OBFUSCATE("UnityEngine.Input"), OBFUSCATE("GetTouch"), 1);

    if (TouchCountMethod && GetTouchMethodInfo) {
        int touchCount = TouchCountMethod->invoke_static<int>();
        if (touchCount > 0) {
            auto touch = GetTouchMethodInfo->invoke_static<UnityEngine_Touch_Fields>(0);
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

// ============================================================
//  Init
// ============================================================

void *Init(void *) {
    ProcMap il2cppMap;
    while (!il2cppMap.isValid()) {
        il2cppMap = KittyMemory::getLibraryBaseMap("libil2cpp.so");
        sleep(1);
    }
    il2cppBase = il2cppMap.startAddr;
    Il2cpp::Init();

    Tools::Hook(
        (void *) DobbySymbolResolver("/system/lib/libandroid.so", "ANativeWindow_getWidth"),
        (void *) _ANativeWindow_getWidth,
        (void **) &orig_ANativeWindow_getWidth
    );
    Tools::Hook(
        (void *) DobbySymbolResolver("/system/lib/libinput.so", "_ZN7android13InputConsumer21initializeMotionEventEPNS_11MotionEventEPKNS_12InputMessageE"),
        (void *) myInput,
        (void **) &origInput
    );

    auto swapBuffers = (uintptr_t) DobbySymbolResolver(OBFUSCATE("libEGL.so"), OBFUSCATE("eglSwapBuffers"));
    DobbyHook((void *)swapBuffers, (void *)_eglSwapBuffers, (void **)&orig_eglSwapBuffers);

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
