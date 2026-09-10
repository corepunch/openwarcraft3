#ifdef BZ_TESTS
#include "test.h"
#include "../g_local.h"
#include "../game/skills/s_skills.h"

LPEDICT alloc_test_unit(DWORD class_id, FLOAT x, FLOAT y);
void reset_entities(void);
void setup_test_world(void);
slkTestData_t *parse_slk_string(const char *slk_text);
void free_slk_rows(slkTestData_t *rows);

static const char slk_spell_data[] =
	"ID;PWXL;N;EBB;Y2;X11\n"
	"C;Y1;X1;K\"alias\"\n"
	"C;Y1;X2;K\"code\"\n"
	"C;Y1;X3;K\"targs\"\n"
	"C;Y1;X4;K\"Cost1\"\n"
	"C;Y1;X5;K\"Cool1\"\n"
	"C;Y1;X6;K\"Rng1\"\n"
	"C;Y1;X7;K\"Dur1\"\n"
	"C;Y1;X8;K\"HeroDur1\"\n"
	"C;Y1;X9;K\"DataA1\"\n"
	"C;Y1;X10;K\"DataB1\"\n"
	"C;Y1;X11;K\"Area1\"\n"
	"C;Y2;X1;K\"AHtb\"\n"
	"C;Y2;X2;K\"AHtb\"\n"
	"C;Y2;X3;K\"air,ground,enemy,neutral\"\n"
	"C;Y2;X4;K\"75\"\n"
	"C;Y2;X5;K\"9\"\n"
	"C;Y2;X6;K\"600\"\n"
	"C;Y2;X7;K\"5\"\n"
	"C;Y2;X8;K\"3\"\n"
	"C;Y2;X9;K\"100\"\n"
	"C;Y2;X10;K\"55\"\n"
	"E\n";

static LPEDICT make_hero(DWORD class_id, FLOAT hp, FLOAT mana, FLOAT x, FLOAT y) {
	reset_entities();
	setup_test_world();
	LPEDICT ent = alloc_test_unit(class_id, x, y);
	ent->health.value = hp;
	ent->health.max_value = hp;
	ent->mana.value = mana;
	ent->mana.max_value = mana;
	ent->svflags |= SVF_MONSTER;
	ent->stand = unit_stand;
	ent->movetype = MOVETYPE_NONE;
	unit_stand(ent);
	return ent;
}

TEST(wc3_spell, relationship_uses_passive_alliance_not_other_flags) {
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 100, 0, 0);
	LPEDICT target = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 64, 0);
	caster->s.player = 0;
	target->s.player = 1;
	target->svflags |= SVF_MONSTER;
	target->health.value = target->health.max_value = 100;
	((LPMAPINFO)level.mapinfo)->players[0].playerType = kPlayerTypeHuman;
	((LPMAPINFO)level.mapinfo)->players[1].playerType = kPlayerTypeHuman;
	memset(level.alliances, 0, sizeof(level.alliances));

	memset(level.alliances, 0, sizeof(level.alliances));
	level.alliances[0][1] = 1 << ALLIANCE_SHARED_VISION;
	T_ASSERT(S_SpellIsEnemy(caster, target));
	T_ASSERT(!S_SpellIsFriend(caster, target));

	level.alliances[0][1] |= 1 << ALLIANCE_PASSIVE;
	T_ASSERT(!S_SpellIsEnemy(caster, target));
	T_ASSERT(S_SpellIsFriend(caster, target));

	target->s.player = PLAYER_NEUTRAL_AGGRESSIVE;
	T_ASSERT(S_SpellIsEnemy(caster, target));
	T_ASSERT(!S_SpellIsFriend(caster, target));
	G_SetPlayerAlliance(&game.clients[0].ps,
	                    &game.clients[PLAYER_NEUTRAL_AGGRESSIVE].ps,
	                    ALLIANCE_PASSIVE, true);
	T_ASSERT(!S_SpellIsEnemy(caster, target));
	T_ASSERT(S_SpellIsFriend(caster, target));

	target->s.player = PLAYER_NEUTRAL_PASSIVE;
	G_SetPlayerAlliance(&game.clients[0].ps,
	                    &game.clients[PLAYER_NEUTRAL_PASSIVE].ps,
	                    ALLIANCE_PASSIVE, true);
	T_ASSERT(!S_SpellIsEnemy(caster, target));
	T_ASSERT(S_SpellIsFriend(caster, target));
	G_SetPlayerAlliance(&game.clients[0].ps,
	                    &game.clients[PLAYER_NEUTRAL_PASSIVE].ps,
	                    ALLIANCE_PASSIVE, false);
	T_ASSERT(S_SpellIsEnemy(caster, target));
	T_ASSERT(!S_SpellIsFriend(caster, target));
}

/* ---- Channel enforcement ---- */

TEST(wc3_spell, channel_cancel_stun) {
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 0, 0, 0);
	caster->channel.code = MAKEFOURCC('A','H','b','z');
	caster->channel.origin = caster->s.origin2;
	caster->stunned = true;
	spell_run_frame(caster);
	T_EQ((int)caster->channel.code, 0);
}

TEST(wc3_spell, channel_cancel_death) {
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 0, 0, 0);
	caster->channel.code = MAKEFOURCC('A','H','b','z');
	caster->channel.origin = caster->s.origin2;
	caster->health.value = 0;
	spell_run_frame(caster);
	T_EQ((int)caster->channel.code, 0);
}

TEST(wc3_spell, channel_cancel_movement) {
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 0, 0, 0);
	caster->channel.code = MAKEFOURCC('A','H','b','z');
	caster->channel.origin.x = 100; caster->channel.origin.y = 100;
	caster->s.origin.x = 101; caster->s.origin.y = 100;
	caster->s.origin2.x = 101; caster->s.origin2.y = 100;
	spell_run_frame(caster);
	T_EQ((int)caster->channel.code, 0);
}

TEST(wc3_spell, channel_persists_no_movement) {
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 0, 0, 0);
	caster->channel.code = MAKEFOURCC('A','H','b','z');
	caster->channel.origin = caster->s.origin2;
	spell_run_frame(caster);
	T_EQ((int)caster->channel.code, (int)MAKEFOURCC('A','H','b','z'));
}

TEST(wc3_spell, channel_cancel_clears_code) {
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 0, 0, 0);
	caster->channel.code = MAKEFOURCC('A','H','b','z');
	S_SpellCancelChannel(caster);
	T_EQ((int)caster->channel.code, 0);
	T_STREQ(caster->currentmove->animation, "stand");
}

TEST(wc3_spell, channel_cancel_noop_empty) {
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 0, 0, 0);
	caster->channel.code = 0;
	S_SpellCancelChannel(caster);
	T_EQ((int)caster->channel.code, 0);
}

TEST(wc3_spell, channel_run_frame_noop_when_idle) {
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 0, 0, 0);
	caster->channel.code = 0;
	caster->stunned = true;
	spell_run_frame(caster);
	T_EQ((int)caster->channel.code, 0);
}

/* ---- Toggle bypass ---- */

TEST(wc3_spell, toggle_immolation_no_mana_spend) {
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 200, 0, 0);
	level.time = 1000;
	caster->mana.value = 150;
	ability_t const *abil = FindAbilityByClassname("AEim");
	T_NOT_NULL(abil); T_NOT_NULL(abil->spell);
	spellTarget_t st = { .type = SPELL_TARGET_NONE };
	abil->spell->execute(caster, st, abil->spell);
	T_FEQ(caster->mana.value, 150, 0.01f);
	T_EQ((int)caster->abilstatus[0].code, (int)MAKEFOURCC('B','i','m','l'));
}

/* ---- Spell helpers ---- */

