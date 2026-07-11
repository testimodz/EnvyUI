#include "EGL/egl.h"
#include "GLES3/gl3.h"
#include <android/input.h>
#include <android/native_window.h>
#include <time.h>
#include <cstring>
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
//  ARM64 Patch Constants
// ============================================================
static const uint8_t V_TRUE[]          = {0x20,0x00,0x80,0xD2};
static const uint8_t V_FALSE[]         = {0x00,0x00,0x80,0xD2};
static const uint8_t V_RET[]           = {0xC0,0x03,0x5F,0xD6};
static const uint8_t V_NOP[]           = {0x1F,0x20,0x03,0xD5};
static const uint8_t V_ASPD[]          = {0x80,0x07,0xA0,0x52};
static const uint8_t V_MOTION[]        = {0x80,0x08,0x80,0xD2};
static const uint8_t V_STAB[]          = {0x80,0xF7,0x9F,0x52};
static const uint8_t V_HIT0[]          = {0x20,0x76,0xA1,0x52};
static const uint8_t V_HIT1[]          = {0x20,0x00,0xA0,0x72};
static const uint8_t V_ANIM1_A[]       = {0xA1,0x00,0x80,0x52};
static const uint8_t V_ANIM1_B[]       = {0x82,0x3E,0x80,0xD2};
static const uint8_t V_ANIM2_CLR[]     = {0xE8,0x03,0x1F,0x2A};
static const uint8_t V_LR_A[]          = {0x00,0x00,0x22,0x1E};
static const uint8_t V_LR_B[]          = {0x00,0xFC,0x1F,0x1E};
static const uint8_t V_PENET[]         = {0x20,0xCC,0x83,0x12};
static const uint8_t V_UMAP0[]         = {0x28,0x00,0x80,0x52};
static const uint8_t V_UMAP1[]         = {0x29,0x00,0x80,0x52};
static const uint8_t V_NODROP0[]       = {0x00,0x00,0x80,0xD2};
static const uint8_t V_NODROP1[]       = {0xC0,0x03,0x5F,0xD6};
static const uint8_t V_FASTRUN0[]      = {0x00,0xD0,0x25,0x1E};
static const uint8_t V_FASTRUN1[]      = {0xC0,0x03,0x5F,0xD6};
static const uint8_t V_FSKIP_A[]       = {0x88,0x00,0xA4,0x52};
static const uint8_t V_FSKIP_B[]       = {0x28,0x69,0x39,0x39};
static const uint8_t V_FSKIP_C[]       = {0x48,0x69,0x39,0x39};
static const uint8_t V_SKIPBOSS0[]     = {0x60,0x00,0xA4,0x52};
static const uint8_t V_SKIPBOSS1[]     = {0x1D,0x00,0x00,0x14};
static const uint8_t V_SKIPMQQ0[]      = {0xA8,0x0F,0x80,0x52};
static const uint8_t V_SKIPMQQ1[]      = {0x68,0x8E,0x00,0xB9};
static const uint8_t V_QUICK_BRAVE[]   = {0xA1,0x1C,0x80,0x52};
static const uint8_t V_CF1[]           = {0x08,0x00,0x00,0x14};
static const uint8_t V_CF2[]           = {0x06,0x00,0x00,0x14};
static const uint8_t V_AILMENT[]       = {0xA0,0x1E,0x80,0xD2};
static const uint8_t V_PRORATE_LOW[]   = {0xC0,0x12,0x80,0x52};
static const uint8_t V_PRORATE_MED[]   = {0x00,0x19,0x80,0x52};
static const uint8_t V_PRORATE_HIGH[]  = {0x80,0x25,0x80,0x52};
static const uint8_t V_CRIT_LOW[]      = {0x00,0x19,0x80,0xD2};
static const uint8_t V_CRIT_MED[]      = {0x40,0x1F,0x80,0xD2};
static const uint8_t V_CRIT_HIGH[]     = {0x80,0x25,0x80,0xD2};
static const uint8_t V_BRAVE_L1[]      = {0x00,0x91,0x01,0x11};
static const uint8_t V_BRAVE_L2[]      = {0x00,0x21,0x03,0x11};
static const uint8_t V_BRAVE_L3[]      = {0x00,0xB1,0x04,0x11};
static const uint8_t V_EL[7][4] = {
    {0x00,0x00,0x80,0x52},{0x20,0x00,0x80,0x52},{0x40,0x00,0x80,0x52},
    {0x60,0x00,0x80,0x52},{0x80,0x00,0x80,0x52},{0xA0,0x00,0x80,0x52},{0xC0,0x00,0x80,0x52},
};

