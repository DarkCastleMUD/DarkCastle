/***************************************************************************
 *  file: spec_ass.c , Special module.                     Part of DIKUMUD *
 *  Usage: Procedures assigning function pointers.                         *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
#include "db.h"
#include "room.h"
#include "levels.h"
#include "player.h"
#include "utility.h"

typedef int    SPEC_FUN  (char_data * ch, obj_data *obj, int cmd, const char *argument, 
                          char_data * owner);
typedef int    ROOM_PROC (CHAR_DATA *ch, int cmd, char *arg);


extern CWorld world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
void boot_the_shops();
void boot_player_shops();
void assign_the_shopkeepers();
void assign_the_player_shopkeepers( );
void assign_non_combat_procs();
void assign_combat_procs();

/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
    assign_non_combat_procs();
    assign_combat_procs();
    boot_the_shops();
    logf(IMMORTAL, LOG_WORLD, "Booting player shops.");
    boot_player_shops();
    assign_the_shopkeepers();
    assign_the_player_shopkeepers( );
}

// The following four functions are just here to make sure when someone removes a mob
// or object from the world, we don't try to assign procs to index[-1]

void assign_one_mob_non(int mob_num, int (*func)(CHAR_DATA*, struct obj_data *, int, const char*, CHAR_DATA*))
{
	int mob = real_mobile(mob_num);

	if(mob < 0)
	{
	  if (DC::instance().cf.test_world == false) {
	    logf(IMMORTAL, LOG_WORLD, "Assigning non_combat proc to non-existant mob '%d'.", mob_num);
	  }
	}
	else mob_index[mob].non_combat_func = func;
}

void assign_one_mob_com(int mob_num, int (*func)(CHAR_DATA*, struct obj_data *, int, const char*, CHAR_DATA*))
{
	int mob = real_mobile(mob_num);

	if(mob < 0)
	{
		logf(IMMORTAL, LOG_WORLD, "Assigning combat proc to non-existant mob '%d'.", mob_num);
	}
	else mob_index[mob].combat_func = func;
}

void assign_one_obj_non(int obj_num, int (*func)(CHAR_DATA*, struct obj_data *, int, const char*, CHAR_DATA*))
{
	int obj = real_object(obj_num);

	if(obj < 0)
		logf(IMMORTAL, LOG_WORLD, "Assigning non-combat proc to non-existant obj '%d'.", obj_num);
	else obj_index[obj].non_combat_func = func;
}

void assign_one_obj_com(int obj_num, int (*func)(CHAR_DATA*, struct obj_data *, int, const char*, CHAR_DATA*))
{
	int obj = real_object(obj_num);

	if(obj < 0)
		logf(IMMORTAL, LOG_WORLD, "Assigning combat proc to non-existant obj '%d'.", obj_num);
	else obj_index[obj].combat_func = func;
}

void assign_non_combat_procs() {
    SPEC_FUN    guild;
    SPEC_FUN    guild_guard;
    SPEC_FUN    francis_guard;
    SPEC_FUN    chain_gossips;
    SPEC_FUN	  fido;
    SPEC_FUN	  janitor;
    SPEC_FUN    backstabber;
    SPEC_FUN    mud_school_adept;
    SPEC_FUN    adept;    
    SPEC_FUN    frosty;
    SPEC_FUN    meta_dude;
    SPEC_FUN    sleazy_vault_guy;
    SPEC_FUN    quest_vendor;
    SPEC_FUN    gl_repair_shop;
    SPEC_FUN    godload_sales;
    SPEC_FUN    skill_master;
    SPEC_FUN    repair_guy;
    SPEC_FUN    super_repair_guy;
    SPEC_FUN    repair_shop;
    SPEC_FUN    stofficer;
    SPEC_FUN    stcrew;
    SPEC_FUN    poet;
    SPEC_FUN    clan_guard;
    SPEC_FUN    mortician;
    SPEC_FUN    apiary_worker;
    SPEC_FUN    newbie_zone_guard;
    SPEC_FUN    humaneater;
    SPEC_FUN    pir_slut;
    SPEC_FUN    generic_guard;
    SPEC_FUN    portal_guard;
    SPEC_FUN    doorcloser;
    SPEC_FUN    panicprisoner;
    SPEC_FUN    iasenko_non_combat;
    SPEC_FUN    koban_non_combat;
    SPEC_FUN    arena_only;
    SPEC_FUN    mage_golem;
    SPEC_FUN    druid_elemental;
    SPEC_FUN    druid_familiar_owl_non;
    SPEC_FUN    mage_familiar_gremlin_non;
    SPEC_FUN    mage_familiar_imp_non;
    SPEC_FUN    druid_familiar_chipmunk_non;
    SPEC_FUN    bodyguard;
    SPEC_FUN    generic_blocker;
    SPEC_FUN    generic_doorpick_blocker;
    SPEC_FUN    startrek_miles;
    SPEC_FUN    cardinal;
    SPEC_FUN    eddie_shopkeeper;
    assign_one_mob_non(3071, cardinal);

    assign_one_mob_non(501, arena_only);
    assign_one_mob_non(502, arena_only);
    assign_one_mob_non(503, arena_only);
    assign_one_mob_non(8, mage_golem);
    assign_one_mob_non(1,  chain_gossips);
    assign_one_mob_non(5,  mage_familiar_imp_non);
    assign_one_mob_non(4, mage_familiar_gremlin_non);
    assign_one_mob_non(6,  druid_familiar_chipmunk_non);
    assign_one_mob_non(7, druid_familiar_owl_non);
    assign_one_mob_non(88, druid_elemental);
    assign_one_mob_non(89, druid_elemental);
    assign_one_mob_non(90, druid_elemental);
    assign_one_mob_non(91, druid_elemental);
    assign_one_mob_non(70,  pir_slut);
    assign_one_mob_non(200,  mud_school_adept);
    assign_one_mob_non(207,  adept);
    assign_one_mob_non(208,  adept);
    assign_one_mob_non(214,  fido);
    assign_one_mob_non(17812,  fido);
    assign_one_mob_non(7134,  francis_guard);
    assign_one_mob_non(222,  newbie_zone_guard);
    assign_one_mob_non(1334, generic_doorpick_blocker);
    assign_one_mob_non(1935,  guild);
    assign_one_mob_non(1937,  guild);
    assign_one_mob_non(10018, gl_repair_shop);
    assign_one_mob_non(10004, godload_sales);
    assign_one_mob_non(10019, godload_sales);
    assign_one_mob_non(10020, godload_sales);
    assign_one_mob_non(10021, godload_sales);
    assign_one_mob_non(10022, godload_sales);
    assign_one_mob_non(10023, godload_sales);
    assign_one_mob_non(10026, godload_sales);
    assign_one_mob_non(10024, godload_sales);
    assign_one_mob_non(1939,  guild);
    assign_one_mob_non(1932,  guild);
    assign_one_mob_non(1929,  guild_guard);
    assign_one_mob_non(1931,  guild_guard);
    assign_one_mob_non(1933,  guild_guard);
    assign_one_mob_non(1936,  guild_guard);
    assign_one_mob_non(3061,  janitor);
    assign_one_mob_non(3062,  fido);
    assign_one_mob_non(3066,  fido);
    assign_one_mob_non(22021,  fido);
    assign_one_mob_non(1941,  guild);
    assign_one_mob_non(1938,  guild_guard);
    assign_one_mob_non(1940,  guild_guard);
    assign_one_mob_non(3709,  generic_doorpick_blocker);
    assign_one_mob_non(4712,  backstabber);
    assign_one_mob_non(4714,  backstabber);
    assign_one_mob_non(4901,  humaneater);
    assign_one_mob_non(6437,  generic_blocker);
//    assign_one_mob_non(6902,  guild); // master shaolin monks teach 'stun'
    assign_one_mob_non(2318,  clan_guard);
    assign_one_mob_non(2300,  clan_guard);
    assign_one_mob_non(2312,  clan_guard);
    assign_one_mob_non(2311,  clan_guard);
    assign_one_mob_non(2301,  clan_guard);
    assign_one_mob_non(2302,  clan_guard);
    assign_one_mob_non(2303,  clan_guard);
    assign_one_mob_non(2304,  clan_guard);
    assign_one_mob_non(2305,  clan_guard);
    assign_one_mob_non(2306,  clan_guard);
    assign_one_mob_non(2307,  clan_guard);
    assign_one_mob_non(2308,  clan_guard);
    assign_one_mob_non(2310,  clan_guard);
    assign_one_mob_non(2313,  clan_guard);
    assign_one_mob_non(2315,  clan_guard);
    assign_one_mob_non(2317,  clan_guard);
    assign_one_mob_non(2399,  clan_guard);
    assign_one_mob_non(2500,  clan_guard);
    assign_one_mob_non(2321,  clan_guard);
    assign_one_mob_non(2322,  clan_guard);
    assign_one_mob_non(9511,  bodyguard);
    assign_one_mob_non(9531,  bodyguard);
    assign_one_mob_non(9532,  bodyguard);
    assign_one_mob_non(9999,  mortician); 
    assign_one_mob_non(1919,  guild_guard);
    assign_one_mob_non(1921,  guild_guard);
    assign_one_mob_non(1923,  guild_guard);
    assign_one_mob_non(1925,  guild_guard);
    assign_one_mob_non(1927,  guild_guard); 
    assign_one_mob_non(1929,  guild_guard);
    assign_one_mob_non(1600, guild);
    assign_one_mob_non(1604, guild);
    assign_one_mob_non(1609, guild);
    assign_one_mob_non(1612, guild);
    assign_one_mob_non(1616, guild);
    assign_one_mob_non(1620, guild);
    assign_one_mob_non(1624, guild);
    assign_one_mob_non(1628, guild);
    assign_one_mob_non(1632, guild);
    assign_one_mob_non(1636, guild);
    assign_one_mob_non(1640, guild);

    assign_one_mob_non(5223, guild);
    assign_one_mob_non(5228, guild);
    assign_one_mob_non(5229, guild);
    assign_one_mob_non(5230, guild);
    assign_one_mob_non(5231, guild);
    assign_one_mob_non(5233, guild);
    assign_one_mob_non(5234, guild);
    assign_one_mob_non(5235, guild);
    assign_one_mob_non(5236, guild);
    assign_one_mob_non(5237, guild);
    assign_one_mob_non(5242, guild);

    assign_one_mob_non(1920,  guild);
    assign_one_mob_non(1922,  guild);
    assign_one_mob_non(5431,  guild);
    assign_one_mob_non(1924,  guild);
    assign_one_mob_non(1926,  guild);
    assign_one_mob_non(10011,  skill_master);
    assign_one_mob_non(10009, meta_dude);
    assign_one_mob_non(10030, sleazy_vault_guy);
    assign_one_mob_non(73,    quest_vendor);
    assign_one_mob_non(1927,  guild_guard);
    assign_one_mob_non(1928,  guild);
    assign_one_mob_non(14537, apiary_worker);
    assign_one_mob_non(1930,  guild);
    assign_one_mob_non(20700, generic_guard);
    assign_one_mob_non(20701, portal_guard);
    assign_one_mob_non(20744, panicprisoner);
    assign_one_mob_non(20745, doorcloser);
    assign_one_mob_non(26715, startrek_miles);
    assign_one_mob_non(26721,  stofficer);
    assign_one_mob_non(26711,  stcrew);
    assign_one_mob_non(27120,  backstabber);
    assign_one_mob_non(27860,  frosty);
    assign_one_mob_non(27871,  poet);
    assign_one_mob_non(32045,  repair_guy);
    assign_one_mob_non(32046,  super_repair_guy);
    assign_one_mob_non(17805,  super_repair_guy);
    assign_one_mob_non(32047,  repair_shop);
    assign_one_mob_non(10031,  eddie_shopkeeper);

    return;
}


void assign_combat_procs() {
    SPEC_FUN	red_dragon;
    SPEC_FUN	green_dragon;
    SPEC_FUN	black_dragon;
    SPEC_FUN	white_dragon;
    SPEC_FUN	brass_dragon;
    SPEC_FUN    fighter;
    SPEC_FUN mage_familiar_gremlin;
    SPEC_FUN    active_grandmaster;
    SPEC_FUN    active_tarrasque;
    SPEC_FUN	mother_moat_and_moad;
    SPEC_FUN    bee;
    SPEC_FUN    hellstreamer;
    SPEC_FUN    firestormer;
    SPEC_FUN    clutchdrone_combat;
    SPEC_FUN    blindingparrot;
    SPEC_FUN    bounder;
    SPEC_FUN    dispelguy;
    SPEC_FUN    marauder;
    SPEC_FUN    foggy_combat;
    SPEC_FUN    iasenko_combat;
    SPEC_FUN    koban_combat;
    SPEC_FUN    druid_familiar_owl;
    SPEC_FUN    kogiro_combat;
    SPEC_FUN    takahashi_combat;
    SPEC_FUN    askari_combat;
    SPEC_FUN    surimoto_combat;
    SPEC_FUN    hiryushi_combat;
    SPEC_FUN    izumi_combat;
    SPEC_FUN    shogura_combat;
    SPEC_FUN    sc_golem;
    SPEC_FUN    mage_familiar_imp;

    /* I spent forever putting these fuckers in numerical order,
       keep um that way.  It makes no sense dividing them by what
       zone they load up in.  - pir */
   assign_one_mob_com(4, mage_familiar_gremlin);
    assign_one_mob_com(5,  mage_familiar_imp);
