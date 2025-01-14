/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2002 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
	*/

#include "../common/rulesys.h"
#include "../common/strings.h"

#include "client.h"
#include "entity.h"
#include "mob.h"
#include "string_ids.h"
#include "lua_parser.h"

#include <string.h>

extern double frame_time;

int Mob::GetBaseSkillDamage(EQ::skills::SkillType skill, Mob *target)
{
	int base = EQ::skills::GetBaseDamage(skill);
	auto skill_level = GetSkill(skill);
	switch (skill) {
		case EQ::skills::SkillDragonPunch:
		case EQ::skills::SkillEagleStrike:
		case EQ::skills::SkillTigerClaw:
		case EQ::skills::SkillRoundKick:
			if (skill_level >= 25) {
				base++;
			}

			if (skill_level >= 75) {
				base++;
			}

			if (skill_level >= 125) {
				base++;
			}

			if (skill_level >= 175) {
				base++;
			}

			if (RuleB(Character, ItemExtraSkillDamageCalcAsPercent) && GetSkillDmgAmt(skill) > 0) {
				base *= std::abs(GetSkillDmgAmt(skill) / 100);
			}

			return base;
		case EQ::skills::SkillFrenzy:
			if (IsClient() && CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary)) {
				if (GetLevel() > 15) {
					base += GetLevel() - 15;
				}

				if (base > 23) {
					base = 23;
				}

				if (GetLevel() > 50) {
					base += 2;
				}

				if (GetLevel() > 54) {
					base++;
				}

				if (GetLevel() > 59) {
					base++;
				}
			}

			if (RuleB(Character, ItemExtraSkillDamageCalcAsPercent) && GetSkillDmgAmt(skill) > 0) {
				base *= std::abs(GetSkillDmgAmt(skill) / 100);
			}

			return base;
		case EQ::skills::SkillFlyingKick: {
			float skill_bonus = skill_level / 9.0f;
			float ac_bonus    = 0.0f;
			if (IsClient()) {
				auto inst = CastToClient()->GetInv().GetItem(EQ::invslot::slotFeet);
				if (inst) {
					ac_bonus = inst->GetItemArmorClass(true) / 25.0f;
				}
			}

			if (ac_bonus > skill_bonus) {
				ac_bonus = skill_bonus;
			}

			if (RuleB(Character, ItemExtraSkillDamageCalcAsPercent) && GetSkillDmgAmt(skill) > 0) {
				return static_cast<int>(ac_bonus + skill_bonus) * std::abs(GetSkillDmgAmt(skill) / 100);
			}

			return static_cast<int>(ac_bonus + skill_bonus);
		}
		case EQ::skills::SkillKick: {
			// there is some base *= 4 case in here?
			float skill_bonus = skill_level / 10.0f;
			float ac_bonus    = 0.0f;
			if (IsClient()) {
				auto inst = CastToClient()->GetInv().GetItem(EQ::invslot::slotFeet);
				if (inst) {
					ac_bonus = inst->GetItemArmorClass(true) / 25.0f;
				}
			}

			if (ac_bonus > skill_bonus) {
				ac_bonus = skill_bonus;
			}

			if (RuleB(Character, ItemExtraSkillDamageCalcAsPercent) && GetSkillDmgAmt(skill) > 0) {
				return static_cast<int>(ac_bonus + skill_bonus) * std::abs(GetSkillDmgAmt(skill) / 100);
			}

			return static_cast<int>(ac_bonus + skill_bonus);
		}
		case EQ::skills::SkillBash: {
			float                  skill_bonus = skill_level / 10.0f;
			float                  ac_bonus    = 0.0f;
			const EQ::ItemInstance *inst       = nullptr;
			if (IsClient()) {
				if (HasShieldEquipped()) {
					inst = CastToClient()->GetInv().GetItem(EQ::invslot::slotSecondary);
				} else if (HasTwoHanderEquipped()) {
					inst = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
				}
			}

			if (inst) {
				ac_bonus = inst->GetItemArmorClass(true) / 25.0f;
			} else {
				return 0;
			} // return 0 in cases where we don't have an item

			if (ac_bonus > skill_bonus) {
				ac_bonus = skill_bonus;
			}

			if (RuleB(Character, ItemExtraSkillDamageCalcAsPercent) && GetSkillDmgAmt(skill) > 0) {
				return static_cast<int>(ac_bonus + skill_bonus) * std::abs(GetSkillDmgAmt(skill) / 100);
			}

			return static_cast<int>(ac_bonus + skill_bonus);
		}
		case EQ::skills::SkillBackstab: {
			float skill_bonus = static_cast<float>(skill_level) * 0.02f;
			base              = 3; // There seems to be a base 3 for NPCs or some how BS w/o weapon?
			// until we get a better inv system for NPCs they get nerfed!
			if (IsClient()) {
				auto *inst = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
				if (inst && inst->GetItem() && inst->GetItem()->ItemType == EQ::item::ItemType1HPiercing) {
					base = inst->GetItemBackstabDamage(true);
					if (!inst->GetItemBackstabDamage()) {
						base += inst->GetItemWeaponDamage(true);
					}

					if (target) {
						if (inst->GetItemElementalFlag(true) && inst->GetItemElementalDamage(true) &&
							!RuleB(Combat, BackstabIgnoresElemental)) {
							base += target->ResistElementalWeaponDmg(inst);
						}

						if ((inst->GetItemBaneDamageBody(true) || inst->GetItemBaneDamageRace(true)) &&
							!RuleB(Combat, BackstabIgnoresBane)) {
							base += target->CheckBaneDamage(inst);
						}
					}
				}
			} else if (IsNPC()) {
				auto *npc = CastToNPC();
				base = std::max(base, npc->GetBaseDamage());
				// parses show relatively low BS mods from lots of NPCs, so either their BS skill is super low
				// or their mod is divided again, this is probably not the right mod, but it's better
				skill_bonus /= 3.0f;
			}

			if (RuleB(Character, ItemExtraSkillDamageCalcAsPercent) && GetSkillDmgAmt(skill) > 0) {
				return static_cast<int>(static_cast<float>(base) * (skill_bonus + 2.0f)) * std::abs(GetSkillDmgAmt(skill) / 100);
			}

			return static_cast<int>(static_cast<float>(base) * (skill_bonus + 2.0f));
		}
		default: {
			return 0;
		}
	}
}

void Mob::DoSpecialAttackDamage(Mob *who, EQ::skills::SkillType skill, int32 base_damage, int32 min_damage,
				int32 hate_override, int ReuseTime)
{
	// this really should go through the same code as normal melee damage to
	// pick up all the special behavior there

	if ((who == nullptr ||
	     ((IsClient() && CastToClient()->dead) || (who->IsClient() && who->CastToClient()->dead)) || HasDied() ||
	     (!IsAttackAllowed(who))))
		return;

	DamageHitInfo my_hit;
	my_hit.damage_done = 1; // min 1 dmg
	my_hit.base_damage = base_damage;
	my_hit.min_damage = min_damage;
	my_hit.skill = skill;

	if (my_hit.base_damage == 0)
		my_hit.base_damage = GetBaseSkillDamage(my_hit.skill);

	if (base_damage == DMG_INVULNERABLE)
		my_hit.damage_done = DMG_INVULNERABLE;

	if (who->GetInvul() || who->GetSpecialAbility(IMMUNE_MELEE))
		my_hit.damage_done = DMG_INVULNERABLE;

	if (who->GetSpecialAbility(IMMUNE_MELEE_EXCEPT_BANE) && skill != EQ::skills::SkillBackstab)
		my_hit.damage_done = DMG_INVULNERABLE;

	int64 hate = my_hit.base_damage;
	if (hate_override > -1)
		hate = hate_override;

	if (skill == EQ::skills::SkillBash) {
		if (IsClient()) {
			EQ::ItemInstance *item = CastToClient()->GetInv().GetItem(EQ::invslot::slotSecondary);
			if (item) {
				if (item->GetItem()->ItemType == EQ::item::ItemTypeShield) {
					hate += item->GetItem()->AC;
				}
				const EQ::ItemData *itm = item->GetItem();
				auto fbash = GetSpellFuriousBash(itm->Focus.Effect);
				hate = hate * (100 + fbash) / 100;
				if (fbash)
					MessageString(Chat::FocusEffect, GLOWS_RED, itm->Name);
			}
		}
	}

	my_hit.offense = offense(my_hit.skill);
	my_hit.tohit = GetTotalToHit(my_hit.skill, 0);

	my_hit.hand = EQ::invslot::slotPrimary; // Avoid checks hand for throwing/archery exclusion, primary should
						  // work for most
	if (skill == EQ::skills::SkillThrowing || skill == EQ::skills::SkillArchery)
		my_hit.hand = EQ::invslot::slotRange;

	DoAttack(who, my_hit);

	who->AddToHateList(this, hate, 0);
	who->Damage(this, my_hit.damage_done, SPELL_UNKNOWN, skill, false);

	//Pyrelight Custom Code - Send info about the hSTA/hSTR damage modification to clients
	// This is a boilerplate with dead code paths.
	if (my_hit.damage_done > 0 && my_hit.original_damage > 0) {
		if (IsClient()) {
			CastToClient()->LoadAccountFlags();
		} else if (GetOwner() && GetOwner()->IsClient()) {
			GetOwner()->CastToClient()->LoadAccountFlags();
		} 
		if (who->IsClient()) {
			who->CastToClient()->LoadAccountFlags();
		} else if (who->GetOwner() && who->GetOwner()->IsClient()) {
			who->GetOwner()->CastToClient()->LoadAccountFlags();
		}
		if ((IsClient() && GetHeroicSTR() > 0) || (IsPetOwnerClient() && GetOwner()->GetHeroicSTR() > 0)) {
			if ((IsClient() || IsPetOwnerClient()) && (my_hit.damage_done > my_hit.original_damage)) {				
				int increase_percentage = ((static_cast<float>(my_hit.damage_done) / static_cast<float>(my_hit.original_damage)) - 1) * 100;
				if (GetOwner() && GetOwner()->IsClient() && GetOwner()->CastToClient()->GetAccountFlag("filter_hSTR") != "off") {
					if (GetOwner()->CastToClient()->GetAccountFlag("filter_hPets") != "off") {
						GetOwner()->Message(Chat::MyPet, "Your pet's strike was increased by %i (%i%%) by your Heroic Strength!", 
											my_hit.damage_done - my_hit.original_damage,
											increase_percentage);
					}
				} else if (IsClient() && CastToClient()->GetAccountFlag("filter_hSTR") != "off") {
					Message(Chat::YouHitOther, "Your strike was increased by %i (%i%%) by your Heroic Strength!", 
							my_hit.damage_done - my_hit.original_damage,
							increase_percentage);
				}
			}
		}
		
		if ((who->IsClient() && who->GetHeroicSTA() > 0) || (who->IsPetOwnerClient() && who->GetOwner()->GetHeroicSTA() > 0)) {
			if ((who->IsClient() || who->IsPetOwnerClient()) && (my_hit.original_damage > my_hit.damage_done)) {				
				int reduction_percentage = (1 - static_cast<float>(my_hit.damage_done) / static_cast<float>(my_hit.original_damage)) * 100;
				if (who->GetOwner() && who->GetOwner()->IsClient()  && who->GetOwner()->CastToClient()->GetAccountFlag("filter_hSTA") != "off") {
					if (who->GetOwner()->CastToClient()->GetAccountFlag("filter_hPets") != "off") {
						who->GetOwner()->Message(Chat::MyPet, "The damage to your pet was reduced by %i (%i%%) by your Heroic Stamina!", 
												my_hit.original_damage - my_hit.damage_done,
												reduction_percentage);
					}
				} else if (who->IsClient() && who->CastToClient()->GetAccountFlag("filter_hSTA") != "off") {
					who->Message(Chat::OtherHitYou,"The damage to you was reduced by %i (%i%%) by your Heroic Stamina!", 
								my_hit.original_damage - my_hit.damage_done,
								reduction_percentage);
				}
			}
		}			
	}

	// Make sure 'this' has not killed the target and 'this' is not dead (Damage shield ect).
	if (!GetTarget())
		return;
	if (HasDied())
		return;

	TryCastOnSkillUse(who, skill);

	if (HasSkillProcs()) {
		TrySkillProc(who, skill, ReuseTime * 1000);
	}
	if (my_hit.damage_done > 0 && HasSkillProcSuccess()) {
		TrySkillProc(who, skill, ReuseTime * 1000, true);
	}
}