TEST(wc3_spell, spell_code_to_string) {
	char buf[8];
	S_SpellCodeString(MAKEFOURCC('A','H','t','b'), buf);
	T_STREQ(buf, "AHtb");
}

TEST(wc3_spell, generic_data_id_keeps_rawcode_view) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y2;X4\n"
		"C;Y1;X1;K\"alias\"\n"
		"C;Y1;X2;K\"code\"\n"
		"C;Y1;X3;K\"DataA1\"\n"
		"C;Y1;X4;K\"DataB1\"\n"
		"C;Y2;X1;K\"Amil\"\n"
		"C;Y2;X2;K\"Amil\"\n"
		"C;Y2;X3;K\"hpea\"\n"
		"C;Y2;X4;K\"hmil\"\n"
		"E\n";
	slkTestData_t *rows = parse_slk_string(slk);
	slkTestData_t *old = G_SetSLKRows("AbilityData", rows);
	T_EQ((int)S_SpellDataId(MAKEFOURCC('A','m','i','l'), 1, 1), (int)MAKEFOURCC('h','p','e','a'));
	T_EQ((int)S_SpellDataId(MAKEFOURCC('A','m','i','l'), 1, 2), (int)MAKEFOURCC('h','m','i','l'));
	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}

TEST(wc3_spell, hero_passives_use_authored_data_and_runtime_consumers) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y7;X6\n"
		"C;Y1;X1;K\"alias\"\nC;Y1;X2;K\"code\"\nC;Y1;X3;K\"Area1\"\n"
		"C;Y1;X4;K\"DataA1\"\nC;Y1;X5;K\"DataB1\"\nC;Y1;X6;K\"DataC1\"\n"
		"C;Y2;X1;K\"AHab\"\nC;Y2;X2;K\"AHab\"\nC;Y2;X3;K\"900\"\nC;Y2;X4;K\"0.75\"\n"
		"C;Y3;X1;K\"AUau\"\nC;Y3;X2;K\"AUau\"\nC;Y3;X3;K\"900\"\nC;Y3;X4;K\"0.1\"\nC;Y3;X5;K\"0.5\"\n"
		"C;Y4;X1;K\"AUav\"\nC;Y4;X2;K\"AUav\"\nC;Y4;X3;K\"900\"\nC;Y4;X4;K\"0.2\"\n"
		"C;Y5;X1;K\"AOcr\"\nC;Y5;X2;K\"AOcr\"\nC;Y5;X4;K\"100\"\nC;Y5;X5;K\"2\"\n"
		"C;Y6;X1;K\"AEev\"\nC;Y6;X2;K\"AEev\"\nC;Y6;X4;K\"1\"\n"
		"C;Y7;X1;K\"AUts\"\nC;Y7;X2;K\"AUts\"\nC;Y7;X4;K\"0.15\"\nC;Y7;X5;K\"1\"\nC;Y7;X6;K\"3\"\nE\n";
	slkTestData_t *rows = parse_slk_string(slk), *old = G_SetSLKRows("AbilityData", rows);
	LPEDICT source = make_hero(MAKEFOURCC('H','a','m','g'), 500, 300, 0, 0);
	LPEDICT target = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 100, 0);
	source->s.player = target->s.player = 0;
	source->heroabilities[0] = MAKE(heroability_t, .code = MAKEFOURCC('A','H','a','b'), .level = 1);
	source->heroabilities[1] = MAKE(heroability_t, .code = MAKEFOURCC('A','U','a','u'), .level = 1);
	source->heroabilities[2] = MAKE(heroability_t, .code = MAKEFOURCC('A','U','a','v'), .level = 1);
	T_FEQ(S_BrillianceManaRegen(target), 0.75f, 0.001f);
	T_FEQ(S_UnholyMoveBonus(target), 0.1f, 0.001f);
	T_FEQ(S_UnholyHealthRegen(target), 0.5f, 0.001f);
	T_FEQ(S_VampiricLifeSteal(target), 0.2f, 0.001f);
	target->s.origin2.x = 901.0f;
	T_FEQ(S_BrillianceManaRegen(target), 0.0f, 0.001f);
	T_FEQ(S_UnholyMoveBonus(target), 0.0f, 0.001f);

	target->heroabilities[0] = MAKE(heroability_t, .code = MAKEFOURCC('A','O','c','r'), .level = 1);
	target->heroabilities[1] = MAKE(heroability_t, .code = MAKEFOURCC('A','E','e','v'), .level = 1);
	target->heroabilities[2] = MAKE(heroability_t, .code = MAKEFOURCC('A','U','t','s'), .level = 1);
	target->armor_value = 2.0f;
	T_EQ(S_CriticalStrikeDamage(target, 10), 20);
	T_ASSERT(S_EvasionRoll(target));
	T_FEQ(G_UnitArmorValue(target), 5.0f, 0.001f);
	T_FEQ(S_SpikedDamageReturn(target, 2.0f), 1.0f, 0.001f);
	T_FEQ(S_SpikedDamageReturn(target, 20.0f), 3.0f, 0.001f);
	memset(target->heroabilities, 0, sizeof(target->heroabilities));
	T_EQ(S_CriticalStrikeDamage(target, 10), 10);
	T_ASSERT(!S_EvasionRoll(target));
	T_FEQ(G_UnitArmorValue(target), 2.0f, 0.001f);

	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}

TEST(wc3_spell, thorns_aura_returns_authored_fraction_for_melee_hits) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y2;X4\n"
		"C;Y1;X1;K\"alias\"\nC;Y1;X2;K\"code\"\nC;Y1;X3;K\"Area1\"\nC;Y1;X4;K\"DataA1\"\n"
		"C;Y2;X1;K\"AEah\"\nC;Y2;X2;K\"AEah\"\nC;Y2;X3;K\"900\"\nC;Y2;X4;K\"0.1\"\nE\n";
	slkTestData_t *rows = parse_slk_string(slk), *old = G_SetSLKRows("AbilityData", rows);
	LPEDICT aura = make_hero(MAKEFOURCC('E', 'd', 'r', 'u'), 500, 0, 0, 0);
	LPEDICT target = alloc_test_unit(MAKEFOURCC('h', 'f', 'o', 'o'), 100, 0);
	LPEDICT attacker = alloc_test_unit(MAKEFOURCC('h', 'p', 'e', 'a'), 100, 0);
	aura->s.player = target->s.player = attacker->s.player = 0;
	aura->heroabilities[0] = MAKE(heroability_t, .code = MAKEFOURCC('A', 'E', 'a', 'h'), .level = 1);
	target->attack1.weapon = WPN_NORMAL;
	attacker->attack1.weapon = WPN_NORMAL;
	T_FEQ(S_ThornsDamageReturn(target, target, 100.0f), 10.0f, 0.001f);
	T_FEQ(S_ThornsDamageReturn(target, attacker, 100.0f), 10.0f, 0.001f);
	attacker->attack1.weapon = WPN_MISSILE;
	T_FEQ(S_ThornsDamageReturn(target, attacker, 100.0f), 0.0f, 0.001f);
	attacker->attack1.weapon = WPN_NORMAL;
	target->s.origin2.x = 901.0f;
	T_FEQ(S_ThornsDamageReturn(target, attacker, 100.0f), 0.0f, 0.001f);
	ability_t const *ability = FindAbilityByClassname("AEah");
	T_NOT_NULL(ability);
	T_ASSERT(ability->flags & ABILITY_PASSIVE);

	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}

