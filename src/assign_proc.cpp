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
#include <db.h>
#include <room.h>
#include <levels.h>
#include <player.h>
#include <utility.h>

#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

typedef int    SPEC_FUN  (struct char_data * ch, struct obj_data *obj, int cmd, char *argument, 
                          struct char_data * owner);
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

void assign_one_mob_non(int mob_num, int (*func)(CHAR_DATA*, struct obj_data *, int, char*, CHAR_DATA*))
{
	extern short code_testing_mode;

	int mob = real_mobile(mob_num);

	if(mob < 0)
	{
		if(!code_testing_mode)
			logf(IMMORTAL, LOG_WORLD, "Assigning non_combat proc to non-existant mob '%d'.", mob_num);
	}
	else mob_index[mob].non_combat_func = func;
}

void assign_one_mob_com(int mob_num, int (*func)(CHAR_DATA*, struct obj_data *, int, char*, CHAR_DATA*))
{
	extern short code_testing_mode;

	int mob = real_mobile(mob_num);

	if(mob < 0)
	{
		if(!code_testing_mode)
			logf(IMMORTAL, LOG_WORLD, "Assigning combat proc to non-existant mob '%d'.", mob_num);
	}
	else mob_index[mob].combat_func = func;
}

void assign_one_obj_non(int obj_num, int (*func)(CHAR_DATA*, struct obj_data *, int, char*, CHAR_DATA*))
{
	int obj = real_object(obj_num);

	if(obj < 0)
		logf(IMMORTAL, LOG_WORLD, "Assigning non-combat proc to non-existant obj '%d'.", obj_num);
	else obj_index[obj].non_combat_func = func;
}

void assign_one_obj_com(int obj_num, int (*func)(CHAR_DATA*, struct obj_data *, int, char*, CHAR_DATA*))
{
	int obj = real_object(obj_num);

	if(obj < 0)
		logf(IMMORTAL, LOG_WORLD, "Assigning combat proc to non-existant obj '%d'.", obj_num);
	else obj_index[obj].combat_func = func;
}