//   assign_one_mob_com(7, druid_familiar_owl);
   assign_one_mob_com(8, sc_golem);
    assign_one_mob_com(1364,  active_grandmaster);
    assign_one_mob_com(3059,  fighter);
    assign_one_mob_com(5005,  brass_dragon);
    assign_one_mob_com(5010,  red_dragon);
    assign_one_mob_com(6112,  green_dragon);
    assign_one_mob_com(6296,  fighter);
    assign_one_mob_com(6500,  fighter);
    assign_one_mob_com(6508,  fighter);
    assign_one_mob_com(6517,  fighter);
    assign_one_mob_com(9005,  fighter);
    assign_one_mob_com(9007,  fighter);
    assign_one_mob_com(9021,  red_dragon);
    assign_one_mob_com(9022,  black_dragon);
    assign_one_mob_com(9023,  white_dragon);
    assign_one_mob_com(9025,  green_dragon);
    assign_one_mob_com(9301,  fighter);
    assign_one_mob_com(9303,  fighter);
    assign_one_mob_com(9304,  fighter);
    assign_one_mob_com(9305,  fighter);
    assign_one_mob_com(9306,  fighter);
    assign_one_mob_com(9307,  fighter);
    assign_one_mob_com(9308,  fighter);
    assign_one_mob_com(9309,  fighter);
    assign_one_mob_com(9310,  fighter);
    assign_one_mob_com(9312,  fighter);
    assign_one_mob_com(9313,  fighter);
    assign_one_mob_com(9314,  fighter);
    assign_one_mob_com(9318,  fighter);
    assign_one_mob_com(9319,  fighter);
    assign_one_mob_com(9320,  fighter);
    assign_one_mob_com(9327,  fighter);
    assign_one_mob_com(9329,  fighter);
    assign_one_mob_com(9335,  fighter);
    assign_one_mob_com(9340,  fighter);
    assign_one_mob_com(9344,  fighter);
    assign_one_mob_com(13302,  clutchdrone_combat);
    assign_one_mob_com(14527,  bee);
    assign_one_mob_com(14528,  bee);
    assign_one_mob_com(14532,  bee);
    assign_one_mob_com(14539,  bee);
    assign_one_mob_com(15009,  fighter);
    assign_one_mob_com(15010,  fighter);
    assign_one_mob_com(20713,  hellstreamer);
    assign_one_mob_com(20714,  hellstreamer);
    assign_one_mob_com(20715,  hellstreamer);
    assign_one_mob_com(20716,  hellstreamer);
    assign_one_mob_com(20717,  hellstreamer);
    assign_one_mob_com(20725,  dispelguy);
    assign_one_mob_com(20726,  dispelguy);
    assign_one_mob_com(20737,  dispelguy);
    assign_one_mob_com(20739,  hellstreamer);
    assign_one_mob_com(20740,  hellstreamer);
    assign_one_mob_com(20741,  hellstreamer);
    assign_one_mob_com(20742,  hellstreamer);
    assign_one_mob_com(20747,  bounder);
    assign_one_mob_com(20733,  blindingparrot);
    assign_one_mob_com(22014,  mother_moat_and_moad);
    assign_one_mob_com(22019,  foggy_combat);
    assign_one_mob_com(22340,  mother_moat_and_moad);    
    assign_one_mob_com(22701,  marauder);
    assign_one_mob_com(26208,  fighter);
    assign_one_mob_com(26200,  fighter);
    assign_one_mob_com(26212,  fighter);
    assign_one_mob_com(27123,  fighter);
    assign_one_mob_com(27133,  firestormer);
    assign_one_mob_com(27140,  hellstreamer);
    assign_one_mob_com(32044,  active_tarrasque);
    return;    
}