TEST(wc3_spell, requested_thirty_have_concrete_handlers) {
	static LPCSTR const rawcodes[] = {
		"AHab", "AHmt", "ANst", "ANsg", "ANsq", "ANsw", "AOww", "AOcr", "AHbn", "AHfs",
		"AHdr", "AHpx", "AUcb", "AUim", "AUls", "AUts", "ANba", "ANsi", "AUan", "AUdc",
		"AUdp", "AUau", "AEev", "AEme", "AUsl", "AUav", "AUin", "AOcl", "AOeq", "AOfs",
	};
	static DWORD const passives[] = {
		MAKEFOURCC('A','H','a','b'), MAKEFOURCC('A','O','c','r'), MAKEFOURCC('A','U','t','s'),
		MAKEFOURCC('A','U','a','u'), MAKEFOURCC('A','E','e','v'), MAKEFOURCC('A','U','a','v'),
	};

	FOR_LOOP(i, sizeof(rawcodes) / sizeof(rawcodes[0])) {
		ability_t const *ability = FindAbilityByClassname(rawcodes[i]);
		DWORD code = MAKEFOURCC(rawcodes[i][0], rawcodes[i][1], rawcodes[i][2], rawcodes[i][3]);
		BOOL passive = false;
		FOR_LOOP(j, sizeof(passives) / sizeof(passives[0])) passive |= code == passives[j];
		T_NOT_NULL(ability);
		T_NE(ability, &a_unimplemented);
		if (passive) {
			T_ASSERT(ability->flags & ABILITY_PASSIVE);
		} else {
			T_NOT_NULL(ability->cmd);
			T_NOT_NULL(ability->spell);
			T_NOT_NULL(ability->spell->execute);
			T_EQ((int)ability->spell->code, (int)code);
		}
	}
}

TEST(wc3_spell, campaign_ability_rawcodes_are_registered_explicitly) {
	static LPCSTR const rawcodes[] = {
		"Aamk", "ACtn", "ANav", "ANsh", "AOw2", "ACs7", "ACs8", "ANr2", "Afbb", "Andm",
		"Asb1", "Asb2", "Asb3", "ANha", "ANen", "ACfu", "ANpa", "Acny", "Ahnl", "Arsq",
		"Arsg", "Arsp", "ANbr", "ANsb", "ANcf", "Acdh", "Acef", "ANhw", "ANhx", "Arsw",
		"AOs2", "AOr2", "AOr3", "AOls",
	};

	FOR_LOOP(i, sizeof(rawcodes) / sizeof(rawcodes[0])) {
		ability_t const *ability = FindAbilityByClassname(rawcodes[i]);
		DWORD code = MAKEFOURCC(rawcodes[i][0], rawcodes[i][1], rawcodes[i][2], rawcodes[i][3]);
		T_NOT_NULL(ability);
		if (!strcmp(rawcodes[i], "ANha")) T_EQ(ability, &a_harvest);
		else {
			T_NE(ability, &a_unimplemented);
			T_NOT_NULL(ability->cmd);
			T_NOT_NULL(ability->spell);
			T_NOT_NULL(ability->spell->execute);
			T_EQ((int)ability->spell->code, (int)code);
		}
	}
}

TEST(wc3_spell, requested_active_callback_families_change_simulation) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y6;X9\n"
		"C;Y1;X1;K\"alias\"\nC;Y1;X2;K\"code\"\nC;Y1;X3;K\"Area1\"\nC;Y1;X4;K\"Dur1\"\n"
		"C;Y1;X5;K\"HeroDur1\"\nC;Y1;X6;K\"DataA1\"\nC;Y1;X7;K\"DataB1\"\nC;Y1;X8;K\"DataC1\"\nC;Y1;X9;K\"BuffID1\"\n"
		"C;Y2;X1;K\"AHbn\"\nC;Y2;X2;K\"AHbn\"\nC;Y2;X4;K\"12\"\nC;Y2;X5;K\"4\"\nC;Y2;X9;K\"BHbn\"\n"
		"C;Y3;X1;K\"ANsi\"\nC;Y3;X2;K\"ANsi\"\nC;Y3;X3;K\"200\"\nC;Y3;X4;K\"16\"\nC;Y3;X5;K\"8\"\nC;Y3;X9;K\"BNsi\"\n"
		"C;Y4;X1;K\"AUsl\"\nC;Y4;X2;K\"AUsl\"\nC;Y4;X4;K\"15\"\nC;Y4;X5;K\"4\"\nC;Y4;X9;K\"BUsl\"\n"
		"C;Y5;X1;K\"AUdp\"\nC;Y5;X2;K\"AUdp\"\nC;Y5;X7;K\"2\"\n"
		"C;Y6;X1;K\"AOcl\"\nC;Y6;X2;K\"AOcl\"\nC;Y6;X3;K\"500\"\nC;Y6;X6;K\"100\"\nC;Y6;X7;K\"3\"\nC;Y6;X8;K\"0.5\"\nE\n";
	LPEDICT caster = make_hero(MAKEFOURCC('u','d','e','a'), 200, 300, 0, 0);
	LPEDICT first = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 50, 0);
	LPEDICT second = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 100, 0);
	LPEDICT third = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 700, 0);
	slkTestData_t *rows = parse_slk_string(slk), *old = G_SetSLKRows("AbilityData", rows);
	caster->s.player = 0; caster->health.max_value = 500; caster->health.value = 100;
	first->s.player = second->s.player = third->s.player = 1;
	first->svflags |= SVF_MONSTER; second->svflags |= SVF_MONSTER; third->svflags |= SVF_MONSTER;
	first->die = unit_die; second->die = unit_die; third->die = unit_die;
	first->health.value = first->health.max_value = 500;
	second->health.value = second->health.max_value = 500;
	third->health.value = third->health.max_value = 500;
	((LPMAPINFO)level.mapinfo)->players[0].playerType = kPlayerTypeHuman;
	((LPMAPINFO)level.mapinfo)->players[1].playerType = kPlayerTypeHuman;

	FindAbilityByClassname("AHbn")->spell->execute(caster, MAKE(spellTarget_t, .type = SPELL_TARGET_UNIT, .entity = first), FindAbilityByClassname("AHbn")->spell);
	T_ASSERT(S_UnitHasStatus(first, MAKEFOURCC('B','H','b','n')));
	FindAbilityByClassname("ANsi")->spell->execute(caster, MAKE(spellTarget_t, .type = SPELL_TARGET_POINT, .point = first->s.origin2), FindAbilityByClassname("ANsi")->spell);
	T_ASSERT(S_UnitHasStatus(first, MAKEFOURCC('B','N','s','i')));
	T_ASSERT(S_UnitHasStatus(second, MAKEFOURCC('B','N','s','i')));
	T_ASSERT(!S_UnitHasStatus(third, MAKEFOURCC('B','N','s','i')));

	FindAbilityByClassname("AUsl")->spell->execute(caster, MAKE(spellTarget_t, .type = SPELL_TARGET_UNIT, .entity = first), FindAbilityByClassname("AUsl")->spell);
	T_ASSERT(S_UnitHasStatus(first, MAKEFOURCC('B','U','s','l')));
	T_Damage(first, caster, 1);
	T_ASSERT(!S_UnitHasStatus(first, MAKEFOURCC('B','U','s','l')));

	FindAbilityByClassname("AUdp")->spell->execute(caster, MAKE(spellTarget_t, .type = SPELL_TARGET_UNIT, .entity = third), FindAbilityByClassname("AUdp")->spell);
	T_FEQ(caster->health.value, 500.0f, 0.001f);
	T_ASSERT(M_IsDead(third));

	FindAbilityByClassname("AOcl")->spell->execute(caster, MAKE(spellTarget_t, .type = SPELL_TARGET_UNIT, .entity = first), FindAbilityByClassname("AOcl")->spell);
	T_FEQ(first->health.value, 399.0f, 0.001f);
	T_FEQ(second->health.value, 450.0f, 0.001f);

	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}