// We should probably refactor this to take the struct not the packet
void Client::OPCombatAbility(const CombatAbility_Struct *ca_atk)
{	
	if (!GetTarget())
		return;
	// make sure were actually able to use such an attack. (Bards can throw while casting. ~Kayen confirmed on live 1/22)
	if ((spellend_timer.Enabled() && GetClass() != BARD) || IsFeared() || IsStunned() || IsMezzed() || DivineAura() || dead)
		return;

	// RoF2+ Tiger Claw is unlinked from who monk skills, if they ever do that for who classes there will need to be more checks here
	pTimerType timer = (ClientVersion() >= EQ::versions::ClientVersion::RoF2 && ca_atk->m_skill == EQ::skills::SkillTigerClaw) ? pTimerCombatAbility2 : pTimerCombatAbility;

	bool CanBypassSkillCheck = false;

	int ReuseTime = 0;
	int ClientHaste = GetHaste();
	int HasteMod = 0;

	int64 dmg = 0;
	int32 skill_reduction = GetSkillReuseTime(ca_atk->m_skill);

	if (ca_atk->m_skill == EQ::skills::SkillBash) { // SLAM - Bash without a shield equipped
		switch (GetRace())
		{
			case OGRE:
			case TROLL:
			case BARBARIAN:
				CanBypassSkillCheck = true;
			default:
				break;
		}
	}

	/* Check to see if actually have skill */
	if (!GetSkill(static_cast<EQ::skills::SkillType>(ca_atk->m_skill)) && !CanBypassSkillCheck) {
		Message(Chat::Red, "You must have learned this skill in order to use it.");
		return;
	}

	if (GetTarget()->GetID() != ca_atk->m_target || GetTarget() == this)
		return; // invalid packet.

	//Archery and Throwing are ranged skills that do all of their reuse checking independently
	if (ca_atk->m_skill != EQ::skills::SkillThrowing && ca_atk->m_skill != EQ::skills::SkillArchery) {
		if (!IsAttackAllowed(GetTarget()) || !CombatRange(GetTarget()))
			return;

		if (!p_timers.Expired(&database, timer, false)) {
			Message(Chat::Red, "Ability recovery time not yet met.");
			return;
		}

		if (ClientHaste >= 0)
			HasteMod = (10000 / (100 + ClientHaste)); //+100% haste = 2x as many attacks
		else
			HasteMod = (100 - ClientHaste); //-100% haste = 1/2 as many attacks
	}

	int32 ht = 0;
	
	// We dropped the magic number check entirely. Only archery\ranged checked for anything different anyway.
	switch (ca_atk->m_skill) {
		case EQ::skills::SkillArchery:
		case EQ::skills::SkillThrowing:
			if (ca_atk->m_atk == EQ::invslot::slotRange) {
				SetAttackTimer();
				bool successful_hit = (ca_atk->m_skill == EQ::skills::SkillArchery) ? RangedAttack(GetTarget()) : ThrowingAttack(GetTarget());
				if (CheckDoubleRangedAttack()) {
					successful_hit = successful_hit || (ca_atk->m_skill == EQ::skills::SkillArchery) ? RangedAttack(GetTarget(), true) : ThrowingAttack(GetTarget(), true);
				}

				// Pyrelight Custom Code
				// Multi-Attack via Heroic DEX
				if (IsClient() && GetHeroicDEX() > 0)  {
					int effective_hDEX = GetHeroicDEX() - zone->random.Int(1,500);
					int attack_count = 0;		
					while (effective_hDEX > 0 && successful_hit) {
						effective_hDEX -= zone->random.Int(100,500);	
						successful_hit = successful_hit || (ca_atk->m_skill == EQ::skills::SkillArchery) ? RangedAttack(GetTarget(), true) : ThrowingAttack(GetTarget(), true);
						attack_count++;
					}	

					if (attack_count) {			
						LoadAccountFlags();
						if (GetAccountFlag("filter_hDEX") != "off") {
							Message(Chat::NPCFlurry, 
									"Your Heroic Dexterity allows you to unleash a flurry of %u additional attack%s.", 
									attack_count, 
									(attack_count > 1 ? "s" : ""));
						}		
					}				
				}
			
			} else {
				Message(Chat::Red, "You must equip an appropriate weapon to use this ability.");
			}
			return; // These don't care about the remainder of this function
			case EQ::skills::SkillBash: {
				CheckIncreaseSkill(EQ::skills::SkillBash, GetTarget(), 10);
				DoAnim(animTailRake, 0, false);
				
				if (GetWeaponDamage(GetTarget(), GetInv().GetItem(EQ::invslot::slotSecondary)) <= 0 &&
					GetWeaponDamage(GetTarget(), GetInv().GetItem(EQ::invslot::slotShoulders)) <= 0)
					dmg = -5;
				else
					ht = dmg = GetBaseSkillDamage(EQ::skills::SkillBash, GetTarget());

				ReuseTime = BashReuseTime - 1 - skill_reduction;
				ReuseTime = (ReuseTime * HasteMod) / 100;
				DoSpecialAttackDamage(GetTarget(), EQ::skills::SkillBash, dmg, 0, ht, ReuseTime);
				break;
			}
			case EQ::skills::SkillKick: {
				CheckIncreaseSkill(EQ::skills::SkillKick, GetTarget(), 10);
				DoAnim(animKick, 0, false);

				if (GetWeaponDamage(GetTarget(), GetInv().GetItem(EQ::invslot::slotFeet)) <= 0)
					dmg = -5;
				else
					ht = dmg = GetBaseSkillDamage(EQ::skills::SkillKick, GetTarget());

				ReuseTime = KickReuseTime - 1 - skill_reduction;
				DoSpecialAttackDamage(GetTarget(), EQ::skills::SkillKick, dmg, 0, ht, ReuseTime);
				break;
			}
			case EQ::skills::SkillBackstab: {
				ReuseTime = BackstabReuseTime-1 - skill_reduction;
				TryBackstab(GetTarget(), ReuseTime);
				break;
			}
			case EQ::skills::SkillFrenzy: {
				CheckIncreaseSkill(EQ::skills::SkillFrenzy, GetTarget(), 10);
				int AtkRounds = 1;
				int32 max_dmg = GetInv().GetItem(EQ::invslot::slotPrimary)->GetItemWeaponDamage();
				DoAnim(anim1HWeapon, 0, false);

				int chance = GetLevel() * 2 + GetSkill(EQ::skills::SkillFrenzy);
				AtkRounds += (zone->random.Roll0(450) < chance) + (zone->random.Roll0(450) < chance);			

				ReuseTime = FrenzyReuseTime - 1 - skill_reduction;
				ReuseTime = (ReuseTime * HasteMod) / 100;

				auto primary_in_use = GetInv().GetItem(EQ::invslot::slotPrimary);
				if (primary_in_use && GetWeaponDamage(GetTarget(), primary_in_use) <= 0) {
					max_dmg = DMG_INVULNERABLE;
				}

				while (AtkRounds > 0) {
					if (GetTarget())
						DoSpecialAttackDamage(GetTarget(), EQ::skills::SkillFrenzy, max_dmg, 0, max_dmg, ReuseTime);
					AtkRounds--;
				}		
				break;
			}
			case EQ::skills::SkillFlyingKick:
			case EQ::skills::SkillDragonPunch:
			case EQ::skills::SkillEagleStrike:
			case EQ::skills::SkillTigerClaw:
			case EQ::skills::SkillRoundKick:				
			default:
				return;
	}

	ReuseTime = (ReuseTime * HasteMod) / 100;
	if (ReuseTime > 0) {
		p_timers.Start(timer, ReuseTime);
	}
}


//returns the reuse time in sec for the special attack used.
int Mob::MonkSpecialAttack(Mob *who, uint8 unchecked_type)
{
	if (!who)
		return 0;

	int64 ndamage = 0;
	int32 max_dmg = 0;
	int32 min_dmg = 0;
	int reuse = 0;
	EQ::skills::SkillType skill_type; // to avoid casting... even though it "would work"
	uint8 itemslot = EQ::invslot::slotFeet;
	if (IsNPC()) {
		auto *npc = CastToNPC();
		min_dmg = npc->GetMinDamage();
	}

	switch (unchecked_type) {
	case EQ::skills::SkillFlyingKick:
		skill_type = EQ::skills::SkillFlyingKick;
		max_dmg = GetBaseSkillDamage(skill_type);
		min_dmg = 0; // revamped FK formula is missing the min mod?
		DoAnim(animFlyingKick, 0, false);
		reuse = FlyingKickReuseTime;
		break;
	case EQ::skills::SkillDragonPunch:
		skill_type = EQ::skills::SkillDragonPunch;
		max_dmg = GetBaseSkillDamage(skill_type);
		itemslot = EQ::invslot::slotHands;
		DoAnim(animTailRake, 0, false);
		reuse = TailRakeReuseTime;
		break;
	case EQ::skills::SkillEagleStrike:
		skill_type = EQ::skills::SkillEagleStrike;
		max_dmg = GetBaseSkillDamage(skill_type);
		itemslot = EQ::invslot::slotHands;
		DoAnim(animEagleStrike, 0, false);
		reuse = EagleStrikeReuseTime;
		break;
	case EQ::skills::SkillTigerClaw:
		skill_type = EQ::skills::SkillTigerClaw;
		max_dmg = GetBaseSkillDamage(skill_type);
		itemslot = EQ::invslot::slotHands;
		DoAnim(animTigerClaw, 0, false);
		reuse = TigerClawReuseTime;
		break;
	case EQ::skills::SkillRoundKick:
		skill_type = EQ::skills::SkillRoundKick;
		max_dmg = GetBaseSkillDamage(skill_type);
		DoAnim(animRoundKick, 0, false);
		reuse = RoundKickReuseTime;
		break;
	case EQ::skills::SkillKick:
		skill_type = EQ::skills::SkillKick;
		max_dmg = GetBaseSkillDamage(skill_type);
		DoAnim(animKick, 0, false);
		reuse = KickReuseTime;
		break;
	default:
		Log(Logs::Detail, Logs::Attack, "Invalid special attack type %d attempted", unchecked_type);
		return (1000); /* nice long delay for them, the caller depends on this! */
	}

	if (IsClient()) {
		if (GetWeaponDamage(who, CastToClient()->GetInv().GetItem(itemslot)) <= 0) {
			max_dmg = DMG_INVULNERABLE;
		}
	} else {
		if (GetWeaponDamage(who, (const EQ::ItemData *)nullptr) <= 0) {
			max_dmg = DMG_INVULNERABLE;
		}
	}

	int32 ht = 0;
	if (max_dmg > 0)
		ht = max_dmg;

	DoSpecialAttackDamage(who, skill_type, max_dmg, min_dmg, ht, reuse);

	return reuse;
}

void Mob::TryBackstab(Mob *who, int ReuseTime) {
	if(!who)
		return;

	bool bIsBehind = false;
	bool bCanFrontalBS = false;

	//make sure we have a proper weapon if we are a client.
	if(IsClient()) {
		const EQ::ItemInstance *wpn = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
		if (!wpn || (wpn->GetItem()->ItemType != EQ::item::ItemType1HPiercing)){
			MessageString(Chat::Red, BACKSTAB_WEAPON);
			return;
		}
	}

	//Live AA - Triple Backstab
	int tripleChance = itembonuses.TripleBackstab + spellbonuses.TripleBackstab + aabonuses.TripleBackstab;

	if (BehindMob(who, GetX(), GetY()))
		bIsBehind = true;

	else {
		//Live AA - Seized Opportunity
		int FrontalBSChance = itembonuses.FrontalBackstabChance + spellbonuses.FrontalBackstabChance + aabonuses.FrontalBackstabChance;

		if (FrontalBSChance && zone->random.Roll(FrontalBSChance))
			bCanFrontalBS = true;
	}

	if (bIsBehind || bCanFrontalBS || (IsNPC() && CanFacestab())) { // Player is behind who OR can do Frontal Backstab
		if (bCanFrontalBS && IsClient()) // I don't think there is any message ...
			CastToClient()->Message(Chat::White,"Your fierce attack is executed with such grace, your target did not see it coming!");

		RogueBackstab(who,false,ReuseTime);
		if (level > 54) {
			// TODO: 55-59 doesn't appear to match just checking double attack, 60+ does though
			if(IsClient() && CastToClient()->CheckDoubleAttack())
			{
				if(who->GetHP() > 0)
					RogueBackstab(who,false,ReuseTime);

				if (tripleChance && who->GetHP() > 0 && zone->random.Roll(tripleChance))
					RogueBackstab(who,false,ReuseTime);
			}
		}

		if(IsClient())
			CastToClient()->CheckIncreaseSkill(EQ::skills::SkillBackstab, who, 10);

	}
	//Live AA - Chaotic Backstab
	else if(aabonuses.FrontalBackstabMinDmg || itembonuses.FrontalBackstabMinDmg || spellbonuses.FrontalBackstabMinDmg) {
		m_specialattacks = eSpecialAttacks::ChaoticStab;

		//we can stab from any angle, we do min damage though.
		// chaotic backstab can't double etc Seized can, but that's because it's a chance to do normal BS
		// Live actually added SPA 473 which grants chance to double here when they revamped chaotic/seized
		RogueBackstab(who, true, ReuseTime);
		if(IsClient())
			CastToClient()->CheckIncreaseSkill(EQ::skills::SkillBackstab, who, 10);
		m_specialattacks = eSpecialAttacks::None;

		int double_bs_front = aabonuses.Double_Backstab_Front + itembonuses.Double_Backstab_Front + spellbonuses.Double_Backstab_Front;
		if (double_bs_front && who->GetHP() > 0 && zone->random.Roll(double_bs_front))
			RogueBackstab(who, false, ReuseTime);
	}
	else { //We do a single regular attack if we attack from the front without chaotic stab
		Attack(who, EQ::invslot::slotPrimary);
	}
}

//heko: backstab
void Mob::RogueBackstab(Mob* who, bool min_damage, int ReuseTime)
{
	if (!who)
		return;

	int64 hate = 0;

	// make sure we can hit (bane, magical, etc)
	if (IsClient()) {
		const EQ::ItemInstance *wpn = CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary);
		if (!GetWeaponDamage(who, wpn))
			return;
	} else if (!GetWeaponDamage(who, (const EQ::ItemData*)nullptr)){
		return;
	}

	int base_damage = GetBaseSkillDamage(EQ::skills::SkillBackstab, who);
	hate = base_damage;

	DoSpecialAttackDamage(who, EQ::skills::SkillBackstab, base_damage, 0, hate, ReuseTime);
	DoAnim(anim1HPiercing, 0, false);
}