// ============================================================
//  State & Backup
// ============================================================
static uintptr_t il2cppBase = 0;

struct BackupEntry { uintptr_t addr; uint8_t orig[8]; size_t sz; };
#include <vector>
#include <map>
#include <string>
static std::map<std::string, std::vector<BackupEntry>> R;

static struct {
    bool godmode, crit, aspd, motion, stab, hit;
    bool fastanim_v1, fastanim_v2, longrange, penet;
    bool mobeva, mobatk, ailment;
    bool fskip, skipmq1, skip_item, skip_mob, skipmqq;
    bool fastrun, unlockmap, nodrop;
    bool cf_qloader, quick_brave;
    bool phys_crit, mag_crit, prorate;
    bool element, brave_dmg;
    int  element_idx;
    int  brave_boost_idx;
    int  prorate_idx;
    int  phys_crit_idx;
    int  mag_crit_idx;
} st = {};

// ============================================================
//  Patch Helpers
// ============================================================
struct Patch { uintptr_t offset; const uint8_t* bytes; size_t len; };

static void ApplyGroup(const std::string& key, const std::vector<Patch>& patches) {
    if (!R.count(key)) {
        R[key] = {};
        for (auto& p : patches) {
            BackupEntry e;
            e.addr = il2cppBase + p.offset;
            e.sz   = p.len;
            KittyMemory::memRead((void*)e.addr, e.orig, p.len);
            R[key].push_back(e);
        }
    }
    for (auto& p : patches)
        KittyMemory::memWrite((void*)(il2cppBase + p.offset), p.bytes, p.len);
}

static void RestoreGroup(const std::string& key) {
    if (R.count(key)) {
        for (auto& e : R[key])
            KittyMemory::memWrite((void*)e.addr, e.orig, e.sz);
        R.erase(key);
    }
}