void assign_non_combat_procs() {
    SPEC_FUN    cityguard;
    SPEC_FUN    guild;
    SPEC_FUN    guild_guard;
    SPEC_FUN	deth;
    SPEC_FUN	fido;
    SPEC_FUN	janitor;
    SPEC_FUN	mayor;
    SPEC_FUN    robber;
    SPEC_FUN    ring_keeper;
    SPEC_FUN    backstabber;
    SPEC_FUN    gossip;
    SPEC_FUN    mud_school_adept;
    SPEC_FUN    adept;    
    SPEC_FUN    baby_troll;
    SPEC_FUN    frosty;
    SPEC_FUN    Thalos_citizen;
    SPEC_FUN    Executioner;
    SPEC_FUN    annoyingbirthdayshout;
    SPEC_FUN    passive_cleric;
    SPEC_FUN    passive_grandmaster;
    SPEC_FUN    passive_tarrasque;
    SPEC_FUN    platinumsmith;
    SPEC_FUN    platmerchant;
    SPEC_FUN    moritician;
    SPEC_FUN    meta_dude;
    SPEC_FUN    skill_master;
    SPEC_FUN    repair_guy;
    SPEC_FUN    super_repair_guy;
    SPEC_FUN    repair_shop;
    SPEC_FUN    stofficer;
    SPEC_FUN    stcrew;
    SPEC_FUN    summonbash; 
    SPEC_FUN    poet;
    SPEC_FUN    clan_guard;
    SPEC_FUN    halfling_people;
    SPEC_FUN    passive_magic_user;
    SPEC_FUN    passive_magic_user2;
    SPEC_FUN    mortician;
    SPEC_FUN    apiary_worker;
    SPEC_FUN    newbie_zone_guard;
    SPEC_FUN    charon;
    SPEC_FUN    humaneater;
    SPEC_FUN    pir_slut;
    SPEC_FUN    ranger_non_combat;
    SPEC_FUN    generic_guard;
    SPEC_FUN    portal_guard;
    SPEC_FUN    doorcloser;
    SPEC_FUN    panicprisoner;
    SPEC_FUN    areanotopen;
    SPEC_FUN    alakarbodyguard;
    SPEC_FUN    mithaxequest;
    SPEC_FUN    turtle_green;
    SPEC_FUN    iasenko_non_combat;
    SPEC_FUN    koban_non_combat;
    SPEC_FUN    arena_only;
    SPEC_FUN    mage_familiar_non;
    SPEC_FUN    bodyguard;
    SPEC_FUN    generic_blocker;
    SPEC_FUN    generic_doorpick_blocker;
    SPEC_FUN    startrek_miles;
    SPEC_FUN    generic_area_guard;

    assign_one_mob_non(1,  deth);
    assign_one_mob_non(2,  charon);
    assign_one_mob_non(3,  platmerchant);
    assign_one_mob_non(4,  mithaxequest);
    assign_one_mob_non(5,  mage_familiar_non);
    //assign_one_mob_non(69,  areanotopen);
    assign_one_mob_non(70,  pir_slut);
    assign_one_mob_non(200,  mud_school_adept);
    assign_one_mob_non(207,  adept);
    assign_one_mob_non(208,  adept);
    assign_one_mob_non(214,  fido);
    assign_one_mob_non(219,  guild);
    assign_one_mob_non(222,  newbie_zone_guard);
    assign_one_mob_non(250,  annoyingbirthdayshout);
    assign_one_mob_non(501,  arena_only);
    assign_one_mob_non(502,  arena_only);
    assign_one_mob_non(503,  arena_only);
    assign_one_mob_non(901, passive_magic_user2);
    assign_one_mob_non(903, passive_magic_user2);
    assign_one_mob_non(904, passive_magic_user2);
    assign_one_mob_non(905, passive_magic_user2);
    assign_one_mob_non(907, passive_magic_user2);
    assign_one_mob_non(908, passive_magic_user);
    assign_one_mob_non(916, passive_magic_user);
    assign_one_mob_non(919, passive_magic_user2);
    assign_one_mob_non(921, passive_magic_user);
    assign_one_mob_non(1000, passive_magic_user); 
    assign_one_mob_non(1203,  Executioner);
    assign_one_mob_non(1332, passive_magic_user);
    assign_one_mob_non(1334, generic_doorpick_blocker);
    assign_one_mob_non(1337, passive_magic_user2);
    assign_one_mob_non(1338, passive_magic_user);
    assign_one_mob_non(1348, passive_magic_user);
    assign_one_mob_non(1349, passive_magic_user2);
    assign_one_mob_non(1354, passive_magic_user);
    assign_one_mob_non(1356, passive_magic_user);
    assign_one_mob_non(1357, passive_magic_user);
    assign_one_mob_non(1359, passive_magic_user);
    assign_one_mob_non(1360, passive_magic_user);
    assign_one_mob_non(1362, passive_magic_user);
    assign_one_mob_non(1352, passive_magic_user);
    assign_one_mob_non(1353, passive_magic_user);
    assign_one_mob_non(1364,  passive_grandmaster);
    assign_one_mob_non(2828,  baby_troll);
    assign_one_mob_non(3020,  guild);
    assign_one_mob_non(3021,  guild);
    assign_one_mob_non(3022,  guild);
    assign_one_mob_non(3023,  guild);
    assign_one_mob_non(3024,  guild_guard);
    assign_one_mob_non(3025,  guild_guard);
    assign_one_mob_non(3026,  guild_guard);
    assign_one_mob_non(3027,  guild_guard);
    assign_one_mob_non(3061,  janitor);
    assign_one_mob_non(3062,  fido);
    assign_one_mob_non(3066,  fido);
    assign_one_mob_non(3072,  guild);
    assign_one_mob_non(3200,  guild_guard);
    assign_one_mob_non(3202,  guild_guard);
    assign_one_mob_non(3201,  guild);
    assign_one_mob_non(3203,  guild);
    assign_one_mob_non(3404, passive_magic_user);
    assign_one_mob_non(3709,  generic_doorpick_blocker);
    assign_one_mob_non(4103,  robber);
    assign_one_mob_non(4713,  passive_cleric);
    assign_one_mob_non(4712,  backstabber);
    assign_one_mob_non(4714,  backstabber);
    assign_one_mob_non(4901,  humaneater);
    assign_one_mob_non(5107, passive_magic_user2);
    assign_one_mob_non(5108, passive_magic_user2);
    assign_one_mob_non(5109, passive_magic_user2);
    assign_one_mob_non(5200, passive_magic_user);
    assign_one_mob_non(5900,  turtle_green);
    assign_one_mob_non(6437,  generic_blocker);
    assign_one_mob_non(6902,  guild); // master shaolin monks teach 'stun'
    assign_one_mob_non(6910, passive_magic_user);
    assign_one_mob_non(7200, passive_magic_user);
    assign_one_mob_non(7201, passive_magic_user);
    assign_one_mob_non(7202, passive_magic_user);
    assign_one_mob_non(8542,  iasenko_non_combat);
    assign_one_mob_non(8543,  koban_non_combat);
    assign_one_mob_non(9003, passive_magic_user);
    assign_one_mob_non(9004,  passive_cleric);
    assign_one_mob_non(9042,  robber);
    assign_one_mob_non(9043,  passive_cleric);
    assign_one_mob_non(9344,  clan_guard);
    assign_one_mob_non(9511,  bodyguard);
    assign_one_mob_non(9531,  bodyguard);
    assign_one_mob_non(9532,  bodyguard);
    assign_one_mob_non(9999,  mortician); 
    assign_one_mob_non(10000,  guild_guard);
    assign_one_mob_non(10001,  guild_guard);
    assign_one_mob_non(10002,  guild_guard);
    assign_one_mob_non(10003,  guild_guard);
    assign_one_mob_non(10004,  platinumsmith); 
    assign_one_mob_non(10009,  meta_dude);
    assign_one_mob_non(10005,  guild);
    assign_one_mob_non(10006,  guild);
    assign_one_mob_non(10007,  guild);
    assign_one_mob_non(10008,  guild);
    assign_one_mob_non(10011,  skill_master);
    assign_one_mob_non(10012,  guild_guard);
    assign_one_mob_non(10013,  guild);
    assign_one_mob_non(14537, apiary_worker);
    assign_one_mob_non(17604,  alakarbodyguard);
    assign_one_mob_non(17605,  alakarbodyguard);
    assign_one_mob_non(17606,  alakarbodyguard);
    assign_one_mob_non(18015,  guild);
    assign_one_mob_non(19304, generic_area_guard);
    assign_one_mob_non(20700, generic_guard);
    assign_one_mob_non(20701, portal_guard);
    assign_one_mob_non(20744, panicprisoner);
    assign_one_mob_non(20745, doorcloser);
    assign_one_mob_non(22310,  robber);
    assign_one_mob_non(22330, passive_magic_user2);
    assign_one_mob_non(26715, startrek_miles);
    assign_one_mob_non(26721,  stofficer);
    assign_one_mob_non(26711,  stcrew);
    assign_one_mob_non(27120,  backstabber);
    assign_one_mob_non(27860,  frosty);
    assign_one_mob_non(27871,  poet);
    assign_one_mob_non(32044,  passive_tarrasque);
    assign_one_mob_non(32045,  repair_guy);
    assign_one_mob_non(32046,  super_repair_guy);
    assign_one_mob_non(32047,  repair_shop);


    return;
}