// assassinate [No longer used for regular assassinate 6-29-14]
void Mob::RogueAssassinate(Mob* who)
{
	//can you dodge, parry, etc.. an assassinate??
	//if so, use DoSpecialAttackDamage(who, BACKSTAB, 32000); instead
	if (GetWeaponDamage(who, IsClient() ? CastToClient()->GetInv().GetItem(EQ::invslot::slotPrimary) : (const EQ::ItemInstance*)nullptr) > 0){
		who->Damage(this, 32000, SPELL_UNKNOWN, EQ::skills::SkillBackstab);
	}else{
		who->Damage(this, -5, SPELL_UNKNOWN, EQ::skills::SkillBackstab);
	}
	DoAnim(anim1HPiercing, 0, false);	//piercing animation
}

bool Client::RangedAttack(Mob* who, bool CanDoubleAttack) {
	//conditions to use an attack checked before we are called
	if (!who)
		return false;
	else if (who == this)
		return false;
	//make sure the attack and ranged timers are up
	//if the ranged timer is disabled, then they have no ranged weapon and shouldent be attacking anyhow
	if(!CanDoubleAttack && ((attack_timer.Enabled() && !attack_timer.Check(false)) || (ranged_timer.Enabled() && !ranged_timer.Check()))) {
		LogCombat("Throwing attack canceled. Timer not up. Attack [{}], ranged [{}]", attack_timer.GetRemainingTime(), ranged_timer.GetRemainingTime());
		// The server and client timers are not exact matches currently, so this would spam too often if enabled
		//Message(0, "Error: Timer not up. Attack %d, ranged %d", attack_timer.GetRemainingTime(), ranged_timer.GetRemainingTime());
		return false;
	}
	const EQ::ItemInstance* RangeWeapon = m_inv[EQ::invslot::slotRange];

	//locate ammo
	int ammo_slot = EQ::invslot::slotAmmo;
	const EQ::ItemInstance* Ammo = m_inv[EQ::invslot::slotAmmo];

	if (!RangeWeapon || !RangeWeapon->IsClassCommon()) {
		LogCombat("Ranged attack canceled. Missing or invalid ranged weapon ([{}]) in slot [{}]", GetItemIDAt(EQ::invslot::slotRange), EQ::invslot::slotRange);
		Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have no bow!", GetItemIDAt(EQ::invslot::slotRange));
		return false;
	}
	if (!Ammo || !Ammo->IsClassCommon()) {
		LogCombat("Ranged attack canceled. Missing or invalid ammo item ([{}]) in slot [{}]", GetItemIDAt(EQ::invslot::slotAmmo), EQ::invslot::slotAmmo);
		Message(0, "Error: Ammo: GetItem(%i)==0, you have no ammo!", GetItemIDAt(EQ::invslot::slotAmmo));
		return false;
	}

	const EQ::ItemData* RangeItem = RangeWeapon->GetItem();
	const EQ::ItemData* AmmoItem = Ammo->GetItem();

	if (RangeItem->ItemType != EQ::item::ItemTypeBow) {
		LogCombat("Ranged attack canceled. Ranged item is not a bow. type [{}]", RangeItem->ItemType);
		Message(0, "Error: Rangeweapon: Item %d is not a bow.", RangeWeapon->GetID());
		return false;
	}
	if (AmmoItem->ItemType != EQ::item::ItemTypeArrow) {
		LogCombat("Ranged attack canceled. Ammo item is not an arrow. type [{}]", AmmoItem->ItemType);
		Message(0, "Error: Ammo: type %d != %d, you have the wrong type of ammo!", AmmoItem->ItemType, EQ::item::ItemTypeArrow);
		return false;
	}

	LogCombat("Shooting [{}] with bow [{}] ([{}]) and arrow [{}] ([{}])", who->GetName(), RangeItem->Name, RangeItem->ID, AmmoItem->Name, AmmoItem->ID);

	//look for ammo in inventory if we only have 1 left...
	if (Ammo->GetCharges() == 1) {
		//first look for quivers
		bool found = false;
		for (int r = EQ::invslot::GENERAL_BEGIN; r <= EQ::invslot::GENERAL_END; r++) {
			const EQ::ItemInstance *pi = m_inv[r];
			if (pi == nullptr || !pi->IsClassBag())
				continue;
			const EQ::ItemData* bagitem = pi->GetItem();
			if (!bagitem || bagitem->BagType != EQ::item::BagTypeQuiver)
				continue;

			//we found a quiver, look for the ammo in it
			for (int i = 0; i < bagitem->BagSlots; i++) {
				const EQ::ItemInstance* baginst = pi->GetItem(i);
				if (!baginst) {
					continue;
				}

				if (baginst->GetID() == Ammo->GetID()) {
					//we found it... use this stack
					//the item wont change, but the instance does
					Ammo = baginst;
					ammo_slot = EQ::InventoryProfile::CalcSlotId(r, i);
					found = true;
					LogCombat("Using ammo from quiver stack at slot [{}]. [{}] in stack", ammo_slot, Ammo->GetCharges());
					break;
				}
			}
			if(found)
				break;
		}

		if(!found) {
			//if we dont find a quiver, look through our inventory again
			//not caring if the thing is a quiver.
			int32 aslot = m_inv.HasItem(AmmoItem->ID, 1, invWherePersonal);
			if (aslot != INVALID_INDEX) {
				ammo_slot = aslot;
				Ammo = m_inv[aslot];
				LogCombat("Using ammo from inventory stack at slot [{}]. [{}] in stack", ammo_slot, Ammo->GetCharges());
			}
		}
	}

	float range = RangeItem->Range + AmmoItem->Range + GetRangeDistTargetSizeMod(GetTarget());
	LogCombat("Calculated bow range to be [{}]", range);
	range *= range;
	if (float dist = DistanceSquared(m_Position, who->GetPosition()); dist > range) {
		LogCombat("Ranged attack out of range client should catch this. ([{}] > [{}]).\n", dist, range);
		MessageString(Chat::Red,TARGET_OUT_OF_RANGE);//Client enforces range and sends the message, this is a backup just incase.
		return false;
	}
	else if (dist < (RuleI(Combat, MinRangedAttackDist)*RuleI(Combat, MinRangedAttackDist))){
		MessageString(Chat::Yellow,RANGED_TOO_CLOSE);//Client enforces range and sends the message, this is a backup just incase.
		return false;
	}

	if (!IsAttackAllowed(who) ||
		IsCasting() ||
		IsSitting() ||
		(DivineAura() && !GetGM()) ||
		IsStunned() ||
		IsFeared() ||
		IsMezzed() ||
		(GetAppearance() == eaDead)){
		return false;
	}

	//Shoots projectile and/or applies the archery damage
	DoArcheryAttackDmg(who, RangeWeapon, Ammo,0,0,0,0,0,0, AmmoItem, ammo_slot);

	//EndlessQuiver AA base1 = 100% Chance to avoid consumption arrow.
	int ChanceAvoidConsume = aabonuses.ConsumeProjectile + itembonuses.ConsumeProjectile + spellbonuses.ConsumeProjectile;

	// Consume Ammo, unless Ammo Consumption is disabled or player has Endless Quiver
	bool consumes_ammo = RuleB(Combat, ArcheryConsumesAmmo);
	if (
		consumes_ammo &&
		(
			RangeItem->ExpendableArrow ||
			!ChanceAvoidConsume ||
			(ChanceAvoidConsume < 100 && zone->random.Int(0,99) > ChanceAvoidConsume)
		)
	) {
		DeleteItemInInventory(ammo_slot, 1, true);
		LogCombat("Consumed Archery Ammo from slot {}.", ammo_slot);
	} else if (!consumes_ammo) {
		LogCombat("Archery Ammo Consumption is disabled.");
	} else {
		LogCombat("Endless Quiver prevented Ammo Consumption.");
	}

	CheckIncreaseSkill(EQ::skills::SkillArchery, GetTarget(), -15);
	CommonBreakInvisibleFromCombat();
	return true;
}

void Mob::DoArcheryAttackDmg(Mob *who, const EQ::ItemInstance *RangeWeapon, const EQ::ItemInstance *Ammo,
							int32 weapon_damage, int16 chance_mod, int16 focus, int ReuseTime, uint32 range_id,
							uint32 ammo_id, const EQ::ItemData *AmmoItem, int AmmoSlot, float speed, bool DisableProcs)
{
	if ((who == nullptr ||
			((IsClient() && CastToClient()->dead) || (who->IsClient() && who->CastToClient()->dead)) ||
			HasDied() || (!IsAttackAllowed(who)) || (who->GetInvul() || who->GetSpecialAbility(IMMUNE_MELEE)))) {
		return;
	}

	const EQ::ItemInstance *_RangeWeapon = nullptr;
	const EQ::ItemInstance *_Ammo = nullptr;
	const EQ::ItemData *last_ammo_used = nullptr;

	/*
	If LaunchProjectile is false this function will do archery damage on target,
	otherwise it will shoot the projectile at the target, once the projectile hits target
	this function is then run again to do the damage portion
	*/
	bool LaunchProjectile = false;

	if (RuleB(Combat, ProjectileDmgOnImpact)) {
		if (AmmoItem) { // won't be null when we are firing the arrow
			LaunchProjectile = true;
		} else {
			/*
			Item sync check on projectile landing.
			Weapon damage is already calculated so this only affects procs!
			Ammo proc check will use database to find proc if you used up your last ammo.
			If you change range item mid projectile flight, you loose your chance to proc from bow (Deal
			with it!).
			*/

			if (!RangeWeapon && !Ammo && range_id && ammo_id) {
				if (IsClient()) {
					_RangeWeapon = CastToClient()->m_inv[EQ::invslot::slotRange];
					if (_RangeWeapon && _RangeWeapon->GetItem() &&
					    _RangeWeapon->GetItem()->ID == range_id)
						RangeWeapon = _RangeWeapon;

					_Ammo = CastToClient()->m_inv[AmmoSlot];
					if (_Ammo && _Ammo->GetItem() && _Ammo->GetItem()->ID == ammo_id)
						Ammo = _Ammo;
					else
						last_ammo_used = database.GetItem(ammo_id);
				}
			}
		}
	} else if (AmmoItem) {
		SendItemAnimation(who, AmmoItem, EQ::skills::SkillArchery);
	}

	LogCombat("Ranged attack hit [{}]", who->GetName());

	int64 hate = 0;
	int64 TotalDmg = 0;
	int WDmg = 0;
	int ADmg = 0;
	if (!weapon_damage) {
		WDmg = GetWeaponDamage(who, RangeWeapon);
		ADmg = GetWeaponDamage(who, Ammo);
	} else {
		WDmg = weapon_damage;
	}

	if (LaunchProjectile) { // 1: Shoot the Projectile once we calculate weapon damage.
		TryProjectileAttack(who, AmmoItem, EQ::skills::SkillArchery, (WDmg + ADmg), RangeWeapon,
							Ammo, AmmoSlot, speed, DisableProcs);
		return;
	}

	if (focus) {
		WDmg += WDmg * focus / 100;
	}

	DamageHitInfo my_hit {};
	if (WDmg > 0 || ADmg > 0) {
		if (WDmg < 0) {
			WDmg = 0;
		}

		if (ADmg < 0) {
			ADmg = 0;
		}

		int MaxDmg = WDmg + ADmg;
		hate = ((WDmg + ADmg));

		if (RuleB(Combat, ProjectileDmgOnImpact)) {
			LogCombat("Bow and Arrow DMG [{}], Max Damage [{}]", WDmg, MaxDmg);
		}
		else {
			LogCombat("Bow DMG [{}], Arrow DMG [{}], Max Damage [{}]", WDmg, ADmg, MaxDmg);
		}

		if (MaxDmg == 0) {
			MaxDmg = 1;
		}
		
		my_hit.base_damage = MaxDmg;
		my_hit.min_damage = 0;
		my_hit.damage_done = 1;

		my_hit.skill = EQ::skills::SkillArchery;
		my_hit.offense = offense(my_hit.skill);
		my_hit.tohit = GetTotalToHit(my_hit.skill, chance_mod);
		my_hit.hand = EQ::invslot::slotRange;

		DoAttack(who, my_hit);
	} else {
		my_hit.damage_done = DMG_INVULNERABLE;
	}

	if (IsClient() && !CastToClient()->GetFeigned()) {
		who->AddToHateList(this, hate, 0);
	}

	who->Damage(this, my_hit.damage_done, SPELL_UNKNOWN, EQ::skills::SkillArchery);

	//Pyrelight Custom Code - Send info about the hSTA/hSTR damage modification to clients
	// This is a boilerplate with dead code paths.
	if (my_hit.damage_done > 0 && my_hit.original_damage > 0) {
		if (IsClient()) {
			CastToClient()->LoadAccountFlags();
		} else if (GetOwner() && GetOwner()->IsClient()) {
			GetOwner()->CastToClient()->LoadAccountFlags();
		} 
		if (who->IsClient()) {
			who->CastToClient()->LoadAccountFlags();
		} else if (who->GetOwner() && who->GetOwner()->IsClient()) {
			who->GetOwner()->CastToClient()->LoadAccountFlags();
		}
		if ((IsClient() && GetHeroicSTR() > 0) || (IsPetOwnerClient() && GetOwner()->GetHeroicSTR() > 0)) {
			if ((IsClient() || IsPetOwnerClient()) && (my_hit.damage_done > my_hit.original_damage)) {				
				int increase_percentage = ((static_cast<float>(my_hit.damage_done) / static_cast<float>(my_hit.original_damage)) - 1) * 100;
				if (GetOwner() && GetOwner()->IsClient() && GetOwner()->CastToClient()->GetAccountFlag("filter_hSTR") != "off") {
					if (GetOwner()->CastToClient()->GetAccountFlag("filter_hPets") != "off") {
						GetOwner()->Message(Chat::MyPet, "Your pet's strike was increased by %i (%i%%) by your Heroic Strength!", 
											my_hit.damage_done - my_hit.original_damage,
											increase_percentage);
					}
				} else if (IsClient() && CastToClient()->GetAccountFlag("filter_hSTR") != "off") {
					Message(Chat::YouHitOther, "Your strike was increased by %i (%i%%) by your Heroic Strength!", 
							my_hit.damage_done - my_hit.original_damage,
							increase_percentage);
				}
			}
		}
		
		if ((who->IsClient() && who->GetHeroicSTA() > 0) || (who->IsPetOwnerClient() && who->GetOwner()->GetHeroicSTA() > 0)) {
			if ((who->IsClient() || who->IsPetOwnerClient()) && (my_hit.original_damage > my_hit.damage_done)) {				
				int reduction_percentage = (1 - static_cast<float>(my_hit.damage_done) / static_cast<float>(my_hit.original_damage)) * 100;
				if (who->GetOwner() && who->GetOwner()->IsClient()  && who->GetOwner()->CastToClient()->GetAccountFlag("filter_hSTA") != "off") {
					if (who->GetOwner()->CastToClient()->GetAccountFlag("filter_hPets") != "off") {
						who->GetOwner()->Message(Chat::MyPet, "The damage to your pet was reduced by %i (%i%%) by your Heroic Stamina!", 
												my_hit.original_damage - my_hit.damage_done,
												reduction_percentage);
					}
				} else if (who->IsClient() && who->CastToClient()->GetAccountFlag("filter_hSTA") != "off") {
					who->Message(Chat::OtherHitYou,"The damage to you was reduced by %i (%i%%) by your Heroic Stamina!", 
								my_hit.original_damage - my_hit.damage_done,
								reduction_percentage);
				}
			}	
		}		
	}

	if (!DisableProcs) {
		// Weapon Proc
		if (RangeWeapon && who && !who->HasDied()) {
			TryCombatProcs(RangeWeapon, who, EQ::invslot::slotRange);
		}

		// Ammo Proc, do not try spell procs if from ammo.
		if (last_ammo_used) {
			TryWeaponProc(nullptr, last_ammo_used, who, EQ::invslot::slotRange);
		}
		else if (Ammo && who && !who->HasDied()) {
			TryWeaponProc(Ammo, Ammo->GetItem(), who, EQ::invslot::slotRange);
		}
	}

	TryCastOnSkillUse(who, EQ::skills::SkillArchery);

	if (!DisableProcs) {
		// Skill Proc Attempt
		if (HasSkillProcs() && who && !who->HasDied()) {
			if (ReuseTime) {
				TrySkillProc(who, EQ::skills::SkillArchery, ReuseTime);
			}
			else {
				TrySkillProc(who, EQ::skills::SkillArchery, 0, false, EQ::invslot::slotRange);
			}
		}

		// Skill Proc Success ... can proc off hits OR misses
		if (HasSkillProcSuccess() && who && !who->HasDied()) {
			if (ReuseTime) {
				TrySkillProc(who, EQ::skills::SkillArchery, ReuseTime, true);
			}
			else {
				TrySkillProc(who, EQ::skills::SkillArchery, 0, true, EQ::invslot::slotRange);
			}
		}
	}
}