// ============================================================
//  Feature Toggles
// ============================================================
static void Toggle_GodMode()    { if(!st.godmode){ApplyGroup("godmode",{{godmode,V_TRUE,4},{godmode+4,V_RET,4}});st.godmode=true;}else{RestoreGroup("godmode");st.godmode=false;} }
static void Toggle_Crit()       { if(!st.crit){ApplyGroup("crit",{{crit1,V_TRUE,4},{crit1+4,V_RET,4},{crit2,V_TRUE,4},{crit2+4,V_RET,4},{crit3,V_TRUE,4},{crit3+4,V_RET,4}});st.crit=true;}else{RestoreGroup("crit");st.crit=false;} }
static void Toggle_ASPD()       { if(!st.aspd){ApplyGroup("aspd",{{aspd,V_ASPD,4},{aspd+4,V_RET,4}});st.aspd=true;}else{RestoreGroup("aspd");st.aspd=false;} }
static void Toggle_Motion()     { if(!st.motion){ApplyGroup("motion",{{motion,V_MOTION,4},{motion+4,V_RET,4}});st.motion=true;}else{RestoreGroup("motion");st.motion=false;} }
static void Toggle_Stab()       { if(!st.stab){ApplyGroup("stab",{{stab,V_STAB,4},{stab+4,V_RET,4}});st.stab=true;}else{RestoreGroup("stab");st.stab=false;} }
static void Toggle_Hit()        { if(!st.hit){ApplyGroup("hit",{{hit,V_HIT0,4},{hit+4,V_HIT1,4},{hit+8,V_RET,4}});st.hit=true;}else{RestoreGroup("hit");st.hit=false;} }
static void Toggle_FastAnimV1() { if(!st.fastanim_v1){ApplyGroup("fastanim_v1",{{anim1+4,V_ANIM1_B,4}});st.fastanim_v1=true;}else{RestoreGroup("fastanim_v1");st.fastanim_v1=false;} }
static void Toggle_FastAnimV2() { if(!st.fastanim_v2){ApplyGroup("fastanim_v2",{{anim1,V_ANIM1_A,4},{anim1+4,V_ANIM1_B,4},{anim2,V_ANIM2_CLR,4}});st.fastanim_v2=true;}else{RestoreGroup("fastanim_v2");st.fastanim_v2=false;} }
static void Toggle_LongRange()  { if(!st.longrange){ApplyGroup("longrange",{{longrange,V_LR_A,4},{longrange+4,V_LR_B,4},{longrange+8,V_RET,4}});st.longrange=true;}else{RestoreGroup("longrange");st.longrange=false;} }
static void Toggle_Penetrate()  { if(!st.penet){ApplyGroup("penet",{{penetrate,V_PENET,4},{penetrate+4,V_RET,4},{magpenet,V_PENET,4},{magpenet+4,V_RET,4}});st.penet=true;}else{RestoreGroup("penet");st.penet=false;} }
static void Toggle_MobEva()     { if(!st.mobeva){ApplyGroup("mobeva",{{mobeva,V_FALSE,4},{mobeva+4,V_RET,4}});st.mobeva=true;}else{RestoreGroup("mobeva");st.mobeva=false;} }
static void Toggle_MobAtk()     { if(!st.mobatk){ApplyGroup("mobatk",{{mobatk,V_FALSE,4},{mobatk+4,V_RET,4}});st.mobatk=true;}else{RestoreGroup("mobatk");st.mobatk=false;} }
static void Toggle_Ailment()    { if(!st.ailment){ApplyGroup("ailment",{{ailment,V_AILMENT,4},{ailment+4,V_RET,4}});st.ailment=true;}else{RestoreGroup("ailment");st.ailment=false;} }
static void Toggle_FSkip()      { if(!st.fskip){ApplyGroup("fskip",{{fskip1+0x22C,V_FSKIP_A,4},{fskip1+0x230,V_FSKIP_B,4},{fskip2+0x78,V_NOP,4},{fskip2+0x7C,V_FSKIP_A,4},{fskip2+0x80,V_FSKIP_C,4}});st.fskip=true;}else{RestoreGroup("fskip");st.fskip=false;} }
static void Toggle_SkipBoss()   { if(!st.skipmq1){ApplyGroup("skipmq1",{{skipbattle+8,V_SKIPBOSS0,4},{skipbattle+0xC,V_SKIPBOSS1,4}});st.skipmq1=true;}else{RestoreGroup("skipmq1");st.skipmq1=false;} }
static void Toggle_FastRun()    { if(!st.fastrun){ApplyGroup("fastrun",{{fast_run,V_FASTRUN0,4},{fast_run+4,V_FASTRUN1,4}});st.fastrun=true;}else{RestoreGroup("fastrun");st.fastrun=false;} }
static void Toggle_SkipItem()   { if(!st.skip_item){ApplyGroup("skip_item",{{mqitem,V_FALSE,4},{mqitem+4,V_RET,4},{eqitem,V_FALSE,4},{eqitem+4,V_RET,4}});st.skip_item=true;}else{RestoreGroup("skip_item");st.skip_item=false;} }
static void Toggle_SkipMob()    { if(!st.skip_mob){ApplyGroup("skip_mob",{{mqmob,V_FALSE,4},{mqmob+4,V_RET,4},{eqmob,V_FALSE,4},{eqmob+4,V_RET,4}});st.skip_mob=true;}else{RestoreGroup("skip_mob");st.skip_mob=false;} }
static void Toggle_SkipMQQ()    { if(!st.skipmqq){ApplyGroup("skipmqq",{{skipmq,V_SKIPMQQ0,4},{skipmq+4,V_SKIPMQQ1,4}});st.skipmqq=true;}else{RestoreGroup("skipmqq");st.skipmqq=false;} }
static void Toggle_UnlockMap()  { if(!st.unlockmap){ApplyGroup("unlockmap",{{unlockmap,V_UMAP0,4},{unlockmap1,V_UMAP1,4}});st.unlockmap=true;}else{RestoreGroup("unlockmap");st.unlockmap=false;} }
static void Toggle_NoDrop()     { if(!st.nodrop){ApplyGroup("nodrop",{{nodrop,V_NODROP0,4},{nodrop+4,V_NODROP1,4}});st.nodrop=true;}else{RestoreGroup("nodrop");st.nodrop=false;} }
static void Toggle_QuickBrave() { if(!st.quick_brave){ApplyGroup("quick_brave",{{quick_aura,V_QUICK_BRAVE,4}});st.quick_brave=true;}else{RestoreGroup("quick_brave");st.quick_brave=false;} }
static void Toggle_CFQloader()  { if(!st.cf_qloader){ApplyGroup("cf_qloader",{{cf_qloader1,V_CF1,4},{cf_qloader2,V_CF2,4}});st.cf_qloader=true;}else{RestoreGroup("cf_qloader");st.cf_qloader=false;} }

