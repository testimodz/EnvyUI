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
//  ARM64 Patch Constants  (matches Lua PATCH CONSTANTS)
// ============================================================
// v_true          = -763363296  → 0xD2800020  MOV W0, #1
// v_false         = -763363328  → 0xD2800000  MOV W0, #0
// v_ret           = -698416192  → 0xD65F03C0  RET
// v_nop           = -721215457  → 0xD503201F  NOP
// v_aspd_patch    = 1384155520  → 0x52A00780  MOV W0, #0x803C  (large ASPD)
// v_motion_patch  = -763361408  → 0xD2800880  MOV W0, #0x44  (motion)
// v_stab_patch    = 1384123520  → 0x529FF780  MOV W0, #0xFFBC  (~stability max)
// v_hit_patch[0]  = 1385223136  → 0x52A17620  MOV W0, #0xBB1
// v_hit_patch[1]  = 1923088416  → 0x72A00020  MOVK W0, #1, LSL#16

static const uint8_t V_TRUE[]          = {0x20,0x00,0x80,0xD2};
static const uint8_t V_FALSE[]         = {0x00,0x00,0x80,0xD2};
static const uint8_t V_RET[]           = {0xC0,0x03,0x5F,0xD6};
static const uint8_t V_NOP[]           = {0x1F,0x20,0x03,0xD5};
static const uint8_t V_ASPD[]          = {0x80,0x07,0xA0,0x52}; // MOV W0,#0x803C
static const uint8_t V_MOTION[]        = {0x80,0x08,0x80,0xD2}; // MOV W0,#0x44
static const uint8_t V_STAB[]          = {0x80,0xF7,0x9F,0x52}; // MOV W0,#0xFFBC
static const uint8_t V_HIT0[]          = {0x20,0x76,0xA1,0x52}; // MOV W0,#0xBB1
static const uint8_t V_HIT1[]          = {0x20,0x00,0xA0,0x72}; // MOVK W0,#1,LSL#16
// FastAnim
static const uint8_t V_ANIM1_A[]       = {0xA1,0x00,0x80,0x52}; // MOV W1,#5
static const uint8_t V_ANIM1_B[]       = {0x82,0x3E,0x80,0xD2}; // MOV X2,#0xF4
static const uint8_t V_ANIM2_CLR[]     = {0xE8,0x03,0x1F,0x2A}; // MOV W8,WZR
// LongRange
static const uint8_t V_LR_A[]          = {0x00,0x00,0x22,0x1E}; // FMOV S0,#0 → large val
static const uint8_t V_LR_B[]          = {0x00,0xFC,0x1F,0x1E}; // large float
// Penetrate
static const uint8_t V_PENET[]         = {0x20,0xCC,0x83,0x12}; // 318929952
// UnlockMap
static const uint8_t V_UMAP0[]         = {0x28,0x00,0x80,0x52}; // MOV W8,#1
static const uint8_t V_UMAP1[]         = {0x29,0x00,0x80,0x52}; // MOV W9,#1
// NoDrop
static const uint8_t V_NODROP0[]       = {0x00,0x00,0x80,0xD2}; // MOV X0,#0
static const uint8_t V_NODROP1[]       = {0xC0,0x03,0x5F,0xD6}; // RET
// FastRun
static const uint8_t V_FASTRUN0[]      = {0x00,0xD0,0x25,0x1E}; // FMOV S0,large
static const uint8_t V_FASTRUN1[]      = {0xC0,0x03,0x5F,0xD6}; // RET
// SkipEvent (fskip)
static const uint8_t V_FSKIP_A[]       = {0x88,0x00,0xA4,0x52}; // 1384120456
static const uint8_t V_FSKIP_B[]       = {0x28,0x69,0x39,0x39}; // 956533352 -- STR
static const uint8_t V_FSKIP_C[]       = {0x48,0x69,0x39,0x39}; // 956533384
// SkipBoss
static const uint8_t V_SKIPBOSS0[]     = {0x60,0x00,0xA4,0x52}; // 1384120352
static const uint8_t V_SKIPBOSS1[]     = {0x1D,0x00,0x00,0x14}; // 335544349 B+0x1D
// SkipItem/Mob → use V_FALSE + V_RET
// SkipMQQ
static const uint8_t V_SKIPMQQ0[]      = {0xA8,0x0F,0x80,0x52}; // MOV W8,#0x7D
static const uint8_t V_SKIPMQQ1[]      = {0x68,0x8E,0x00,0xB9}; // STR W8,[X19,#0x8C]
// QuickToBrave
static const uint8_t V_QUICK_BRAVE[]   = {0xA1,0x1C,0x80,0x52}; // MOV W1,#0xE5
// CF Qloader
static const uint8_t V_CF1[]           = {0x08,0x00,0x00,0x14}; // B +0x20
static const uint8_t V_CF2[]           = {0x06,0x00,0x00,0x14}; // B +0x18
// Ailment
static const uint8_t V_AILMENT[]       = {0xA0,0x1E,0x80,0xD2}; // -763354144 = 0xD2801EA0
// MobEva / MobAtk → use V_FALSE + V_RET
// Proration levels
static const uint8_t V_PRORATE_LOW[]   = {0xC0,0x12,0x80,0x52}; // 150
static const uint8_t V_PRORATE_MED[]   = {0x00,0x19,0x80,0x52}; // 200
static const uint8_t V_PRORATE_HIGH[]  = {0x80,0x25,0x80,0x52}; // 300
// CritDmg levels (+50/+100/+150)
static const uint8_t V_CRIT_LOW[]      = {0x00,0x19,0x80,0xD2}; // 50  hex:001980D2
static const uint8_t V_CRIT_MED[]      = {0x40,0x1F,0x80,0xD2}; // 100 hex:401F80D2
static const uint8_t V_CRIT_HIGH[]     = {0x80,0x25,0x80,0xD2}; // 150 hex:802580D2
// BraveDmg levels
static const uint8_t V_BRAVE_L1[]      = {0x00,0x91,0x01,0x11}; // +100 hex:00910111
static const uint8_t V_BRAVE_L2[]      = {0x00,0x21,0x03,0x11}; // +200 hex:00210311
static const uint8_t V_BRAVE_L3[]      = {0x00,0xB1,0x04,0x11}; // +300 hex:00B10411
// Element
static const uint8_t V_EL[7][4] = {
    {0x00,0x00,0x80,0x52}, // 0 Neutral
    {0x20,0x00,0x80,0x52}, // 1 Fire
    {0x40,0x00,0x80,0x52}, // 2 Water
    {0x60,0x00,0x80,0x52}, // 3 Wind
    {0x80,0x00,0x80,0x52}, // 4 Earth
    {0xA0,0x00,0x80,0x52}, // 5 Light
    {0xC0,0x00,0x80,0x52}, // 6 Dark
};