bool Mob::TryProjectileAttack(Mob *who, const EQ::ItemData *item, EQ::skills::SkillType skillInUse,
			      uint64 weapon_dmg, const EQ::ItemInstance *RangeWeapon,
			      const EQ::ItemInstance *Ammo, int AmmoSlot, float speed, bool DisableProcs)
{
	if (!who)
		return false;

	int slot = -1;

	// Make sure there is an avialable slot.
	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
		if (ProjectileAtk[i].target_id == 0) {
			slot = i;
			break;
		}
	}

	if (slot < 0)
		return false;

	float distance_mod = 0.0f;
	float distance = who->CalculateDistance(GetX(), GetY(), GetZ());

	/*
	New Distance Mod constant (7/25/21 update), modifier is needed to adjust slower speeds to have correct impact times at short distances.
	We use archery 4.0 speed as a baseline for the forumla.  At speed 1.5 at 50 pct distance mod is needed, where as speed 4.0 there is no modifer.
	Therefore, we derive out our modifer as follows. distance_mod = (speed - 4) * ((50 - 0)/(1.5-4)). The ratio there is -20.0f. distance_mod = (speed - 4) * -20.0f
	For distances >125 we use different modifier, this was all meticulously tested by eye to get the best possible outcome for projectile impact times. Not perfect though.
	*/

	if (distance <= 125.0f) {
		if (speed != 4.0f) { //Standard functions will always be 4.0f for archery.
			distance_mod = (speed - 4.0f) * -20.0f;
			distance += distance * distance_mod / 100.0f;
		}
	}
	else if (distance > 125.0f && distance <= 200.0f)
		distance = 3.14f * (distance / 2.0f); //Get distance of arc to better reflect projectile path length

	else if (distance > 200.0f) {
		distance = distance * 1.30f; //Add 30% to base distance if over 200 range to tighten up hit timing.
		distance = 3.14f * (distance / 2.0f); //Get distance of arc to better reflect projectile path length
	}

	float hit = 1200.0f + (10 * distance / speed);

	ProjectileAtk[slot].increment = 1;
	ProjectileAtk[slot].hit_increment = static_cast<uint16>(hit); // This projected hit time if target does NOT MOVE
	ProjectileAtk[slot].target_id = who->GetID();
	ProjectileAtk[slot].wpn_dmg = weapon_dmg;
	ProjectileAtk[slot].origin_x = GetX();
	ProjectileAtk[slot].origin_y = GetY();
	ProjectileAtk[slot].origin_z = GetZ();

	if (RangeWeapon && RangeWeapon->GetItem())
		ProjectileAtk[slot].ranged_id = RangeWeapon->GetItem()->ID;

	if (Ammo && Ammo->GetItem())
		ProjectileAtk[slot].ammo_id = Ammo->GetItem()->ID;

	ProjectileAtk[slot].ammo_slot = AmmoSlot;
	ProjectileAtk[slot].skill = skillInUse;
	ProjectileAtk[slot].speed_mod = speed;
	ProjectileAtk[slot].disable_procs = DisableProcs;

	SetProjectileAttack(true);

	if (item)
		SendItemAnimation(who, item, skillInUse, speed);

	return true;
}

void Mob::ProjectileAttack()
{
	if (!HasProjectileAttack())
		return;

	Mob *target = nullptr;
	bool disable = true;

	for (int i = 0; i < MAX_SPELL_PROJECTILE; i++) {
		if (ProjectileAtk[i].increment == 0)
			continue;

		disable = false;
		Mob *target = entity_list.GetMobID(ProjectileAtk[i].target_id);

		if (target && target->IsMoving()) {
			/*
			Only recalculate hit increment if target is moving.
			Due to frequency that we need to check increment the targets position variables may not be
			updated even if moving. Do a simple check before calculating distance.
			*/
			if (ProjectileAtk[i].tlast_x != target->GetX() || ProjectileAtk[i].tlast_y != target->GetY()) {

				ProjectileAtk[i].tlast_x = target->GetX();
				ProjectileAtk[i].tlast_y = target->GetY();

				//Recalculate from the original location the projectile was fired in relation to the current targets location.
				float distance = target->CalculateDistance(ProjectileAtk[i].origin_x, ProjectileAtk[i].origin_y, ProjectileAtk[i].origin_z);
				float distance_mod = 0.0f;

				if (distance <= 125.0f) {
					distance_mod = (ProjectileAtk[i].speed_mod - 4.0f) * -20.0f;
					distance += distance * distance_mod / 100.0f;
				}
				else if (distance > 125.0f && distance <= 200.0f)
					distance = 3.14f * (distance / 2.0f); //Get distance of arc to better reflect projectile path length

				else if (distance > 200.0f) {
					distance = distance * 1.30f; //Add 30% to base distance if over 200 range to tighten up hit timing.
					distance = 3.14f * (distance / 2.0f); //Get distance of arc to better reflect projectile path length
				}

				float hit = 1200.0f + (10 * distance / ProjectileAtk[i].speed_mod);

				ProjectileAtk[i].hit_increment = static_cast<uint16>(hit);
			}
		}

		// Check if we hit.
		if (ProjectileAtk[i].hit_increment <= ProjectileAtk[i].increment) {
			if (target) {
				if (IsNPC()) {
					if (ProjectileAtk[i].skill == EQ::skills::SkillConjuration) {
						if (IsValidSpell(ProjectileAtk[i].wpn_dmg))
							SpellOnTarget(ProjectileAtk[i].wpn_dmg, target, false, true,
								      spells[ProjectileAtk[i].wpn_dmg].resist_difficulty,
								      true);
					} else {
						CastToNPC()->DoRangedAttackDmg(
						    target, false, ProjectileAtk[i].wpn_dmg, 0,
						    static_cast<EQ::skills::SkillType>(ProjectileAtk[i].skill));
					}
				} else {
					if (ProjectileAtk[i].skill == EQ::skills::SkillArchery)
						DoArcheryAttackDmg(target, nullptr, nullptr, ProjectileAtk[i].wpn_dmg,
								   0, 0, 0, ProjectileAtk[i].ranged_id,
								   ProjectileAtk[i].ammo_id, nullptr,
								   ProjectileAtk[i].ammo_slot, 4.0f, ProjectileAtk[i].disable_procs);
					else if (ProjectileAtk[i].skill == EQ::skills::SkillThrowing)
						DoThrowingAttackDmg(target, nullptr, nullptr, ProjectileAtk[i].wpn_dmg,
								    0, 0, 0, ProjectileAtk[i].ranged_id,
								    ProjectileAtk[i].ammo_slot, 4.0f, ProjectileAtk[i].disable_procs);
					else if (ProjectileAtk[i].skill == EQ::skills::SkillConjuration &&
						 IsValidSpell(ProjectileAtk[i].wpn_dmg))
						SpellOnTarget(ProjectileAtk[i].wpn_dmg, target, false, true,
							      spells[ProjectileAtk[i].wpn_dmg].resist_difficulty, true);
				}
			}

			ProjectileAtk[i].increment = 0;
			ProjectileAtk[i].target_id = 0;
			ProjectileAtk[i].wpn_dmg = 0;
			ProjectileAtk[i].origin_x = 0.0f;
			ProjectileAtk[i].origin_y = 0.0f;
			ProjectileAtk[i].origin_z = 0.0f;
			ProjectileAtk[i].tlast_x = 0.0f;
			ProjectileAtk[i].tlast_y = 0.0f;
			ProjectileAtk[i].ranged_id = 0;
			ProjectileAtk[i].ammo_id = 0;
			ProjectileAtk[i].ammo_slot = 0;
			ProjectileAtk[i].skill = 0;
			ProjectileAtk[i].speed_mod = 0.0f;
		} else {
			ProjectileAtk[i].increment += 1000 * frame_time;
		}
	}

	if (disable)
		SetProjectileAttack(false);
}

float Mob::GetRangeDistTargetSizeMod(Mob* who)
{
	/*
	Range is enforced client side, therefore these numbers do not need to be 100% accurate just close enough to
	prevent any exploitation. The range mod changes in some situations depending on if size is from spawn or from SendIllusionPacket changes.
	At present time only calculate from spawn (it is no consistent what happens to the calc when changing it after spawn).
	*/
	if (!who)
		return 0.0f;

	float tsize = who->GetSize();

	if (GetSize() > tsize)
		tsize = GetSize();

	float mod = 0.0f;
	/*These are correct numbers if mob size is changed via #size (Don't know why it matters but it does)
	if (tsize < 7)
		mod = 16.0f;
	else if (tsize >=7 && tsize <= 20)
		mod = 16.0f + (0.6f * (tsize - 6.0f));
	else if (tsize >=20 && tsize <= 60)
		mod = 25.0f + (1.25f * (tsize - 20.0f));
	else
		mod = 75.0f;
	*/

	if (tsize < 10)
		mod = 18.0f;
	else if (tsize >=10 && tsize < 15)
		mod = 20.0f + (4.0f * (tsize - 10.0f));
	else if (tsize >=15 && tsize <= 20)
		mod = 42.0f + (5.8f * (tsize - 15.0f));
	else
		mod = 75.0f;

	return (mod + 2.0f); //Add 2.0f as buffer to prevent any chance of failures, client enforce range check regardless.
}