static void SetPhysCrit(int l)  { RestoreGroup("phys_crit");st.phys_crit=false;st.phys_crit_idx=0;if(!l)return;const uint8_t*v=(l==1)?V_CRIT_LOW:(l==2)?V_CRIT_MED:V_CRIT_HIGH;ApplyGroup("phys_crit",{{phys_crit,v,4},{phys_crit+4,V_RET,4}});st.phys_crit=true;st.phys_crit_idx=l; }
static void SetMagCrit(int l)   { RestoreGroup("mag_crit");st.mag_crit=false;st.mag_crit_idx=0;if(!l)return;const uint8_t*v=(l==1)?V_CRIT_LOW:(l==2)?V_CRIT_MED:V_CRIT_HIGH;ApplyGroup("mag_crit",{{mag_crit,v,4},{mag_crit+4,V_RET,4}});st.mag_crit=true;st.mag_crit_idx=l; }
static void SetProrate(int l)   { RestoreGroup("prorate");st.prorate=false;st.prorate_idx=0;if(!l)return;const uint8_t*v=(l==1)?V_PRORATE_LOW:(l==2)?V_PRORATE_MED:V_PRORATE_HIGH;ApplyGroup("prorate",{{p_boss_n,v,4},{p_boss_n+4,V_RET,4},{p_boss_s,v,4},{p_boss_s+4,V_RET,4},{p_boss_m,v,4},{p_boss_m+4,V_RET,4},{p_mob_n,v,4},{p_mob_n+4,V_RET,4},{p_mob_s,v,4},{p_mob_s+4,V_RET,4},{p_mob_m,v,4},{p_mob_m+4,V_RET,4}});st.prorate=true;st.prorate_idx=l; }
static void SetBraveDmg(int l)  { RestoreGroup("brave_dmg");st.brave_dmg=false;st.brave_boost_idx=0;if(!l)return;const uint8_t*v=(l==1)?V_BRAVE_L1:(l==2)?V_BRAVE_L2:V_BRAVE_L3;ApplyGroup("brave_dmg",{{brave_dmg,v,4}});st.brave_dmg=true;st.brave_boost_idx=l; }
static void SetElement(int i)   { RestoreGroup("element");st.element=false;st.element_idx=0;if(!i)return;ApplyGroup("element",{{element_setting,V_EL[i-1],4},{element_setting+4,V_RET,4}});st.element=true;st.element_idx=i; }

static void ApplyPreset(int ch) {
    if(!st.godmode) Toggle_GodMode();
    if(!st.mobatk)  Toggle_MobAtk();
    if(!st.fskip)   Toggle_FSkip();
    if(!st.nodrop)  Toggle_NoDrop();
    if(ch==1||ch==3){
        if(!st.crit)        Toggle_Crit();
        if(!st.stab)        Toggle_Stab();
        if(!st.hit)         Toggle_Hit();
        if(!st.fastanim_v2) Toggle_FastAnimV2();
        if(!st.longrange)   Toggle_LongRange();
        if(!st.mobeva)      Toggle_MobEva();
        if(!st.ailment)     Toggle_Ailment();
        if(!st.quick_brave) Toggle_QuickBrave();
        if(!st.prorate_idx)     SetProrate(2);
        if(!st.brave_boost_idx) SetBraveDmg(2);
    }
    if(ch==3){ if(!st.cf_qloader) Toggle_CFQloader(); }
}

static void RestoreAll() {
    for(auto& kv:R) for(auto& e:kv.second) KittyMemory::memWrite((void*)e.addr,e.orig,e.sz);
    R.clear(); st={};
}