TEST(wc3_spell, militia_zero_pair_area_means_unbounded_search) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y3;X3\n"
		"C;Y1;X1;K\"alias\"\n"
		"C;Y1;X2;K\"code\"\n"
		"C;Y1;X3;K\"Area1\"\n"
		"C;Y2;X1;K\"Amil\"\n"
		"C;Y2;X2;K\"Amil\"\n"
		"C;Y2;X3;K\"0\"\n"
		"C;Y3;X1;K\"Amic\"\n"
		"C;Y3;X2;K\"Amic\"\n"
		"C;Y3;X3;K\"600\"\n"
		"E\n";
	slkTestData_t *rows = parse_slk_string(slk);
	slkTestData_t *old = G_SetSLKRows("AbilityData", rows);

	T_FEQ(S_MilitiaPairSearchRadius(MAKEFOURCC('A','m','i','l')), FLT_MAX, 1.0f);
	T_FEQ(S_MilitiaPairSearchRadius(MAKEFOURCC('A','m','i','c')), 600.0f, 0.01f);

	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}

TEST(wc3_spell, spell_unit_id_from_slk) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y2;X3\n"
		"C;Y1;X1;K\"alias\"\n"
		"C;Y1;X2;K\"code\"\n"
		"C;Y1;X3;K\"UnitID1\"\n"
		"C;Y2;X1;K\"AHwe\"\n"
		"C;Y2;X2;K\"AHwe\"\n"
		"C;Y2;X3;K\"hwat\"\n"
		"E\n";
	slkTestData_t *rows = parse_slk_string(slk);
	slkTestData_t *old = G_SetSLKRows("AbilityData", rows);
	T_EQ((int)S_SpellUnitId(MAKEFOURCC('A','H','w','e'), 1), (int)MAKEFOURCC('h','w','a','t'));
	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}

TEST(wc3_spell, hero_duration_uses_herodur_col) {
	slkTestData_t *rows = parse_slk_string(slk_spell_data);
	slkTestData_t *old = G_SetSLKRows("AbilityData", rows);
	T_FEQ(S_SpellDuration(MAKEFOURCC('A','H','t','b'), 1, true), 3.0f, 0.01f);
	T_FEQ(S_SpellDuration(MAKEFOURCC('A','H','t','b'), 1, false), 5.0f, 0.01f);
	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}

TEST(wc3_spell, spell_is_channeling_detects_active) {
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 0, 0, 0);
	T_ASSERT(!S_SpellIsChanneling(caster));
	caster->channel.code = MAKEFOURCC('A','H','b','z');
	T_ASSERT(S_SpellIsChanneling(caster));
	caster->channel.code = 0;
	T_ASSERT(!S_SpellIsChanneling(caster));
}

TEST(wc3_spell, mirror_image_immediate_order_spawns_summoned_illusion) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y2;X6\n"
		"C;Y1;X1;K\"alias\"\n"
		"C;Y1;X2;K\"code\"\n"
		"C;Y1;X3;K\"Cost1\"\n"
		"C;Y1;X4;K\"Cool1\"\n"
		"C;Y1;X5;K\"Dur1\"\n"
		"C;Y1;X6;K\"DataA1\"\n"
		"C;Y2;X1;K\"AOmi\"\n"
		"C;Y2;X2;K\"AOmi\"\n"
		"C;Y2;X3;K\"0\"\n"
		"C;Y2;X4;K\"0\"\n"
		"C;Y2;X5;K\"0\"\n"
		"C;Y2;X6;K\"1\"\n"
		"E\n";
	slkTestData_t *rows = parse_slk_string(slk);
	slkTestData_t *old = G_SetSLKRows("AbilityData", rows);
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 100, 0, 0);
	LPEDICT image;
	DWORD before;

	caster->heroabilities[0].code = MAKEFOURCC('A','O','m','i');
	caster->heroabilities[0].level = 1;
	caster->hero.level = 3;
	memset(&level.events, 0, sizeof(level.events));
	before = globals.num_edicts;

	T_ASSERT(unit_issueimmediateorder(caster, "mirrorimage"));
	T_EQ((int)globals.num_edicts, (int)before + 1);
	image = &globals.edicts[before];
	T_ASSERT(image->inuse);
	T_ASSERT(image != caster);
	T_EQ((int)image->class_id, (int)caster->class_id);
	T_ASSERT(image->aiflags & AI_ILLUSION);
	T_EQ((int)image->hero.level, (int)caster->hero.level);
	T_ASSERT(image->hero.suspend_xp);
	T_EQ((int)level.events.queue[0].type, (int)EVENT_PLAYER_UNIT_SUMMON);
	T_ASSERT(level.events.queue[0].edict == caster);
	T_ASSERT(level.events.queue[0].source == image);
	T_EQ((int)level.events.queue[1].type, (int)EVENT_UNIT_SUMMON);

	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}

/* ---- spell_info_t registration ---- */

TEST(wc3_spell, spell_info_attached_to_ability) {
	ability_t const *abil = FindAbilityByClassname("AHtb");
	T_NOT_NULL(abil);
	T_NOT_NULL(abil->spell);
	T_EQ((int)abil->spell->code, (int)MAKEFOURCC('A','H','t','b'));
	T_EQ((int)abil->spell->target_type, (int)SPELL_TARGET_UNIT);
}

TEST(wc3_spell, forked_lightning_bounces_without_damage_decay) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y2;X6\n"
		"C;Y1;X1;K\"alias\"\nC;Y1;X2;K\"code\"\nC;Y1;X3;K\"DataA1\"\n"
		"C;Y1;X4;K\"DataB1\"\nC;Y1;X5;K\"Area1\"\nC;Y1;X6;K\"Dur1\"\n"
		"C;Y2;X1;K\"ANfl\"\nC;Y2;X2;K\"ANfl\"\nC;Y2;X3;K\"85\"\n"
		"C;Y2;X4;K\"3\"\nC;Y2;X5;K\"125\"\nC;Y2;X6;K\"0.7\"\nE\n";
	slkTestData_t *rows = parse_slk_string(slk), *old = G_SetSLKRows("AbilityData", rows);
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 100, 0, 0);
	LPEDICT first = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 100, 0);
	LPEDICT second = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 200, 0);
	LPEDICT third = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 300, 0);
	LPEDICT fourth = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 400, 0);
	spell_info_t const *spell = FindAbilityByClassname("ANfl")->spell;
	spellTarget_t st = { .type = SPELL_TARGET_UNIT, .entity = first };

	caster->s.player = 0;
	caster->heroabilities[0] = MAKE(heroability_t, .code = MAKEFOURCC('A','N','f','l'), .level = 1);
	first->s.player = second->s.player = third->s.player = fourth->s.player = 1;
	    first->svflags |= SVF_MONSTER;
	    second->svflags |= SVF_MONSTER;
	    third->svflags |= SVF_MONSTER;
	    fourth->svflags |= SVF_MONSTER;
	((LPMAPINFO)level.mapinfo)->players[0].playerType = kPlayerTypeHuman;
	((LPMAPINFO)level.mapinfo)->players[1].playerType = kPlayerTypeHuman;
	first->health.value = second->health.value = third->health.value = fourth->health.value = 100;
	spell->execute(caster, st, spell);
	T_FEQ(first->health.value, 15.0f, 0.01f);
	T_FEQ(second->health.value, 15.0f, 0.01f);
	T_FEQ(third->health.value, 15.0f, 0.01f);
	T_FEQ(fourth->health.value, 100.0f, 0.01f);

	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}

