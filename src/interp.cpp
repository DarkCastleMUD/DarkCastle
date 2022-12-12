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
/* $Id: interp.cpp,v 1.200 2015/06/14 02:38:12 pirahna Exp $ */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

#include <string>
#include <tuple>

#include <fmt/format.h>

#include "structs.h"   // MAX_STRING_LENGTH
#include "character.h" // POSITION_*
#include "interp.h"
#include "levels.h"
#include "utility.h"
#include "player.h"
#include "fight.h"
#include "spells.h" // ETHERAL consts
#include "mobile.h"
#include "connect.h" // descriptor_data
#include "room.h"
#include "db.h"
#include "act.h"
#include "returnvals.h"
#include "terminal.h"
#include "CommandStack.h"
#include "const.h"

using namespace std;
using namespace fmt;

#define SKILL_HIDE 337

int clan_guard(char_data *ch, struct obj_data *obj, int cmd, const char *arg, char_data *owner);
int check_ethereal_focus(char_data *ch, int trigger_type); // class/cl_mage.cpp

extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern CWorld world;

// globals to store last command that was done.
// this is used for debugging.  We output it in case of a crash
// to the log files.  (char name is so long, in case it was a mob)
string last_processed_cmd = {};
string last_char_name = {};
int last_char_room = {};
unsigned int cmd_size = 0;

void update_wizlist(char_data *ch);
// int system(const char *);

bool can_use_command(char_data *ch, int cmdnum);

void add_command_to_radix(struct command_info *cmd);

struct command_lag *command_lag_list = NULL;