// ============================================================
//  ENVY BLUE MODERN THEME
// ============================================================
static void ApplyEnvyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c = s.Colors;

    ImVec4 bgDeep    = ImVec4(0.04f, 0.06f, 0.14f, 0.97f);
    ImVec4 bgMid     = ImVec4(0.07f, 0.10f, 0.22f, 1.00f);
    ImVec4 bgPanel   = ImVec4(0.09f, 0.13f, 0.28f, 1.00f);
    ImVec4 accent    = ImVec4(0.09f, 0.64f, 0.96f, 1.00f);
    ImVec4 accentHov = ImVec4(0.20f, 0.76f, 1.00f, 1.00f);
    ImVec4 accentAct = ImVec4(0.04f, 0.44f, 0.75f, 1.00f);
    ImVec4 teal      = ImVec4(0.04f, 0.80f, 0.76f, 1.00f);
    ImVec4 tealDim   = ImVec4(0.04f, 0.80f, 0.76f, 0.25f);
    ImVec4 txtMain   = ImVec4(0.90f, 0.96f, 1.00f, 1.00f);
    ImVec4 txtDim    = ImVec4(0.55f, 0.70f, 0.85f, 1.00f);
    ImVec4 border    = ImVec4(0.09f, 0.64f, 0.96f, 0.55f);

    c[ImGuiCol_WindowBg]             = bgDeep;
    c[ImGuiCol_ChildBg]              = bgMid;
    c[ImGuiCol_PopupBg]              = bgMid;
    c[ImGuiCol_TitleBg]              = bgPanel;
    c[ImGuiCol_TitleBgActive]        = ImVec4(0.05f, 0.28f, 0.60f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]     = bgDeep;
    c[ImGuiCol_Header]               = tealDim;
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.04f, 0.80f, 0.76f, 0.45f);
    c[ImGuiCol_HeaderActive]         = ImVec4(0.04f, 0.80f, 0.76f, 0.65f);
    c[ImGuiCol_Button]               = ImVec4(0.06f, 0.22f, 0.50f, 1.00f);
    c[ImGuiCol_ButtonHovered]        = accentHov;
    c[ImGuiCol_ButtonActive]         = accentAct;
    c[ImGuiCol_FrameBg]              = ImVec4(0.06f, 0.10f, 0.22f, 1.00f);
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.09f, 0.20f, 0.42f, 1.00f);
    c[ImGuiCol_FrameBgActive]        = accentAct;
    c[ImGuiCol_CheckMark]            = accent;
    c[ImGuiCol_SliderGrab]           = accent;
    c[ImGuiCol_SliderGrabActive]     = accentHov;
    c[ImGuiCol_Tab]                  = bgPanel;
    c[ImGuiCol_TabHovered]           = accentHov;
    c[ImGuiCol_TabActive]            = accent;
    c[ImGuiCol_TabUnfocused]         = bgDeep;
    c[ImGuiCol_TabUnfocusedActive]   = bgPanel;
    c[ImGuiCol_Separator]            = border;
    c[ImGuiCol_SeparatorHovered]     = accent;
    c[ImGuiCol_SeparatorActive]      = accentHov;
    c[ImGuiCol_Border]               = border;
    c[ImGuiCol_BorderShadow]         = ImVec4(0.09f, 0.64f, 0.96f, 0.12f);
    c[ImGuiCol_Text]                 = txtMain;
    c[ImGuiCol_TextDisabled]         = txtDim;
    c[ImGuiCol_ScrollbarBg]          = bgDeep;
    c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.09f, 0.35f, 0.65f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = accent;
    c[ImGuiCol_ScrollbarGrabActive]  = accentHov;
    c[ImGuiCol_ResizeGrip]           = tealDim;
    c[ImGuiCol_ResizeGripHovered]    = teal;
    c[ImGuiCol_ResizeGripActive]     = accent;

    s.WindowRounding    = 12.0f;
    s.FrameRounding     = 8.0f;
    s.GrabRounding      = 8.0f;
    s.TabRounding       = 6.0f;
    s.PopupRounding     = 8.0f;
    s.ChildRounding     = 8.0f;
    s.ScrollbarRounding = 8.0f;
    s.WindowBorderSize  = 1.5f;
    s.FrameBorderSize   = 0.8f;
    s.ItemSpacing       = ImVec2(10.0f, 8.0f);
    s.FramePadding      = ImVec2(10.0f, 6.0f);
    s.WindowPadding     = ImVec2(14.0f, 12.0f);
    s.WindowTitleAlign  = ImVec2(0.5f, 0.5f);
    s.IndentSpacing     = 20.0f;
}

// ============================================================
//  ImGui Menu helpers
// ============================================================
static const char* EL_NAMES[]     = {"Neutral","Fire","Water","Wind","Earth","Light","Dark"};
static const char* PRORATE_NAMES[]= {"OFF","Low 150%","Med 200%","High 300%"};
static const char* CRIT_NAMES[]   = {"OFF","+50","+100","+150"};
static const char* BRAVE_NAMES[]  = {"OFF","Lv1 +100","Lv2 +200","Lv3 +300"};