// ============================================================
//  State & Backup
// ============================================================
static uintptr_t il2cppBase = 0;

struct BackupEntry { uintptr_t addr; uint8_t orig[8]; size_t sz; };
#include <vector>
#include <map>
#include <string>
static std::map<std::string, std::vector<BackupEntry>> R; // backup map, same as Lua R[]

static struct {
    bool godmode, crit, aspd, motion, stab, hit;
    bool fastanim_v1, fastanim_v2, longrange, penet;
    bool mobeva, mobatk, ailment;
    bool fskip, skipmq1, skip_item, skip_mob, skipmqq;
    bool fastrun, unlockmap, nodrop;
    bool cf_qloader, quick_brave;
    bool phys_crit, mag_crit, prorate;
    bool element, brave_dmg;
    int  element_idx;      // 0=off, 1-7=element
    int  brave_boost_idx;  // 0=off, 1-3=level
    int  prorate_idx;      // 0=off, 1-3=level
    int  phys_crit_idx;
    int  mag_crit_idx;
} st = {};

// ============================================================
//  Low-level patch helpers
// ============================================================
static void WriteBytes(uintptr_t offset, const uint8_t* bytes, size_t len) {
    KittyMemory::memWrite((void*)(il2cppBase + offset), bytes, len);
}

static void BackupAndWrite(const std::string& key, uintptr_t offset, const uint8_t* newBytes, size_t len) {
    // backup original only once
    if (R.find(key) == R.end() || R[key].empty()) {
        BackupEntry e;
        e.addr = il2cppBase + offset;
        e.sz   = len;
        KittyMemory::memRead((void*)e.addr, e.orig, len);
        R[key].push_back(e);
    }
    KittyMemory::memWrite((void*)(il2cppBase + offset), newBytes, len);
}