// **DEFINE LIST FOUND IN interp.h**

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
// The command number should be CMD_DEFAULT for any user command that is not used
// in a spec_proc.  If it is, then it should be a number that is not
// already in use.
struct command_info cmd_info[] =
    {
        // Movement commands
        {"north", do_move, nullptr, POSITION_STANDING, 0, CMD_NORTH, COM_CHARMIE_OK, 1},
        {"east", do_move, nullptr, POSITION_STANDING, 0, CMD_EAST, COM_CHARMIE_OK, 1},
        {"south", do_move, nullptr, POSITION_STANDING, 0, CMD_SOUTH, COM_CHARMIE_OK, 1},
        {"west", do_move, nullptr, POSITION_STANDING, 0, CMD_WEST, COM_CHARMIE_OK, 1},
        {"up", do_move, nullptr, POSITION_STANDING, 0, CMD_UP, COM_CHARMIE_OK, 1},
        {"down", do_move, nullptr, POSITION_STANDING, 0, CMD_DOWN, COM_CHARMIE_OK, 1},

        // Common commands
        {"newbie", do_newbie, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"cast", do_cast, nullptr, POSITION_SITTING, 0, CMD_DEFAULT, 0, 0},
        {"filter", do_cast, nullptr, POSITION_SITTING, 0, CMD_FILTER, 0, 0},
        {"sing", do_sing, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"exits", do_exits, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"f", do_fire, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 25},
        {"get", do_get, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"inventory", do_inventory, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"k", do_kill, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"ki", do_ki, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"kill", do_kill, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"look", do_look, nullptr, POSITION_RESTING, 0, CMD_LOOK, COM_CHARMIE_OK, 1},
        {"loot", do_get, nullptr, POSITION_RESTING, 0, CMD_LOOT, 0, 0},
        {"glance", do_look, nullptr, POSITION_RESTING, 0, CMD_GLANCE, COM_CHARMIE_OK, 1},
        {"order", do_order, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"rest", do_rest, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"recite", do_recite, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"recall", do_recall, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"score", do_score, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"scan", do_scan, nullptr, POSITION_RESTING, 1, CMD_DEFAULT, 0, 25},
        {"stand", do_stand, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"switch", do_switch, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 25},
        {"tell", nullptr, do_tell, POSITION_RESTING, 0, CMD_TELL, 0, 1},
        {"tellhistory", nullptr, do_tellhistory, POSITION_RESTING, 0, CMD_TELLH, 0, 1},
        {"wield", do_wield, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"innate", do_innate, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"orchestrate", do_sing, nullptr, POSITION_RESTING, 0, CMD_ORCHESTRATE, 0, 0},

        // Informational commands
        {"alias", do_alias, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"toggle", do_toggle, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"consider", do_consider, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"configure", do_config, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"credits", do_credits, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"equipment", do_equipment, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"ohelp", do_help, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"help", do_new_help, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"idea", do_idea, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"info", do_info, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"leaderboard", do_leaderboard, nullptr, POSITION_DEAD, 3, CMD_DEFAULT, 0, 1},
        {"news", do_news, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"thenews", do_news, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"story", do_story, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"tick", do_tick, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 0},
        {"time", do_time, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"title", do_title, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"typo", do_typo, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"weather", do_weather, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"who", do_who, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"wizlist", do_wizlist, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"socials", do_social, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 0},
        {"index", do_index, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"areas", do_areas, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"commands", do_new_help, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"experience", do_show_exp, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"version", do_version, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 0},
        {"identify", do_identify, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},

        // Communication commands
        {"ask", do_ask, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"auction", do_auction, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"awaymsgs", do_awaymsgs, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"channel", do_channel, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"dream", do_dream, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"emote", do_emote, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {":", do_emote, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"gossip", do_gossip, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"trivia", do_trivia, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"gtell", do_grouptell, nullptr, POSITION_DEAD, 0, CMD_GTELL, 0, 1},
        {".", do_grouptell, nullptr, POSITION_DEAD, 0, CMD_GTELL, 0, 1},
        {"ignore", nullptr, do_ignore, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"insult", do_insult, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"reply", nullptr, do_reply, POSITION_RESTING, 0, CMD_REPLY, 0, 1},
        {"report", do_report, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"say", nullptr, do_say, POSITION_RESTING, 0, CMD_SAY, COM_CHARMIE_OK, 0},
        {"psay", nullptr, do_psay, POSITION_RESTING, 0, CMD_SAY, COM_CHARMIE_OK, 0},
        {"'", nullptr, do_say, POSITION_RESTING, 0, CMD_SAY, COM_CHARMIE_OK, 0},
        {"shout", do_shout, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"whisper", do_whisper, nullptr, POSITION_RESTING, 0, CMD_WHISPER, 0, 0},

        // Object manipulation
        {"slip", do_slip, nullptr, POSITION_STANDING, 0, CMD_SLIP, 0, 1},
        {"batter", do_batter, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0},
        {"brace", do_brace, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0},
        {"close", do_close, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"donate", do_donate, nullptr, POSITION_RESTING, 0, CMD_DONATE, COM_CHARMIE_OK, 1},
        {"drink", do_drink, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"drop", do_drop, nullptr, POSITION_RESTING, 0, CMD_DROP, COM_CHARMIE_OK, 25},
        {"eat", do_eat, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"fill", do_fill, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"give", do_give, nullptr, POSITION_RESTING, 0, CMD_GIVE, COM_CHARMIE_OK, 25},
        {"grab", do_grab, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"hold", do_grab, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"lock", do_lock, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"open", do_open, nullptr, POSITION_RESTING, 0, CMD_OPEN, COM_CHARMIE_OK, 25},
        {"pour", do_pour, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"put", do_put, nullptr, POSITION_RESTING, 0, CMD_PUT, COM_CHARMIE_OK, 0},
        {"quaff", do_quaff, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"read", do_read, nullptr, POSITION_RESTING, 0, CMD_READ, 0, 1},
        {"remove", do_remove, nullptr, POSITION_RESTING, 0, CMD_REMOVE, COM_CHARMIE_OK, 25},
        {"erase", do_not_here, nullptr, POSITION_RESTING, 0, CMD_ERASE, 0, 0},
        {"sip", do_sip, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"track", do_track, nullptr, POSITION_STANDING, 0, CMD_TRACK, 0, 10},
        {"take", do_get, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"palm", do_get, nullptr, POSITION_RESTING, 3, CMD_PALM, 0, 1},
        {"sacrifice", do_sacrifice, nullptr, POSITION_RESTING, 0, CMD_SACRIFICE, COM_CHARMIE_OK, 25},
        {"taste", do_taste, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"unlock", do_unlock, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"use", do_use, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"wear", do_wear, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25},
        {"scribe", do_scribe, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"brew", do_brew, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        //  { "poisonmaking", do_poisonmaking, nullptr, POSITION_RESTING, 0, 9,   0, 0 },

        // Combat commands
        {"bash", do_bash, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0},
        {"retreat", do_retreat, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0},
        {"disarm", do_disarm, nullptr, POSITION_FIGHTING, 2, CMD_DEFAULT, 0, 0},
        {"flee", do_flee, nullptr, POSITION_FIGHTING, 0, CMD_FLEE, COM_CHARMIE_OK, 0},
        {"escape", do_flee, nullptr, POSITION_FIGHTING, 0, CMD_ESCAPE, 0, 0},
        {"hit", do_hit, nullptr, POSITION_FIGHTING, 0, CMD_HIT, COM_CHARMIE_OK, 0},
        {"join", do_join, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"battlesense", do_battlesense, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0},
        {"stance", do_defenders_stance, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0},
        {"perseverance", do_perseverance, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0},
        {"smite", do_smite, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0},

        // Junk movedso join precedes it
        {"junk", do_sacrifice, nullptr, POSITION_RESTING, 0, CMD_SACRIFICE, COM_CHARMIE_OK, 25},

        {"murder", do_murder, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"rescue", do_rescue, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0},
        {"trip", do_trip, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0},
        {"deathstroke", do_deathstroke, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"circle", do_circle, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"kick", do_kick, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0},
        {"battlecry", do_battlecry, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"behead", do_behead, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"rage", do_rage, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"berserk", do_berserk, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"golemscore", do_golem_score, nullptr, POSITION_DEAD, 1, CMD_DEFAULT, 0, 1},
        {"stun", do_stun, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"redirect", do_redirect, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"hitall", do_hitall, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"quiveringpalm", do_quivering_palm, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"eagleclaw", do_eagle_claw, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"headbutt", do_headbutt, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"cripple", do_cripple, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"fire", do_fire, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 25},
        {"layhands", do_layhands, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"harmtouch", do_harmtouch, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"bloodfury", do_bloodfury, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"primalfury", do_primalfury, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"bladeshield", do_bladeshield, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"repelance", do_focused_repelance, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"vitalstrike", do_vitalstrike, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"crazedassault", do_crazedassault, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"bullrush", do_bullrush, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0},
        {"ferocity", do_ferocity, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0},
        {"tactics", do_tactics, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0},
        {"deceit", do_deceit, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0},
        {"knockback", do_knockback, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0},
        {"appraise", do_appraise, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 1},
        {"make camp", do_make_camp, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"leadership", do_leadership, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"triage", do_triage, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"onslaught", do_onslaught, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"pursue", do_pursue, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0},

        // Position commands
        {"sit", do_sit, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0},
        {"sleep", do_sleep, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 0},
        {"wake", do_wake, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 0},

        // Miscellaneous commands
        {"editor", do_editor, nullptr, POSITION_SLEEPING, CMD_EDITOR, CMD_DEFAULT, 0, 1},
        {"autojoin", nullptr, do_autojoin, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"visible", do_visible, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"ctell", do_ctell, nullptr, POSITION_SLEEPING, 0, CMD_CTELL, 0, 1},
        {"outcast", do_outcast, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"accept", do_accept, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"whoclan", do_whoclan, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"cpromote", do_cpromote, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"clans", do_clans, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"clanarea", do_clanarea, nullptr, POSITION_RESTING, 11, CMD_DEFAULT, 0, 1},
        {"cinfo", do_cinfo, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"ambush", do_ambush, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"whoarena", do_whoarena, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"joinarena", do_joinarena, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 0},
        {"backstab", do_backstab, nullptr, POSITION_STANDING, 0, CMD_BACKSTAB, 0, 0},
        {"bs", do_backstab, nullptr, POSITION_STANDING, 0, CMD_BACKSTAB, 0, 0},
        {"sbs", do_backstab, nullptr, POSITION_STANDING, 0, CMD_SBS, 0, 0}, // single backstab
        {"boss", do_boss, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"jab", do_jab, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0},
        {"enter", do_enter, nullptr, POSITION_STANDING, 0, CMD_ENTER, COM_CHARMIE_OK, 20},
        {"climb", do_climb, nullptr, POSITION_STANDING, 0, CMD_CLIMB, COM_CHARMIE_OK, 20},
        {"examine", do_examine, nullptr, POSITION_RESTING, 0, CMD_EXAMINE, 0, 1},
        {"follow", do_follow, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"stalk", do_stalk, nullptr, POSITION_STANDING, 6, CMD_DEFAULT, 0, 10},
        {"group", do_group, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 0},
        {"found", do_found, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"disband", do_disband, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"abandon", do_abandon, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"consent", do_consent, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"whogroup", do_whogroup, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"forage", do_forage, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0},
        {"whosolo", do_whosolo, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"count", do_count, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"hide", do_hide, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"leave", do_leave, nullptr, POSITION_STANDING, 0, CMD_LEAVE, COM_CHARMIE_OK, 20},
        {"name", do_name, nullptr, POSITION_DEAD, 1, CMD_DEFAULT, 0, 1},
        {"pick", do_pick, nullptr, POSITION_STANDING, 0, CMD_PICK, 0, 20},
        {"quest", do_quest, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"qui", do_qui, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"levels", do_levels, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"quit", do_quit, nullptr, POSITION_DEAD, 0, CMD_QUIT, 0, 1},
        {"return", do_return, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, COM_CHARMIE_OK, 1},
        {"tame", do_tame, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"free animal", do_free_animal, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"prompt", do_prompt, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"lastprompt", do_lastprompt, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"save", do_save, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"sneak", do_sneak, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0},
        {"home", do_home, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"split", do_split, nullptr, POSITION_RESTING, 0, CMD_SPLIT, 0, 0},
        {"spells", do_spells, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"skills", do_skills, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"songs", do_songs, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1},
        {"steal", do_steal, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 10},
        {"pocket", do_pocket, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 10},
        {"motd", do_motd, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"cmotd", do_cmotd, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"cbalance", do_cbalance, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0},
        {"cdeposit", do_cdeposit, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0},
        {"cwithdraw", do_cwithdraw, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0},
        {"ctax", do_ctax, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 1},
        {"where", do_where, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"write", do_write, nullptr, POSITION_STANDING, 0, CMD_WRITE, 0, 0},
        {"beacon", do_beacon, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"leak", do_memoryleak, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"beep", do_beep, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"guard", do_guard, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"release", do_release, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 1},
        {"eyegouge", do_eyegouge, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0},
        {"vault", do_vault, nullptr, POSITION_DEAD, 10, CMD_DEFAULT, 0, 0},
        {"suicide", do_suicide, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"vote", do_vote, nullptr, POSITION_RESTING, 0, CMD_VOTE, 0, 0},
        {"huntitems", do_showhunt, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"random", do_random, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        // Special procedure commands

        {"vend", do_vend, nullptr, POSITION_STANDING, 2, CMD_VEND, 0, 0},
        {"gag", do_not_here, nullptr, POSITION_STANDING, 0, CMD_GAG, 0, 0},
        {"design", do_not_here, nullptr, POSITION_STANDING, 0, CMD_DESIGN, 0, 0},
        {"stock", do_not_here, nullptr, POSITION_STANDING, 0, CMD_STOCK, 0, 0},
        {"buy", do_not_here, nullptr, POSITION_STANDING, 0, CMD_BUY, 0, 0},
        {"sell", do_not_here, nullptr, POSITION_STANDING, 0, CMD_SELL, 0, 0},
        {"value", do_not_here, nullptr, POSITION_STANDING, 0, CMD_VALUE, 0, 0},
        {"watch", do_not_here, nullptr, POSITION_STANDING, 0, CMD_WATCH, 0, 0},
        {"list", do_not_here, nullptr, POSITION_STANDING, 0, CMD_LIST, 0, 0},
        {"estimate", do_not_here, nullptr, POSITION_STANDING, 0, CMD_ESTIMATE, 0, 0},
        {"repair", do_not_here, nullptr, POSITION_STANDING, 0, CMD_REPAIR, 0, 0},
        {"practice", do_practice, nullptr, POSITION_SLEEPING, 1, CMD_PRACTICE, 0, 0},
        {"practise", do_practice, nullptr, POSITION_SLEEPING, 1, CMD_PRACTICE, 0, 0},
        {"pray", do_pray, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"profession", do_profession, nullptr, POSITION_SLEEPING, 1, CMD_PROFESSION, 0, 0},
        {"promote", do_promote, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0},
        {"price", do_not_here, nullptr, POSITION_RESTING, 1, CMD_PRICE, 0, 0},
        {"train", do_not_here, nullptr, POSITION_RESTING, 1, CMD_TRAIN, 0, 0},
        {"gain", do_not_here, nullptr, POSITION_STANDING, 1, CMD_GAIN, 0, 0},
        {"balance", do_not_here, nullptr, POSITION_STANDING, 0, CMD_BALANCE, 0, 0},
        {"deposit", do_not_here, nullptr, POSITION_STANDING, 0, CMD_DEPOSIT, 0, 0},
        {"withdraw", do_not_here, nullptr, POSITION_STANDING, 0, CMD_WITHDRAW, 0, 0},
        {"clean", do_not_here, nullptr, POSITION_RESTING, 0, CMD_CLEAN, 0, 0},
        {"play", do_not_here, nullptr, POSITION_RESTING, 0, CMD_PLAY, 0, 0},
        {"finish", do_not_here, nullptr, POSITION_RESTING, 0, CMD_FINISH, 0, 0},
        {"veternarian", do_not_here, nullptr, POSITION_RESTING, 0, CMD_VETERNARIAN, 0, 0},
        {"feed", do_not_here, nullptr, POSITION_RESTING, 0, CMD_FEED, 0, 0},
        {"assemble", do_assemble, nullptr, POSITION_RESTING, 0, CMD_ASSEMBLE, 0, 0},
        {"pay", do_not_here, nullptr, POSITION_STANDING, 0, CMD_PAY, 0, 0},
        {"restring", do_not_here, nullptr, POSITION_STANDING, 0, CMD_RESTRING, 0, 0},
        {"push", do_not_here, nullptr, POSITION_STANDING, 0, CMD_PUSH, 0, 0},
        {"pull", do_not_here, nullptr, POSITION_STANDING, 0, CMD_PULL, 0, 0},
        {"gaze", do_not_here, nullptr, POSITION_FIGHTING, 0, CMD_GAZE, 0, 0},
        {"tremor", do_not_here, nullptr, POSITION_FIGHTING, 0, CMD_TREMOR, 0, 0},
        {"bet", do_not_here, nullptr, POSITION_STANDING, 0, CMD_BET, 0, 0},
        {"insurance", do_not_here, nullptr, POSITION_STANDING, 0, CMD_INSURANCE, 0, 0},
        {"double", do_not_here, nullptr, POSITION_STANDING, 0, CMD_DOUBLE, 0, 0},
        {"stay", do_not_here, nullptr, POSITION_STANDING, 0, CMD_STAY, 0, 0},
        {"select", do_natural_selection, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0},
        {"sector", do_sector, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1},
        {"remort", do_not_here, nullptr, POSITION_RESTING, GIFTED_COMMAND, CMD_REMORT, 0, 1},
        {"reroll", do_not_here, nullptr, POSITION_RESTING, 0, CMD_REROLL, 0, 0},
        {"choose", do_not_here, nullptr, POSITION_RESTING, 0, CMD_CHOOSE, 0, 0},
        {"confirm", do_not_here, nullptr, POSITION_RESTING, 0, CMD_CONFIRM, 0, 0},
        {"cancel", do_not_here, nullptr, POSITION_RESTING, 0, CMD_CANCEL, 0, 0},

        // Immortal commands
        {"voteset", do_setvote, nullptr, POSITION_DEAD, 108, CMD_SETVOTE, 0, 1},
        {"thunder", do_thunder, nullptr, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},
        {"wizlock", do_wizlock, nullptr, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},
        {"processes", do_processes, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1},
        {"bestow", nullptr, do_bestow, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},
        {"oclone", do_oclone, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1},
        {"mclone", do_mclone, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1},
        {"huntclear", do_huntclear, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1},
        {"areastats", do_areastats, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1},
        {"huntstart", do_huntstart, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1},
        {"revoke", do_revoke, nullptr, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},
        {"chpwd", do_chpwd, nullptr, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},
        {"advance", do_advance, nullptr, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},
        {"skillmax", do_maxes, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1},
        {"damage", do_dmg_eq, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1},
        {"affclear", do_clearaff, nullptr, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1},
        {"guide", do_guide, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"addnews", do_addnews, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"linkload", do_linkload, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1},
        {"listproc", do_listproc, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1},
        {"zap", do_zap, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1},
        {"slay", do_slay, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1},
        {"rename", do_rename_char, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"archive", do_archive, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1},
        {"unarchive", do_unarchive, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1},
        {"stealth", do_stealth, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1},
        {"disconnect", do_disconnect, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"force", nullptr, do_force, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"pardon", do_pardon, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1},
        {"goto", nullptr, do_goto, POSITION_DEAD, 102, CMD_DEFAULT, 0, 1},
        {"restore", do_restore, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"purloin", do_purloin, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"set", do_set, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"unban", do_unban, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1},
        {"ban", do_ban, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1},
        {"echo", do_echo, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"eqmax", do_eqmax, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1},
        {"send", do_send, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"at", do_at, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"fakelog", do_fakelog, nullptr, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},
        {"global", do_global, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1},
        {"log", do_log, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"snoop", do_snoop, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"pview", do_pview, nullptr, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1},
        {"arena", do_arena, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"load", do_load, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"prize", do_load, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_PRIZE, 0, 1},
        {"testport", do_testport, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"testuser", do_testuser, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"shutdow", do_shutdow, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"shutdown", do_shutdown, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"opedit", do_opedit, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"opstat", do_opstat, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"procedit", do_procedit, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"procstat", do_mpstat, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"range", do_range, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        // { "pshopedit",	do_pshopedit,	POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1 },
        {"sedit", do_sedit, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"sockets", do_sockets, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"punish", do_punish, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"sqedit", do_sqedit, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"qedit", do_qedit, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"install", do_install, nullptr, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},
        //  { "motdload",       do_motdload,    POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1 },
        {"hedit", do_hedit, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"hindex", do_hindex, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"reload", do_reload, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1},
        {"plats", do_plats, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"bellow", do_thunder, nullptr, POSITION_DEAD, DEITY, CMD_BELLOW, 0, 1},
        {"string", do_string, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"transfer", nullptr, do_transfer, POSITION_DEAD, DEITY, CMD_DEFAULT, 0, 1},
        {"gtrans", do_gtrans, nullptr, POSITION_DEAD, DEITY, CMD_DEFAULT, 0, 1},
        {"boot", do_boot, nullptr, POSITION_DEAD, DEITY, CMD_DEFAULT, 0, 1},
        {"linkdead", do_linkdead, nullptr, POSITION_DEAD, DEITY, CMD_DEFAULT, 0, 1},
        {"teleport", do_teleport, nullptr, POSITION_DEAD, DEITY, CMD_DEFAULT, 0, 1},
        {"purge", do_purge, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1},
        {"show", do_show, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"search", do_search, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"fighting", do_fighting, nullptr, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1},
        {"peace", do_peace, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"check", do_check, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1},
        {"zoneexits", do_zoneexits, nullptr, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1},
        {"find", do_find, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"stat", do_stat, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"redit", do_redit, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"guild", do_guild, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"oedit", do_oedit, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"clear", do_clear, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"repop", nullptr, do_repop, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"medit", do_medit, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"rdelete", do_rdelete, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"oneway", do_oneway, nullptr, POSITION_DEAD, ANGEL, 1, 0, 1},
        {"twoway", do_oneway, nullptr, POSITION_DEAD, ANGEL, 2, 0, 1},
        {"zsave", do_zsave, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"rsave", do_rsave, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"msave", do_msave, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"osave", do_osave, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"rstat", do_rstat, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"possess", do_possess, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"fsave", nullptr, do_fsave, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1},
        {"zedit", do_zedit, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1},
        {"colors", do_colors, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"colours", do_colors, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"incognito", do_incognito, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"high5", do_highfive, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"holylite", do_holylite, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"immort", nullptr, do_wiz, POSITION_DEAD, IMMORTAL, CMD_IMMORT, 0, 1},
        {";", nullptr, do_wiz, POSITION_DEAD, IMMORTAL, CMD_IMMORT, 0, 1},
        {"impchan", nullptr, do_wiz, POSITION_DEAD, GIFTED_COMMAND, CMD_IMPCHAN, 0, 1},
        {"/", nullptr, do_wiz, POSITION_DEAD, GIFTED_COMMAND, CMD_IMPCHAN, 0, 1},
        {"nohassle", do_nohassle, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"wizinvis", do_wizinvis, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"poof", do_poof, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"wizhelp", do_wizhelp, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"imotd", do_imotd, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"mhelp", do_mortal_help, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1},
        {"testhand", do_testhand, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"varstat", do_varstat, nullptr, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1},
        {"matrixinfo", do_matrixinfo, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1},
        {"maxcheck", do_findfix, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1},
        {"export", do_export, nullptr, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},
        {"mscore", do_mscore, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1},
        {"world", nullptr, do_world, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},

        // Special procedure commands placed to not disrupt god commands
        {"setup", do_mortal_set, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 1},
        {"metastat", do_metastat, nullptr, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},
        {"testhit", do_testhit, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"acfinder", do_acfinder, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"findpath", do_findPath, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"findpath2", do_findpath, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"addroom", do_addRoom, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"newpath", do_newPath, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"listpathsbyzone", do_listPathsByZone, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"listallpaths", do_listAllPaths, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"dopathpath", do_pathpath, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1},
        {"botcheck", do_botcheck, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1},
        {"showbits", do_showbits, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1},
        {"debug", do_debug, nullptr, POSITION_DEAD, IMP, CMD_DEFAULT, 0, 1},

        // Bug way down here after 'buy'
        {"bug", do_bug, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        // imbue after 'im' for us lazy immortal types :)
        {"imbue", do_imbue, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0},

        // MOBprogram commands
        {"mpasound", do_mpasound, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpbestow", do_mpbestow, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpjunk", do_mpjunk, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpecho", do_mpecho, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpechoat", do_mpechoat, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpechoaround", do_mpechoaround, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpechoaroundnotbad", do_mpechoaroundnotbad, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpkill", do_mpkill, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mphit", do_mphit, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpaddlag", do_mpaddlag, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpmload", do_mpmload, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpoload", do_mpoload, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mppurge", do_mppurge, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpgoto", do_mpgoto, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpat", do_mpat, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mptransfer", do_mptransfer, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpthrow", do_mpthrow, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpforce", do_mpforce, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mppeace", do_mppeace, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpsetalign", do_mpsetalign, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpsettemp", do_mpsettemp, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpxpreward", do_mpxpreward, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpteachskill", do_mpteachskill, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpdamage", do_mpdamage, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpothrow", do_mpothrow, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mppause", do_mppause, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpretval", do_mpretval, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpsetmath", do_mpsetmath, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},
        {"mpteleport", do_mpteleport, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1},

        // End of the line
        {"", do_not_here, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0}};

const char *fillwords[] =
    {
        "in",
        "from",
        "with",
        "the",
        "on",
        "at",
        "to",
        "\n"};

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
  cmd_radix->left = 0;
  cmd_radix->right = 0;
  cmd_size = (sizeof(cmd_info) / sizeof(cmd_info[0]) - 1);

  for (x = 1; (unsigned)x < cmd_size; x++)
    add_command_to_radix(&cmd_info[x]);
}

void free_command_radix_nodes(struct cmd_hash_info *curr)
{
  if (curr->left)
    free_command_radix_nodes(curr->left);
  if (curr->right)
    free_command_radix_nodes(curr->right);
  dc_free(curr);
}

void add_command_to_radix(struct command_info *cmd)
{
  struct cmd_hash_info *curr = NULL;
  struct cmd_hash_info *temp = NULL;
  struct cmd_hash_info *next = NULL;
  int whichway = 0;

  // At the end of this loop, temp will contain the parent of
  // the new node.  Whether it is the left or right node depends
  // on whether whichway is positive or negative.
  for (curr = cmd_radix; curr; curr = next)
  {
    if ((whichway = strcmp(cmd->command_name, curr->command->command_name)) < 0)
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
  curr->left = 0;
  curr->right = 0;

  if (whichway < 0)
    temp->left = curr;
  else
    temp->right = curr;
}

int len_cmp(const char *s1, const char *s2)
{
  for (; *s1 && *s1 != ' '; s1++, s2++)
    if (*s1 != *s2)
      return *s1 - *s2;

  return 0;
}

struct command_info *find_cmd_in_radix(const char *arg)
{
  struct cmd_hash_info *curr;
  struct cmd_hash_info *next;
  int whichway;

  for (curr = cmd_radix; curr; curr = next)
  {
    if ((whichway = len_cmp(arg, curr->command->command_name)) == 0)
      return curr->command;
    if (whichway < 0)
      next = curr->left;
    else
      next = curr->right;
  }

  return 0;
}

int do_motd(char_data *ch, char *arg, int cmd)
{
  extern char motd[];

  page_string(ch->desc, motd, 1);
  return eSUCCESS;
}

int do_imotd(char_data *ch, char *arg, int cmd)
{
  extern char imotd[];

  page_string(ch->desc, imotd, 1);
  return eSUCCESS;
}

int command_interpreter(char_data *ch, string pcomm, bool procced)
{
  CommandStack cstack;

  if (cstack.isOverflow() == true)
  {
    // Prevent errors from showing up multiple times per loop
    if (cstack.getOverflowCount() < 2)
    {
      if (ch != nullptr && pcomm.empty() == false && GET_NAME(ch) != nullptr)
      {
        log(format("Command stack exceeded. depth: {}, max_depth: {}, name: {}, cmd: {}", cstack.getDepth(), cstack.getMax(), GET_NAME(ch), pcomm), IMMORTAL, LogChannels::LOG_BUG);
      }
      else
      {
        log(format("CommandStack::depth {} exceeds CommandStack::max_depth {}", cstack.getDepth(), cstack.getMax()), IMMORTAL, LogChannels::LOG_BUG);
      }
    }
    return eFAILURE;
  }

  int look_at;
  int retval;
  struct command_info *found = 0;
  string buf;

  // Handle logged players.
  if (IS_PC(ch) && IS_SET(ch->pcdata->punish, PUNISH_LOG))
  {
    buf = format("Log {}: {}", GET_NAME(ch), pcomm);
    log(buf, 110, LogChannels::LOG_PLAYER, ch);
  }

  // Implement freeze command.
  if (IS_PC(ch) && IS_SET(ch->pcdata->punish, PUNISH_FREEZE) && pcomm != "quit")
  {
    send_to_char("You've been frozen by an immortal.\r\n", ch);
    return eSUCCESS;
  }
  if (IS_AFFECTED(ch, AFF_PRIMAL_FURY))
  {
    send_to_char("SOMEMESSAGE\r\n", ch);
    return eSUCCESS;
  }

  // Berserk checks
  if (IS_SET(ch->combat, COMBAT_BERSERK))
  {
    if (ch->fighting)
    {
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
  if (ch == nullptr || ch->desc == nullptr || ch->desc->connected != conn::EDITING)
  {
    pcomm = ltrim(pcomm);
  }

  // MOBprogram commands weeded out
  if (pcomm.size() >= 2 && tolower(pcomm[0]) == 'm' && tolower(pcomm[1]) == 'p')
  {
    if (ch->desc)
    {
      send_to_char("Huh?\r\n", ch);
      return eSUCCESS;
    }
  }

  for (look_at = 0; pcomm[look_at] > ' '; look_at++)
    pcomm[look_at] = LOWER(pcomm[look_at]);

  if (look_at == 0)
    return eFAILURE;

  // if we got this far, we're going to play with the command, so put
  // it into the debugging globals
  last_processed_cmd = pcomm;
  last_char_name = GET_NAME(ch);
  last_char_room = ch->in_room;

  if (!pcomm.empty())
  {
    retval = oprog_command_trigger(pcomm.c_str(), ch, &pcomm[look_at]);
    if (SOMEONE_DIED(retval) || IS_SET(retval, eEXTRA_VALUE))
      return retval;
  }

  // Look for command in command table.
  // Old method used a linear search. *yuck* (Sadus)
  if ((found = find_cmd_in_radix(pcomm.c_str())))
  {
    if (GET_LEVEL(ch) >= found->minimum_level && (found->command_pointer != nullptr || found->command_pointer2 != nullptr))
    {
      if (found->minimum_level == GIFTED_COMMAND)
      {

        // search bestowable_god_commands for the command skill number to lookup with has_skill
        int command_skill = 0;
        for (int i = 0; *bestowable_god_commands[i].name != '\n'; i++)
        {
          if (string(bestowable_god_commands[i].name) == string(found->command_name))
          {
            command_skill = bestowable_god_commands[i].num;
            break;
          }
        }

        if (command_skill == 0 || IS_NPC(ch) || !has_skill(ch, command_skill))
        {
          send_to_char("Huh?\r\n", ch);

          if (command_skill == 0)
          {
            logf(IMMORTAL, LogChannels::LOG_BUG, "Unable to find command [%s] within bestowable_god_commands", found->command_name);
          }
          return eFAILURE;
        }
      }

      // Paralysis stops everything but ...
      if (IS_AFFECTED(ch, AFF_PARALYSIS) &&
          found->command_number != CMD_GTELL && // gtell
          found->command_number != CMD_CTELL    // ctell
      )
      {
        send_to_char("You've been paralyzed and are unable to move.\r\n", ch);
        return eSUCCESS;
      }
      // Character not in position for command?
      if (GET_POS(ch) == POSITION_FIGHTING && !ch->fighting)
        GET_POS(ch) = POSITION_STANDING;
      // fix for thin air thing
      if (GET_POS(ch) < found->minimum_position)
      {
        switch (GET_POS(ch))
        {
        case POSITION_DEAD:
          send_to_char("Lie still; you are DEAD.\r\n", ch);
          break;
        case POSITION_STUNNED:
          send_to_char("You are too stunned to do that.\n\r", ch);
          break;
        case POSITION_SLEEPING:
          send_to_char("In your dreams, or what?\r\n", ch);
          break;
        case POSITION_RESTING:
          send_to_char("Nah... You feel too relaxed...\r\n", ch);
          break;
        case POSITION_SITTING:
          send_to_char("Maybe you should stand up first?\r\n", ch);
          break;
        case POSITION_FIGHTING:
          send_to_char("No way!  You are still fighting!\r\n", ch);
          break;
        }
        return eSUCCESS;
      }

      // charmies can only use charmie "ok" commands
      if (!procced) // Charmed mobs can still use their procs.
        if ((IS_AFFECTED(ch, AFF_FAMILIAR) || IS_AFFECTED(ch, AFF_CHARM)) && !IS_SET(found->flags, COM_CHARMIE_OK))
          return do_say(ch, "I'm sorry master, I cannot do that.", CMD_DEFAULT);
      if (IS_NPC(ch) && ch->desc && ch->desc->original &&
          ch->desc->original->level <= MAX_MORTAL && !IS_SET(found->flags, COM_CHARMIE_OK))
      {
        send_to_char("The spirit cannot perform that action.\r\n", ch);
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
      log (DEBUGbuf, 0, LogChannels::LOG_MISC);
      */
      if (!can_use_command(ch, found->command_number))
      {
        send_to_char("You are still recovering from your last attempt.\r\n", ch);
        return eSUCCESS;
      }

      if (IS_PC(ch))
      {
        DC *dc = dynamic_cast<DC *>(DC::instance());
        // Don't log communication
        if (found->command_number != CMD_GTELL && found->command_number != CMD_CTELL && found->command_number != CMD_SAY && found->command_number != CMD_IMMORT && found->command_number != CMD_IMPCHAN && found->command_number != CMD_TELL && found->command_number != CMD_WHISPER && found->command_number != CMD_REPLY && (GET_LEVEL(ch) >= 100 || (ch->pcdata->multi == true && dc->cf.allow_multi == false)) && IS_SET(ch->pcdata->punish, PUNISH_LOG) == false)
        {
          log(format("Log {}: {}", GET_NAME(ch), pcomm), 110, LogChannels::LOG_PLAYER, ch);
        }
      }

      // We're going to execute, check for usable special proc.
      retval = special(ch, found->command_number, &pcomm[look_at]);
      if (IS_SET(retval, eSUCCESS) || IS_SET(retval, eCH_DIED))
        return retval;

      // Normal dispatch
      if (found->command_pointer)
      {
        retval = (*(found->command_pointer))(ch, &pcomm[look_at], found->command_number);
      }
      else if (found->command_pointer2)
      {
        retval = (*(found->command_pointer2))(ch, &pcomm[look_at], found->command_number);
      }

      // Next bit for the DUI client, they needed it.
      if (!SOMEONE_DIED(retval) && !selfpurge)
      {
        ch->send(format("{}{}", BOLD, NTEXT));
      }
      // This call is here to prevent gcc from tail-chaining the
      // previous call, which screws up the debugger call stack.
      // -- Furey
      // number(0, 0);

      return retval;
    }
  } // end if((found = find_cmd_in_radix(pcomm)))

  // If we're at this point, Paralyze stops everything so get out.
  if (IS_AFFECTED(ch, AFF_PARALYSIS))
  {
    send_to_char("You've been paralyzed and are unable to move.\r\n", ch);
    return eSUCCESS;
  }
  // Check social table
  if ((retval = check_social(ch, pcomm, look_at)))
  {
    if (SOCIAL_TRUE_WITH_NOISE == retval)
      return check_ethereal_focus(ch, ETHEREAL_FOCUS_TRIGGER_SOCIAL);
    else
      return eSUCCESS;
  }

  // Unknown command (or char too low level)
  send_to_char("Huh?\r\n", ch);
  return eSUCCESS;
}

int old_search_block(const char *arg, const char **list, bool exact)
{
  if (arg == nullptr)
  {
    return -1;
  }

  int i = 0;
  int l = strlen(arg);

  if (exact)
  {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcasecmp(arg, *(list + i)))
        return (i);
  }
  else
  {
    if (!l)
      // Avoid "" to match the first available string
      l = 1;
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncasecmp(arg, *(list + i), l))
        return (i);
  }

  return (-1);
}

int search_block(const char *orig_arg, const char **list, bool exact)
{
  int i;

  string needle = string(orig_arg);

  // Make into lower case and get length of string
  transform(needle.begin(), needle.end(), needle.begin(), ::tolower);

  if (exact)
  {
    for (i = 0; **(list + i) != '\n'; i++)
    {
      string haystack = *(list + i);
      if (needle == haystack)
      {
        return (i);
      }
    }
  }
  else
  {
    for (i = 0; **(list + i) != '\n'; i++)
    {
      string haystack = *(list + i);
      if (haystack.size() == 0 && needle.size() == 0)
      {
        return i;
      }

      if (needle.size() > 0 && haystack.compare(0, needle.size(), needle) == 0)
      {
        return i;
      }
    }
  }

  return (-1);
}

int search_blocknolow(char *arg, const char **list, bool exact)
{
  int i;
  unsigned int l = strlen(arg);

  if (exact)
  {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
        return (i);
  }
  else
  {
    if (!l)
      // Avoid "" to match the first available string
      l = 1;
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
        return (i);
  }

  return (-1);
}

int do_boss(char_data *ch, char *arg, int cmd)
{
  char buf[200];
  int x;

  for (x = 0; x <= 60; x++)
  {
    sprintf(buf, "NUMBER-CRUNCHER: %d crunched to %d converted to black"
                 "/white tree node %d\n\r",
            x, 50 - x, x + x);
    send_to_char(buf, ch);
  }

  return eSUCCESS;
}

int old_search_block(const char *argument, int begin, int length, const char **list, int mode)
{
  int guess, found, search;

  // If the word contains 0 letters, a match is already found
  found = (length < 1);
  guess = 0;

  // Search for a match
  if (mode)
    while (!found && *(list[guess]) != '\n')
    {
      found = ((unsigned)length == strlen(list[guess]));
      for (search = 0; search < length && found; search++)
      {
        found = (*(argument + begin + search) == *(list[guess] + search));
      }
      guess++;
    }
  else
  {
    while (!found && *(list[guess]) != '\n')
    {
      found = 1;
      for (search = 0; (search < length && found); search++)
      {
        found = (*(argument + begin + search) == *(list[guess] + search));
      }
      guess++;
    }
  }

  return (found ? guess : -1);
}

void argument_interpreter(const char *argument, char *first_arg, char *second_arg)
{
  int look_at, begin;

  begin = 0;

  do
  {
    /* Find first non blank */
    for (; *(argument + begin) == ' '; begin++)
      ;
    /* Find length of first word */
    for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /* Make all letters lower case, and copy them to first_arg */
      *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
    *(first_arg + look_at) = '\0';
    begin += look_at;
  } while (fill_word(first_arg));

  do
  {
    /* Find first non blank */
    for (; *(argument + begin) == ' '; begin++)
      ;
    /* Find length of first word */
    for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /* Make all letters lower case, and copy them to second_arg */
      *(second_arg + look_at) = LOWER(*(argument + begin + look_at));
    *(second_arg + look_at) = '\0';
    begin += look_at;
  } while (fill_word(second_arg));
}

// If the string is ALL numbers, return TRUE
// If there is a non-numeric in string, return FALSE
int is_number(const char *str)
{
  int look_at;

  if (*str == '\0')
    return (0);

  for (look_at = 0; *(str + look_at) != '\0'; look_at++)
    if ((*(str + look_at) < '0') || (*(str + look_at) > '9'))
      return (0);

  return (1);
}

// Multiline arguments, used for mobprogs
char *one_argument_long(char *argument, char *first_arg)
{
  int begin, look_at;
  bool end = FALSE;
  begin = 0;

  /* Find first non blank */
  for (; isspace(*(argument + begin)); begin++)
    ;
  if (*(argument + begin) == '{')
  {
    end = TRUE;
    begin++;
  }

  if (*(argument + begin) == '{')
  {
    end = TRUE;
    begin++;
  }
  /* Find length of first word */
  for (look_at = 0;; look_at++)
    if (!end && *(argument + begin + look_at) <= ' ')
      break;
    else if (end && (*(argument + begin + look_at) == '}' || *(argument + begin + look_at) == '\0'))
    {
      begin++;
      break;
    }
    else
    {
      if (!end)
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
      else
        *(first_arg + look_at) = *(argument + begin + look_at);
    }

  /* Make all letters lower case, and copy them to first_arg */
  *(first_arg + look_at) = '\0';
  begin += look_at;

  return argument + begin;
}

const char *one_argument_long(const char *argument, char *first_arg)
{
  int begin, look_at;
  bool end = FALSE;
  begin = 0;

  /* Find first non blank */
  for (; isspace(*(argument + begin)); begin++)
    ;
  if (*(argument + begin) == '{')
  {
    end = TRUE;
    begin++;
  }

  if (*(argument + begin) == '{')
  {
    end = TRUE;
    begin++;
  }
  /* Find length of first word */
  for (look_at = 0;; look_at++)
    if (!end && *(argument + begin + look_at) <= ' ')
      break;
    else if (end && (*(argument + begin + look_at) == '}' || *(argument + begin + look_at) == '\0'))
    {
      begin++;
      break;
    }
    else
    {
      if (!end)
        *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
      else
        *(first_arg + look_at) = *(argument + begin + look_at);
    }

  /* Make all letters lower case, and copy them to first_arg */
  *(first_arg + look_at) = '\0';
  begin += look_at;

  return argument + begin;
}

/* find the first sub-argument of a string, return pointer to first char in
   primary argument, following the sub-arg                      */
char *one_argument(char *argument, char *first_arg)
{
  return one_argument_long(argument, first_arg);
  int begin, look_at;

  begin = 0;

  do
  {
    /* Find first non blank */
    for (; isspace(*(argument + begin)); begin++)
      ;
    /* Find length of first word */
    for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /* Make all letters lower case, and copy them to first_arg */
      *(first_arg + look_at) = LOWER(*(argument + begin + look_at));
    *(first_arg + look_at) = '\0';
    begin += look_at;
  } while (fill_word(first_arg));

  return (argument + begin);
}

const char *one_argument(const char *argument, char *first_arg)
{
  return one_argument_long(argument, first_arg);
}

int fill_wordnolow(char *argument)
{
  return (search_blocknolow(argument, fillwords, TRUE) >= 0);
}

char *one_argumentnolow(char *argument, char *first_arg)
{
  int begin, look_at;
  begin = 0;

  do
  {
    /* Find first non blank */
    for (; isspace(*(argument + begin)); begin++)
      ;
    /* Find length of first word */
    for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /* copy to first_arg */
      *(first_arg + look_at) = *(argument + begin + look_at);
    *(first_arg + look_at) = '\0';
    begin += look_at;
  } while (fill_wordnolow(first_arg));

  return (argument + begin);
}

int fill_word(char *argument)
{
  return (search_block(argument, fillwords, TRUE) >= 0);
}

void automail(char *name)
{
  FILE *blah;
  char buf[100];

  blah = dc_fopen("../lib/whassup.txt", "w");
  fprintf(blah, name);
  dc_fclose(blah);
  sprintf(buf, "mail void@dcastle.org < ../lib/whassup.txt");
  system(buf);
}

bool is_abbrev(const string &aabrev, const string &word)
{
  if (aabrev.empty())
  {
    return false;
  }

  return equal(aabrev.begin(), aabrev.end(), word.begin(),
               [](char a, char w)
               {
                 return tolower(a) == tolower(w);
               });
}

/* determine if a given string is an abbreviation of another */
int is_abbrev(char *arg1, char *arg2) /* arg1 is short, arg2 is long */
{
  if (!*arg1)
    return (0);

  for (; *arg1; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return (0);

  return (1);
}

string ltrim(string str)
{
  // remove leading spaces
  try
  {
    auto first_non_space = str.find_first_not_of(' ');
    if (first_non_space != str.npos)
    {
      str.erase(0, first_non_space);
    }
    else
    {
      str.clear();
    }
  }
  catch (...)
  {
  }

  return str;
}

string rtrim(string str)
{
  // remove leading spaces
  try
  {
    auto last_non_space = str.find_last_not_of(' ');
    if (last_non_space != str.npos)
    {
      str.erase(last_non_space + 1, str.length() + 1);
    }
    else
    {
      str.clear();
    }
  }
  catch (...)
  {
  }

  return str;
}

tuple<string, string> half_chop(string arguments, const char token)
{
  arguments = ltrim(arguments);

  auto space_after_arg1 = arguments.find_first_of(token);
  auto arg1 = arguments.substr(0, space_after_arg1);

  // remove arg1 from arguments
  arguments.erase(0, space_after_arg1);

  arguments = ltrim(arguments);

  return tuple<string, string>(arg1, arguments);
}

tuple<string, string> last_argument(string arguments)
{
  try
  {
    // remove leading spaces
    auto first_non_space = arguments.find_first_not_of(' ');
    arguments.erase(0, first_non_space);

    // remove trailing spaces
    auto last_non_space = arguments.find_last_not_of(' ');
    arguments.erase(last_non_space + 1, arguments.length() + 1);

    auto space_after_last_arg = arguments.find_last_of(' ');
    auto last_arg = arguments.substr(space_after_last_arg + 1, arguments.length() + 1);

    arguments.erase(space_after_last_arg, arguments.length() + 1);

    // remove trailing spaces
    last_non_space = arguments.find_last_not_of(' ');
    arguments.erase(last_non_space + 1, arguments.length() + 1);

    return tuple<string, string>(last_arg, arguments);
  }
  catch (...)
  {
    logf(IMMORTAL, LogChannels::LOG_BUG, "Error in last_argument(%s)", arguments.c_str());
  }

  return tuple<string, string>(string(), string());
}

/* return first 'word' plus trailing substring of input string */
void half_chop(const char *string, char *arg1, char *arg2)
{
  // strip leading whitespace from original
  for (; isspace(*string); string++)
    ;

  // copy everything up to next space
  for (; !isspace(*arg1 = *string) && *string; string++, arg1++)
    ;

  // terminate
  *arg1 = '\0';

  // strip leading whitepace
  for (; isspace(*string); string++)
    ;

  // copy rest of string to arg2
  for (; (*arg2 = *string) != '\0'; string++, arg2++)
    ;
}

/* return last 'word' plus leading substring of input string */
void chop_half(char *string, char *arg1, char *arg2)
{
  int32_t i, j;

  // skip over trailing space
  i = strlen(string) - 1;
  j = 0;
  while (isspace(string[i]))
    i--;

  // find beginning of last 'word'
  while (!isspace(string[i]))
  {
    i--;
    j++;
  }

  // copy last word to arg1
  strncpy(arg1, string + i + 1, j);
  arg1[j] = '\0';

  // skip over leading space in string
  while (isspace(*string))
  {
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

int special(char_data *ch, int cmd, char *arg)
{
  struct obj_data *i;
  char_data *k;
  int j;
  int retval;

  /* special in room? */
  if (world[ch->in_room].funct)
  {
    if ((retval = (*world[ch->in_room].funct)(ch, cmd, arg)) != eFAILURE)
      return retval;
  }

  /* special in equipment list? */
  for (j = 0; j <= (MAX_WEAR - 1); j++)
    if (ch->equipment[j] && ch->equipment[j]->item_number >= 0)
      if (obj_index[ch->equipment[j]->item_number].non_combat_func)
      {
        retval = ((*obj_index[ch->equipment[j]->item_number].non_combat_func)(ch, ch->equipment[j], cmd, arg, ch));
        if (IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
          return retval;
      }

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (i->item_number >= 0)
      if (obj_index[i->item_number].non_combat_func)
      {
        retval = ((*obj_index[i->item_number].non_combat_func)(ch, i, cmd, arg, ch));
        if (IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
          return retval;
      }

  /* special in mobile present? */
  for (k = world[ch->in_room].people; k; k = k->next_in_room)
  {
    if (IS_MOB(k))
    {
      if (((char_data *)mob_index[k->mobdata->nr].item)->mobdata->mob_flags.type == MOB_CLAN_GUARD)
      {
        retval = clan_guard(ch, 0, cmd, arg, k);
        if (IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
          return retval;
      }
      else if (mob_index[k->mobdata->nr].non_combat_func)
      {
        retval = ((*mob_index[k->mobdata->nr].non_combat_func)(ch, 0,
                                                               cmd, arg, k));
        if (IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
          return retval;
      }
    }
  }

  /* special in object present? */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
    if (i->item_number >= 0)
      if (obj_index[i->item_number].non_combat_func)
      {
        retval = ((*obj_index[i->item_number].non_combat_func)(ch, i, cmd, arg, ch));
        if (IS_SET(retval, eCH_DIED) || IS_SET(retval, eSUCCESS))
          return retval;
      }

  return eFAILURE;
}

void add_command_lag(char_data *ch, int cmdnum, int lag)
{
  if (!ch)
    return;

  struct command_lag *cmdl;
#ifdef LEAK_CHECK
  cmdl = (struct command_lag *)
      calloc(1, sizeof(struct command_lag));
#else
  cmdl = (struct command_lag *)
      dc_alloc(1, sizeof(struct command_lag));
#endif
  cmdl->next = command_lag_list;
  command_lag_list = cmdl;
  cmdl->ch = ch;
  cmdl->cmd_number = cmdnum;
  cmdl->lag = lag;
}

bool can_use_command(char_data *ch, int cmdnum)
{
  struct command_lag *cmdl;
  for (cmdl = command_lag_list; cmdl; cmdl = cmdl->next)
  {

    if (cmdl->ch == ch && cmdl->cmd_number == cmdnum)
      return FALSE;
  }
  return TRUE;
}

void pulse_command_lag()
{
  struct command_lag *cmdl, *cmdlp = NULL, *cmdlnext = NULL;

  for (cmdl = command_lag_list; cmdl; cmdl = cmdlnext)
  {
    cmdlnext = cmdl->next;
    if ((cmdl->lag--) <= 0)
    {
      if (cmdlp)
        cmdlp->next = cmdl->next;
      else
        command_lag_list = cmdl->next;

      cmdl->ch = 0;
      dc_free(cmdl);
    }
    else
      cmdlp = cmdl;
  }
}

char *remove_trailing_spaces(char *arg)
{
  int len = strlen(arg) - 1;

  if (len < 1)
    return arg;

  for (; len > 0; len--)
  {
    if (arg[len] != ' ')
    {
      arg[len + 1] = '\0';
      return arg;
    }
  }
  return arg;
}

// The End