TEST(wc3_spell, first_new_ability_handlers_are_real_spells) {
	ability_t const *force = FindAbilityByClassname("AEfn");
	ability_t const *starfall = FindAbilityByClassname("AEsf");
	ability_t const *shockwave = FindAbilityByClassname("AOsh");
	ability_t const *rain_of_fire = FindAbilityByClassname("ANrf");
	ability_t const *tranquility = FindAbilityByClassname("AEtq");
	ability_t const *dark_ritual = FindAbilityByClassname("AUdr");
	ability_t const *frost_armor = FindAbilityByClassname("AUfa");
	ability_t const *frost_armor_variant = FindAbilityByClassname("AUfu");
	ability_t const *divine_shield = FindAbilityByClassname("AHds");
	ability_t const *death_and_decay = FindAbilityByClassname("AUdd");
	ability_t const *frost_nova = FindAbilityByClassname("AUfn");
	ability_t const *thunder_clap = FindAbilityByClassname("AHtc");

	T_NOT_NULL(force);
	T_NOT_NULL(force->spell);
	T_EQ((int)force->spell->code, (int)MAKEFOURCC('A', 'E', 'f', 'n'));
	T_NOT_NULL(starfall);
	T_NOT_NULL(starfall->spell);
	T_EQ((int)starfall->spell->code, (int)MAKEFOURCC('A', 'E', 's', 'f'));
	T_ASSERT(starfall->spell->flags & SPELL_CHANNEL);
	T_NOT_NULL(shockwave);
	T_NOT_NULL(shockwave->spell);
	T_EQ((int)shockwave->spell->code, (int)MAKEFOURCC('A', 'O', 's', 'h'));
	T_NOT_NULL(rain_of_fire);
	T_NOT_NULL(rain_of_fire->spell);
	T_EQ((int)rain_of_fire->spell->code, (int)MAKEFOURCC('A', 'N', 'r', 'f'));
	T_ASSERT(rain_of_fire->spell->flags & SPELL_CHANNEL);
	T_NOT_NULL(tranquility);
	T_NOT_NULL(tranquility->spell);
	T_EQ((int)tranquility->spell->code, (int)MAKEFOURCC('A', 'E', 't', 'q'));
	T_ASSERT(tranquility->spell->flags & SPELL_CHANNEL);
	T_NOT_NULL(dark_ritual);
	T_NOT_NULL(dark_ritual->spell);
	T_EQ((int)dark_ritual->spell->code, (int)MAKEFOURCC('A', 'U', 'd', 'r'));
	T_EQ((int)dark_ritual->spell->target_type, (int)SPELL_TARGET_UNIT);
	T_NOT_NULL(frost_armor);
	T_NOT_NULL(frost_armor->spell);
	T_EQ((int)frost_armor->spell->code, (int)MAKEFOURCC('A', 'U', 'f', 'a'));
	T_NOT_NULL(frost_armor_variant);
	T_NOT_NULL(frost_armor_variant->spell);
	T_EQ((int)frost_armor_variant->spell->code, (int)MAKEFOURCC('A', 'U', 'f', 'u'));
	T_NOT_NULL(divine_shield);
	T_NOT_NULL(divine_shield->spell);
	T_EQ((int)divine_shield->spell->code, (int)MAKEFOURCC('A', 'H', 'd', 's'));
	T_EQ((int)divine_shield->spell->target_type, (int)SPELL_TARGET_NONE);
	T_NOT_NULL(death_and_decay);
	T_NOT_NULL(death_and_decay->spell);
	T_EQ((int)death_and_decay->spell->code, (int)MAKEFOURCC('A', 'U', 'd', 'd'));
	T_ASSERT(death_and_decay->spell->flags & SPELL_CHANNEL);
	T_NOT_NULL(frost_nova);
	T_NOT_NULL(frost_nova->spell);
	T_EQ((int)frost_nova->spell->code, (int)MAKEFOURCC('A', 'U', 'f', 'n'));
	T_NOT_NULL(thunder_clap);
	T_NOT_NULL(thunder_clap->spell);
	T_EQ((int)thunder_clap->spell->code, (int)MAKEFOURCC('A', 'H', 't', 'c'));
}

TEST(wc3_spell, tornado_uses_whirlwind_channel_handler) {
	ability_t const *tornado = FindAbilityByClassname("ANto");

	T_NOT_NULL(tornado);
	T_NOT_NULL(tornado->spell);
	T_NOT_NULL(tornado->spell->execute);
	T_EQ((int)tornado->spell->code, (int)MAKEFOURCC('A', 'N', 't', 'o'));
	T_EQ((int)tornado->spell->target_type, (int)SPELL_TARGET_NONE);
	T_ASSERT(tornado->spell->flags & SPELL_CHANNEL);
}

TEST(wc3_spell, requested_neutral_hero_abilities_have_contracts) {
	ability_t const *mana_shield = FindAbilityByClassname("ANms");
	ability_t const *revive = FindAbilityByClassname("AHre");
	ability_t const *breath = FindAbilityByClassname("ANbf");
	ability_t const *brawler = FindAbilityByClassname("ANdb");
	ability_t const *haze = FindAbilityByClassname("ANdh");
	ability_t const *doom = FindAbilityByClassname("ANdo");
	ability_t const *howl = FindAbilityByClassname("ANht");
	ability_t const *cleave = FindAbilityByClassname("ANca");

	T_NOT_NULL(mana_shield);
	T_NOT_NULL(revive);
	T_NOT_NULL(breath);
	T_NOT_NULL(brawler);
	T_NOT_NULL(haze);
	T_NOT_NULL(doom);
	T_NOT_NULL(howl);
	T_NOT_NULL(cleave);
	T_ASSERT(mana_shield->flags & ABILITY_PASSIVE);
	T_ASSERT(brawler->flags & ABILITY_PASSIVE);
	T_ASSERT(cleave->flags & ABILITY_PASSIVE);
	T_EQ((int)revive->spell->target_type, (int)SPELL_TARGET_POINT);
	T_EQ((int)breath->spell->target_type, (int)SPELL_TARGET_POINT);
	T_EQ((int)haze->spell->target_type, (int)SPELL_TARGET_UNIT);
	T_EQ((int)doom->spell->target_type, (int)SPELL_TARGET_UNIT);
	T_EQ((int)howl->spell->target_type, (int)SPELL_TARGET_NONE);
}

TEST(wc3_spell, selected_hero_ability_contracts_are_registered) {
	ability_t const *searing = FindAbilityByClassname("AHfa");
	ability_t const *trueshot = FindAbilityByClassname("AEar");
	ability_t const *reincarnation = FindAbilityByClassname("AOre");
	ability_t const *wave = FindAbilityByClassname("AOhw");
	ability_t const *hex = FindAbilityByClassname("AOhx");
	ability_t const *voodoo = FindAbilityByClassname("AOvd");
	ability_t const *vengeance = FindAbilityByClassname("AEsv");
	ability_t const *acid = FindAbilityByClassname("ANab");

	T_NOT_NULL(searing); T_NOT_NULL(trueshot); T_NOT_NULL(reincarnation); T_NOT_NULL(wave);
	T_NOT_NULL(hex); T_NOT_NULL(voodoo); T_NOT_NULL(vengeance); T_NOT_NULL(acid);
	T_ASSERT(searing->spell->flags & SPELL_TOGGLE);
	T_ASSERT(trueshot->flags & ABILITY_PASSIVE);
	T_ASSERT(reincarnation->flags & ABILITY_PASSIVE);
	T_EQ((int)wave->spell->target_type, (int)SPELL_TARGET_UNIT);
	T_EQ((int)hex->spell->target_type, (int)SPELL_TARGET_UNIT);
	T_EQ((int)voodoo->spell->target_type, (int)SPELL_TARGET_NONE);
	T_EQ((int)vengeance->spell->target_type, (int)SPELL_TARGET_NONE);
	T_EQ((int)acid->spell->target_type, (int)SPELL_TARGET_UNIT);
}