static void RestoreKey(const std::string& key, bool& flag) {
    if (R.count(key)) {
        for (auto& e : R[key])
            KittyMemory::memWrite((void*)e.addr, e.orig, e.sz);
        R.erase(key);
    }
    flag = false;
}

// Backup + write multiple patches under same key
struct Patch { uintptr_t offset; const uint8_t* bytes; size_t len; };
static void ApplyGroup(const std::string& key, const std::vector<Patch>& patches) {
    // backup all first
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
//  Feature Toggles  (mirrors Lua ToggleFeature)
// ============================================================

static void Toggle_GodMode() {
    if (!st.godmode) {
        ApplyGroup("godmode", {
            {godmode,     V_TRUE,  4},
            {godmode + 4, V_RET,   4},
        });
        st.godmode = true;
    } else {
        RestoreGroup("godmode"); st.godmode = false;
    }
}

static void Toggle_Crit() {
    if (!st.crit) {
        ApplyGroup("crit", {
            {crit1,     V_TRUE, 4}, {crit1 + 4, V_RET, 4},
            {crit2,     V_TRUE, 4}, {crit2 + 4, V_RET, 4},
            {crit3,     V_TRUE, 4}, {crit3 + 4, V_RET, 4},
        });
        st.crit = true;
    } else {
        RestoreGroup("crit"); st.crit = false;
    }
}

static void Toggle_ASPD() {
    if (!st.aspd) {
        ApplyGroup("aspd", {
            {aspd,     V_ASPD, 4},
            {aspd + 4, V_RET,  4},
        });
        st.aspd = true;
    } else {
        RestoreGroup("aspd"); st.aspd = false;
    }
}

static void Toggle_Motion() {
    if (!st.motion) {
        ApplyGroup("motion", {
            {motion,     V_MOTION, 4},
            {motion + 4, V_RET,    4},
        });
        st.motion = true;
    } else {
        RestoreGroup("motion"); st.motion = false;
    }
}

static void Toggle_Stab() {
    if (!st.stab) {
        ApplyGroup("stab", {
            {stab,     V_STAB, 4},
            {stab + 4, V_RET,  4},
        });
        st.stab = true;
    } else {
        RestoreGroup("stab"); st.stab = false;
    }
}

static void Toggle_Hit() {
    if (!st.hit) {
        ApplyGroup("hit", {
            {hit,     V_HIT0, 4},
            {hit + 4, V_HIT1, 4},
            {hit + 8, V_RET,  4},
        });
        st.hit = true;
    } else {
        RestoreGroup("hit"); st.hit = false;
    }
}

static void Toggle_FastAnimV1() {
    if (!st.fastanim_v1) {
        ApplyGroup("fastanim_v1", {
            {anim1 + 4, V_ANIM1_B, 4}, // only +4 patch for V1
        });
        st.fastanim_v1 = true;
    } else {
        RestoreGroup("fastanim_v1"); st.fastanim_v1 = false;
    }
}

static void Toggle_FastAnimV2() {
    if (!st.fastanim_v2) {
        ApplyGroup("fastanim_v2", {
            {anim1,     V_ANIM1_A,  4},
            {anim1 + 4, V_ANIM1_B,  4},
            {anim2,     V_ANIM2_CLR,4},
        });
        st.fastanim_v2 = true;
    } else {
        RestoreGroup("fastanim_v2"); st.fastanim_v2 = false;
    }
}

static void Toggle_LongRange() {
    if (!st.longrange) {
        ApplyGroup("longrange", {
            {longrange,     V_LR_A, 4},
            {longrange + 4, V_LR_B, 4},
            {longrange + 8, V_RET,  4},
        });
        st.longrange = true;
    } else {
        RestoreGroup("longrange"); st.longrange = false;
    }
}

static void Toggle_Penetrate() {
    if (!st.penet) {
        ApplyGroup("penet", {
            {penetrate,     V_PENET, 4}, {penetrate + 4, V_RET, 4},
            {magpenet,      V_PENET, 4}, {magpenet  + 4, V_RET, 4},
        });
        st.penet = true;
    } else {
        RestoreGroup("penet"); st.penet = false;
    }
}

static void Toggle_MobEva() {
    if (!st.mobeva) {
        ApplyGroup("mobeva", {
            {mobeva,     V_FALSE, 4},
            {mobeva + 4, V_RET,   4},
        });
        st.mobeva = true;
    } else {
        RestoreGroup("mobeva"); st.mobeva = false;
    }
}

static void Toggle_MobAtk() {
    if (!st.mobatk) {
        ApplyGroup("mobatk", {
            {mobatk,     V_FALSE, 4},
            {mobatk + 4, V_RET,   4},
        });
        st.mobatk = true;
    } else {
        RestoreGroup("mobatk"); st.mobatk = false;
    }
}

static void Toggle_Ailment() {
    if (!st.ailment) {
        ApplyGroup("ailment", {
            {ailment,     V_AILMENT, 4},
            {ailment + 4, V_RET,     4},
        });
        st.ailment = true;
    } else {
        RestoreGroup("ailment"); st.ailment = false;
    }
}

static void Toggle_FSkip() {
    if (!st.fskip) {
        ApplyGroup("fskip", {
            {fskip1 + 0x22C, V_FSKIP_A, 4},
            {fskip1 + 0x230, V_FSKIP_B, 4},
            {fskip2 + 0x78,  V_NOP,     4},
            {fskip2 + 0x7C,  V_FSKIP_A, 4},
            {fskip2 + 0x80,  V_FSKIP_C, 4},
        });
        st.fskip = true;
    } else {
        RestoreGroup("fskip"); st.fskip = false;
    }
}

static void Toggle_SkipBoss() {
    if (!st.skipmq1) {
        ApplyGroup("skipmq1", {
            {skipbattle + 8,   V_SKIPBOSS0, 4},
            {skipbattle + 0xC, V_SKIPBOSS1, 4},
        });
        st.skipmq1 = true;
    } else {
        RestoreGroup("skipmq1"); st.skipmq1 = false;
    }
}

static void Toggle_FastRun() {
    if (!st.fastrun) {
        ApplyGroup("fastrun", {
            {fast_run,     V_FASTRUN0, 4},
            {fast_run + 4, V_FASTRUN1, 4},
        });
        st.fastrun = true;
    } else {
        RestoreGroup("fastrun"); st.fastrun = false;
    }
}

static void Toggle_SkipItem() {
    if (!st.skip_item) {
        ApplyGroup("skip_item", {
            {mqitem,     V_FALSE, 4}, {mqitem + 4, V_RET, 4},
            {eqitem,     V_FALSE, 4}, {eqitem + 4, V_RET, 4},
        });
        st.skip_item = true;
    } else {
        RestoreGroup("skip_item"); st.skip_item = false;
    }
}

static void Toggle_SkipMob() {
    if (!st.skip_mob) {
        ApplyGroup("skip_mob", {
            {mqmob,     V_FALSE, 4}, {mqmob + 4, V_RET, 4},
            {eqmob,     V_FALSE, 4}, {eqmob + 4, V_RET, 4},
        });
        st.skip_mob = true;
    } else {
        RestoreGroup("skip_mob"); st.skip_mob = false;
    }
}

static void Toggle_SkipMQQ() {
    if (!st.skipmqq) {
        ApplyGroup("skipmqq", {
            {skipmq,     V_SKIPMQQ0, 4},
            {skipmq + 4, V_SKIPMQQ1, 4},
        });
        st.skipmqq = true;
    } else {
        RestoreGroup("skipmqq"); st.skipmqq = false;
    }
}

static void Toggle_UnlockMap() {
    if (!st.unlockmap) {
        ApplyGroup("unlockmap", {
            {unlockmap,  V_UMAP0, 4},
            {unlockmap1, V_UMAP1, 4},
        });
        st.unlockmap = true;
    } else {
        RestoreGroup("unlockmap"); st.unlockmap = false;
    }
}

static void Toggle_NoDrop() {
    if (!st.nodrop) {
        ApplyGroup("nodrop", {
            {nodrop,     V_NODROP0, 4},
            {nodrop + 4, V_NODROP1, 4},
        });
        st.nodrop = true;
    } else {
        RestoreGroup("nodrop"); st.nodrop = false;
    }
}

static void Toggle_QuickBrave() {
    if (!st.quick_brave) {
        ApplyGroup("quick_brave", {
            {quick_aura, V_QUICK_BRAVE, 4},
        });
        st.quick_brave = true;
    } else {
        RestoreGroup("quick_brave"); st.quick_brave = false;
    }
}

static void Toggle_CFQloader() {
    if (!st.cf_qloader) {
        ApplyGroup("cf_qloader", {
            {cf_qloader1, V_CF1, 4},
            {cf_qloader2, V_CF2, 4},
        });
        st.cf_qloader = true;
    } else {
        RestoreGroup("cf_qloader"); st.cf_qloader = false;
    }
}

// ============================================================
//  Multi-Level: PhysCrit  (+50/+100/+150 or disable)
// ============================================================
static void SetPhysCrit(int level) { // 0=off, 1=low, 2=med, 3=high
    RestoreGroup("phys_crit"); st.phys_crit = false; st.phys_crit_idx = 0;
    if (level == 0) return;
    const uint8_t* val = (level==1)?V_CRIT_LOW:(level==2)?V_CRIT_MED:V_CRIT_HIGH;
    ApplyGroup("phys_crit", {
        {phys_crit,     val,   4},
        {phys_crit + 4, V_RET, 4},
    });
    st.phys_crit = true; st.phys_crit_idx = level;
}

static void SetMagCrit(int level) { // 0=off, 1=low, 2=med, 3=high
    RestoreGroup("mag_crit"); st.mag_crit = false; st.mag_crit_idx = 0;
    if (level == 0) return;
    const uint8_t* val = (level==1)?V_CRIT_LOW:(level==2)?V_CRIT_MED:V_CRIT_HIGH;
    ApplyGroup("mag_crit", {
        {mag_crit,     val,   4},
        {mag_crit + 4, V_RET, 4},
    });
    st.mag_crit = true; st.mag_crit_idx = level;
}

// ============================================================
//  Multi-Level: Proration  (150% / 200% / 300% or disable)
// ============================================================
static void SetProrate(int level) { // 0=off, 1=150%, 2=200%, 3=300%
    RestoreGroup("prorate"); st.prorate = false; st.prorate_idx = 0;
    if (level == 0) return;
    const uint8_t* val = (level==1)?V_PRORATE_LOW:(level==2)?V_PRORATE_MED:V_PRORATE_HIGH;
    ApplyGroup("prorate", {
        {p_boss_n,     val,   4}, {p_boss_n + 4, V_RET, 4},
        {p_boss_s,     val,   4}, {p_boss_s + 4, V_RET, 4},
        {p_boss_m,     val,   4}, {p_boss_m + 4, V_RET, 4},
        {p_mob_n,      val,   4}, {p_mob_n  + 4, V_RET, 4},
        {p_mob_s,      val,   4}, {p_mob_s  + 4, V_RET, 4},
        {p_mob_m,      val,   4}, {p_mob_m  + 4, V_RET, 4},
    });
    st.prorate = true; st.prorate_idx = level;
}

// ============================================================
//  Multi-Level: BraveDmg  (Lv1/Lv2/Lv3 or disable)
// ============================================================
static void SetBraveDmg(int level) { // 0=off, 1-3=level
    RestoreGroup("brave_dmg"); st.brave_dmg = false; st.brave_boost_idx = 0;
    if (level == 0) return;
    const uint8_t* val = (level==1)?V_BRAVE_L1:(level==2)?V_BRAVE_L2:V_BRAVE_L3;
    ApplyGroup("brave_dmg", {{brave_dmg, val, 4}});
    st.brave_dmg = true; st.brave_boost_idx = level;
}

// ============================================================
//  Element Setting  (0=off/neutral, 1-7=element)
// ============================================================
static void SetElement(int idx) { // 0=disable, 1=neutral...7=dark
    RestoreGroup("element"); st.element = false; st.element_idx = 0;
    if (idx == 0) return;
    ApplyGroup("element", {
        {element_setting,     V_EL[idx-1], 4},
        {element_setting + 4, V_RET,        4},
    });
    st.element = true; st.element_idx = idx;
}

// ============================================================
//  ONE CLICK PRESETS  (mirrors Lua OneClickModActive)
// ============================================================
static void ApplyPreset(int ch) { // 1=Attacker, 2=Support, 3=BowgunCF
    // --- Global (all presets) ---
    if (!st.godmode)  Toggle_GodMode();
    if (!st.mobatk)   Toggle_MobAtk();
    if (!st.fskip)    Toggle_FSkip();
    if (!st.nodrop)   Toggle_NoDrop();

    // --- Attacker (1) & Bowgun CF (3) ---
    if (ch == 1 || ch == 3) {
        if (!st.crit)        Toggle_Crit();
        if (!st.stab)        Toggle_Stab();
        if (!st.hit)         Toggle_Hit();
        if (!st.fastanim_v2) Toggle_FastAnimV2();
        if (!st.longrange)   Toggle_LongRange();
        if (!st.mobeva)      Toggle_MobEva();
        if (!st.ailment)     Toggle_Ailment();
        if (!st.quick_brave) Toggle_QuickBrave();
        if (st.prorate_idx == 0)     SetProrate(2);    // Medium 200%
        if (st.brave_boost_idx == 0) SetBraveDmg(2);   // Level 2
    }

    // --- Bowgun CF exclusive ---
    if (ch == 3) {
        if (!st.cf_qloader) Toggle_CFQloader();
    }
}

// ============================================================
//  Restore All  (mirrors Lua RestoreAll)
// ============================================================
static void RestoreAll() {
    for (auto& kv : R)
        for (auto& e : kv.second)
            KittyMemory::memWrite((void*)e.addr, e.orig, e.sz);
    R.clear();
    st = {}; // zero all state
}

// ============================================================
//  ImGui Menu
// ============================================================

static const char* EL_NAMES[] = {"Neutral","Fire","Water","Wind","Earth","Light","Dark"};
static const char* PRORATE_NAMES[] = {"OFF","Low 150%","Med 200%","High 300%"};
static const char* CRIT_NAMES[] = {"OFF","+50","+100","+150"};
static const char* BRAVE_NAMES[] = {"OFF","Lv1 +100","Lv2 +200","Lv3 +300"};

static void BeginDraw() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(950, 680), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("EnvyUI - Toram Online", 0, ImGuiWindowFlags_NoSavedSettings)) {

        // --- PRESET ---
        if (ImGui::CollapsingHeader("[Preset / One Click]")) {
            ImGui::Text("Activate combined features by role:");
            if (ImGui::Button("Attacker",  ImVec2(200,50))) ApplyPreset(1);
            ImGui::SameLine();
            if (ImGui::Button("Support",   ImVec2(200,50))) ApplyPreset(2);
            ImGui::SameLine();
            if (ImGui::Button("Bowgun CF", ImVec2(200,50))) ApplyPreset(3);
            ImGui::SameLine();
            if (ImGui::Button("Restore All", ImVec2(200,50))) RestoreAll();
        }

        // --- PLAYER ---
        if (ImGui::CollapsingHeader("[Player]")) {
            if (ImGui::Checkbox("God Mode",          &st.godmode))    { if(st.godmode) Toggle_GodMode(); else Toggle_GodMode(); }
            if (ImGui::Checkbox("100% Critical",     &st.crit))       { Toggle_Crit(); st.crit = !st.crit; }
            if (ImGui::Checkbox("Max ASPD",          &st.aspd))       { Toggle_ASPD(); st.aspd = !st.aspd; }
            if (ImGui::Checkbox("Max Motion Speed",  &st.motion))     { Toggle_Motion(); st.motion = !st.motion; }
            if (ImGui::Checkbox("Max Stability",     &st.stab))       { Toggle_Stab(); st.stab = !st.stab; }
            if (ImGui::Checkbox("Max Accuracy",      &st.hit))        { Toggle_Hit(); st.hit = !st.hit; }
            if (ImGui::Checkbox("Fast Anim V1",      &st.fastanim_v1)){ Toggle_FastAnimV1(); st.fastanim_v1 = !st.fastanim_v1; }
            if (ImGui::Checkbox("Fast Anim V2 (MAX)",&st.fastanim_v2)){ Toggle_FastAnimV2(); st.fastanim_v2 = !st.fastanim_v2; }
            if (ImGui::Checkbox("Long Range",        &st.longrange))  { Toggle_LongRange(); st.longrange = !st.longrange; }
            if (ImGui::Checkbox("Always Penetrate",  &st.penet))      { Toggle_Penetrate(); st.penet = !st.penet; }
        }

        // --- DAMAGE ---
        if (ImGui::CollapsingHeader("[Damage Options]")) {
            ImGui::Text("Phys Crit Dmg: %s", CRIT_NAMES[st.phys_crit_idx]);
            if (ImGui::Button("Phys OFF"))  SetPhysCrit(0);
            ImGui::SameLine(); if (ImGui::Button("+50P"))  SetPhysCrit(1);
            ImGui::SameLine(); if (ImGui::Button("+100P")) SetPhysCrit(2);
            ImGui::SameLine(); if (ImGui::Button("+150P")) SetPhysCrit(3);

            ImGui::Text("Magic Crit Dmg: %s", CRIT_NAMES[st.mag_crit_idx]);
            if (ImGui::Button("Mag OFF"))   SetMagCrit(0);
            ImGui::SameLine(); if (ImGui::Button("+50M"))  SetMagCrit(1);
            ImGui::SameLine(); if (ImGui::Button("+100M")) SetMagCrit(2);
            ImGui::SameLine(); if (ImGui::Button("+150M")) SetMagCrit(3);

            ImGui::Text("Proration: %s", PRORATE_NAMES[st.prorate_idx]);
            if (ImGui::Button("Prorate OFF"))   SetProrate(0);
            ImGui::SameLine(); if (ImGui::Button("150%")) SetProrate(1);
            ImGui::SameLine(); if (ImGui::Button("200%")) SetProrate(2);
            ImGui::SameLine(); if (ImGui::Button("300%")) SetProrate(3);
        }

        // --- SKILL MODS ---
        if (ImGui::CollapsingHeader("[Skill Mods]")) {
            if (ImGui::Checkbox("Quick > Brave Aura", &st.quick_brave)) { Toggle_QuickBrave(); st.quick_brave = !st.quick_brave; }
            ImGui::Text("Brave Aura Dmg: %s", BRAVE_NAMES[st.brave_boost_idx]);
            if (ImGui::Button("Brave OFF")) SetBraveDmg(0);
            ImGui::SameLine(); if (ImGui::Button("Lv1")) SetBraveDmg(1);
            ImGui::SameLine(); if (ImGui::Button("Lv2")) SetBraveDmg(2);
            ImGui::SameLine(); if (ImGui::Button("Lv3")) SetBraveDmg(3);

            if (ImGui::Checkbox("Auto Quick Loader (CF)", &st.cf_qloader)) { Toggle_CFQloader(); st.cf_qloader = !st.cf_qloader; }
        }

        // --- ELEMENT ---
        if (ImGui::CollapsingHeader("[Element Setting]")) {
            ImGui::Text("Current: %s", st.element_idx==0?"OFF":EL_NAMES[st.element_idx-1]);
            if (ImGui::Button("Off")) SetElement(0);
            for (int i = 0; i < 7; i++) {
                ImGui::SameLine();
                if (ImGui::Button(EL_NAMES[i])) SetElement(i+1);
            }
        }

        // --- MOB ---
        if (ImGui::CollapsingHeader("[Mob Control]")) {
            if (ImGui::Checkbox("Mob No Evasion", &st.mobeva))  { Toggle_MobEva();  st.mobeva  = !st.mobeva;  }
            if (ImGui::Checkbox("Mob No ATK",     &st.mobatk))  { Toggle_MobAtk();  st.mobatk  = !st.mobatk;  }
            if (ImGui::Checkbox("100% Ailment/CC",&st.ailment)) { Toggle_Ailment(); st.ailment = !st.ailment; }
        }

        // --- OTHER / QoL ---
        if (ImGui::CollapsingHeader("[Other / QoL]")) {
            if (ImGui::Checkbox("Skip Event/Scenario",   &st.fskip))    { Toggle_FSkip();    st.fskip    = !st.fskip;    }
            if (ImGui::Checkbox("Skip Boss Battle",      &st.skipmq1))  { Toggle_SkipBoss(); st.skipmq1  = !st.skipmq1;  }
            if (ImGui::Checkbox("Fast Run",              &st.fastrun))  { Toggle_FastRun();  st.fastrun  = !st.fastrun;  }
            if (ImGui::Checkbox("Skip Quest Item",       &st.skip_item)){ Toggle_SkipItem(); st.skip_item= !st.skip_item;}
            if (ImGui::Checkbox("Skip Quest Mob",        &st.skip_mob)) { Toggle_SkipMob();  st.skip_mob = !st.skip_mob; }
            if (ImGui::Checkbox("Fake MQ Completion",   &st.skipmqq))  { Toggle_SkipMQQ();  st.skipmqq  = !st.skipmqq;  }
            if (ImGui::Checkbox("Unlock World Map",      &st.unlockmap)){ Toggle_UnlockMap();st.unlockmap= !st.unlockmap;}
            if (ImGui::Checkbox("No Drop Animation",     &st.nodrop))   { Toggle_NoDrop();   st.nodrop   = !st.nodrop;   }
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
    if (glWidth <= 0 || glHeight <= 0)
        return eglSwapBuffers(dpy, surface);

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
    bool wantTextInput = io.WantTextInput;
    if (wantTextInput != s_lastWantTextInput) {
        s_lastWantTextInput = wantTextInput;
        ShowKeyboard(wantTextInput);
    }

    static MethodInfo *TouchCountMethod    = nullptr;
    static MethodInfo *GetTouchMethodInfo  = nullptr;
    if (!TouchCountMethod)
        TouchCountMethod   = Il2cpp::FindMethod(OBFUSCATE("UnityEngine.Input"), OBFUSCATE("get_touchCount"), 0);
    if (!GetTouchMethodInfo)
        GetTouchMethodInfo = Il2cpp::FindMethod(OBFUSCATE("UnityEngine.Input"), OBFUSCATE("GetTouch"), 1);

    if (TouchCountMethod && GetTouchMethodInfo) {
        int touchCount = TouchCountMethod->invoke_static<int>();
        if (touchCount > 0) {
            auto touch    = GetTouchMethodInfo->invoke_static<UnityEngine_Touch_Fields>(0);
            float reverseY = io.DisplaySize.y - touch.m_Position.fields.y;
            switch (touch.m_Phase) {
                case TouchPhase::Began:
                case TouchPhase::Stationary:
                    io.MousePos    = ImVec2(touch.m_Position.fields.x, reverseY);
                    io.MouseDown[0]= true; break;
                case TouchPhase::Ended:
                case TouchPhase::Canceled:
                    io.MouseDown[0]      = false;
                    should_clear_mouse_pos = true; break;
                case TouchPhase::Moved:
                    io.MousePos = ImVec2(touch.m_Position.fields.x, reverseY); break;
                default: break;
            }
        } else { io.MouseDown[0] = false; }
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
        (void*)DobbySymbolResolver("/system/lib/libandroid.so", "ANativeWindow_getWidth"),
        (void*)_ANativeWindow_getWidth, (void**)&orig_ANativeWindow_getWidth);
    Tools::Hook(
        (void*)DobbySymbolResolver("/system/lib/libinput.so",
            "_ZN7android13InputConsumer21initializeMotionEventEPNS_11MotionEventEPKNS_12InputMessageE"),
        (void*)myInput, (void**)&origInput);

    auto swapBuffers = (uintptr_t)DobbySymbolResolver(OBFUSCATE("libEGL.so"), OBFUSCATE("eglSwapBuffers"));
    DobbyHook((void*)swapBuffers, (void*)_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
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