#define TOGGLE(label, flag, fn) \
    do { \
        bool _v = flag; \
        if(ImGui::Checkbox(label, &_v)) { fn(); } \
    } while(0)

// ============================================================
//  BeginDraw
// ============================================================
static void BeginDraw() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(980, 720), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.97f);

    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.05f,0.28f,0.60f,1.00f));
    bool open = ImGui::Begin("  EnvyMOD  |  Toram Online  |  by Envy  ", 0,
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleColor();
    if(!open){ ImGui::End(); return; }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.04f,0.80f,0.76f,1.00f));
    ImGui::Text("~ EnvyMOD v1.0  |  Toram Online ~");
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.09f,0.64f,0.96f,0.70f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // PRESET
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.05f,0.28f,0.60f,0.55f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.09f,0.64f,0.96f,0.45f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.04f,0.80f,0.76f,1.00f));
    bool showPreset = ImGui::CollapsingHeader("  Preset / One Click");
    ImGui::PopStyleColor(3);
    if(showPreset) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.05f,0.35f,0.70f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.09f,0.64f,0.96f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.02f,0.25f,0.55f,1.00f));
        if(ImGui::Button("  Attacker ",  ImVec2(210,55))) ApplyPreset(1);
        ImGui::SameLine();
        if(ImGui::Button("  Support ",   ImVec2(210,55))) ApplyPreset(2);
        ImGui::SameLine();
        if(ImGui::Button("  Bowgun CF ", ImVec2(210,55))) ApplyPreset(3);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f,0.10f,0.10f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f,0.20f,0.20f,1.00f));
        if(ImGui::Button("  Restore All ", ImVec2(210,55))) RestoreAll();
        ImGui::PopStyleColor(5);
        ImGui::Spacing();
    }

    // PLAYER
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.04f,0.18f,0.45f,0.55f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.09f,0.64f,0.96f,0.35f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.90f,0.96f,1.00f,1.00f));
    bool showPlayer = ImGui::CollapsingHeader("  Player");
    ImGui::PopStyleColor(3);
    if(showPlayer) {
        ImGui::Spacing();
        TOGGLE("  God Mode",          st.godmode,    Toggle_GodMode);
        TOGGLE("  100% Critical",     st.crit,       Toggle_Crit);
        TOGGLE("  Max ASPD",          st.aspd,       Toggle_ASPD);
        TOGGLE("  Max Motion Speed",  st.motion,     Toggle_Motion);
        TOGGLE("  Max Stability",     st.stab,       Toggle_Stab);
        TOGGLE("  Max Accuracy",      st.hit,        Toggle_Hit);
        TOGGLE("  Fast Anim V1",      st.fastanim_v1,Toggle_FastAnimV1);
        TOGGLE("  Fast Anim V2 (MAX)",st.fastanim_v2,Toggle_FastAnimV2);
        TOGGLE("  Long Range",        st.longrange,  Toggle_LongRange);
        TOGGLE("  Always Penetrate",  st.penet,      Toggle_Penetrate);
        ImGui::Spacing();
    }

    // DAMAGE
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.04f,0.18f,0.45f,0.55f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.09f,0.64f,0.96f,0.35f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.90f,0.96f,1.00f,1.00f));
    bool showDmg = ImGui::CollapsingHeader("  Damage Options");
    ImGui::PopStyleColor(3);
    if(showDmg) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f,0.85f,1.00f,1.00f));
        ImGui::Text("Phys Crit Dmg : %s", CRIT_NAMES[st.phys_crit_idx]);
        ImGui::Text("Magic Crit Dmg: %s", CRIT_NAMES[st.mag_crit_idx]);
        ImGui::Text("Proration     : %s", PRORATE_NAMES[st.prorate_idx]);
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.04f,0.20f,0.48f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.09f,0.64f,0.96f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.02f,0.15f,0.40f,1.00f));
        if(ImGui::Button("P:OFF"))  SetPhysCrit(0); ImGui::SameLine();
        if(ImGui::Button("+50P"))   SetPhysCrit(1); ImGui::SameLine();
        if(ImGui::Button("+100P"))  SetPhysCrit(2); ImGui::SameLine();
        if(ImGui::Button("+150P"))  SetPhysCrit(3);
        if(ImGui::Button("M:OFF"))  SetMagCrit(0);  ImGui::SameLine();
        if(ImGui::Button("+50M"))   SetMagCrit(1);  ImGui::SameLine();
        if(ImGui::Button("+100M"))  SetMagCrit(2);  ImGui::SameLine();
        if(ImGui::Button("+150M"))  SetMagCrit(3);
        if(ImGui::Button("PR:OFF")) SetProrate(0);  ImGui::SameLine();
        if(ImGui::Button("150%"))   SetProrate(1);  ImGui::SameLine();
        if(ImGui::Button("200%"))   SetProrate(2);  ImGui::SameLine();
        if(ImGui::Button("300%"))   SetProrate(3);
        ImGui::PopStyleColor(3);
        ImGui::Spacing();
    }

    // SKILL MODS
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.04f,0.18f,0.45f,0.55f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.09f,0.64f,0.96f,0.35f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.90f,0.96f,1.00f,1.00f));
    bool showSkill = ImGui::CollapsingHeader("  Skill Mods");
    ImGui::PopStyleColor(3);
    if(showSkill) {
        ImGui::Spacing();
        TOGGLE("  Quick > Brave Aura",     st.quick_brave, Toggle_QuickBrave);
        TOGGLE("  Auto Quick Loader (CF)", st.cf_qloader,  Toggle_CFQloader);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f,0.85f,1.00f,1.00f));
        ImGui::Text("Brave Aura Dmg: %s", BRAVE_NAMES[st.brave_boost_idx]);
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.04f,0.20f,0.48f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.09f,0.64f,0.96f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.02f,0.15f,0.40f,1.00f));
        if(ImGui::Button("Brave OFF")) SetBraveDmg(0); ImGui::SameLine();
        if(ImGui::Button("Lv1"))       SetBraveDmg(1); ImGui::SameLine();
        if(ImGui::Button("Lv2"))       SetBraveDmg(2); ImGui::SameLine();
        if(ImGui::Button("Lv3"))       SetBraveDmg(3);
        ImGui::PopStyleColor(3);
        ImGui::Spacing();
    }

    // ELEMENT
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.04f,0.18f,0.45f,0.55f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.09f,0.64f,0.96f,0.35f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.90f,0.96f,1.00f,1.00f));
    bool showEl = ImGui::CollapsingHeader("  Element Setting");
    ImGui::PopStyleColor(3);
    if(showEl) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f,0.85f,1.00f,1.00f));
        ImGui::Text("Active: %s", st.element_idx==0?"OFF":EL_NAMES[st.element_idx-1]);
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.04f,0.20f,0.48f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.09f,0.64f,0.96f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.02f,0.15f,0.40f,1.00f));
        if(ImGui::Button("OFF")) SetElement(0);
        for(int i=0;i<7;i++){ ImGui::SameLine(); if(ImGui::Button(EL_NAMES[i])) SetElement(i+1); }
        ImGui::PopStyleColor(3);
        ImGui::Spacing();
    }

    // MOB
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.04f,0.18f,0.45f,0.55f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.09f,0.64f,0.96f,0.35f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.90f,0.96f,1.00f,1.00f));
    bool showMob = ImGui::CollapsingHeader("  Mob Control");
    ImGui::PopStyleColor(3);
    if(showMob) {
        ImGui::Spacing();
        TOGGLE("  Mob No Evasion",  st.mobeva,  Toggle_MobEva);
        TOGGLE("  Mob No ATK",      st.mobatk,  Toggle_MobAtk);
        TOGGLE("  100% Ailment/CC", st.ailment, Toggle_Ailment);
        ImGui::Spacing();
    }

    // OTHER
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.04f,0.18f,0.45f,0.55f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.09f,0.64f,0.96f,0.35f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.90f,0.96f,1.00f,1.00f));
    bool showOther = ImGui::CollapsingHeader("  Other / QoL");
    ImGui::PopStyleColor(3);
    if(showOther) {
        ImGui::Spacing();
        TOGGLE("  Skip Event/Scenario", st.fskip,     Toggle_FSkip);
        TOGGLE("  Skip Boss Battle",    st.skipmq1,   Toggle_SkipBoss);
        TOGGLE("  Fast Run",            st.fastrun,   Toggle_FastRun);
        TOGGLE("  Skip Quest Item",     st.skip_item, Toggle_SkipItem);
        TOGGLE("  Skip Quest Mob",      st.skip_mob,  Toggle_SkipMob);
        TOGGLE("  Fake MQ Completion",  st.skipmqq,   Toggle_SkipMQQ);
        TOGGLE("  Unlock World Map",    st.unlockmap, Toggle_UnlockMap);
        TOGGLE("  No Drop Animation",   st.nodrop,    Toggle_NoDrop);
        ImGui::Spacing();
    }

    // Footer
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.09f,0.64f,0.96f,0.40f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f,0.55f,0.75f,1.00f));
    ImGui::Text("  EnvyMOD by Envy  |  Use responsibly.");
    ImGui::PopStyleColor();

    ImGui::End();
}

