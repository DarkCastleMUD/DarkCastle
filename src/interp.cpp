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
#include <QStringList>

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
        {"north", do_move, nullptr, nullptr, POSITION_STANDING, 0, CMD_NORTH, COM_CHARMIE_OK, 1, CommandType::all},
        {"east", do_move, nullptr, nullptr, POSITION_STANDING, 0, CMD_EAST, COM_CHARMIE_OK, 1, CommandType::all},
        {"south", do_move, nullptr, nullptr, POSITION_STANDING, 0, CMD_SOUTH, COM_CHARMIE_OK, 1, CommandType::all},
        {"west", do_move, nullptr, nullptr, POSITION_STANDING, 0, CMD_WEST, COM_CHARMIE_OK, 1, CommandType::all},
        {"up", do_move, nullptr, nullptr, POSITION_STANDING, 0, CMD_UP, COM_CHARMIE_OK, 1, CommandType::all},
        {"down", do_move, nullptr, nullptr, POSITION_STANDING, 0, CMD_DOWN, COM_CHARMIE_OK, 1, CommandType::all},

        // Common commands
        {"newbie", do_newbie, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"cast", do_cast, nullptr, nullptr, POSITION_SITTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"filter", do_cast, nullptr, nullptr, POSITION_SITTING, 0, CMD_FILTER, 0, 0, CommandType::all},
        {"sing", do_sing, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"exits", do_exits, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"f", do_fire, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 25, CommandType::all},
        {"get", do_get, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"inventory", do_inventory, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"k", do_kill, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"ki", do_ki, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"kill", do_kill, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"look", do_look, nullptr, nullptr, POSITION_RESTING, 0, CMD_LOOK, COM_CHARMIE_OK, 1, CommandType::all},
        {"loot", do_get, nullptr, nullptr, POSITION_RESTING, 0, CMD_LOOT, 0, 0, CommandType::all},
        {"glance", do_look, nullptr, nullptr, POSITION_RESTING, 0, CMD_GLANCE, COM_CHARMIE_OK, 1, CommandType::all},
        {"order", do_order, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"rest", do_rest, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"recite", do_recite, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"recall", do_recall, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"score", do_score, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"scan", do_scan, nullptr, nullptr, POSITION_RESTING, 1, CMD_DEFAULT, 0, 25, CommandType::all},
        {"stand", do_stand, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"switch", do_switch, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 25, CommandType::all},
        {"tell", nullptr, do_tell, nullptr, POSITION_RESTING, 0, CMD_TELL, 0, 1, CommandType::all},
        {"tellhistory", nullptr, do_tellhistory, nullptr, POSITION_RESTING, 0, CMD_TELLH, 0, 1, CommandType::all},
        {"wield", do_wield, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"innate", do_innate, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"orchestrate", do_sing, nullptr, nullptr, POSITION_RESTING, 0, CMD_ORCHESTRATE, 0, 0, CommandType::all},

        // Informational commands
        {"alias", do_alias, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"toggle", do_toggle, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"consider", do_consider, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"configure", nullptr, nullptr, &char_data::do_config, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::players_only},
        {"credits", do_credits, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"equipment", do_equipment, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"ohelp", do_help, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"help", do_new_help, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"idea", do_idea, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"info", do_info, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"leaderboard", do_leaderboard, nullptr, nullptr, POSITION_DEAD, 3, CMD_DEFAULT, 0, 1, CommandType::all},
        {"news", do_news, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"thenews", do_news, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"story", do_story, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"tick", do_tick, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"time", do_time, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"title", do_title, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"typo", do_typo, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"weather", do_weather, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"who", do_who, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"wizlist", do_wizlist, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"socials", do_social, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"index", do_index, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"areas", do_areas, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"commands", do_new_help, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"experience", nullptr, nullptr, &char_data::do_experience, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::players_only},
        {"version", do_version, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"identify", do_identify, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},

        // Communication commands
        {"ask", do_ask, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"auction", do_auction, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"awaymsgs", do_awaymsgs, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"channel", do_channel, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"dream", do_dream, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"emote", do_emote, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {":", do_emote, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"gossip", do_gossip, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"trivia", do_trivia, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"gtell", do_grouptell, nullptr, nullptr, POSITION_DEAD, 0, CMD_GTELL, 0, 1, CommandType::all},
        {".", do_grouptell, nullptr, nullptr, POSITION_DEAD, 0, CMD_GTELL, 0, 1, CommandType::all},
        {"ignore", nullptr, do_ignore, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"insult", do_insult, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"reply", nullptr, do_reply, nullptr, POSITION_RESTING, 0, CMD_REPLY, 0, 1, CommandType::all},
        {"report", do_report, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"say", nullptr, do_say, nullptr, POSITION_RESTING, 0, CMD_SAY, COM_CHARMIE_OK, 0, CommandType::all},
        {"psay", nullptr, do_psay, nullptr, POSITION_RESTING, 0, CMD_SAY, COM_CHARMIE_OK, 0, CommandType::all},
        {"'", nullptr, do_say, nullptr, POSITION_RESTING, 0, CMD_SAY, COM_CHARMIE_OK, 0, CommandType::all},
        {"shout", do_shout, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"whisper", do_whisper, nullptr, nullptr, POSITION_RESTING, 0, CMD_WHISPER, 0, 0, CommandType::all},

        // Object manipulation
        {"slip", do_slip, nullptr, nullptr, POSITION_STANDING, 0, CMD_SLIP, 0, 1, CommandType::all},
        {"batter", do_batter, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"brace", do_brace, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"close", do_close, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"donate", do_donate, nullptr, nullptr, POSITION_RESTING, 0, CMD_DONATE, COM_CHARMIE_OK, 1, CommandType::all},
        {"drink", do_drink, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"drop", do_drop, nullptr, nullptr, POSITION_RESTING, 0, CMD_DROP, COM_CHARMIE_OK, 25, CommandType::all},
        {"eat", do_eat, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"fill", do_fill, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"give", do_give, nullptr, nullptr, POSITION_RESTING, 0, CMD_GIVE, COM_CHARMIE_OK, 25, CommandType::all},
        {"grab", do_grab, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"hold", do_grab, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"lock", do_lock, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"open", do_open, nullptr, nullptr, POSITION_RESTING, 0, CMD_OPEN, COM_CHARMIE_OK, 25, CommandType::all},
        {"pour", do_pour, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"put", do_put, nullptr, nullptr, POSITION_RESTING, 0, CMD_PUT, COM_CHARMIE_OK, 0, CommandType::all},
        {"quaff", do_quaff, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"read", do_read, nullptr, nullptr, POSITION_RESTING, 0, CMD_READ, 0, 1, CommandType::all},
        {"remove", do_remove, nullptr, nullptr, POSITION_RESTING, 0, CMD_REMOVE, COM_CHARMIE_OK, 25, CommandType::all},
        {"erase", do_not_here, nullptr, nullptr, POSITION_RESTING, 0, CMD_ERASE, 0, 0, CommandType::all},
        {"sip", do_sip, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"track", do_track, nullptr, nullptr, POSITION_STANDING, 0, CMD_TRACK, 0, 10, CommandType::all},
        {"take", do_get, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"palm", do_get, nullptr, nullptr, POSITION_RESTING, 3, CMD_PALM, 0, 1, CommandType::all},
        {"sacrifice", do_sacrifice, nullptr, nullptr, POSITION_RESTING, 0, CMD_SACRIFICE, COM_CHARMIE_OK, 25, CommandType::all},
        {"taste", do_taste, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"unlock", do_unlock, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"use", do_use, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"wear", do_wear, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 25, CommandType::all},
        {"scribe", do_scribe, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"brew", do_brew, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        //  { "poisonmaking", do_poisonmaking, nullptr,nullptr, POSITION_RESTING, 0, 9,   0, 0  ,CommandType::all},

        // Combat commands
        {"bash", do_bash, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"retreat", do_retreat, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"disarm", do_disarm, nullptr, nullptr, POSITION_FIGHTING, 2, CMD_DEFAULT, 0, 0, CommandType::all},
        {"flee", do_flee, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_FLEE, COM_CHARMIE_OK, 0, CommandType::all},
        {"escape", do_flee, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_ESCAPE, 0, 0, CommandType::all},
        {"hit", do_hit, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_HIT, COM_CHARMIE_OK, 0, CommandType::all},
        {"join", do_join, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"battlesense", do_battlesense, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"stance", do_defenders_stance, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"perseverance", do_perseverance, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"smite", do_smite, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},

        // Junk movedso join precedes it
        {"junk", do_sacrifice, nullptr, nullptr, POSITION_RESTING, 0, CMD_SACRIFICE, COM_CHARMIE_OK, 25, CommandType::all},

        {"murder", do_murder, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"rescue", do_rescue, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"trip", do_trip, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"deathstroke", do_deathstroke, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"circle", do_circle, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"kick", do_kick, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"battlecry", do_battlecry, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"behead", do_behead, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"rage", do_rage, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"berserk", do_berserk, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"golemscore", do_golem_score, nullptr, nullptr, POSITION_DEAD, 1, CMD_DEFAULT, 0, 1, CommandType::all},
        {"stun", do_stun, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"redirect", do_redirect, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"hitall", do_hitall, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"quiveringpalm", do_quivering_palm, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"eagleclaw", do_eagle_claw, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"headbutt", do_headbutt, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"cripple", do_cripple, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"fire", do_fire, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 25, CommandType::all},
        {"layhands", do_layhands, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"harmtouch", do_harmtouch, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"bloodfury", do_bloodfury, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"primalfury", do_primalfury, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"bladeshield", do_bladeshield, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"repelance", do_focused_repelance, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"vitalstrike", do_vitalstrike, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"crazedassault", do_crazedassault, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"bullrush", do_bullrush, nullptr, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"ferocity", do_ferocity, nullptr, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"tactics", do_tactics, nullptr, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"deceit", do_deceit, nullptr, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"knockback", do_knockback, nullptr, nullptr, POSITION_FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"appraise", do_appraise, nullptr, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 1, CommandType::all},
        {"make camp", do_make_camp, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"leadership", do_leadership, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"triage", do_triage, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"onslaught", do_onslaught, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"pursue", do_pursue, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},

        // Position commands
        {"sit", do_sit, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0, CommandType::all},
        {"sleep", do_sleep, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"wake", do_wake, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 0, CommandType::all},

        // Miscellaneous commands
        {"editor", do_editor, nullptr, nullptr, POSITION_SLEEPING, CMD_EDITOR, CMD_DEFAULT, 0, 1, CommandType::all},
        {"autojoin", nullptr, do_autojoin, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"visible", do_visible, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"ctell", do_ctell, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_CTELL, 0, 1, CommandType::all},
        {"outcast", do_outcast, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"accept", do_accept, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"whoclan", do_whoclan, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"cpromote", do_cpromote, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"clans", do_clans, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"clanarea", nullptr, nullptr, &char_data::do_clanarea, POSITION_RESTING, 11, CMD_DEFAULT, 0, 1, CommandType::all},
        {"cinfo", do_cinfo, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"ambush", do_ambush, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"whoarena", do_whoarena, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"joinarena", do_joinarena, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"backstab", do_backstab, nullptr, nullptr, POSITION_STANDING, 0, CMD_BACKSTAB, 0, 0, CommandType::all},
        {"bs", do_backstab, nullptr, nullptr, POSITION_STANDING, 0, CMD_BACKSTAB, 0, 0, CommandType::all},
        {"sbs", do_backstab, nullptr, nullptr, POSITION_STANDING, 0, CMD_SBS, 0, 0, CommandType::all}, // single backstab
        {"boss", do_boss, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"jab", do_jab, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"enter", do_enter, nullptr, nullptr, POSITION_STANDING, 0, CMD_ENTER, COM_CHARMIE_OK, 20, CommandType::all},
        {"climb", do_climb, nullptr, nullptr, POSITION_STANDING, 0, CMD_CLIMB, COM_CHARMIE_OK, 20, CommandType::all},
        {"examine", do_examine, nullptr, nullptr, POSITION_RESTING, 0, CMD_EXAMINE, 0, 1, CommandType::all},
        {"follow", do_follow, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"stalk", do_stalk, nullptr, nullptr, POSITION_STANDING, 6, CMD_DEFAULT, 0, 10, CommandType::all},
        {"group", do_group, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"found", do_found, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"disband", do_disband, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"abandon", do_abandon, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"consent", do_consent, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"whogroup", do_whogroup, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"forage", do_forage, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"whosolo", do_whosolo, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"count", do_count, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"hide", do_hide, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"leave", do_leave, nullptr, nullptr, POSITION_STANDING, 0, CMD_LEAVE, COM_CHARMIE_OK, 20, CommandType::all},
        {"name", do_name, nullptr, nullptr, POSITION_DEAD, 1, CMD_DEFAULT, 0, 1, CommandType::all},
        {"pick", do_pick, nullptr, nullptr, POSITION_STANDING, 0, CMD_PICK, 0, 20, CommandType::all},
        {"quest", do_quest, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"qui", do_qui, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"levels", do_levels, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"quit", do_quit, nullptr, nullptr, POSITION_DEAD, 0, CMD_QUIT, 0, 1, CommandType::all},
        {"return", do_return, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, COM_CHARMIE_OK, 1, CommandType::all},
        {"tame", do_tame, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"free animal", do_free_animal, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"prompt", do_prompt, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"lastprompt", do_lastprompt, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"save", nullptr, nullptr, &char_data::do_save, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"sneak", do_sneak, nullptr, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"home", do_home, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"split", nullptr, nullptr, &char_data::do_split, POSITION_RESTING, 0, CMD_SPLIT, 0, 0, CommandType::all},
        {"spells", do_spells, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"skills", do_skills, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"songs", do_songs, nullptr, nullptr, POSITION_SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"steal", do_steal, nullptr, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 10, CommandType::all},
        {"pocket", do_pocket, nullptr, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 10, CommandType::all},
        {"motd", do_motd, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"cmotd", do_cmotd, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"cbalance", do_cbalance, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"cdeposit", do_cdeposit, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"cwithdraw", do_cwithdraw, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"ctax", do_ctax, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"where", do_where, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"write", do_write, nullptr, nullptr, POSITION_STANDING, 0, CMD_WRITE, 0, 0, CommandType::all},
        {"beacon", do_beacon, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"leak", do_memoryleak, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"beep", do_beep, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"guard", do_guard, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"release", do_release, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"eyegouge", do_eyegouge, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"vault", do_vault, nullptr, nullptr, POSITION_DEAD, 10, CMD_DEFAULT, 0, 0, CommandType::all},
        {"suicide", do_suicide, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"vote", do_vote, nullptr, nullptr, POSITION_RESTING, 0, CMD_VOTE, 0, 0, CommandType::all},
        {"huntitems", do_showhunt, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"random", do_random, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        // Special procedure commands

        {"vend", do_vend, nullptr, nullptr, POSITION_STANDING, 2, CMD_VEND, 0, 0, CommandType::all},
        {"gag", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_GAG, 0, 0, CommandType::all},
        {"design", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_DESIGN, 0, 0, CommandType::all},
        {"stock", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_STOCK, 0, 0, CommandType::all},
        {"buy", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_BUY, 0, 0, CommandType::all},
        {"sell", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_SELL, 0, 0, CommandType::all},
        {"value", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_VALUE, 0, 0, CommandType::all},
        {"watch", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_WATCH, 0, 0, CommandType::all},
        {"list", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_LIST, 0, 0, CommandType::all},
        {"estimate", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_ESTIMATE, 0, 0, CommandType::all},
        {"repair", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_REPAIR, 0, 0, CommandType::all},
        {"practice", do_practice, nullptr, nullptr, POSITION_SLEEPING, 1, CMD_PRACTICE, 0, 0, CommandType::all},
        {"practise", do_practice, nullptr, nullptr, POSITION_SLEEPING, 1, CMD_PRACTICE, 0, 0, CommandType::all},
        {"pray", do_pray, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"profession", do_profession, nullptr, nullptr, POSITION_SLEEPING, 1, CMD_PROFESSION, 0, 0, CommandType::all},
        {"promote", do_promote, nullptr, nullptr, POSITION_STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"price", do_not_here, nullptr, nullptr, POSITION_RESTING, 1, CMD_PRICE, 0, 0, CommandType::all},
        {"train", do_not_here, nullptr, nullptr, POSITION_RESTING, 1, CMD_TRAIN, 0, 0, CommandType::all},
        {"gain", do_not_here, nullptr, nullptr, POSITION_STANDING, 1, CMD_GAIN, 0, 0, CommandType::all},
        {"balance", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_BALANCE, 0, 0, CommandType::all},
        {"deposit", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEPOSIT, 0, 0, CommandType::all},
        {"withdraw", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_WITHDRAW, 0, 0, CommandType::all},
        {"clean", do_not_here, nullptr, nullptr, POSITION_RESTING, 0, CMD_CLEAN, 0, 0, CommandType::all},
        {"play", do_not_here, nullptr, nullptr, POSITION_RESTING, 0, CMD_PLAY, 0, 0, CommandType::all},
        {"finish", do_not_here, nullptr, nullptr, POSITION_RESTING, 0, CMD_FINISH, 0, 0, CommandType::all},
        {"veternarian", do_not_here, nullptr, nullptr, POSITION_RESTING, 0, CMD_VETERNARIAN, 0, 0, CommandType::all},
        {"feed", do_not_here, nullptr, nullptr, POSITION_RESTING, 0, CMD_FEED, 0, 0, CommandType::all},
        {"assemble", do_assemble, nullptr, nullptr, POSITION_RESTING, 0, CMD_ASSEMBLE, 0, 0, CommandType::all},
        {"pay", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_PAY, 0, 0, CommandType::all},
        {"restring", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_RESTRING, 0, 0, CommandType::all},
        {"push", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_PUSH, 0, 0, CommandType::all},
        {"pull", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_PULL, 0, 0, CommandType::all},
        {"gaze", do_not_here, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_GAZE, 0, 0, CommandType::all},
        {"tremor", do_not_here, nullptr, nullptr, POSITION_FIGHTING, 0, CMD_TREMOR, 0, 0, CommandType::all},
        {"bet", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_BET, 0, 0, CommandType::all},
        {"insurance", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_INSURANCE, 0, 0, CommandType::all},
        {"double", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_DOUBLE, 0, 0, CommandType::all},
        {"stay", do_not_here, nullptr, nullptr, POSITION_STANDING, 0, CMD_STAY, 0, 0, CommandType::all},
        {"select", do_natural_selection, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"sector", do_sector, nullptr, nullptr, POSITION_RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"remort", do_not_here, nullptr, nullptr, POSITION_RESTING, GIFTED_COMMAND, CMD_REMORT, 0, 1, CommandType::all},
        {"reroll", do_not_here, nullptr, nullptr, POSITION_RESTING, 0, CMD_REROLL, 0, 0, CommandType::all},
        {"choose", do_not_here, nullptr, nullptr, POSITION_RESTING, 0, CMD_CHOOSE, 0, 0, CommandType::all},
        {"confirm", do_not_here, nullptr, nullptr, POSITION_RESTING, 0, CMD_CONFIRM, 0, 0, CommandType::all},
        {"cancel", do_not_here, nullptr, nullptr, POSITION_RESTING, 0, CMD_CANCEL, 0, 0, CommandType::all},

        // Immortal commands
        {"voteset", do_setvote, nullptr, nullptr, POSITION_DEAD, 108, CMD_SETVOTE, 0, 1, CommandType::all},
        {"thunder", do_thunder, nullptr, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"wizlock", do_wizlock, nullptr, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"processes", do_processes, nullptr, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"bestow", nullptr, do_bestow, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"oclone", do_oclone, nullptr, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mclone", do_mclone, nullptr, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"huntclear", do_huntclear, nullptr, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"areastats", do_areastats, nullptr, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"huntstart", do_huntstart, nullptr, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"revoke", do_revoke, nullptr, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"chpwd", do_chpwd, nullptr, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"advance", do_advance, nullptr, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"skillmax", do_maxes, nullptr, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"damage", do_dmg_eq, nullptr, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"affclear", do_clearaff, nullptr, nullptr, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"guide", do_guide, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"addnews", do_addnews, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"linkload", do_linkload, nullptr, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"listproc", do_listproc, nullptr, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"zap", do_zap, nullptr, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"slay", do_slay, nullptr, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"rename", do_rename_char, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"archive", do_archive, nullptr, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"unarchive", do_unarchive, nullptr, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"stealth", do_stealth, nullptr, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"disconnect", do_disconnect, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"force", nullptr, do_force, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"pardon", do_pardon, nullptr, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"goto", nullptr, nullptr, &char_data::do_goto, POSITION_DEAD, 102, CMD_DEFAULT, 0, 1, CommandType::immortals_only},
        {"restore", do_restore, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"purloin", do_purloin, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"set", do_set, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"unban", do_unban, nullptr, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"ban", do_ban, nullptr, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"echo", do_echo, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"eqmax", do_eqmax, nullptr, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"send", do_send, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"at", do_at, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"fakelog", do_fakelog, nullptr, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"global", do_global, nullptr, nullptr, POSITION_DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"log", do_log, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"snoop", do_snoop, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"pview", do_pview, nullptr, nullptr, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"arena", do_arena, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"load", do_load, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"prize", do_load, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_PRIZE, 0, 1, CommandType::all},
        {"testport", do_testport, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"testuser", do_testuser, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"shutdow", do_shutdow, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"shutdown", do_shutdown, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"opedit", do_opedit, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"opstat", do_opstat, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"procedit", do_procedit, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"procstat", do_mpstat, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"range", do_range, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        // { "pshopedit",	do_pshopedit,	POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1  ,CommandType::all},
        {"sedit", do_sedit, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"sockets", do_sockets, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"punish", do_punish, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"sqedit", do_sqedit, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"qedit", do_qedit, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"install", do_install, nullptr, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        //  { "motdload",       do_motdload,    POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1  ,CommandType::all},
        {"hedit", do_hedit, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"hindex", do_hindex, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"reload", do_reload, nullptr, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"plats", do_plats, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"bellow", do_thunder, nullptr, nullptr, POSITION_DEAD, DEITY, CMD_BELLOW, 0, 1, CommandType::all},
        {"string", do_string, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"transfer", nullptr, do_transfer, nullptr, POSITION_DEAD, DEITY, CMD_DEFAULT, 0, 1, CommandType::all},
        {"gtrans", do_gtrans, nullptr, nullptr, POSITION_DEAD, DEITY, CMD_DEFAULT, 0, 1, CommandType::all},
        {"boot", do_boot, nullptr, nullptr, POSITION_DEAD, DEITY, CMD_DEFAULT, 0, 1, CommandType::all},
        {"linkdead", do_linkdead, nullptr, nullptr, POSITION_DEAD, DEITY, CMD_DEFAULT, 0, 1, CommandType::all},
        {"teleport", do_teleport, nullptr, nullptr, POSITION_DEAD, DEITY, CMD_DEFAULT, 0, 1, CommandType::all},
        {"purge", do_purge, nullptr, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"show", do_show, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"search", nullptr, nullptr, &char_data::do_search, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"fighting", do_fighting, nullptr, nullptr, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"peace", do_peace, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"check", do_check, nullptr, nullptr, POSITION_DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"zoneexits", do_zoneexits, nullptr, nullptr, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"find", do_find, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"stat", do_stat, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"redit", do_redit, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"guild", do_guild, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"oedit", do_oedit, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"clear", do_clear, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"repop", nullptr, do_repop, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"medit", do_medit, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"rdelete", do_rdelete, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"oneway", do_oneway, nullptr, nullptr, POSITION_DEAD, ANGEL, 1, 0, 1, CommandType::all},
        {"twoway", do_oneway, nullptr, nullptr, POSITION_DEAD, ANGEL, 2, 0, 1, CommandType::all},
        {"zsave", nullptr, nullptr, &char_data::do_zsave, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"rsave", do_rsave, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"msave", do_msave, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"osave", do_osave, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"rstat", do_rstat, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"possess", do_possess, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"fsave", nullptr, do_fsave, nullptr, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"zedit", do_zedit, nullptr, nullptr, POSITION_DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"colors", do_colors, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"colours", do_colors, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"incognito", do_incognito, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"high5", do_highfive, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"holylite", do_holylite, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"immort", nullptr, do_wiz, nullptr, POSITION_DEAD, IMMORTAL, CMD_IMMORT, 0, 1, CommandType::all},
        {";", nullptr, do_wiz, nullptr, POSITION_DEAD, IMMORTAL, CMD_IMMORT, 0, 1, CommandType::all},
        {"impchan", nullptr, do_wiz, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_IMPCHAN, 0, 1, CommandType::all},
        {"/", nullptr, do_wiz, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_IMPCHAN, 0, 1, CommandType::all},
        {"nohassle", do_nohassle, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"wizinvis", do_wizinvis, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"poof", do_poof, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"wizhelp", do_wizhelp, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"imotd", do_imotd, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mhelp", do_mortal_help, nullptr, nullptr, POSITION_DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"testhand", do_testhand, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"varstat", do_varstat, nullptr, nullptr, POSITION_DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"matrixinfo", do_matrixinfo, nullptr, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"maxcheck", do_findfix, nullptr, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"export", do_export, nullptr, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mscore", do_mscore, nullptr, nullptr, POSITION_DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"world", nullptr, do_world, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},

        // Special procedure commands placed to not disrupt god commands
        {"setup", do_mortal_set, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"metastat", do_metastat, nullptr, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"testhit", do_testhit, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"acfinder", do_acfinder, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"findpath", do_findPath, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"findpath2", do_findpath, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"addroom", do_addRoom, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"newpath", do_newPath, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"listpathsbyzone", do_listPathsByZone, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"listallpaths", do_listAllPaths, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"dopathpath", do_pathpath, nullptr, nullptr, POSITION_DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"botcheck", do_botcheck, nullptr, nullptr, POSITION_DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"showbits", do_showbits, nullptr, nullptr, POSITION_DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"debug", do_debug, nullptr, nullptr, POSITION_DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},

        // Bug way down here after 'buy'
        {"bug", do_bug, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        // imbue after 'im' for us lazy immortal types :)
        {"imbue", do_imbue, nullptr, nullptr, POSITION_STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},

        // MOBprogram commands
        {"mpasound", do_mpasound, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpbestow", do_mpbestow, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpjunk", do_mpjunk, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpecho", do_mpecho, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpechoat", do_mpechoat, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpechoaround", do_mpechoaround, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpechoaroundnotbad", do_mpechoaroundnotbad, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpkill", do_mpkill, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mphit", do_mphit, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpaddlag", do_mpaddlag, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpmload", do_mpmload, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpoload", do_mpoload, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mppurge", do_mppurge, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpgoto", do_mpgoto, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpat", do_mpat, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mptransfer", do_mptransfer, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpthrow", do_mpthrow, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpforce", do_mpforce, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mppeace", do_mppeace, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpsetalign", do_mpsetalign, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpsettemp", do_mpsettemp, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpxpreward", do_mpxpreward, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpteachskill", do_mpteachskill, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpdamage", do_mpdamage, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpothrow", do_mpothrow, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mppause", do_mppause, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpretval", do_mpretval, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpsetmath", do_mpsetmath, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpteleport", do_mpteleport, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},

        // End of the line
        {"", do_not_here, nullptr, nullptr, POSITION_DEAD, 0, CMD_DEFAULT, COM_CHARMIE_OK, 0}};

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
        logentry(QString("Command stack exceeded. depth: %1, max_depth: %2, name: %3, cmd: %4").arg(cstack.getDepth()).arg(cstack.getMax()).arg(GET_NAME(ch)).arg(pcomm.c_str()), IMMORTAL, LogChannels::LOG_BUG);
      }
      else
      {
        logentry(QString("CommandStack::depth %1 exceeds CommandStack::max_depth %2").arg(cstack.getDepth()).arg(cstack.getMax()), IMMORTAL, LogChannels::LOG_BUG);
      }
    }
    return eFAILURE;
  }

  int look_at;
  int retval;
  struct command_info *found = 0;
  QString buf;

  // Handle logged players.
  if (IS_PC(ch) && IS_SET(ch->pcdata->punish, PUNISH_LOG))
  {
    buf = QString("Log %1: %2").arg(GET_NAME(ch)).arg(pcomm.c_str());
    logentry(buf, 110, LogChannels::LOG_PLAYER, ch);
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
    if (GET_LEVEL(ch) >= found->minimum_level && (found->command_pointer != nullptr || found->command_pointer2 != nullptr || found->command_pointer3 != nullptr))
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
          send_to_char("You are too stunned to do that.\r\n", ch);
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
        if (found->command_number != CMD_GTELL && found->command_number != CMD_CTELL && found->command_number != CMD_SAY && found->command_number != CMD_TELL && found->command_number != CMD_WHISPER && found->command_number != CMD_REPLY && (GET_LEVEL(ch) >= 100 || (ch->pcdata->multi == true && dc->cf.allow_multi == false)) && IS_SET(ch->pcdata->punish, PUNISH_LOG) == false)
        {
          logentry(QString("Log %1: %2").arg(GET_NAME(ch)).arg(pcomm.c_str()), 110, LogChannels::LOG_PLAYER, ch);
        }
      }

      // We're going to execute, check for usable special proc.
      retval = special(ch, found->command_number, &pcomm[look_at]);
      if (IS_SET(retval, eSUCCESS) || IS_SET(retval, eCH_DIED))
        return retval;

      switch (found->type)
      {
      case CommandType::all:
        if (ch == nullptr)
        {
          return eFAILURE;
        }
        break;

      case CommandType::players_only:
        if (ch == nullptr || ch->pcdata == nullptr)
        {
          return eFAILURE;
        }
        break;

      case CommandType::non_players_only:
        if (ch == nullptr || ch->mobdata == nullptr)
        {
          return eFAILURE;
        }
        break;

      case CommandType::immortals_only:
        if (ch == nullptr || ch->pcdata == nullptr || GET_LEVEL(ch) < IMMORTAL)
        {
          return eFAILURE;
        }

        break;

      case CommandType::implementors_only:
        if (ch == nullptr || ch->pcdata == nullptr || GET_LEVEL(ch) < IMP_ONLY)
        {
          return eFAILURE;
        }
        break;

      default:
        if (ch == nullptr)
        {
          return eFAILURE;
        }
        break;
      }

      // Normal dispatch
      if (found->command_pointer)
      {
        retval = (*(found->command_pointer))(ch, &pcomm[look_at], found->command_number);
      }
      else if (found->command_pointer2)
      {
        retval = (*(found->command_pointer2))(ch, &pcomm[look_at], found->command_number);
      }
      else if (found->command_pointer3)
      {
        QString command = QString(&pcomm[look_at]).trimmed();
        QStringList arguments;

        // This code splits command by spaces and doublequotes
        while (command.count('"') > 1)
        {
          qsizetype section_start, first_quote;
          first_quote = command.indexOf('"');

          // Find section start is the location of the character after a space next to first double quote
          section_start = command.sliced(0, first_quote + 1).lastIndexOf(' ') + 1;
          if (section_start == -1)
          {
            section_start = first_quote;
          }
          QString before_quotes = command.sliced(0, section_start);

          auto second_quote = command.indexOf('"', first_quote + 1);
          QString quoted = command.sliced(section_start, second_quote - section_start + 1);
          arguments.append(before_quotes.split(' ', Qt::SkipEmptyParts));
          arguments.append(quoted);
          command = command.remove(0, second_quote + 1);
        }

        arguments.append(command.split(' ', Qt::SkipEmptyParts));

        retval = (*ch.*(found->command_pointer3))(arguments, found->command_number);
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

  blah = fopen("../lib/whassup.txt", "w");
  fprintf(blah, name);
  fclose(blah);
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
    logf(IMMORTAL, LogChannels::LOG_BUG, "Error in last_argument(%s)",
         arguments.c_str());
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