void NPC::RangedAttack(Mob *who)
{
	if (!who)
		return;
	// make sure the attack and ranged timers are up
	// if the ranged timer is disabled, then they have no ranged weapon and shouldent be attacking anyhow
	if ((attack_timer.Enabled() && !attack_timer.Check(false)) ||
	    (ranged_timer.Enabled() && !ranged_timer.Check())) {
		LogCombat("Archery canceled. Timer not up. Attack [{}], ranged [{}]", attack_timer.GetRemainingTime(),
			  ranged_timer.GetRemainingTime());
		return;
	}

	if (!HasBowAndArrowEquipped() && !GetSpecialAbility(SPECATK_RANGED_ATK))
		return;

	if (!CheckLosFN(who))
		return;

	int attacks = 1;
	float min_range = static_cast<float>(RuleI(Combat, MinRangedAttackDist));
	float max_range = 250.0f; // needs to be longer than 200(most spells)

	if (GetSpecialAbility(SPECATK_RANGED_ATK)) {
		int temp_attacks = GetSpecialAbilityParam(SPECATK_RANGED_ATK, 0);
		attacks = temp_attacks > 0 ? temp_attacks : 1;

		int temp_min_range = GetSpecialAbilityParam(SPECATK_RANGED_ATK, 4); // Min Range of NPC attack
		int temp_max_range = GetSpecialAbilityParam(SPECATK_RANGED_ATK, 1); // Max Range of NPC attack
		if (temp_max_range)
			max_range = static_cast<float>(temp_max_range);
		if (temp_min_range)
			min_range = static_cast<float>(temp_min_range);
	}

	max_range *= max_range;
	min_range *= min_range;

	for (int i = 0; i < attacks; ++i) {
		if (DistanceSquared(m_Position, who->GetPosition()) > max_range)
			return;
		else if (DistanceSquared(m_Position, who->GetPosition()) < min_range)
			return;

		if (!who || !IsAttackAllowed(who) || IsCasting() || DivineAura() || IsStunned() || IsFeared() ||
		    IsMezzed() || (GetAppearance() == eaDead)) {
			return;
		}

		FaceTarget(who);

		DoRangedAttackDmg(who);

		CommonBreakInvisibleFromCombat();
	}
}

void NPC::DoRangedAttackDmg(Mob* who, bool Launch, int16 damage_mod, int16 chance_mod, EQ::skills::SkillType skill, float speed, const char *IDFile)
{
	if ((who == nullptr ||
		(who->HasDied())) ||
		HasDied() ||
		(!IsAttackAllowed(who)) ||
		(who->GetInvul() ||
		who->GetSpecialAbility(IMMUNE_MELEE)))
	{
		return;
	}

	EQ::skills::SkillType skillInUse = static_cast<EQ::skills::SkillType>(GetRangedSkill());

	if (skill != skillInUse)
		skillInUse = skill;

	if (Launch)
	{
		const char *ammo = "IT10";

		if (IDFile != nullptr)
			ammo = IDFile;
		else if (GetAmmoIDfile())
			ammo = GetAmmoIDfile();

		ProjectileAnimation(who, 0,false,speed,0,0,0,ammo,skillInUse);

		if (RuleB(Combat, ProjectileDmgOnImpact))
		{
			TryProjectileAttack(who, nullptr, skillInUse, damage_mod, nullptr, nullptr, 0, speed);
			return;
		}
	}

	if (!chance_mod)
		chance_mod = GetSpecialAbilityParam(SPECATK_RANGED_ATK, 2);

	int TotalDmg = 0;
	int MaxDmg = GetBaseDamage() * RuleR(Combat, ArcheryNPCMultiplier); // should add a field to npc_types
	int MinDmg = GetMinDamage() * RuleR(Combat, ArcheryNPCMultiplier);

	if (!damage_mod)
		 damage_mod = GetSpecialAbilityParam(SPECATK_RANGED_ATK, 3);//Damage modifier

	DamageHitInfo my_hit;
	my_hit.base_damage = MaxDmg;
	my_hit.min_damage = MinDmg;
	my_hit.damage_done = 1;

	my_hit.skill = skill;
	my_hit.offense = offense(my_hit.skill);
	my_hit.tohit = GetTotalToHit(my_hit.skill, chance_mod);
	my_hit.hand = EQ::invslot::slotRange;

	DoAttack(who, my_hit);

	TotalDmg = my_hit.damage_done;

	if (my_hit.damage_done > 0) {
		my_hit.damage_done += my_hit.damage_done * damage_mod / 100;
		who->AddToHateList(this, my_hit.damage_done, 0);
	} else {
		who->AddToHateList(this, 0, 0);
	}

	who->Damage(this, my_hit.damage_done, SPELL_UNKNOWN, skillInUse);

	//Pyrelight Custom Code - Send info about the hSTA/hSTR damage modification to clients
	// This is a boilerplate with dead code paths.
	if (my_hit.damage_done > 0 && my_hit.original_damage > 0) {
		if (IsClient()) {
			CastToClient()->LoadAccountFlags();
		} else if (GetOwner() && GetOwner()->IsClient()) {
			GetOwner()->CastToClient()->LoadAccountFlags();
		} 
		if (who->IsClient()) {
			who->CastToClient()->LoadAccountFlags();
		} else if (who->GetOwner() && who->GetOwner()->IsClient()) {
			who->GetOwner()->CastToClient()->LoadAccountFlags();
		} 
		if ((IsClient() && GetHeroicSTR() > 0) || (IsPetOwnerClient() && GetOwner()->GetHeroicSTR() > 0)) {
			if ((IsClient() || IsPetOwnerClient()) && (my_hit.damage_done > my_hit.original_damage)) {				
				int increase_percentage = ((static_cast<float>(my_hit.damage_done) / static_cast<float>(my_hit.original_damage)) - 1) * 100;
				if (GetOwner() && GetOwner()->IsClient() && GetOwner()->CastToClient()->GetAccountFlag("filter_hSTR") != "off") {
					if (GetOwner()->CastToClient()->GetAccountFlag("filter_hPets") != "off") {
						GetOwner()->Message(Chat::MyPet, "Your pet's strike was increased by %i (%i%%) by your Heroic Strength!", 
											my_hit.damage_done - my_hit.original_damage,
											increase_percentage);
					}
				} else if (IsClient() && CastToClient()->GetAccountFlag("filter_hSTR") != "off") {
					Message(Chat::YouHitOther, "Your strike was increased by %i (%i%%) by your Heroic Strength!", 
							my_hit.damage_done - my_hit.original_damage,
							increase_percentage);
				}
			}
		}
		
		if ((who->IsClient() && who->GetHeroicSTA() > 0) || (who->IsPetOwnerClient() && who->GetOwner()->GetHeroicSTA() > 0)) {
			if ((who->IsClient() || who->IsPetOwnerClient()) && (my_hit.original_damage > my_hit.damage_done)) {				
				int reduction_percentage = (1 - static_cast<float>(my_hit.damage_done) / static_cast<float>(my_hit.original_damage)) * 100;
				if (who->GetOwner() && who->GetOwner()->IsClient()  && who->GetOwner()->CastToClient()->GetAccountFlag("filter_hSTA") != "off") {
					if (who->GetOwner()->CastToClient()->GetAccountFlag("filter_hPets") != "off") {
						who->GetOwner()->Message(Chat::MyPet, "The damage to your pet was reduced by %i (%i%%) by your Heroic Stamina!", 
												my_hit.original_damage - my_hit.damage_done,
												reduction_percentage);
					}
				} else if (who->IsClient() && who->CastToClient()->GetAccountFlag("filter_hSTA") != "off") {
					who->Message(Chat::OtherHitYou,"The damage to you was reduced by %i (%i%%) by your Heroic Stamina!", 
								my_hit.original_damage - my_hit.damage_done,
								reduction_percentage);
				}
			}		
		}	
	}

	//try proc on hits and misses
	if (who && !who->HasDied()) {
		TrySpellProc(nullptr, (const EQ::ItemData*)nullptr, who, EQ::invslot::slotRange);
	}

	TryCastOnSkillUse(who, skillInUse);

	if (HasSkillProcs() && who && !who->HasDied()) {
		TrySkillProc(who, skillInUse, 0, false, EQ::invslot::slotRange);
	}

	if (HasSkillProcSuccess() && who && !who->HasDied()) {
		TrySkillProc(who, skillInUse, 0, true, EQ::invslot::slotRange);
	}
}

bool Client::ThrowingAttack(Mob* who, bool CanDoubleAttack) { //old was 51
	//conditions to use an attack checked before we are called
	if (!who)
		return false;
	//make sure the attack and ranged timers are up
	//if the ranged timer is disabled, then they have no ranged weapon and shouldent be attacking anyhow
	if((!CanDoubleAttack && (attack_timer.Enabled() && !attack_timer.Check(false)) || (ranged_timer.Enabled() && !ranged_timer.Check()))) {
		LogCombat("Throwing attack canceled. Timer not up. Attack [{}], ranged [{}]", attack_timer.GetRemainingTime(), ranged_timer.GetRemainingTime());
		// The server and client timers are not exact matches currently, so this would spam too often if enabled
		//Message(0, "Error: Timer not up. Attack %d, ranged %d", attack_timer.GetRemainingTime(), ranged_timer.GetRemainingTime());
		return false;
	}

	int ammo_slot = EQ::invslot::slotRange;
	const EQ::ItemInstance* RangeWeapon = m_inv[EQ::invslot::slotRange];

	if (!RangeWeapon || !RangeWeapon->IsClassCommon()) {
		LogCombat("Ranged attack canceled. Missing or invalid ranged weapon ([{}]) in slot [{}]", GetItemIDAt(EQ::invslot::slotRange), EQ::invslot::slotRange);
		Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have nothing to throw!", GetItemIDAt(EQ::invslot::slotRange));
		return false;
	}

	const EQ::ItemData* item = RangeWeapon->GetItem();
	if (item->ItemType != EQ::item::ItemTypeLargeThrowing && item->ItemType != EQ::item::ItemTypeSmallThrowing) {
		LogCombat("Ranged attack canceled. Ranged item [{}] is not a throwing weapon. type [{}]", item->ID, item->ItemType);
		Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have nothing useful to throw!", GetItemIDAt(EQ::invslot::slotRange));
		return false;
	}

	LogCombat("Throwing [{}] ([{}]) at [{}]", item->Name, item->ID, who->GetName());

	if(RangeWeapon->GetCharges() == 1) {
		//first check ammo
		const EQ::ItemInstance* AmmoItem = m_inv[EQ::invslot::slotAmmo];
		if(AmmoItem != nullptr && AmmoItem->GetID() == RangeWeapon->GetID()) {
			//more in the ammo slot, use it
			RangeWeapon = AmmoItem;
			ammo_slot = EQ::invslot::slotAmmo;
			LogCombat("Using ammo from ammo slot, stack at slot [{}]. [{}] in stack", ammo_slot, RangeWeapon->GetCharges());
		} else {
			//look through our inventory for more
			int32 aslot = m_inv.HasItem(item->ID, 1, invWherePersonal);
			if (aslot != INVALID_INDEX) {
				//the item wont change, but the instance does, not that it matters
				ammo_slot = aslot;
				RangeWeapon = m_inv[aslot];
				LogCombat("Using ammo from inventory slot, stack at slot [{}]. [{}] in stack", ammo_slot, RangeWeapon->GetCharges());
			}
		}
	}

	float range = std::max(item->Range, static_cast<uint8>(50)) + GetRangeDistTargetSizeMod(who);
	LogCombat("Calculated bow range to be [{}]", range);
	range *= range;
	float dist = DistanceSquared(m_Position, who->GetPosition());
	if(dist > range) {
		LogCombat("Throwing attack out of range client should catch this. ([{}] > [{}]).\n", dist, range);
		MessageString(Chat::Red,TARGET_OUT_OF_RANGE);//Client enforces range and sends the message, this is a backup just incase.
		return false;
	}
	else if(dist < (RuleI(Combat, MinRangedAttackDist)*RuleI(Combat, MinRangedAttackDist))){
		MessageString(Chat::Yellow,RANGED_TOO_CLOSE);//Client enforces range and sends the message, this is a backup just incase.
		return false;
	}

	if(!IsAttackAllowed(who) ||
		(IsCasting() && GetClass() != BARD) ||
		IsSitting() ||
		(DivineAura() && !GetGM()) ||
		IsStunned() ||
		IsFeared() ||
		IsMezzed() ||
		(GetAppearance() == eaDead)){
		return false;
	}

	DoThrowingAttackDmg(who, RangeWeapon, item, 0, 0, 0, 0, 0,ammo_slot);

	// Consume Ammo, unless Ammo Consumption is disabled
	if (RuleB(Combat, ThrowingConsumesAmmo) && RangeWeapon->IsStackable() && !RangeWeapon->GetItemMagical(false)) {		
		DeleteItemInInventory(ammo_slot, 1, true);
		LogCombat("Consumed Throwing Ammo from slot {}.", ammo_slot);
	} else {
		LogCombat("Throwing Ammo Consumption is disabled.");
	}

	CommonBreakInvisibleFromCombat();
	return true;
}