TEST(wc3_spell, human_ability_rawcodes_have_concrete_contracts) {
	static LPCSTR const spells[] = {
		"Amls", "Acmg", "Amdf", "Asps", "Aclf", "Adef", "Afla", "Ainf", "Adis", "Ahea", "Aslo", "Aivs", "Aply", "AHav",
	};
	static LPCSTR const passives[] = {
		"Afbk", "Aflk", "Afsh", "Aroc", "Asph", "Aphx", "Agyb", "Asth", "Agyv", "Adts",
	};

	FOR_LOOP(i, sizeof(spells) / sizeof(spells[0])) {
		ability_t const *ability = FindAbilityByClassname(spells[i]);
		T_NOT_NULL(ability); T_NE(ability, &a_unimplemented); T_NOT_NULL(ability->cmd);
		T_NOT_NULL(ability->spell); T_NOT_NULL(ability->spell->execute);
		T_EQ((int)ability->spell->code, (int)MAKEFOURCC(spells[i][0], spells[i][1], spells[i][2], spells[i][3]));
	}
	FOR_LOOP(i, sizeof(passives) / sizeof(passives[0])) {
		ability_t const *ability = FindAbilityByClassname(passives[i]);
		T_NOT_NULL(ability); T_NE(ability, &a_unimplemented); T_ASSERT(ability->flags & ABILITY_PASSIVE);
	}
}

TEST(wc3_spell, human_support_spells_use_authored_status_and_heal_values) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y4;X9\n"
		"C;Y1;X1;K\"alias\"\nC;Y1;X2;K\"code\"\nC;Y1;X3;K\"targs\"\nC;Y1;X4;K\"Rng1\"\n"
		"C;Y1;X5;K\"Dur1\"\nC;Y1;X6;K\"HeroDur1\"\nC;Y1;X7;K\"DataA1\"\nC;Y1;X8;K\"DataB1\"\nC;Y1;X9;K\"BuffID1\"\n"
		"C;Y2;X1;K\"Ahea\"\nC;Y2;X2;K\"Ahea\"\nC;Y2;X3;K\"ground,friend\"\nC;Y2;X4;K\"250\"\nC;Y2;X7;K\"25\"\nC;Y2;X9;K\"Bhea\"\n"
		"C;Y3;X1;K\"Ainf\"\nC;Y3;X2;K\"Ainf\"\nC;Y3;X3;K\"ground,friend\"\nC;Y3;X4;K\"500\"\nC;Y3;X5;K\"60\"\nC;Y3;X6;K\"60\"\nC;Y3;X7;K\"0.1\"\nC;Y3;X8;K\"5\"\nC;Y3;X9;K\"Binf\"\n"
		"C;Y4;X1;K\"Aslo\"\nC;Y4;X2;K\"Aslo\"\nC;Y4;X3;K\"ground,enemy\"\nC;Y4;X4;K\"700\"\nC;Y4;X5;K\"60\"\nC;Y4;X6;K\"10\"\nC;Y4;X7;K\"0.6\"\nC;Y4;X8;K\"0.25\"\nC;Y4;X9;K\"Bslo\"\nE\n";
	slkTestData_t *rows = parse_slk_string(slk), *old = G_SetSLKRows("AbilityData", rows);
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','r','i'), 300, 300, 0, 0);
	LPEDICT ally = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 50, 0);
	LPEDICT enemy = alloc_test_unit(MAKEFOURCC('o','g','r','u'), 100, 0);
	ability_t const *heal = FindAbilityByClassname("Ahea"), *inner = FindAbilityByClassname("Ainf"), *slow = FindAbilityByClassname("Aslo");
	caster->s.player = ally->s.player = 0; enemy->s.player = 1;
	ally->health.value = 60; ally->health.max_value = 100; ally->armor_value = 2;
	ally->svflags |= SVF_MONSTER; enemy->svflags |= SVF_MONSTER;
	heal->spell->execute(caster, MAKE(spellTarget_t, .type = SPELL_TARGET_UNIT, .entity = ally), heal->spell);
	T_FEQ(ally->health.value, 85.0f, 0.001f);
	inner->spell->execute(caster, MAKE(spellTarget_t, .type = SPELL_TARGET_UNIT, .entity = ally), inner->spell);
	T_ASSERT(S_UnitHasStatus(ally, MAKEFOURCC('B','i','n','f'))); T_FEQ(G_UnitArmorValue(ally), 7.0f, 0.001f);
	slow->spell->execute(caster, MAKE(spellTarget_t, .type = SPELL_TARGET_UNIT, .entity = enemy), slow->spell);
	T_ASSERT(S_UnitHasStatus(enemy, MAKEFOURCC('B','s','l','o'))); T_FEQ(S_HumanMoveFactor(enemy), 0.4f, 0.001f);

	G_SetSLKRows("AbilityData", old); free_slk_rows(rows);
}

TEST(wc3_spell, human_attack_passives_and_defend_change_damage) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y3;X10\n"
		"C;Y1;X1;K\"alias\"\nC;Y1;X2;K\"code\"\nC;Y1;X3;K\"DataA1\"\nC;Y1;X4;K\"DataB1\"\n"
		"C;Y1;X5;K\"DataC1\"\nC;Y1;X6;K\"DataD1\"\nC;Y1;X7;K\"DataE1\"\nC;Y1;X8;K\"DataF1\"\nC;Y1;X9;K\"DataG1\"\nC;Y1;X10;K\"DataH1\"\n"
		"C;Y2;X1;K\"Afbk\"\nC;Y2;X2;K\"Afbk\"\nC;Y2;X3;K\"20\"\nC;Y2;X4;K\"1\"\nC;Y2;X5;K\"4\"\nC;Y2;X6;K\"1\"\n"
		"C;Y3;X1;K\"Adef\"\nC;Y3;X2;K\"Adef\"\nC;Y3;X3;K\"0.5\"\nC;Y3;X4;K\"1\"\nC;Y3;X5;K\"0.3\"\nC;Y3;X8;K\"0\"\nE\n";
	slkTestData_t *rows = parse_slk_string(slk), *old = G_SetSLKRows("AbilityData", rows);
	LPEDICT attacker = make_hero(MAKEFOURCC('h','b','r','e'), 300, 100, 0, 0);
	LPEDICT target = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 50, 0);
	attacker->heroabilities[0] = MAKE(heroability_t, .code = MAKEFOURCC('A','f','b','k'), .level = 1);
	target->mana.value = target->mana.max_value = 50; attacker->attack1.type = ATK_NORMAL;
	T_EQ(S_HumanAttackDamage(attacker, target, 10), 30); T_FEQ(target->mana.value, 30.0f, 0.001f);
	unit_addstatus(target, "Adef", 1); attacker->attack1.type = ATK_PIERCE;
	T_EQ(S_HumanAttackDamage(attacker, target, 100), 60);

	G_SetSLKRows("AbilityData", old); free_slk_rows(rows);
}

TEST(wc3_spell, defend_fractional_chance_can_guarantee_reflection) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y2;X8\n"
		"C;Y1;X1;K\"alias\"\nC;Y1;X2;K\"code\"\nC;Y1;X3;K\"DataA1\"\nC;Y1;X4;K\"DataB1\"\n"
		"C;Y1;X5;K\"DataC1\"\nC;Y1;X6;K\"DataD1\"\nC;Y1;X7;K\"DataE1\"\nC;Y1;X8;K\"DataF1\"\n"
		"C;Y2;X1;K\"Adef\"\nC;Y2;X2;K\"Adef\"\nC;Y2;X3;K\"0.5\"\nC;Y2;X4;K\"1\"\nC;Y2;X5;K\"0.3\"\nC;Y2;X8;K\"1\"\nE\n";
	slkTestData_t *rows = parse_slk_string(slk), *old = G_SetSLKRows("AbilityData", rows);
	LPEDICT attacker = make_hero(MAKEFOURCC('h','b','r','e'), 300, 100, 0, 0);
	LPEDICT target = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 50, 0);
	attacker->health.value = 200; attacker->attack1.type = ATK_PIERCE; unit_addstatus(target, "Adef", 1);
	T_EQ(S_HumanAttackDamage(attacker, target, 100), 0); T_FEQ(attacker->health.value, 100.0f, 0.001f);
	G_SetSLKRows("AbilityData", old); free_slk_rows(rows);
}