/* assign special procedures to objects */
void assign_objects(void)
{
  SPEC_FUN board;
  SPEC_FUN bank;
  SPEC_FUN casino_atm;
  SPEC_FUN holyavenger;
  SPEC_FUN drainingstaff;
  SPEC_FUN souldrainer;
  SPEC_FUN emoting_object;
  SPEC_FUN returner;
  SPEC_FUN pushwand;
  SPEC_FUN barbweap;
  SPEC_FUN pfe_word;
  SPEC_FUN devilsword;
  SPEC_FUN eliara_non_combat;
  SPEC_FUN eliara_combat;
  SPEC_FUN arenaporter;
  SPEC_FUN movingarenaporter;
  SPEC_FUN restring_machine;
  SPEC_FUN weenie_weedy;
  SPEC_FUN pagoda_shield_restorer;
  SPEC_FUN pagoda_balance;
  SPEC_FUN phish_locator;
  SPEC_FUN portal_word;
  SPEC_FUN full_heal_word;
  SPEC_FUN mana_box;
  SPEC_FUN stupid_button;
  SPEC_FUN fireshield_word;
  SPEC_FUN teleport_word;
  SPEC_FUN alignment_word;
  SPEC_FUN protection_word;
  SPEC_FUN pull_proc;
  SPEC_FUN no_magic_while_alive;
  SPEC_FUN boat_proc;
  SPEC_FUN leave_boat_proc;
  SPEC_FUN mob_summoner;
  SPEC_FUN globe_of_darkness_proc;
  SPEC_FUN hornoplenty;
  SPEC_FUN dk_rend;
  SPEC_FUN magic_missile_boots;
  SPEC_FUN shield_combat_procs;
  SPEC_FUN generic_push_proc;
  SPEC_FUN generic_weapon_combat;
  SPEC_FUN TOHS_locator;
  SPEC_FUN stupid_message;
  SPEC_FUN hooktippedsteelhalberd;
  SPEC_FUN goldenbatleth;
  SPEC_FUN gotta_dance_boots;
  SPEC_FUN random_dir_boots;
  SPEC_FUN noremove_eq;
  SPEC_FUN glove_combat_procs;
  SPEC_FUN hot_potato;
  SPEC_FUN exploding_mortar_shells;
  SPEC_FUN talkingsword;
  SPEC_FUN gazeofgaiot;
  SPEC_FUN moving_portals;
  SPEC_FUN spellcraft_glyphs;
  SPEC_FUN angie_proc;
  SPEC_FUN godload_cassock;
  SPEC_FUN godload_phyraz;
  SPEC_FUN godload_leprosy;
  SPEC_FUN godload_stargazer;
  SPEC_FUN godload_armbands;
  SPEC_FUN godload_gaze;
  SPEC_FUN godload_defender;
  SPEC_FUN godload_tovmier;
  SPEC_FUN godload_wailka;
  SPEC_FUN godload_choker;
  SPEC_FUN godload_lorne;
  SPEC_FUN godload_quiver;
  SPEC_FUN godload_aligngood;
  SPEC_FUN godload_alignevil;
  SPEC_FUN godload_hammer;
  SPEC_FUN hellmouth_thing;
  SPEC_FUN blackjack_table;
  SPEC_FUN slot_machine;
  SPEC_FUN roulette_table;
 // combvat procs
  SPEC_FUN godload_banshee;
  SPEC_FUN godload_claws;
    SPEC_FUN    godload_foecrusher;
    SPEC_FUN    godload_hydratail;
    SPEC_FUN    godload_jaelgreth;
    SPEC_FUN    godload_grathelok;

    SPEC_FUN    szrildor_pass;
    SPEC_FUN    szrildor_pass_checks;

    SPEC_FUN    chaosblade;
    SPEC_FUN    eternitystaff;
    SPEC_FUN    rubybrooch;

    SPEC_FUN    dancevest;
    SPEC_FUN    durendal; 
    SPEC_FUN    orrowand;
    SPEC_FUN    lilithring;
    SPEC_FUN    songstaff;
    SPEC_FUN    dawnsword;

    assign_one_obj_non(30062, dancevest);
    assign_one_obj_non(30019, durendal);
    assign_one_obj_non(3124, orrowand);
    assign_one_obj_non(30027, lilithring);
    assign_one_obj_non(30003, dawnsword);
    assign_one_obj_non(30008, songstaff);

    assign_one_obj_non(30097,   szrildor_pass);
    assign_one_obj_non(30096,   szrildor_pass_checks);

    assign_one_obj_non(30010,   rubybrooch);
    assign_one_obj_non(30036,   eternitystaff);
    assign_one_obj_non(30033,   chaosblade);



  //Godload procs follow.
    assign_one_obj_com(565,   godload_banshee);
  //  assign_one_obj_com(511,   godload_claws);
   // assign_one_obj_com(506,  glove_combat_procs);
   // assign_one_obj_com(528, godload_leprosy);

    assign_one_obj_non(29204, angie_proc);
    assign_one_obj_non(70, blackjack_table);
    assign_one_obj_non(72,  stupid_button);
    assign_one_obj_non(21900, blackjack_table);
    assign_one_obj_non(21901, blackjack_table);
    assign_one_obj_non(21902, blackjack_table);
    assign_one_obj_non(21903, blackjack_table);
    assign_one_obj_non(21906, slot_machine);
    assign_one_obj_non(21907, slot_machine);
    assign_one_obj_non(21908, slot_machine);
    assign_one_obj_non(21909, slot_machine);
    assign_one_obj_non(21910, slot_machine);
    assign_one_obj_non(21911, slot_machine);
    assign_one_obj_non(21912, slot_machine);
    assign_one_obj_non(21913, slot_machine);
    assign_one_obj_non(21914, slot_machine);
    assign_one_obj_non(21915, slot_machine);
    assign_one_obj_non(21916, slot_machine);
    assign_one_obj_non(21917, slot_machine);
    assign_one_obj_non(21918, roulette_table);
    assign_one_obj_non(534, godload_cassock);
    assign_one_obj_non(500, godload_stargazer);
    assign_one_obj_non(526, godload_armbands);
    assign_one_obj_non(548, godload_gaze);
    assign_one_obj_non(561, godload_phyraz);
    assign_one_obj_non(556, godload_defender);
    assign_one_obj_non(578, godload_tovmier);
    assign_one_obj_non(514, godload_wailka);
    assign_one_obj_non(517, godload_choker);
    assign_one_obj_non(519, godload_lorne);
    assign_one_obj_non(540, godload_quiver);
    assign_one_obj_non(558, godload_aligngood);
    assign_one_obj_non(559, godload_alignevil);
    assign_one_obj_non(594, godload_hammer);
  assign_one_obj_com(555, shield_combat_procs); // GL shield

  assign_one_obj_non(16225, pushwand);
  assign_one_obj_non(11300, moving_portals);
  assign_one_obj_non(11301, moving_portals);
  assign_one_obj_non(11302, moving_portals);
  assign_one_obj_non(11303, moving_portals);
  assign_one_obj_non(11304, moving_portals);
  assign_one_obj_non(11305, moving_portals);
  assign_one_obj_non(5911, moving_portals);
  assign_one_obj_non(358, barbweap);
  assign_one_obj_non(9,  phish_locator);	
  assign_one_obj_non(13,  board); /* Quest Board */
  assign_one_obj_non(98, board);
  assign_one_obj_non(2343,  board);
  assign_one_obj_non(2463,  board);
  assign_one_obj_non(2376, board);
  assign_one_obj_non(2402, board);
  assign_one_obj_non(2303, board);
  assign_one_obj_non(2363, board);
  assign_one_obj_non(2308, board);
  assign_one_obj_non(2502, board);
  assign_one_obj_non(2313, board);
  assign_one_obj_non(2390, board);
  assign_one_obj_non(2328, board);
  assign_one_obj_non(2360, board);
  assign_one_obj_non(2451, board);
  assign_one_obj_non(2349, board);
  assign_one_obj_non(2339, board);
  assign_one_obj_non(2410, board);
  assign_one_obj_non(2471, board); // Continuum board
  assign_one_obj_non(2406, board); /*Urukhai Board */
  assign_one_obj_non(2370,  board); /* Quest Board */
  assign_one_obj_non(25,  mob_summoner);
  assign_one_obj_non(41,  restring_machine);
  assign_one_obj_non(40,  emoting_object);
  assign_one_obj_non(71,  board); /* Punishment Board */
  assign_one_obj_non(101,  globe_of_darkness_proc);
  assign_one_obj_non(185,  devilsword);
  assign_one_obj_non(4801, hellmouth_thing);
  assign_one_obj_non(225,  weenie_weedy);
  assign_one_obj_non(393,  hot_potato);
  assign_one_obj_non(113,  exploding_mortar_shells);
  assign_one_obj_non(27997, talkingsword);
  assign_one_obj_non(396,  noremove_eq);
  assign_one_obj_non(397,  random_dir_boots);
  assign_one_obj_non(398,  gotta_dance_boots);
//  assign_one_obj_non(557,  hornoplenty);
  assign_one_obj_non(3090,  TOHS_locator);
  assign_one_obj_non(3099,  board); /* Mortal Board */ 
  assign_one_obj_non(70,  board); /* Broker Board */ 
  assign_one_obj_non(3611,  pfe_word);
  assign_one_obj_non(9529,  pull_proc);
  assign_one_obj_non(29203,  pull_proc);
 // assign_one_obj_non(9530,  no_magic_while_alive);
  assign_one_obj_non(9531,  boat_proc);
  assign_one_obj_non(9532,  leave_boat_proc);
  assign_one_obj_non(9603, gazeofgaiot);
  assign_one_obj_non(9606,  stupid_message);
  assign_one_obj_non(50,  board); /* builder board */
  assign_one_obj_non(9997,  board); /* imp board */ 
  assign_one_obj_non(9999,  board); /* wiz board */ 
  assign_one_obj_non(9995,  bank);  /* bank */
  assign_one_obj_non(21904,  casino_atm);  /* ATM */
//  assign_one_obj_non(17800,  arenaporter);
  assign_one_obj_non(26723,  generic_push_proc);
  assign_one_obj_non(30627,  eliara_non_combat);  


  //spellcraft glyphs
  assign_one_obj_non(6351, spellcraft_glyphs);
  assign_one_obj_non(6352, spellcraft_glyphs);
  assign_one_obj_non(6353, spellcraft_glyphs);  
    
  //assign_one_obj_com(4818, glove_combat_procs);
  //assign_one_obj_com(4819, glove_combat_procs);


  assign_one_obj_non(22399,  board);  // quests2do Board

  assign_one_obj_com(2715,   shield_combat_procs);
  assign_one_obj_com(8208,   shield_combat_procs);
  assign_one_obj_com(9520,   dk_rend);
  assign_one_obj_com(9565, hooktippedsteelhalberd);
  assign_one_obj_com(17904, hooktippedsteelhalberd);
  assign_one_obj_com(10001,  holyavenger);
  assign_one_obj_com(26807,  goldenbatleth);
  assign_one_obj_com(30627,  eliara_combat);
  assign_one_obj_com(570,  godload_jaelgreth);
  assign_one_obj_com(575,  godload_grathelok);
  assign_one_obj_com(584,  godload_foecrusher);
  assign_one_obj_com(586,  godload_hydratail);
//  assign_one_obj_com(22743,  bonusattack);
//  assign_one_obj_com(22605,  souldrainer);
    int i;
    for (i = 1940; i <= 1950; i++) /* Guild boards.*/
      assign_one_obj_non(i, board); 

	return;
}



/* assign special procedures to rooms */
void assign_rooms(void)
{
    ROOM_PROC pet_shops;
    if(real_room(3031) >= 0) 
		world[real_room(3031)].funct = pet_shops;
}