void Mob::DoThrowingAttackDmg(Mob *who, const EQ::ItemInstance *RangeWeapon, const EQ::ItemData *AmmoItem,
	int32 weapon_damage, int16 chance_mod, int16 focus, int ReuseTime, uint32 range_id,
	int AmmoSlot, float speed, bool DisableProcs)
{
	if ((who == nullptr ||
		((IsClient() && CastToClient()->dead) || (who->IsClient() && who->CastToClient()->dead)) ||
		HasDied() || (!IsAttackAllowed(who)) || (who->GetInvul() || who->GetSpecialAbility(IMMUNE_MELEE)))) {
		return;
	}

	const EQ::ItemInstance *m_RangeWeapon = nullptr;//throwing weapon
	const EQ::ItemData *last_ammo_used = nullptr;

	/*
	If LaunchProjectile is false this function will do archery damage on target,
	otherwise it will shoot the projectile at the target, once the projectile hits target
	this function is then run again to do the damage portion
	*/
	bool LaunchProjectile = false;

	if (RuleB(Combat, ProjectileDmgOnImpact)) {
		if (AmmoItem) {
			LaunchProjectile = true;
		}
		else {
			if (!RangeWeapon && range_id) {
				if (IsClient()) {
					m_RangeWeapon = CastToClient()->m_inv[AmmoSlot];

					if (m_RangeWeapon && m_RangeWeapon->GetItem() && m_RangeWeapon->GetItem()->ID == range_id) {
						RangeWeapon = m_RangeWeapon;
					}
					else {
						last_ammo_used = database.GetItem(range_id);
					}
				}
			}
		}
	}
	else if (AmmoItem) {
		SendItemAnimation(who, AmmoItem, EQ::skills::SkillThrowing);
	}

	LogCombat("Throwing attack hit [{}]", who->GetName());

	int WDmg = 0;

	if (!weapon_damage) {
		if (IsOfClientBot() && RangeWeapon) {
			WDmg = GetWeaponDamage(who, RangeWeapon);
		}
		else if (AmmoItem) {
			WDmg = GetWeaponDamage(who, AmmoItem);
		}

		if (LaunchProjectile) {
			TryProjectileAttack(who, AmmoItem, EQ::skills::SkillThrowing, WDmg, RangeWeapon,
				nullptr, AmmoSlot, speed);
			return;
		}
	}
	else {
		WDmg = weapon_damage;
	}

	if (focus) { // no longer used, keep for quests
		WDmg += WDmg * focus / 100;
	}

	int TotalDmg = 0;

	DamageHitInfo my_hit;
	my_hit.damage_done = 1; // min 1 dmg
	my_hit.base_damage = 1;
	my_hit.min_damage = 1;
	my_hit.skill = EQ::skills::SkillThrowing;

	if (WDmg > 0) {
		my_hit.base_damage = WDmg;
		my_hit.min_damage = 0;
		my_hit.damage_done = 1;

		my_hit.skill = EQ::skills::SkillThrowing;
		my_hit.offense = offense(my_hit.skill);
		my_hit.tohit = GetTotalToHit(my_hit.skill, chance_mod);
		my_hit.hand = EQ::invslot::slotRange;

		DoAttack(who, my_hit);
		TotalDmg = my_hit.damage_done;

		LogCombat("Item DMG [{}]. Hit for damage [{}]", WDmg, TotalDmg);
	}
	else {
		TotalDmg = DMG_INVULNERABLE;
	}

	if (IsClient() && !CastToClient()->GetFeigned()) {
		who->AddToHateList(this, WDmg, 0);
	}

	who->Damage(this, TotalDmg, SPELL_UNKNOWN, EQ::skills::SkillThrowing);

	//Pyrelight Custom Code - Send info about the hSTA/hSTR damage modification to clients
	// This is a boilerplate with dead code paths.
	if (my_hit.damage_done > 0 && my_hit.original_damage > 0) {
		if (IsClient()) {
			CastToClient()->LoadAccountFlags();
		} else if (GetOwner() && GetOwner()->IsClient()) {
			GetOwner()->CastToClient()->LoadAccountFlags();
		} 
		if (who->IsClient()) {
			who->CastToClient()->LoadAccountFlags();
		} else if (who->GetOwner() && who->GetOwner()->IsClient()) {
			who->GetOwner()->CastToClient()->LoadAccountFlags();
		} 
		if ((IsClient() && GetHeroicSTR() > 0) || (IsPetOwnerClient() && GetOwner()->GetHeroicSTR() > 0)) { 
			if ((IsClient() || IsPetOwnerClient()) && (my_hit.damage_done > my_hit.original_damage)) {				
				int increase_percentage = ((static_cast<float>(my_hit.damage_done) / static_cast<float>(my_hit.original_damage)) - 1) * 100;
				if (GetOwner() && GetOwner()->IsClient() && GetOwner()->CastToClient()->GetAccountFlag("filter_hSTR") != "off") {
					if (GetOwner()->CastToClient()->GetAccountFlag("filter_hPets") != "off") {
						GetOwner()->Message(Chat::MyPet, "Your pet's strike was increased by %i (%i%%) by your Heroic Strength!", 
											my_hit.damage_done - my_hit.original_damage,
											increase_percentage);
					}
				} else if (IsClient() && CastToClient()->GetAccountFlag("filter_hSTR") != "off") {
					Message(Chat::YouHitOther, "Your strike was increased by %i (%i%%) by your Heroic Strength!", 
							my_hit.damage_done - my_hit.original_damage,
							increase_percentage);
				}
			}
		}
		
		if ((who->IsClient() && who->GetHeroicSTA() > 0) || (who->IsPetOwnerClient() && who->GetOwner()->GetHeroicSTA() > 0)) {
			if ((who->IsClient() || who->IsPetOwnerClient()) && (my_hit.original_damage > my_hit.damage_done)) {				
				int reduction_percentage = (1 - static_cast<float>(my_hit.damage_done) / static_cast<float>(my_hit.original_damage)) * 100;
				if (who->GetOwner() && who->GetOwner()->IsClient()  && who->GetOwner()->CastToClient()->GetAccountFlag("filter_hSTA") != "off") {
					if (who->GetOwner()->CastToClient()->GetAccountFlag("filter_hPets") != "off") {
						who->GetOwner()->Message(Chat::MyPet, "The damage to your pet was reduced by %i (%i%%) by your Heroic Stamina!", 
												my_hit.original_damage - my_hit.damage_done,
												reduction_percentage);
					}
				} else if (who->IsClient() && who->CastToClient()->GetAccountFlag("filter_hSTA") != "off") {
					who->Message(Chat::OtherHitYou,"The damage to you was reduced by %i (%i%%) by your Heroic Stamina!", 
								my_hit.original_damage - my_hit.damage_done,
								reduction_percentage);
				}
			}		
		}	
	}

	if (!DisableProcs) {
		// Weapon Proc
		if (RangeWeapon && who && !who->HasDied()) {
			TryCombatProcs(RangeWeapon, who, EQ::invslot::slotRange);
		}
	}

	TryCastOnSkillUse(who, EQ::skills::SkillThrowing);

	if (!DisableProcs) {
		if (HasSkillProcs() && who && !who->HasDied()) {
			if (ReuseTime) {
				TrySkillProc(who, EQ::skills::SkillThrowing, ReuseTime);
			}
			else {
				TrySkillProc(who, EQ::skills::SkillThrowing, 0, false, EQ::invslot::slotRange);
			}
		}

		if (HasSkillProcSuccess() && who && !who->HasDied()) {
			if (ReuseTime) {
				TrySkillProc(who, EQ::skills::SkillThrowing, ReuseTime, true);
			}
			else {
				TrySkillProc(who, EQ::skills::SkillThrowing, 0, true, EQ::invslot::slotRange);
			}
		}
	}

	if (IsClient()) {
		CastToClient()->CheckIncreaseSkill(EQ::skills::SkillThrowing, GetTarget());
	}
}

void Mob::SendItemAnimation(Mob *to, const EQ::ItemData *item, EQ::skills::SkillType skillInUse, float velocity) {
	auto outapp = new EQApplicationPacket(OP_SomeItemPacketMaybe, sizeof(Arrow_Struct));
	Arrow_Struct *as = (Arrow_Struct *) outapp->pBuffer;
	as->type = 1;
	as->src_x = GetX();
	as->src_y = GetY();
	as->src_z = GetZ();
	as->source_id = GetID();
	as->target_id = to->GetID();
	as->item_id = item->ID;

	as->item_type = item->ItemType;
	as->skill = (uint8)skillInUse;

	strn0cpy(as->model_name, item->IDFile, 16);


	/*
		The angular field affects how the object flies towards the target.
		A low angular (10) makes it circle the target widely, where a high
		angular (20000) makes it go straight at them.

		The tilt field causes the object to be tilted flying through the air
		and also seems to have an effect on how it behaves when circling the
		target based on the angular field.

		Arc causes the object to form an arc in motion. A value too high will
	*/
	as->velocity = velocity;

	//these angle and tilt used together seem to make the arrow/knife throw as straight as I can make it

	as->launch_angle = CalculateHeadingToTarget(to->GetX(), to->GetY());
	as->tilt = 125;
	as->arc = 50;


	//fill in some unknowns, we dont know their meaning yet
	//neither of these seem to change the behavior any
	as->unknown088 = 125;
	as->unknown092 = 16;

	entity_list.QueueCloseClients(this, outapp);
	safe_delete(outapp);
}

void Mob::ProjectileAnimation(Mob* to, int item_id, bool IsArrow, float speed, float angle, float tilt, float arc, const char *IDFile, EQ::skills::SkillType skillInUse) {
	if (!to)
		return;

	const EQ::ItemData* item = nullptr;
	uint8 item_type = 0;

	if(!item_id) {
		item = database.GetItem(8005); // Arrow will be default
	}
	else {
		item = database.GetItem(item_id); // Use the item input into the command
	}

	if(!item) {
		return;
	}
	if(IsArrow) {
		item_type = 27;
	}
	if(!item_type && !skillInUse) {
		item_type = item->ItemType;
	}
	else if (skillInUse)
		item_type = GetItemTypeBySkill(skillInUse);

	if(!speed) {
		speed = 4.0;
	}
	if(!angle) {
		angle = CalculateHeadingToTarget(to->GetX(), to->GetY());
	}
	if(!tilt) {
		tilt = 125;
	}
	if(!arc) {
		arc = 50;
	}

	const char *item_IDFile = item->IDFile;

	if (IDFile && (strncmp(IDFile, "IT", 2) == 0))
		item_IDFile = IDFile;

	// See SendItemAnimation() for some notes on this struct
	auto outapp = new EQApplicationPacket(OP_SomeItemPacketMaybe, sizeof(Arrow_Struct));
	Arrow_Struct *as = (Arrow_Struct *) outapp->pBuffer;
	as->type = 1;
	as->src_x = GetX();
	as->src_y = GetY();
	as->src_z = GetZ();
	as->source_id = GetID();
	as->target_id = to->GetID();
	as->item_id = item->ID;
	as->item_type = item_type;
	as->skill = skillInUse;	// Doesn't seem to have any effect
	strn0cpy(as->model_name, item_IDFile, 16);
	as->velocity = speed;
	as->launch_angle = angle;
	as->tilt = tilt;
	as->arc = arc;
	as->unknown088 = 125;
	as->unknown092 = 16;

	entity_list.QueueCloseClients(this, outapp);
	safe_delete(outapp);

}