void assign_combat_procs() {
    SPEC_FUN	snake;
    SPEC_FUN    secret_agent;
    SPEC_FUN    active_magic_user;
    SPEC_FUN    active_magic_user2;
    SPEC_FUN	red_dragon;
    SPEC_FUN	blue_dragon;
    SPEC_FUN	green_dragon;
    SPEC_FUN	black_dragon;
    SPEC_FUN	white_dragon;
    SPEC_FUN	brass_dragon;
    SPEC_FUN    active_cleric;
    SPEC_FUN    fighter;
    SPEC_FUN    active_grandmaster;
    SPEC_FUN    active_tarrasque;
    SPEC_FUN	cry_dragon;
    SPEC_FUN    bee;
    SPEC_FUN    hellstreamer;
    SPEC_FUN    firestormer;
    SPEC_FUN    ranger_combat;
    SPEC_FUN    clutchdrone_combat;
    SPEC_FUN    blindingparrot;
    SPEC_FUN    bounder;
    SPEC_FUN    dispelguy;
    SPEC_FUN    marauder;
    SPEC_FUN    acidhellstreamer;
    SPEC_FUN    turtle_green_combat;
    SPEC_FUN    foggy_combat;
    SPEC_FUN    iasenko_combat;
    SPEC_FUN    koban_combat;
    SPEC_FUN    kogiro_combat;
    SPEC_FUN    takahashi_combat;
    SPEC_FUN    askari_combat;
    SPEC_FUN    surimoto_combat;
    SPEC_FUN    hiryushi_combat;
    SPEC_FUN    izumi_combat;
    SPEC_FUN    shogura_combat;
    SPEC_FUN    mage_familiar;

    /* I spent forever putting these fuckers in numerical order,
       keep um that way.  It makes no sense dividing them by what
       zone they load up in.  - pir */

    assign_one_mob_com(5,  mage_familiar);
    assign_one_mob_com(901,  active_magic_user2);
    assign_one_mob_com(903,  active_magic_user2);
    assign_one_mob_com(904,  active_magic_user2);
    assign_one_mob_com(905,  active_magic_user2);
    assign_one_mob_com(906,  fighter);
    assign_one_mob_com(907,  active_magic_user2);
    assign_one_mob_com(908,  active_magic_user);
    assign_one_mob_com(910,  fighter);
    assign_one_mob_com(914,  fighter);
    assign_one_mob_com(916,  active_magic_user);
    assign_one_mob_com(917,  fighter);
    assign_one_mob_com(918,  fighter);
    assign_one_mob_com(919,  active_magic_user2);
    assign_one_mob_com(921,  active_magic_user);
    assign_one_mob_com(1000,  active_magic_user); 
    assign_one_mob_com(1332,  active_magic_user);
    assign_one_mob_com(1337,  active_magic_user2);
    assign_one_mob_com(1338,  active_magic_user);
    assign_one_mob_com(1348,  active_magic_user);
    assign_one_mob_com(1349,  active_magic_user2);
    assign_one_mob_com(1352,  active_magic_user);
    assign_one_mob_com(1353,  active_magic_user);
    assign_one_mob_com(1354,  active_magic_user);
    assign_one_mob_com(1356,  active_magic_user);
    assign_one_mob_com(1357,  active_magic_user);
    assign_one_mob_com(1359,  active_magic_user);
    assign_one_mob_com(1360,  active_magic_user);
    assign_one_mob_com(1362,  active_magic_user);
    assign_one_mob_com(1364,  active_grandmaster);
    assign_one_mob_com(2704,  active_magic_user2);
    assign_one_mob_com(3059,  fighter);
    assign_one_mob_com(3404,  active_magic_user);
    assign_one_mob_com(3500,  snake);
    assign_one_mob_com(4713,  active_cleric);
    assign_one_mob_com(5005,  brass_dragon);
    assign_one_mob_com(5010,  red_dragon);
    assign_one_mob_com(5107,  active_magic_user2);
    assign_one_mob_com(5108,  active_magic_user2);
    assign_one_mob_com(5109,  active_magic_user2);
    assign_one_mob_com(5200,  active_magic_user2);
    assign_one_mob_com(5900,  turtle_green_combat);
    assign_one_mob_com(6112,  green_dragon);
    assign_one_mob_com(6113,  snake);
    assign_one_mob_com(6114,  snake);
    assign_one_mob_com(6296,  fighter);
    assign_one_mob_com(6302,  red_dragon);
    assign_one_mob_com(6316,  green_dragon);
    assign_one_mob_com(6317,  green_dragon);
    assign_one_mob_com(6500,  fighter);
    assign_one_mob_com(6508,  fighter);
    assign_one_mob_com(6517,  fighter);
    assign_one_mob_com(6910,  active_magic_user);
    assign_one_mob_com(8542,  iasenko_combat);
    assign_one_mob_com(8543,  koban_combat);
    assign_one_mob_com(8604,  kogiro_combat);
    assign_one_mob_com(8605,  takahashi_combat);
    assign_one_mob_com(8645,  askari_combat);
    assign_one_mob_com(8646,  surimoto_combat);
    assign_one_mob_com(8656,  hiryushi_combat);
    assign_one_mob_com(8657,  izumi_combat);
    assign_one_mob_com(8667,  shogura_combat);
    assign_one_mob_com(9003,  active_magic_user);
    assign_one_mob_com(9004,  active_cleric);
    assign_one_mob_com(9005,  fighter);
    assign_one_mob_com(9007,  fighter);
    assign_one_mob_com(9021,  red_dragon);
    assign_one_mob_com(9022,  black_dragon);
    assign_one_mob_com(9023,  white_dragon);
    assign_one_mob_com(9025,  green_dragon);
    assign_one_mob_com(9043,  active_cleric);
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
    assign_one_mob_com(10010,  secret_agent);
//    assign_one_mob_com(10100,  active_magic_user2);
//    assign_one_mob_com(10112,  fighter);
//    assign_one_mob_com(10116,  fighter);
//    assign_one_mob_com(10121,  fighter);
//    assign_one_mob_com(10123,  fighter);
//    assign_one_mob_com(10126,  fighter);
    assign_one_mob_com(13302,  clutchdrone_combat);
    assign_one_mob_com(14527,  bee);
    assign_one_mob_com(14528,  bee);
    assign_one_mob_com(14532,  bee);
    assign_one_mob_com(14539,  bee);
    assign_one_mob_com(15009,  fighter);
    assign_one_mob_com(15010,  fighter);
    assign_one_mob_com(17601,  acidhellstreamer);
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
    assign_one_mob_com(22014,  cry_dragon);
    assign_one_mob_com(22019,  foggy_combat);
    assign_one_mob_com(22340,  cry_dragon);    
    assign_one_mob_com(22330,  active_magic_user2);
    assign_one_mob_com(22701,  marauder);
    assign_one_mob_com(26208,  fighter);
    assign_one_mob_com(26200,  fighter);
    assign_one_mob_com(26212,  fighter);
    assign_one_mob_com(27123,  fighter);
    assign_one_mob_com(27124,  active_magic_user2);
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
  SPEC_FUN holyavenger;
  SPEC_FUN drainingstaff;
  SPEC_FUN souldrainer;
  SPEC_FUN emoting_object;
  SPEC_FUN returner;
  SPEC_FUN gem_assembler;
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
  SPEC_FUN gl_dragon_fire;
  SPEC_FUN dk_rend;
  SPEC_FUN magic_missile_boots;
  SPEC_FUN shield_combat_procs;
  SPEC_FUN generic_push_proc;
  SPEC_FUN generic_weapon_combat;
  SPEC_FUN TOHS_locator;
  SPEC_FUN stupid_message;
  SPEC_FUN goldenbatleth;

  assign_one_obj_non(9,  phish_locator);	
  assign_one_obj_non(13,  board); /* Quest Board */
  assign_one_obj_non(25,  mob_summoner);
  assign_one_obj_non(41,  restring_machine);
  assign_one_obj_non(40,  emoting_object);
  assign_one_obj_non(71,  board); /* Punishment Board */
  assign_one_obj_non(101,  globe_of_darkness_proc);
  assign_one_obj_non(185,  devilsword);
  assign_one_obj_non(225,  weenie_weedy);
  assign_one_obj_non(557,  hornoplenty);
  assign_one_obj_non(3090,  TOHS_locator);
  assign_one_obj_non(3099,  board); /* Mortal Board */ 
  assign_one_obj_non(3611,  pfe_word);
  assign_one_obj_non(9529,  pull_proc);
  assign_one_obj_non(9530,  no_magic_while_alive);
  assign_one_obj_non(9531,  boat_proc);
  assign_one_obj_non(9532,  leave_boat_proc);
  assign_one_obj_non(9606,  stupid_message);
  assign_one_obj_non(9996,  board); /* builder board */
  assign_one_obj_non(9997,  board); /* imp board */ 
  assign_one_obj_non(9999,  board); /* wiz board */ 
  assign_one_obj_non(9995,  bank);  /* bank */
//  assign_one_obj_non(17800,  arenaporter);
  assign_one_obj_non(26723,  generic_push_proc);
  assign_one_obj_non(30627,  eliara_non_combat);  


  // assembled items
  // forage arrow
//  assign_one_obj_non(2055,  gem_assembler);  
//  assign_one_obj_non(2056,  gem_assembler);  
//  assign_one_obj_non(2068,  gem_assembler);  


  // Crystalline tir
  assign_one_obj_non(12607,  gem_assembler);  
  assign_one_obj_non(2602,  gem_assembler);  
  assign_one_obj_non(2714,  gem_assembler);  

  // Etala
  assign_one_obj_non(181,  gem_assembler);  
  assign_one_obj_non(182,  gem_assembler);  
  assign_one_obj_non(183,  gem_assembler);  

  // DK Giaot key
  assign_one_obj_non(9502,  gem_assembler);  
  assign_one_obj_non(9503,  gem_assembler);  
  assign_one_obj_non(9504,  gem_assembler);  
  assign_one_obj_non(9505,  gem_assembler);  
  assign_one_obj_non(9506,  gem_assembler);  

  assign_one_obj_non(22399,  board);  // quests2do Board

  assign_one_obj_com(542,    gl_dragon_fire);
  assign_one_obj_com(740,    magic_missile_boots);
  assign_one_obj_com(2715,   shield_combat_procs);
  assign_one_obj_com(8208,   shield_combat_procs);
  assign_one_obj_com(9520,   dk_rend);
  assign_one_obj_com(30627,  eliara_combat);  
  assign_one_obj_com(10000,  holyavenger);
  assign_one_obj_com(10001,  holyavenger);
  assign_one_obj_com(26807,  goldenbatleth);
  assign_one_obj_com(16903,  generic_weapon_combat);

//  assign_one_obj_com(22732,  drainingstaff);
//  assign_one_obj_com(22743,  bonusattack);
//  assign_one_obj_com(22605,  souldrainer);

	return;
}



/* assign special procedures to rooms */
void assign_rooms(void)
{
    ROOM_PROC pet_shops;
    if(real_room(3031) >= 0) 
		world[real_room(3031)].funct = pet_shops;
}
