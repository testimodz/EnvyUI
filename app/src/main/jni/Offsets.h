#pragma once

// ============================================================
//  Toram Online - Offsets
//  Format: (uintptr_t) offset from libil2cpp.so base
// ============================================================

namespace Toram {
    namespace Offsets {

        // --- God Mode ---
        constexpr uintptr_t godmode        = 33480328; // PlayerStatus$$get_IsInvincibility

        // --- Critical ---
        constexpr uintptr_t crit1          = 33614860; // PlayerAttackBase$$checkCriticalPercent
        constexpr uintptr_t crit2          = 33615140; // PlayerAttackBase$$CheckMagicCritical
        constexpr uintptr_t crit3          = 33615360; // PlayerAttackBase$$CheckMagicCritical

        // --- Attack Speed ---
        constexpr uintptr_t aspd           = 33440196; // PlayerSecondaryStatus$$get_Aspd
        constexpr uintptr_t motion         = 33454204; // PlayerSecondaryStatus$$GetMotionSpeed

        // --- Status ---
        constexpr uintptr_t stab           = 33436992; // PlayerSecondaryStatus$$get_Stable
        constexpr uintptr_t hit            = 33437968; // PlayerSecondaryStatus$$get_Hit

        // --- Range / Penetration ---
        constexpr uintptr_t longrange      = 31984864; // EnemyMobActionManagerBase$$get_Size
        constexpr uintptr_t penetrate      = 33586564; // PlayerAttackBase$$CalcPowerResistDamage
        constexpr uintptr_t magpenet       = 33586768; // PlayerAttackBase$$CalcMagicResistDamage

        // --- Critical Damage ---
        constexpr uintptr_t phys_crit      = 33446932; // PlayerSecondaryStatus$$get_CriticalDmg
        constexpr uintptr_t mag_crit       = 33447748; // PlayerSecondaryStatus$$get_CriticalMagicDmg

        // --- Boss Defense ---
        constexpr uintptr_t p_boss_n       = 31876768; // BossMobBattleStatus$$get_ExpDefNormal
        constexpr uintptr_t p_boss_s       = 31876792; // BossMobBattleStatus$$get_ExpDefSkill
        constexpr uintptr_t p_boss_m       = 31876816; // BossMobBattleStatus$$get_ExpDefMagic

        // --- Mob Defense ---
        constexpr uintptr_t p_mob_n        = 32107216; // MobBattleStatus$$get_ExpDefNormal
        constexpr uintptr_t p_mob_s        = 32107240; // MobBattleStatus$$get_ExpDefSkill
        constexpr uintptr_t p_mob_m        = 32107264; // MobBattleStatus$$get_ExpDefMagic

        // --- Movement ---
        constexpr uintptr_t fast_run       = 33174416; // PlayerActionManager$$get_MoveSpeed

        // --- Element / Misc ---
        constexpr uintptr_t element_setting = 33488440; // PlayerStatusBase$$GetEquipElement
        constexpr uintptr_t mobeva         = 33565364; // PlayerAttackBase$$checkMobReaction
        constexpr uintptr_t mobatk         = 28557304; // MobBattleSystemManager$$CreateMobAction
        constexpr uintptr_t ailment        = 33613468; // PlayerAttackBase$$checkAbnormalPercent

        // --- Field Skip ---
        constexpr uintptr_t fskip1         = 38757392; // FieldScriptManager$$ChangeEventScene
        constexpr uintptr_t fskip2         = 38758120; // FieldScriptManager$$GetUIEventMessageWindow
        constexpr uintptr_t skipbattle     = 34910916; // RoomManager$$get_IsBattleEnd

        // --- Quest Items ---
        constexpr uintptr_t mqitem         = 55473764; // Toram.Common.Scenarios.Quests.QuestItemCommon$$get_ItemNum
        constexpr uintptr_t eqitem         = 55480256; // Toram.Common.Scenarios.Missions.MissionItemCommon$$get_ItemNum
        constexpr uintptr_t mqmob          = 55475256; // Toram.Common.Scenarios.Quests.QuestMobCommon$$get_SubdueNum
        constexpr uintptr_t eqmob          = 55481740; // Toram.Common.Scenarios.Missions.MissionMobCommon$$get_SubdueNum

        // --- Animation ---
        constexpr uintptr_t anim1          = 38662396; // TakePlayer$$TakePlay +340
        constexpr uintptr_t anim2          = 33454172; // PlayerSecondaryStatus$$GetNextAtkTime +8

        // --- Map Unlock (wildcard patch) ---
        constexpr uintptr_t unlockmap      = 28172724; // UIWorldMapPanel$$MoveNext +3424
        constexpr uintptr_t unlockmap1     = 28172728; // UIWorldMapPanel$$MoveNext +3428

        // --- Aura ---
        constexpr uintptr_t quick_aura     = 35427560; // QuickAuraAction$$ActionHit +156
        constexpr uintptr_t brave_dmg      = 36392004; // BraveAuraBuf$$GetParam +120

        // --- Skip / Drop ---
        constexpr uintptr_t skipmq         = 34610964; // QuestManager$$LoadLocalizeData +656
        constexpr uintptr_t nodrop         = 35890428; // EffectPlayer$$PlayDropBox

        // --- CrossFire ---
        constexpr uintptr_t cf_qloader1    = 36186240; // CrossFireAction$$ActionStart +320
        constexpr uintptr_t cf_qloader2    = 36186296; // CrossFireAction$$ActionStart +376

    } // namespace Offsets
} // namespace Toram