// ============================================================
//  EGL Hook
// ============================================================
EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
EGLBoolean _eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);
    if(glWidth<=0 || glHeight<=0)
        return eglSwapBuffers(dpy, surface);

    if(!setup) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        ImGui::StyleColorsDark();
        ApplyEnvyTheme();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        ImFontConfig font_cfg;
        io.Fonts->AddFontFromMemoryTTF(&Roboto_Regular, sizeof(Roboto_Regular), 40.0, &font_cfg,
            io.Fonts->GetGlyphRangesCyrillic());
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        setup = true;
    }

    ImGuiIO &io = ImGui::GetIO();
    static bool s_lastWantTextInput = false;
    bool wantTextInput = io.WantTextInput;
    if(wantTextInput != s_lastWantTextInput){
        s_lastWantTextInput = wantTextInput;
        ShowKeyboard(wantTextInput);
    }

    static MethodInfo *TouchCountMethod   = nullptr;
    static MethodInfo *GetTouchMethodInfo = nullptr;
    if(!TouchCountMethod)   TouchCountMethod   = Il2cpp::FindMethod(OBFUSCATE("UnityEngine.Input"),OBFUSCATE("get_touchCount"),0);
    if(!GetTouchMethodInfo) GetTouchMethodInfo = Il2cpp::FindMethod(OBFUSCATE("UnityEngine.Input"),OBFUSCATE("GetTouch"),1);

    if(TouchCountMethod && GetTouchMethodInfo) {
        int touchCount = TouchCountMethod->invoke_static<int>();
        if(touchCount>0) {
            auto touch = GetTouchMethodInfo->invoke_static<UnityEngine_Touch_Fields>(0);
            float ry = io.DisplaySize.y - touch.m_Position.fields.y;
            switch(touch.m_Phase) {
                case TouchPhase::Began:
                case TouchPhase::Stationary:
                    io.MousePos=ImVec2(touch.m_Position.fields.x,ry); io.MouseDown[0]=true; break;
                case TouchPhase::Ended:
                case TouchPhase::Canceled:
                    io.MouseDown[0]=false; should_clear_mouse_pos=true; break;
                case TouchPhase::Moved:
                    io.MousePos=ImVec2(touch.m_Position.fields.x,ry); break;
                default: break;
            }
        } else { io.MouseDown[0]=false; }
    }

    ImGui_ImplOpenGL3_NewFrame();
    io.DisplaySize = ImVec2((float)glWidth,(float)glHeight);
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
    while(!il2cppMap.isValid()){
        il2cppMap = KittyMemory::getLibraryBaseMap("libil2cpp.so");
        sleep(1);
    }
    il2cppBase = il2cppMap.startAddress;  // fixed: startAddr -> startAddress
    Il2cpp::Init();

    Tools::Hook(
        (void*)DobbySymbolResolver("/system/lib/libandroid.so","ANativeWindow_getWidth"),
        (void*)_ANativeWindow_getWidth,(void**)&orig_ANativeWindow_getWidth);
    Tools::Hook(
        (void*)DobbySymbolResolver("/system/lib/libinput.so",
            "_ZN7android13InputConsumer21initializeMotionEventEPNS_11MotionEventEPKNS_12InputMessageE"),
        (void*)myInput,(void**)&origInput);

    auto swapBuffers=(uintptr_t)DobbySymbolResolver(OBFUSCATE("libEGL.so"),OBFUSCATE("eglSwapBuffers"));
    DobbyHook((void*)swapBuffers,(void*)_eglSwapBuffers,(void**)&orig_eglSwapBuffers);
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