TEST(wc3_spell, selected_common_ability_contracts_are_registered) {
	static LPCSTR const passives[] = {
		"Abdt", "Arev", "Aawa", "Adet", "AHer", "Aalr", "Afih", "Afin", "Afio", "Afir", "Afiu", "Aloc", "Attu",
	};
	static struct { LPCSTR code; ability_t const *ability; } const commands[] = {
		{ "AEbu", &a_build }, { "AGbu", &a_build }, { "AHbu", &a_build }, { "ANbu", &a_build },
		{ "AObu", &a_build }, { "ARal", &a_rally }, { "AUbu", &a_build }, { "Aatk", &a_attack },
		{ "Amov", &a_move }, { "Atdp", &a_drop }, { "Atlp", &a_load },
	};
	ability_t const *poison = FindAbilityByClassname("AEpa");

	FOR_LOOP(i, sizeof(passives) / sizeof(passives[0])) {
		ability_t const *ability = FindAbilityByClassname(passives[i]);
		T_NOT_NULL(ability);
		T_ASSERT(ability->flags & ABILITY_PASSIVE);
	}
	FOR_LOOP(i, sizeof(commands) / sizeof(commands[0]))
		T_EQ(FindAbilityByClassname(commands[i].code), commands[i].ability);
	T_EQ(FindAbilityByClassname("Afih"), &a_on_fire);
	T_EQ(FindAbilityByClassname("Afin"), &a_on_fire);
	T_EQ(FindAbilityByClassname("Afio"), &a_on_fire);
	T_EQ(FindAbilityByClassname("Afir"), &a_on_fire);
	T_EQ(FindAbilityByClassname("Afiu"), &a_on_fire);
	T_EQ(a_on_fire.level_changed, FindAbilityByClassname("Afir")->level_changed);
	T_NOT_NULL(a_on_fire.level);
	T_NOT_NULL(a_on_fire.level_changed);
	T_NOT_NULL(poison);
	T_NOT_NULL(poison->spell);
	T_EQ((int)poison->spell->code, (int)MAKEFOURCC('A', 'E', 'p', 'a'));
	T_ASSERT(poison->spell->flags & SPELL_TOGGLE);
	T_ASSERT(poison->spell->flags & SPELL_AUTOCAST);
}

TEST(wc3_spell, intrinsic_on_fire_level_zero_clears_effect) {
	edict_t building = {0};

	building.inuse = true;
	building.s.effect = 77;
	building.s.effect_flags = EFX_MODEL;
	building.s.flags = EF_BUILDING;
	building.health.value = building.health.max_value = 1000.0f;
	S_RefreshAbilityLevel(&building, &a_on_fire);
	T_EQ(building.s.effect, 0);
	T_EQ(building.s.effect_flags, 0);
}

TEST(wc3_spell, runtime_ability_membership_calls_enable_and_disable) {
	UnitAbilities_t abilities = { .abilList = "Afir" };
	edict_t unit = { .inuse = true, .s.effect = 77, .s.effect_flags = EFX_MODEL };
	DWORD const code = MAKEFOURCC('A', 'f', 'i', 'r');

	unit.data.UnitAbilities = &abilities;
	T_ASSERT(G_ActorRemoveSkill(&unit, code));
	T_EQ(unit.s.effect, 0);
	T_EQ(unit.s.effect_flags, 0);
	unit.s.effect = 77; unit.s.effect_flags = EFX_MODEL;
	T_ASSERT(G_ActorAddSkill(&unit, code));
	T_EQ(unit.s.effect, 0);
	T_EQ(unit.s.effect_flags, 0);
}

TEST(wc3_spell, poison_arrows_uses_its_own_authored_bonus_damage) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y2;X3\n"
		"C;Y1;X1;K\"alias\"\nC;Y1;X2;K\"code\"\nC;Y1;X3;K\"DataA1\"\n"
		"C;Y2;X1;K\"AEpa\"\nC;Y2;X2;K\"AEpa\"\nC;Y2;X3;K\"13\"\nE\n";
	slkTestData_t *rows = parse_slk_string(slk), *old = G_SetSLKRows("AbilityData", rows);
	LPEDICT attacker = make_hero(MAKEFOURCC('N', 'n', 's', 'w'), 100, 100, 0, 0);

	attacker->attack1.weapon = WPN_MISSILE;
	unit_addstatus(attacker, "AEpa", 1);
	T_EQ(S_SearingArrowDamage(attacker, 20), 33);
	attacker->attack1.weapon = WPN_NORMAL;
	T_EQ(S_SearingArrowDamage(attacker, 20), 20);

	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}

TEST(wc3_spell, mana_shield_consumes_authored_mana_before_life) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y2;X4\n"
		"C;Y1;X1;K\"alias\"\nC;Y1;X2;K\"code\"\nC;Y1;X3;K\"DataA1\"\nC;Y1;X4;K\"Area1\"\n"
		"C;Y2;X1;K\"ANms\"\nC;Y2;X2;K\"ANms\"\nC;Y2;X3;K\"2\"\nC;Y2;X4;K\"128\"\nE\n";
	slkTestData_t *rows = parse_slk_string(slk), *old = G_SetSLKRows("AbilityData", rows);
	LPEDICT caster = make_hero(MAKEFOURCC('h', 'p', 'e', 'a'), 100, 10, 0, 0);
	caster->heroabilities[0] = MAKE(heroability_t, .code = MAKEFOURCC('A', 'N', 'm', 's'), .level = 1);
	T_EQ(S_ManaShieldDamage(caster, 4), 0);
	T_FEQ(caster->mana.value, 2.0f, 0.01f);
	T_EQ(S_ManaShieldDamage(caster, 4), 3);
	T_FEQ(caster->mana.value, 0.0f, 0.01f);
	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}

TEST(wc3_spell, beastmaster_summons_use_force_of_nature_contract) {
	ability_t const *bear = FindAbilityByClassname("ANsg");
	ability_t const *quilbeast = FindAbilityByClassname("ANsq");
	ability_t const *hawk = FindAbilityByClassname("ANsw");

	T_ASSERT(bear == &a_summon_bear);
	T_ASSERT(quilbeast == &a_summon_quilbeast);
	T_ASSERT(hawk == &a_summon_hawk);
	T_EQ((int)bear->spell->code, (int)MAKEFOURCC('A', 'N', 's', 'g'));
	T_EQ((int)quilbeast->spell->code, (int)MAKEFOURCC('A', 'N', 's', 'q'));
	T_EQ((int)hawk->spell->code, (int)MAKEFOURCC('A', 'N', 's', 'w'));
	T_ASSERT(bear->spell->execute == quilbeast->spell->execute);
	T_ASSERT(bear->spell->execute == hawk->spell->execute);
}

TEST(wc3_spell, entangling_roots_is_a_timed_unit_spell) {
	ability_t const *roots = FindAbilityByClassname("AEer");

	T_NOT_NULL(roots);
	T_NOT_NULL(roots->spell);
	T_EQ((int)roots->spell->code, (int)MAKEFOURCC('A', 'E', 'e', 'r'));
	T_EQ((int)roots->spell->target_type, (int)SPELL_TARGET_UNIT);
}