void NPC::DoClassAttacks(Mob *target) {
	if(target == nullptr)
		return;	//gotta have a target for all these

	bool taunt_time = taunt_timer.Check();
	bool ca_time = classattack_timer.Check(false);
	bool ka_time = knightattack_timer.Check(false);

	const EQ::ItemData* boots = database.GetItem(equipment[EQ::invslot::slotFeet]);

	//only check attack allowed if we are going to do something
	if((taunt_time || ca_time || ka_time) && !IsAttackAllowed(target))
		return;

	if(ka_time){
		int knightreuse = 1000; //lets give it a small cooldown actually.

		switch(GetClass()){
			case SHADOWKNIGHT: case SHADOWKNIGHTGM:{
				if (CastSpell(SPELL_NPC_HARM_TOUCH, target->GetID())) {
					knightreuse = HarmTouchReuseTime * 1000;
					}
				break;
			}
			case PALADIN: case PALADINGM:{
				if(GetHPRatio() < 20) {
					if (CastSpell(SPELL_LAY_ON_HANDS, GetID())) {
						knightreuse = LayOnHandsReuseTime * 1000;
					}
				} else {
					knightreuse = 2000; //Check again in two seconds.
				}
				break;
			}
		}
		knightattack_timer.Start(knightreuse);
	}

	//general stuff, for all classes....
	//only gets used when their primary ability get used too
	if (
		IsTaunting() &&
		HasOwner() &&
		target->IsNPC() &&
		target->GetBodyType() != BT_Undead &&
		taunt_time
	) {
		GetOwner()->MessageString(Chat::PetResponse, PET_TAUNTING);
		Taunt(target->CastToNPC(), false);
	}

	if(!ca_time)
		return;

	float HasteModifier = GetHaste() * 0.01f;

	int level = GetLevel();
	int reuse = TauntReuseTime * 1000;	//make this very long since if they dont use it once, they prolly never will
	bool did_attack = false;
	//class specific stuff...
	switch(GetClass()) {
		case ROGUE: case ROGUEGM:
			if(level >= 10) {
				reuse = BackstabReuseTime * 1000;
				TryBackstab(target, reuse);
				did_attack = true;
			}
			break;
		case MONK: case MONKGM: {
			uint8 satype = EQ::skills::SkillKick;
			if (level > 29) { satype = EQ::skills::SkillFlyingKick; }
			else if (level > 24) { satype = EQ::skills::SkillDragonPunch; }
			else if (level > 19) { satype = EQ::skills::SkillEagleStrike; }
			else if (level > 9) { satype = EQ::skills::SkillTigerClaw; }
			else if (level > 4) { satype = EQ::skills::SkillRoundKick; }
			reuse = MonkSpecialAttack(target, satype);

			reuse *= 1000;
			did_attack = true;
			break;
		}
		case WARRIOR: case WARRIORGM:{
			if(level >= RuleI(Combat, NPCBashKickLevel)){
				if(zone->random.Roll(75)) { //tested on live, warrior mobs both kick and bash, kick about 75% of the time, casting doesn't seem to make a difference.
					DoAnim(animKick, 0, false);
					int64 dmg = GetBaseSkillDamage(EQ::skills::SkillKick);

					if (GetWeaponDamage(target, boots) <= 0) {
						dmg = DMG_INVULNERABLE;
					}

					reuse = (KickReuseTime + 3) * 1000;
					DoSpecialAttackDamage(target, EQ::skills::SkillKick, dmg, GetMinDamage(), -1, reuse);
					did_attack = true;
				}
				else {
					DoAnim(animTailRake, 0, false);
					int64 dmg = GetBaseSkillDamage(EQ::skills::SkillBash);

					if (GetWeaponDamage(target, (const EQ::ItemData*)nullptr) <= 0)
						dmg = DMG_INVULNERABLE;

					reuse = (BashReuseTime + 3) * 1000;
					DoSpecialAttackDamage(target, EQ::skills::SkillBash, dmg, GetMinDamage(), -1, reuse);
					did_attack = true;
				}
			}
			break;
		}
		case SHADOWKNIGHT: case SHADOWKNIGHTGM:
		case BERSERKER: case BERSERKERGM:{
			int AtkRounds = 1;
			int32 max_dmg = GetBaseSkillDamage(EQ::skills::SkillFrenzy);
			DoAnim(anim2HSlashing, 0, false);

			if (GetClass() == BERSERKER || GetClass() == SHADOWKNIGHT) {
				int chance = GetLevel() * 2 + GetSkill(EQ::skills::SkillFrenzy);
				if (zone->random.Roll0(450) < chance)
					AtkRounds++;
				if (zone->random.Roll0(450) < chance)
					AtkRounds++;
			}

			while (AtkRounds > 0) {
				if (GetTarget())
					DoSpecialAttackDamage(GetTarget(), EQ::skills::SkillFrenzy, max_dmg, GetMinDamage(), -1, reuse);
				AtkRounds--;
			}

			did_attack = true;
			break;
		}
		case RANGER: case RANGERGM:
		case BEASTLORD: case BEASTLORDGM: {
			//kick
			if(level >= RuleI(Combat, NPCBashKickLevel)){
				DoAnim(animKick, 0, false);
				int64 dmg = GetBaseSkillDamage(EQ::skills::SkillKick);

				if (GetWeaponDamage(target, boots) <= 0)
					dmg = DMG_INVULNERABLE;

				reuse = (KickReuseTime + 3) * 1000;
				DoSpecialAttackDamage(target, EQ::skills::SkillKick, dmg, GetMinDamage(), -1, reuse);
				did_attack = true;
			}
			break;
		}
		case CLERIC: case CLERICGM: //clerics can bash too.				
		case PALADIN: case PALADINGM:{
			if(level >= RuleI(Combat, NPCBashKickLevel)){
				DoAnim(animTailRake, 0, false);
				int64 dmg = GetBaseSkillDamage(EQ::skills::SkillBash);

				if (GetWeaponDamage(target, (const EQ::ItemData*)nullptr) <= 0)
					dmg = DMG_INVULNERABLE;

				reuse = (BashReuseTime + 3) * 1000;
				DoSpecialAttackDamage(target, EQ::skills::SkillBash, dmg, GetMinDamage(), -1, reuse);
				did_attack = true;
			}
			break;
		}
	}

	classattack_timer.Start(reuse / HasteModifier);
}

// this should be refactored to generate an OP_CombatAbility struct and call OPCombatAbility
void Client::DoClassAttacks(Mob *ca_target, uint16 skill, bool IsRiposte)
{
	if(!ca_target)
		return;

	if(spellend_timer.Enabled() || IsFeared() || IsStunned() || IsMezzed() || DivineAura() || dead)
		return;

	if(!IsAttackAllowed(ca_target))
		return;

	//check range for all these abilities, they are all close combat stuff
	if(!CombatRange(ca_target)){
		return;
	}

	if(!IsRiposte && (!p_timers.Expired(&database, pTimerCombatAbility, false))) {
		return;
	}

	int ReuseTime = 0;
	float HasteMod = GetHaste() * 0.01f;

	uint16 skill_to_use = -1;

	if (skill == -1){
		switch(GetClass()){
		case WARRIOR:
		case RANGER:
		case BEASTLORD:
			skill_to_use = EQ::skills::SkillKick;
			break;
		case SHADOWKNIGHT:
		case BERSERKER:
			skill_to_use = EQ::skills::SkillFrenzy;
			break;		
		case PALADIN:
			skill_to_use = EQ::skills::SkillBash;
			break;
		case MONK:
			if(GetLevel() >= 30)
			{
				skill_to_use = EQ::skills::SkillFlyingKick;
			}
			else if(GetLevel() >= 25)
			{
				skill_to_use = EQ::skills::SkillDragonPunch;
			}
			else if(GetLevel() >= 20)
			{
				skill_to_use = EQ::skills::SkillEagleStrike;
			}
			else if(GetLevel() >= 10)
			{
				skill_to_use = EQ::skills::SkillTigerClaw;
			}
			else if(GetLevel() >= 5)
			{
				skill_to_use = EQ::skills::SkillRoundKick;
			}
			else
			{
				skill_to_use = EQ::skills::SkillKick;
			}
			break;
		case ROGUE:
			skill_to_use = EQ::skills::SkillBackstab;
			break;
		}
	}

	else
		skill_to_use = skill;

	if(skill_to_use == -1)
		return;

	int64 dmg = GetBaseSkillDamage(static_cast<EQ::skills::SkillType>(skill_to_use), GetTarget());

	if (skill_to_use == EQ::skills::SkillBash) {
		if (ca_target!=this) {
			DoAnim(animTailRake, 0, false);

			if (GetWeaponDamage(ca_target, GetInv().GetItem(EQ::invslot::slotSecondary)) <= 0 && GetWeaponDamage(ca_target, GetInv().GetItem(EQ::invslot::slotShoulders)) <= 0)
				dmg = DMG_INVULNERABLE;

			ReuseTime = (BashReuseTime - 1) / HasteMod;

			DoSpecialAttackDamage(ca_target, EQ::skills::SkillBash, dmg, 0, -1, ReuseTime);

			if(ReuseTime > 0 && !IsRiposte) {
				p_timers.Start(pTimerCombatAbility, ReuseTime);
			}
		}
		return;
	}

	if (skill_to_use == EQ::skills::SkillFrenzy) {
		CheckIncreaseSkill(EQ::skills::SkillFrenzy, GetTarget(), 10);
		int AtkRounds = 1;
		DoAnim(anim1HWeapon, 0, false);

		ReuseTime = (FrenzyReuseTime - 1) / HasteMod;

		// bards can do riposte frenzy for some reason
		if (!IsRiposte && (GetClass() == BERSERKER || GetClass() == SHADOWKNIGHT)) {
			int chance = GetLevel() * 2 + GetSkill(EQ::skills::SkillFrenzy);
			if (zone->random.Roll0(450) < chance)
				AtkRounds++;
			if (zone->random.Roll0(450) < chance)
				AtkRounds++;
		}

		while(AtkRounds > 0) {
			if (ca_target!=this)
				DoSpecialAttackDamage(ca_target, EQ::skills::SkillFrenzy, dmg, 0, dmg, ReuseTime);
			AtkRounds--;
		}

		if(ReuseTime > 0 && !IsRiposte) {
			p_timers.Start(pTimerCombatAbility, ReuseTime);
		}
		return;
	}

	if (skill_to_use == EQ::skills::SkillKick){
		if(ca_target!=this){
			DoAnim(animKick, 0, false);

			if (GetWeaponDamage(ca_target, GetInv().GetItem(EQ::invslot::slotFeet)) <= 0)
				dmg = DMG_INVULNERABLE;

			ReuseTime = KickReuseTime-1;

			DoSpecialAttackDamage(ca_target, EQ::skills::SkillKick, dmg, 0, -1, ReuseTime);
		}
	}

	if (skill_to_use == EQ::skills::SkillFlyingKick || skill_to_use == EQ::skills::SkillDragonPunch || skill_to_use == EQ::skills::SkillEagleStrike || skill_to_use == EQ::skills::SkillTigerClaw || skill_to_use == EQ::skills::SkillRoundKick) {
		ReuseTime = MonkSpecialAttack(ca_target, skill_to_use) - 1;
		MonkSpecialAttack(ca_target, skill_to_use);

		if (IsRiposte)
			return;

		//Live AA - Technique of Master Wu
		int wuchance = itembonuses.DoubleSpecialAttack + spellbonuses.DoubleSpecialAttack + aabonuses.DoubleSpecialAttack;
		if (wuchance) {
			const int MonkSPA[5] = {EQ::skills::SkillFlyingKick, EQ::skills::SkillDragonPunch,
						EQ::skills::SkillEagleStrike, EQ::skills::SkillTigerClaw,
						EQ::skills::SkillRoundKick};
			int extra = 0;
			// always 1/4 of the double attack chance, 25% at rank 5 (100/4)
			while (wuchance > 0) {
				if (zone->random.Roll(wuchance))
					extra++;
				else
					break;
				wuchance /= 4;
			}
			// They didn't add a string ID for this.
			std::string msg = StringFormat(
			    "The spirit of Master Wu fills you!  You gain %d additional attack(s).", extra);
			// live uses 400 here -- not sure if it's the best for all clients though
			SendColoredText(400, msg);
			auto classic = RuleB(Combat, ClassicMasterWu);
			while (extra) {
				MonkSpecialAttack(GetTarget(),
						  classic ? MonkSPA[zone->random.Int(0, 4)] : skill_to_use);
				extra--;
			}
		}
	}

	if (skill_to_use == EQ::skills::SkillBackstab){
		ReuseTime = BackstabReuseTime-1;

		if (IsRiposte)
			ReuseTime=0;

		TryBackstab(ca_target,ReuseTime);
	}

	ReuseTime = ReuseTime / HasteMod;
	if(ReuseTime > 0 && !IsRiposte){
		p_timers.Start(pTimerCombatAbility, ReuseTime);
	}
}

void Mob::Taunt(NPC *who, bool always_succeed, int chance_bonus, bool FromSpell, int32 bonus_hate)
{
	if (who == nullptr)
		return;

	if (DivineAura())
		return;

	if (!FromSpell && !CombatRange(who))
		return;

	if (!always_succeed && IsClient())
		CastToClient()->CheckIncreaseSkill(EQ::skills::SkillTaunt, who, 10);

	Mob *hate_top = who->GetHateMost();

	int level_difference = GetLevel() - who->GetLevel();
	bool success = false;

	// Support for how taunt worked pre 2000 on LIVE - Can not taunt NPC over your level.
	if ((RuleB(Combat, TauntOverLevel) == false) && (level_difference < 0) ||
	    who->GetSpecialAbility(IMMUNE_TAUNT)) {
		MessageString(Chat::SpellFailure, FAILED_TAUNT);
		return;
	}

	// All values used based on live parses after taunt was updated in 2006.
	if ((hate_top && hate_top->GetHPRatio() >= 20) || hate_top == nullptr || chance_bonus) {
		// SE_Taunt this is flat chance
		if (chance_bonus) {
			success = zone->random.Roll(chance_bonus);
		} else {
			float tauntchance = 50.0f;

			if (always_succeed)
				tauntchance = 101.0f;

			else {

				if (level_difference < 0) {
					tauntchance += static_cast<float>(level_difference) * 3.0f;
					if (tauntchance < 20)
						tauntchance = 20.0f;
				}

				else {
					tauntchance += static_cast<float>(level_difference) * 5.0f;
					if (tauntchance > 65)
						tauntchance = 65.0f;
				}
			}

			// TauntSkillFalloff rate is not based on any real data. Default of 33% gives a reasonable
			// result.
			if (IsClient() && !always_succeed)
				tauntchance -= (RuleR(Combat, TauntSkillFalloff) *
						(CastToClient()->MaxSkill(EQ::skills::SkillTaunt) -
						 GetSkill(EQ::skills::SkillTaunt)));

			if (tauntchance < 1)
				tauntchance = 1.0f;

			tauntchance /= 100.0f;

			success = tauntchance > zone->random.Real(0, 1);

			LogHate(
				"Taunter mob {} target npc {} tauntchance [{}] success [{}] hate_top [{}]",
				GetMobDescription(),
				who->GetMobDescription(),
				tauntchance,
				success ? "true" : "false",
				hate_top ? hate_top->GetMobDescription() : "not found"
			);
		}

		if (success) {
			if (hate_top && hate_top != this) {
				int64 newhate = (who->GetNPCHate(hate_top) - who->GetNPCHate(this)) + 1 + bonus_hate;

				LogHate(
					"Taunter mob {} target npc {} newhate [{}] hated_top {} hate_of_top [{}] this_hate [{}] bonus_hate [{}]",
					GetMobDescription(),
					who->GetMobDescription(),
					newhate,
					hate_top->GetMobDescription(),
					who->GetNPCHate(hate_top),
					who->GetNPCHate(this),
					bonus_hate
				);

				who->CastToNPC()->AddToHateList(this, newhate);
				success = true;
			} else {
				who->CastToNPC()->AddToHateList(this, 12);
			}

			if (who->CanTalk())
				who->SayString(SUCCESSFUL_TAUNT, GetCleanName());
		} else {
			MessageString(Chat::SpellFailure, FAILED_TAUNT);
		}
	} else {
		MessageString(Chat::SpellFailure, FAILED_TAUNT);
	}

	TryCastOnSkillUse(who, EQ::skills::SkillTaunt);

	if (HasSkillProcs()) {
		TrySkillProc(who, EQ::skills::SkillTaunt, TauntReuseTime * 1000);
	}

	if (success && HasSkillProcSuccess()) {
		TrySkillProc(who, EQ::skills::SkillTaunt, TauntReuseTime * 1000, true);
	}
}