TEST(wc3_spell, death_and_decay_uses_percentage_damage_and_enemy_filter) {
	const char slk[] =
		"ID;PWXL;N;EBB;Y2;X6\n"
		"C;Y1;X1;K\"alias\"\n"
		"C;Y1;X2;K\"code\"\n"
		"C;Y1;X3;K\"Dur1\"\n"
		"C;Y1;X4;K\"DataA1\"\n"
		"C;Y1;X5;K\"DataB1\"\n"
		"C;Y1;X6;K\"Area1\"\n"
		"C;Y2;X1;K\"AUdd\"\n"
		"C;Y2;X2;K\"AUdd\"\n"
		"C;Y2;X3;K\"2\"\n"
		"C;Y2;X4;K\".04\"\n"
		"C;Y2;X5;K\"1\"\n"
		"C;Y2;X6;K\"128\"\n"
		"E\n";
	slkTestData_t *rows = parse_slk_string(slk);
	slkTestData_t *old = G_SetSLKRows("AbilityData", rows);
	LPEDICT caster = make_hero(MAKEFOURCC('h','p','e','a'), 250, 100, 0, 0);
	LPEDICT enemy = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 32, 0);
	LPEDICT thinker;
	DWORD thinker_slot = globals.num_edicts;
	spellTarget_t st = { .type = SPELL_TARGET_POINT, .point = caster->s.origin2 };

	caster->s.player = 0;
	enemy->s.player = 1;
	enemy->svflags |= SVF_MONSTER;
	enemy->health.value = enemy->health.max_value = 100;
	((LPMAPINFO)level.mapinfo)->players[0].playerType = kPlayerTypeHuman;
	((LPMAPINFO)level.mapinfo)->players[1].playerType = kPlayerTypeHuman;
	memset(level.alliances, 0, sizeof(level.alliances));
	FindAbilityByClassname("AUdd")->spell->execute(caster, st, FindAbilityByClassname("AUdd")->spell);
	thinker = &globals.edicts[thinker_slot];
	T_FEQ(enemy->health.value, 96.0f, 0.01f);
	T_FEQ(caster->health.value, 250.0f, 0.01f);
	T_ASSERT(thinker->inuse);

	G_SetSLKRows("AbilityData", old);
	free_slk_rows(rows);
}

TEST(wc3_spell, holy_light_rawcode_lookup_is_nul_safe) {
	DWORD code = MAKEFOURCC('A','H','h','b');
	spell_info_t const *spell = S_SpellInfoForCode(code);
	ability_t const *abil = FindAbilityForCommand(GetClassName(code));

	/* Runtime spell dispatch starts from a DWORD rawcode. This specifically
	 * guards against treating &code as a C string: that only worked when the
	 * unrelated byte after the four rawcode bytes happened to be zero. */
	T_NOT_NULL(abil);
	T_ASSERT(abil->cmd == spell_cmd);
	T_NOT_NULL(spell);
	T_EQ((int)spell->code, (int)code);
	T_EQ((int)spell->target_type, (int)SPELL_TARGET_UNIT);
}

TEST(wc3_spell, blizzard_is_channel) {
	ability_t const *abil = FindAbilityByClassname("AHbz");
	T_NOT_NULL(abil);
	T_NOT_NULL(abil->spell);
	T_ASSERT(abil->spell->flags & SPELL_CHANNEL);
	T_EQ((int)abil->spell->target_type, (int)SPELL_TARGET_POINT);
}

TEST(wc3_spell, cold_arrows_has_autocast_flag) {
	ability_t const *abil = FindAbilityByClassname("AHca");
	T_NOT_NULL(abil);
	T_NOT_NULL(abil->spell);
	T_ASSERT(abil->spell->flags & SPELL_AUTOCAST);
	T_ASSERT(abil->spell->flags & SPELL_TOGGLE);
}

TEST(wc3_spell, non_spell_ability_has_null_spell) {
	ability_t const *abil = FindAbilityByClassname(STR_CmdMove);
	T_NOT_NULL(abil);
	T_NULL(abil->spell);  /* move is not a spell */
}


TEST(wc3_spell, unit_target_click_accepts_out_of_range_target_and_casts_after_approach) {
    const char slk[] =
        "ID;PWXL;N;EBB;Y2;X9\n"
        "C;Y1;X1;K\"alias\"\nC;Y1;X2;K\"code\"\nC;Y1;X3;K\"targs\"\n"
        "C;Y1;X4;K\"Cost1\"\nC;Y1;X5;K\"Cool1\"\nC;Y1;X6;K\"Rng1\"\n"
        "C;Y1;X7;K\"DataA1\"\nC;Y1;X8;K\"DataB1\"\nC;Y1;X9;K\"DataC1\"\n"
        "C;Y2;X1;K\"AOcl\"\nC;Y2;X2;K\"AOcl\"\n"
        "C;Y2;X3;K\"air,ground,enemy,neutral\"\nC;Y2;X4;K\"75\"\n"
        "C;Y2;X5;K\"9\"\nC;Y2;X6;K\"100\"\nC;Y2;X7;K\"100\"\n"
        "C;Y2;X8;K\"1\"\nC;Y2;X9;K\"0\"\nE\n";
    UnitAbilities_t abilities = { .abilList = "AOcl" };
    slkTestData_t *rows = parse_slk_string(slk);
    slkTestData_t *old;
    LPEDICT caster = make_hero(MAKEFOURCC('O','f','a','r'), 500, 300, 0, 0);
    LPEDICT clent = &g_edicts[0];
    LPGAMECLIENT client;
    LPEDICT target = alloc_test_unit(MAKEFOURCC('h','f','o','o'), 500, 0);
    LPEDICT thinker;
    DWORD thinker_slot;
    char target_number[16];
    LPCSTR button[] = { "button", "AOcl" };
    LPCSTR select_target[] = { "select", target_number };

    /* make_hero resets edicts, so restore the player-slot client for commands. */
    clent->client = client = &game.clients[0];
    old = G_SetSLKRows("AbilityData", rows);
    caster->data.UnitAbilities = &abilities;
    caster->s.player = client->ps.number;
    caster->collision = 16.0f;
    target->s.player = 1;
    target->svflags |= SVF_MONSTER;
    target->targtype = TARG_GROUND;
    target->health.value = target->health.max_value = 500.0f;
    ((LPMAPINFO)level.mapinfo)->players[caster->s.player].playerType = kPlayerTypeHuman;
    ((LPMAPINFO)level.mapinfo)->players[target->s.player].playerType = kPlayerTypeHuman;
    memset(level.alliances, 0, sizeof(level.alliances));
    G_SelectEntity(client, caster);
    snprintf(target_number, sizeof(target_number), "%u", (unsigned)target->s.number);

    G_ClientCommand(clent, 2, button);
    T_NOT_NULL(client->menu.on_entity_selected);
    thinker_slot = globals.num_edicts;
    G_ClientCommand(clent, 2, select_target);

    T_NULL(client->menu.on_entity_selected);
    T_ASSERT(caster->goalentity == target);
    T_ASSERT(move_is_active_order_walk(caster));
    T_FEQ(target->health.value, 500.0f, 0.001f);
    T_FEQ(caster->mana.value, 300.0f, 0.001f);
    thinker = &globals.edicts[thinker_slot];
    T_ASSERT(thinker->inuse);
    T_NOT_NULL(thinker->think);

    caster->s.origin2.x = caster->s.origin.x = 425.0f;
    caster->s.origin2.y = caster->s.origin.y = 0.0f;
    thinker->think(thinker);

    T_ASSERT(!thinker->inuse);
    T_FEQ(target->health.value, 400.0f, 0.001f);
    T_FEQ(caster->mana.value, 225.0f, 0.001f);
    T_ASSERT(!S_SpellCooldownReady(caster, MAKEFOURCC('A','O','c','l')));

    G_SetSLKRows("AbilityData", old);
    free_slk_rows(rows);
}

#endif /* BZ_TESTS */