void Mob::InstillDoubt(Mob *who) {
	//make sure we can use this skill
	/*int skill = GetSkill(INTIMIDATION);*/	//unused

	//make sure our target is an NPC
	if(!who || !who->IsNPC())
		return;

	if(DivineAura())
		return;

	//range check
	if(!CombatRange(who))
		return;

	if(IsClient()) {
		CastToClient()->CheckIncreaseSkill(EQ::skills::SkillIntimidation, who, 10);
	}

	//I think this formula needs work
	int value = 0;
	bool success = false;

	//user's bonus
	value += GetSkill(EQ::skills::SkillIntimidation) + GetCHA() / 4;

	//target's counters
	value -= target->GetLevel()*4 + who->GetWIS()/4;

	if (zone->random.Roll(value)) {
		//temporary hack...
		//cast fear on them... should prolly be a different spell
		//and should be un-resistable.
		SpellOnTarget(229, who, 0, true, -2000);
		success = true;
		//is there a success message?
	} else {
		MessageString(Chat::LightBlue,NOT_SCARING);
		//Idea from WR:
		/* if (target->IsNPC() && zone->random.Int(0,99) < 10 ) {
			entity_list.MessageClose(target, false, 50, Chat::NPCRampage, "%s lashes out in anger!",target->GetName());
			//should we actually do this? and the range is completely made up, unconfirmed
			entity_list.AEAttack(target, 50);
		}*/
	}

	TryCastOnSkillUse(who, EQ::skills::SkillIntimidation);

	if (HasSkillProcs()) {
		TrySkillProc(who, EQ::skills::SkillIntimidation, InstillDoubtReuseTime * 1000);
	}

	if (success && HasSkillProcSuccess()) {
		TrySkillProc(who, EQ::skills::SkillIntimidation, InstillDoubtReuseTime * 1000, true);
	}
}

int Mob::TryHeadShot(Mob *defender, EQ::skills::SkillType skillInUse)
{
	// Only works on YOUR target.
	if (
		defender &&
		!defender->IsClient() &&
		skillInUse == EQ::skills::SkillArchery &&
		GetTarget() == defender &&
		(defender->GetBodyType() == BT_Humanoid || !RuleB(Combat, HeadshotOnlyHumanoids))
	) {
		uint32 HeadShot_Dmg = aabonuses.HeadShot[SBIndex::FINISHING_EFFECT_DMG] + spellbonuses.HeadShot[SBIndex::FINISHING_EFFECT_DMG] + itembonuses.HeadShot[SBIndex::FINISHING_EFFECT_DMG];
		uint8 HeadShot_Level = 0; // Get Highest Headshot Level
		HeadShot_Level = std::max({aabonuses.HSLevel[SBIndex::FINISHING_EFFECT_LEVEL_MAX], spellbonuses.HSLevel[SBIndex::FINISHING_EFFECT_LEVEL_MAX], itembonuses.HSLevel[SBIndex::FINISHING_EFFECT_LEVEL_MAX]});

		if (HeadShot_Dmg && HeadShot_Level && (defender->GetLevel() <= HeadShot_Level)) {
			int chance = GetDEX();
			chance = 100 * chance / (chance + 3500);
			if (IsClient() || IsBot()) {
				chance += GetHeroicDEX() / 25;
			}
			chance *= 10;
			int norm = aabonuses.HSLevel[SBIndex::FINISHING_EFFECT_LEVEL_CHANCE_BONUS];
			if (norm > 0)
				chance = chance * norm / 100;
			chance += aabonuses.HeadShot[SBIndex::FINISHING_EFFECT_PROC_CHANCE] + spellbonuses.HeadShot[SBIndex::FINISHING_EFFECT_PROC_CHANCE] + itembonuses.HeadShot[SBIndex::FINISHING_EFFECT_PROC_CHANCE];
			if (zone->random.Int(1, 1000) <= chance) {
				entity_list.MessageCloseString(
					this, false, 200, Chat::MeleeCrit, FATAL_BOW_SHOT,
					GetName());
				return HeadShot_Dmg;
			}
		}
	}

	return 0;
}

int Mob::TryAssassinate(Mob *defender, EQ::skills::SkillType skillInUse)
{
	if (
		defender &&
		!defender->IsClient() &&
		GetLevel() >= 60 &&
		(skillInUse == EQ::skills::SkillBackstab || skillInUse == EQ::skills::SkillThrowing) &&
		(defender->GetBodyType() == BT_Humanoid || !RuleB(Combat, AssassinateOnlyHumanoids))
	) {
		int chance = GetDEX();
		if (skillInUse == EQ::skills::SkillBackstab) {
			chance = 100 * chance / (chance + 3500);
			if (IsClient() || IsBot()) {
				chance += GetHeroicDEX();
			}
			chance *= 10;
			int norm = aabonuses.AssassinateLevel[SBIndex::FINISHING_EFFECT_LEVEL_CHANCE_BONUS];
			if (norm > 0)
				chance = chance * norm / 100;
		} else if (skillInUse == EQ::skills::SkillThrowing) {
			if (chance > 255)
				chance = 260;
			else
				chance += 5;
		}

		chance += aabonuses.Assassinate[SBIndex::FINISHING_EFFECT_PROC_CHANCE] + spellbonuses.Assassinate[SBIndex::FINISHING_EFFECT_PROC_CHANCE] + itembonuses.Assassinate[SBIndex::FINISHING_EFFECT_PROC_CHANCE];

		uint32 Assassinate_Dmg =
				   aabonuses.Assassinate[SBIndex::FINISHING_EFFECT_DMG] + spellbonuses.Assassinate[SBIndex::FINISHING_EFFECT_DMG] + itembonuses.Assassinate[SBIndex::FINISHING_EFFECT_DMG];

		uint8 Assassinate_Level = 0; // Get Highest Headshot Level
		Assassinate_Level = std::max(
		    {aabonuses.AssassinateLevel[SBIndex::FINISHING_EFFECT_LEVEL_MAX], spellbonuses.AssassinateLevel[SBIndex::FINISHING_EFFECT_LEVEL_MAX], itembonuses.AssassinateLevel[SBIndex::FINISHING_EFFECT_LEVEL_MAX]});

		// revamped AAs require AA line I believe?
		if (!Assassinate_Level)
			return 0;

		if (Assassinate_Dmg && Assassinate_Level && (defender->GetLevel() <= Assassinate_Level)) {
			if (zone->random.Int(1, 1000) <= chance) {
				entity_list.MessageCloseString(
					this, false, 200, Chat::MeleeCrit, ASSASSINATES,
					GetName());
				return Assassinate_Dmg;
			}
		}
	}

	return 0;
}

void Mob::DoMeleeSkillAttackDmg(Mob *who, int32 weapon_damage, EQ::skills::SkillType skillinuse, int16 chance_mod,
				int16 focus, bool CanRiposte, int ReuseTime)
{
	if (!CanDoSpecialAttack(who)) {
		return;
	}

	/*
		For spells using skill value 98 (feral swipe ect) server sets this to 67 automatically.
		Kayen: This is unlikely to be completely accurate but use OFFENSE skill value for these effects.
		TODO: We need to stop moving skill 98, it's suppose to just be a dummy skill AFAIK
		Spells using offense should use the skill of your primary, if you can use it, otherwise h2h
	*/
	if (skillinuse == EQ::skills::SkillBegging) {
		skillinuse = EQ::skills::SkillOffense;
	}

	int64 damage = 0;
	int64 hate = 0;
	if (hate == 0 && weapon_damage > 1) {
		hate = weapon_damage;
	}

	DamageHitInfo my_hit;
	my_hit.damage_done = 1; // min 1 dmg

	if (weapon_damage > 0) {
		if (focus) {
			weapon_damage += weapon_damage * focus / 100;
		}

		if (skillinuse == EQ::skills::SkillBash && IsClient()) {
			EQ::ItemInstance *item =
				CastToClient()->GetInv().GetItem(EQ::invslot::slotSecondary);
			if (item) {
				if (item->GetItem()->ItemType == EQ::item::ItemTypeShield) {
					hate += item->GetItem()->AC;
				}
				const EQ::ItemData *itm = item->GetItem();
				hate = hate * (100 + GetSpellFuriousBash(itm->Focus.Effect)) / 100;
			}
		}

		my_hit.base_damage = weapon_damage;
		my_hit.min_damage = 0;
		my_hit.damage_done = 1;

		my_hit.skill = skillinuse;
		my_hit.offense = offense(my_hit.skill);
		my_hit.tohit = GetTotalToHit(my_hit.skill, chance_mod);
		// slot range exclude ripe etc ...
		my_hit.hand = CanRiposte ? EQ::invslot::slotRange : EQ::invslot::slotPrimary;

		if (IsNPC()) {
			my_hit.min_damage = CastToNPC()->GetMinDamage();
		}

		DoAttack(who, my_hit);
		damage = my_hit.damage_done;
	} else {
		damage = DMG_INVULNERABLE;
	}

	if (skillinuse == EQ::skills::SkillOffense) {    // Hack to allow damage to display.
		skillinuse = EQ::skills::SkillTigerClaw; //'strike' your opponent - Arbitrary choice for message.
	}

	who->AddToHateList(this, hate, 0);
	who->Damage(this, damage, SPELL_UNKNOWN, skillinuse);

	//Pyrelight Custom Code - Send info about the hSTA/hSTR damage modification to clients
	// This is a boilerplate with dead code paths.
	if (my_hit.damage_done > 0 && my_hit.original_damage > 0) {
		if (IsClient()) {
			CastToClient()->LoadAccountFlags();
		} else if (GetOwner() && GetOwner()->IsClient()) {
			GetOwner()->CastToClient()->LoadAccountFlags();
		} 
		if (who->IsClient()) {
			who->CastToClient()->LoadAccountFlags();
		} else if (who->GetOwner() && who->GetOwner()->IsClient()) {
			who->GetOwner()->CastToClient()->LoadAccountFlags();
		} 
		if ((IsClient() && GetHeroicSTR() > 0) || (IsPetOwnerClient() && GetOwner()->GetHeroicSTR() > 0)) {
			if ((IsClient() || IsPetOwnerClient()) && (my_hit.damage_done > my_hit.original_damage)) {				
				int increase_percentage = ((static_cast<float>(my_hit.damage_done) / static_cast<float>(my_hit.original_damage)) - 1) * 100;
				if (GetOwner() && GetOwner()->IsClient() && GetOwner()->CastToClient()->GetAccountFlag("filter_hSTR") != "off") {
					if (GetOwner()->CastToClient()->GetAccountFlag("filter_hPets") != "off") {
						GetOwner()->Message(Chat::MyPet, "Your pet's strike was increased by %i (%i%%) by your Heroic Strength!", 
											my_hit.damage_done - my_hit.original_damage,
											increase_percentage);
					}
				} else if (IsClient() && CastToClient()->GetAccountFlag("filter_hSTR") != "off") {
					Message(Chat::YouHitOther, "Your strike was increased by %i (%i%%) by your Heroic Strength!", 
							my_hit.damage_done - my_hit.original_damage,
							increase_percentage);
				}
			}
		}
		
		if ((who->IsClient() && who->GetHeroicSTA() > 0) || (who->IsPetOwnerClient() && who->GetOwner()->GetHeroicSTA() > 0)) {
			if ((who->IsClient() || who->IsPetOwnerClient()) && (my_hit.original_damage > my_hit.damage_done)) {				
				int reduction_percentage = (1 - static_cast<float>(my_hit.damage_done) / static_cast<float>(my_hit.original_damage)) * 100;
				if (who->GetOwner() && who->GetOwner()->IsClient()  && who->GetOwner()->CastToClient()->GetAccountFlag("filter_hSTA") != "off") {
					if (who->GetOwner()->CastToClient()->GetAccountFlag("filter_hPets") != "off") {
						who->GetOwner()->Message(Chat::MyPet, "The damage to your pet was reduced by %i (%i%%) by your Heroic Stamina!", 
												my_hit.original_damage - my_hit.damage_done,
												reduction_percentage);
					}
				} else if (who->IsClient() && who->CastToClient()->GetAccountFlag("filter_hSTA") != "off") {
					who->Message(Chat::OtherHitYou,"The damage to you was reduced by %i (%i%%) by your Heroic Stamina!", 
								my_hit.original_damage - my_hit.damage_done,
								reduction_percentage);
				}
			}	
		}		
	}

	if (HasDied()) {
		return;
	}

	TryCastOnSkillUse(who, skillinuse);
}

bool Mob::CanDoSpecialAttack(Mob *who) {
	//Make sure everything is valid before doing any attacks.
	if (!who) {
		SetTarget(nullptr);
		return false;
	}

	if(!GetTarget())
		SetTarget(who);

	if ((who == nullptr || ((IsClient() && CastToClient()->dead) || (who->IsClient() && who->CastToClient()->dead)) || HasDied() || (!IsAttackAllowed(who)))) {
		return false;
	}

	if(who->GetInvul() || who->GetSpecialAbility(IMMUNE_MELEE))
		return false;

	return true;
}
