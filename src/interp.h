/***************************************************************************
 *  file: Interpreter.h , Command interpreter module.      Part of DIKUMUD *
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
/* $Id: interp.h,v 1.115 2011/12/28 01:26:50 jhhudso Exp $ */

#ifndef INTERP_H_
#define INTERP_H_

#include "character.h"
#include "returnvals.h"

struct char_data;

char *remove_trailing_spaces(char *arg);
int command_interpreter(char_data *ch, string argument, bool procced = 0);
int search_block(const char *arg, const char **l, bool exact);
int old_search_block(const char *argument, int begin, int length, const char **list, int mode);
void argument_interpreter(const char *argument, char *first_arg, char *second_arg);
char *one_argument(char *argument, char *first_arg);
const char *one_argument(const char *argument, char *first_arg);
char *one_argument_long(char *argument, char *first_arg);
char *one_argumentnolow(char *argument, char *first_arg);
int fill_word(char *argument);
void half_chop(const char *string, char *arg1, char *arg2);
tuple<string, string> last_argument(string arguments);
tuple<string, string> half_chop(string arguments, const char token = ' ');
void chop_half(char *string, char *arg1, char *arg2);
void nanny(struct descriptor_data *d, string arg = "");
int is_abbrev(char *arg1, char *arg2);
bool is_abbrev(const string &abbrev, const string &word);
int len_cmp(const char *s1, const char *s2);
void add_command_lag(char_data *ch, int cmdnum, int lag);
string ltrim(string str);
string rtrim(string str);

const auto CMD_NORTH = 1;
const auto CMD_EAST = 2;
const auto CMD_SOUTH = 3;
const auto CMD_WEST = 4;
const auto CMD_UP = 5;
const auto CMD_DOWN = 6;

const auto CMD_BELLOW = 8;
const auto CMD_DEFAULT = 9;
const auto CMD_TRACK = 10;
const auto CMD_PALM = 10;
const auto CMD_SAY = 11;
const auto CMD_LOOK = 12;
const auto CMD_BACKSTAB = 13;
const auto CMD_SBS = 14;
const auto CMD_ORCHESTRATE = 15;
const auto CMD_REPLY = 16;
const auto CMD_WHISPER = 17;
const auto CMD_GLANCE = 20;
const auto CMD_FLEE = 28;
const auto CMD_ESCAPE = 29;
const auto CMD_PICK = 35;
const auto CMD_STOCK = 56;
const auto CMD_BUY = 56;
const auto CMD_SELL = 57;
const auto CMD_VALUE = 58;
const auto CMD_LIST = 59;
const auto CMD_ENTER = 60;
const auto CMD_CLIMB = 60;
const auto CMD_DESIGN = 62;
const auto CMD_PRICE = 65;
const auto CMD_REPAIR = 66;
const auto CMD_READ = 67;
const auto CMD_REMOVE = 69;
const auto CMD_ERASE = 70;
const auto CMD_ESTIMATE = 71;
const auto CMD_REMORT = 80;
const auto CMD_REROLL = 81;
const auto CMD_CHOOSE = 82;
const auto CMD_CONFIRM = 83;
const auto CMD_CANCEL = 84;
const auto CMD_SLIP = 87;
const auto CMD_GIVE = 88;
const auto CMD_DROP = 89;
const auto CMD_DONATE = 90;
const auto CMD_QUIT = 91;
const auto CMD_SACRIFICE = 92;
const auto CMD_PUT = 93;
const auto CMD_OPEN = 98;
const auto CMD_EDITOR = 100;
const auto CMD_FORCE = 123;
const auto CMD_WRITE = 128;
const auto CMD_WATCH = 155;
const auto CMD_PRACTICE = 164;
const auto CMD_TRAIN = 165;
const auto CMD_PROFESSION = 166;
const auto CMD_GAIN = 171;
const auto CMD_BALANCE = 172;
const auto CMD_DEPOSIT = 173;
const auto CMD_WITHDRAW = 174;
const auto CMD_CLEAN = 177;
const auto CMD_PLAY = 178;
const auto CMD_FINISH = 179;
const auto CMD_VETERNARIAN = 180;
const auto CMD_FEED = 181;
const auto CMD_ASSEMBLE = 182;
const auto CMD_PAY = 183;
const auto CMD_RESTRING = 184;
const auto CMD_PUSH = 185;
const auto CMD_PULL = 186;
const auto CMD_LEAVE = 187;
const auto CMD_TREMOR = 188;
const auto CMD_BET = 189;
const auto CMD_INSURANCE = 190;
const auto CMD_DOUBLE = 191;
const auto CMD_STAY = 192;
const auto CMD_SPLIT = 193;
const auto CMD_HIT = 194;
const auto CMD_LOOT = 195;
const auto CMD_GTELL = 200;
const auto CMD_CTELL = 201;
const auto CMD_SETVOTE = 202;
const auto CMD_VOTE = 203;
const auto CMD_VEND = 204;
const auto CMD_FILTER = 205;
const auto CMD_EXAMINE = 206;
const auto CMD_GAG = 207;
const auto CMD_IMMORT = 208;
const auto CMD_IMPCHAN = 209;
const auto CMD_TELL = 210;
const auto CMD_TELLH = 211;
const auto CMD_PRIZE = 999;
const auto CMD_OTHER = 999;
const auto CMD_TELL_REPLY = 9999;
const auto CMD_GAZE = 1820;

// Temp removal to perfect system. 1/25/06 Eas
// WARNING WARNING WARNING WARNING WARNING
// The command list was modified to account for toggle_hide.
// The last integer will affect a char being removed from hide when they perform the command.

/*
 * Command functions.
 */
typedef int DO_FUN(char_data *ch, char *argument, int cmd);
typedef int command_return_t;

enum class CommandType
{
    all,
    players_only,
    non_players_only,
    immortals_only,
    implementors_only
};

struct command_info
{
    char *command_name;                                                                   /* Name of ths command             */
    int (*command_pointer)(char_data *ch, char *argument, int cmd);                       /* Function that does it            */
    command_return_t (*command_pointer2)(char_data *ch, string argument, int cmd);        /* Function that does it            */
    command_return_t (*command_pointer3)(char_data *ch, QStringList &arguments, int cmd); /* Function that does it            */
    uint8_t minimum_position;                                                             /* Position commander must be in    */
    uint8_t minimum_level;                                                                /* Minimum level needed             */
    int command_number;                                                                   /* Passed to function as argument   */
    int flags;                                                                            // what flags the skills has
    uint8_t toggle_hide;
    CommandType type;
};

struct command_lag
{
    struct command_lag *next;
    char_data *ch;
    int cmd_number;
    int lag;
};

struct cmd_hash_info
{
    struct command_info *command;
    struct cmd_hash_info *left;
    struct cmd_hash_info *right;
};

#define COM_CHARMIE_OK 1

command_return_t do_mscore(char_data *ch, char *argument, int cmd);
command_return_t do_clanarea(char_data *ch, char *argument, int cmd);
command_return_t do_huntstart(char_data *ch, char *argument, int cmd);
command_return_t do_huntclear(char_data *ch, char *argument, int cmd);

command_return_t do_metastat(char_data *ch, char *argument, int cmd);
command_return_t do_findfix(char_data *ch, char *argument, int cmd);
command_return_t do_reload(char_data *ch, char *argument, int cmd);
command_return_t do_abandon(char_data *ch, char *argument, int cmd);
command_return_t do_accept(char_data *ch, char *argument, int cmd);
command_return_t do_acfinder(char_data *ch, char *argument, int cmd);
command_return_t do_action(char_data *ch, char *argument, int cmd);
command_return_t do_addnews(char_data *ch, char *argument, int cmd);
command_return_t do_addRoom(char_data *ch, char *argument, int cmd);
command_return_t do_advance(char_data *ch, char *argument, int cmd);
command_return_t do_areastats(char_data *ch, char *argument, int cmd);
command_return_t do_awaymsgs(char_data *ch, char *argument, int cmd);
command_return_t do_guide(char_data *ch, char *argument, int cmd);
command_return_t do_alias(char_data *ch, char *argument, int cmd);
command_return_t do_archive(char_data *ch, char *argument, int cmd);
command_return_t do_autoeat(char_data *ch, char *argument, int cmd);
command_return_t do_autojoin(char_data *ch, string argument, int cmd);
command_return_t do_unban(char_data *ch, char *argument, int cmd);
command_return_t do_ambush(char_data *ch, char *argument, int cmd);
command_return_t do_anonymous(char_data *ch, char *argument, int cmd);
command_return_t do_ansi(char_data *ch, char *argument, int cmd);
command_return_t do_appraise(char_data *ch, char *argument, int cmd);
command_return_t do_assemble(char_data *ch, char *argument, int cmd);
command_return_t do_release(char_data *ch, char *argument, int cmd);
command_return_t do_jab(char_data *ch, char *argument, int cmd);
command_return_t do_areas(char_data *ch, char *argument, int cmd);
command_return_t do_arena(char_data *ch, char *argument, int cmd);
command_return_t do_ask(char_data *ch, char *argument, int cmd);
command_return_t do_at(char_data *ch, char *argument, int cmd);
command_return_t do_auction(char_data *ch, char *argument, int cmd);
command_return_t do_backstab(char_data *ch, char *argument, int cmd);
command_return_t do_ban(char_data *ch, char *argument, int cmd);
command_return_t do_bandwidth(char_data *ch, char *argument, int cmd);
command_return_t do_bard_song_toggle(char_data *ch, char *argument, int cmd);
command_return_t do_bash(char_data *ch, char *argument, int cmd);
command_return_t do_batter(char_data *ch, char *argument, int cmd);
command_return_t do_battlecry(char_data *ch, char *argument, int cmd);
command_return_t do_battlesense(char_data *ch, char *argument, int cmd);
command_return_t do_beacon(char_data *ch, char *argument, int cmd);
command_return_t do_beep(char_data *ch, char *argument, int cmd);
command_return_t do_beep_set(char_data *ch, char *argument, int cmd);
command_return_t do_behead(char_data *ch, char *argument, int cmd);
command_return_t do_berserk(char_data *ch, char *argument, int cmd);
command_return_t do_bestow(char_data *ch, string argument, int cmd);
command_return_t do_bladeshield(char_data *ch, char *argument, int cmd);
command_return_t do_bloodfury(char_data *ch, char *argument, int cmd);
command_return_t do_boot(char_data *ch, char *argument, int cmd);
command_return_t do_boss(char_data *ch, char *argument, int cmd);
command_return_t do_brace(char_data *ch, char *argument, int cmd);
command_return_t do_brief(char_data *ch, char *argument, int cmd);
command_return_t do_brew(char_data *ch, char *argument, int cmd);
command_return_t do_news_toggle(char_data *ch, char *argument, int cmd);
command_return_t do_ascii_toggle(char_data *ch, char *argument, int cmd);
command_return_t do_damage_toggle(char_data *ch, char *argument, int cmd);
command_return_t do_bug(char_data *ch, char *argument, int cmd);
command_return_t do_bullrush(char_data *ch, char *argument, int cmd);
command_return_t do_cast(char_data *ch, char *argument, int cmd);
command_return_t do_channel(char_data *ch, char *argument, int cmd);
command_return_t do_charmiejoin_toggle(char_data *ch, char *argument, int cmd);
command_return_t do_check(char_data *ch, char *argument, int cmd);
command_return_t do_chpwd(char_data *ch, char *argument, int cmd);
command_return_t do_cinfo(char_data *ch, char *argument, int cmd);
command_return_t do_circle(char_data *ch, char *argument, int cmd);
command_return_t do_clans(char_data *ch, char *argument, int cmd);
command_return_t do_clear(char_data *ch, char *argument, int cmd);
command_return_t do_clearaff(char_data *ch, char *argument, int cmd);
command_return_t do_climb(char_data *ch, char *argument, int cmd);
command_return_t do_close(char_data *ch, char *argument, int cmd);
command_return_t do_cmotd(char_data *ch, char *argument, int cmd);
command_return_t do_ctax(char_data *ch, char *argument, int cmd);
command_return_t do_cdeposit(char_data *ch, char *argument, int cmd);
command_return_t do_cwithdraw(char_data *ch, char *argument, int cmd);
command_return_t do_cbalance(char_data *ch, char *argument, int cmd);
command_return_t do_colors(char_data *ch, char *argument, int cmd);
command_return_t do_compact(char_data *ch, char *argument, int cmd);
command_return_t do_config(char_data *ch, QStringList &arguments, int cmd);
command_return_t do_consent(char_data *ch, char *argument, int cmd);
command_return_t do_consider(char_data *ch, char *argument, int cmd);
command_return_t do_count(char_data *ch, char *argument, int cmd);
command_return_t do_cpromote(char_data *ch, char *argument, int cmd);
command_return_t do_crazedassault(char_data *ch, char *argument, int cmd);
command_return_t do_credits(char_data *ch, char *argument, int cmd);
command_return_t do_cripple(char_data *ch, char *argument, int cmd);
command_return_t do_ctell(char_data *ch, char *argument, int cmd);
command_return_t do_deathstroke(char_data *ch, char *argument, int cmd);
command_return_t do_debug(char_data *ch, char *argument, int cmd);
command_return_t do_deceit(char_data *ch, char *argument, int cmd);
command_return_t do_defenders_stance(char_data *ch, char *argument, int cmd);
command_return_t do_disarm(char_data *ch, char *argument, int cmd);
command_return_t do_disband(char_data *ch, char *argument, int cmd);
command_return_t do_disconnect(char_data *ch, char *argument, int cmd);
command_return_t do_dmg_eq(char_data *ch, char *argument, int cmd);
command_return_t do_donate(char_data *ch, char *argument, int cmd);
command_return_t do_dream(char_data *ch, char *argument, int cmd);
command_return_t do_drink(char_data *ch, char *argument, int cmd);
command_return_t do_drop(char_data *ch, char *argument, int cmd);
command_return_t do_eat(char_data *ch, char *argument, int cmd);
command_return_t do_eagle_claw(char_data *ch, char *argument, int cmd);
command_return_t do_echo(char_data *ch, char *argument, int cmd);
command_return_t do_emote(char_data *ch, char *argument, int cmd);
command_return_t do_setvote(char_data *ch, char *argument, int cmd);
command_return_t do_vote(char_data *ch, char *argument, int cmd);
command_return_t do_enter(char_data *ch, char *argument, int cmd);
command_return_t do_equipment(char_data *ch, char *argument, int cmd);
command_return_t do_eyegouge(char_data *ch, char *argument, int cmd);
command_return_t do_examine(char_data *ch, char *argument, int cmd);
command_return_t do_exits(char_data *ch, char *argument, int cmd);
command_return_t do_experience(char_data *ch, QStringList &arguments, int cmd);
command_return_t do_export(char_data *ch, char *argument, int cmd);
command_return_t do_ferocity(char_data *ch, char *argument, int cmd);
command_return_t do_fighting(char_data *ch, char *argument, int cmd);
command_return_t do_fill(char_data *ch, char *argument, int cmd);
command_return_t do_find(char_data *ch, char *argument, int cmd);
command_return_t do_findpath(char_data *ch, char *argument, int cmd);
command_return_t do_findPath(char_data *ch, char *argument, int cmd);
command_return_t do_fire(char_data *ch, char *argument, int cmd);
command_return_t do_flee(char_data *ch, char *argument, int cmd);
/* DO_FUN  do_fly; */
command_return_t do_focused_repelance(char_data *ch, char *argument, int cmd);
command_return_t do_follow(char_data *ch, char *argument, int cmd);
command_return_t do_forage(char_data *ch, char *argument, int cmd);
command_return_t do_force(char_data *ch, string argument, int cmd);
command_return_t do_found(char_data *ch, char *argument, int cmd);
command_return_t do_free_animal(char_data *ch, char *argument, int cmd);
command_return_t do_freeze(char_data *ch, char *argument, int cmd);
command_return_t do_fsave(char_data *ch, string argument, int cmd);
command_return_t do_get(char_data *ch, char *argument, int cmd);
command_return_t do_give(char_data *ch, char *argument, int cmd);
command_return_t do_global(char_data *ch, char *argument, int cmd);
command_return_t do_gossip(char_data *ch, char *argument, int cmd);
command_return_t do_golem_score(char_data *ch, char *argument, int cmd);
command_return_t do_goto(char_data *ch, string argument, int cmd);
command_return_t do_guild(char_data *ch, char *argument, int cmd);
command_return_t do_install(char_data *ch, char *argument, int cmd);
command_return_t do_reload_help(char_data *ch, char *argument, int cmd);
command_return_t do_hindex(char_data *ch, char *argument, int cmd);
command_return_t do_hedit(char_data *ch, char *argument, int cmd);
command_return_t do_grab(char_data *ch, char *argument, int cmd);
command_return_t do_group(char_data *ch, char *argument, int cmd);
command_return_t do_grouptell(char_data *ch, char *argument, int cmd);
command_return_t do_gtrans(char_data *ch, char *argument, int cmd);
command_return_t do_guard(char_data *ch, char *argument, int cmd);
command_return_t do_harmtouch(char_data *ch, char *argument, int cmd);
command_return_t do_help(char_data *ch, char *argument, int cmd);
command_return_t do_mortal_help(char_data *ch, char *argument, int cmd);
command_return_t do_new_help(char_data *ch, char *argument, int cmd);
command_return_t do_hide(char_data *ch, char *argument, int cmd);
command_return_t do_highfive(char_data *ch, char *argument, int cmd);
command_return_t do_hit(char_data *ch, char *argument, int cmd);
command_return_t do_hitall(char_data *ch, char *argument, int cmd);
command_return_t do_holylite(char_data *ch, char *argument, int cmd);
command_return_t do_home(char_data *ch, char *argument, int cmd);
command_return_t do_idea(char_data *ch, char *argument, int cmd);
command_return_t do_identify(char_data *ch, char *argument, int cmd);
command_return_t do_ignore(char_data *ch, string argument, int cmd);
command_return_t do_imotd(char_data *ch, char *argument, int cmd);
command_return_t do_imbue(char_data *ch, char *argument, int cmd);
command_return_t do_incognito(char_data *ch, char *argument, int cmd);
command_return_t do_index(char_data *ch, char *argument, int cmd);
command_return_t do_info(char_data *ch, char *argument, int cmd);
command_return_t do_initiate(char_data *ch, char *argument, int cmd);
command_return_t do_innate(char_data *ch, char *argument, int cmd);
command_return_t do_instazone(char_data *ch, char *argument, int cmd);
command_return_t do_insult(char_data *ch, char *argument, int cmd);
command_return_t do_inventory(char_data *ch, char *argument, int cmd);
command_return_t do_join(char_data *ch, char *argument, int cmd);
command_return_t do_joinarena(char_data *ch, char *argument, int cmd);
command_return_t do_ki(char_data *ch, char *argument, int cmd);
command_return_t do_kick(char_data *ch, char *argument, int cmd);
command_return_t do_kill(char_data *ch, char *argument, int cmd);
command_return_t do_knockback(char_data *ch, char *argument, int cmd);
// command_return_t do_land (char_data *ch, char *argument, int cmd);
command_return_t do_layhands(char_data *ch, char *argument, int cmd);
command_return_t do_leaderboard(char_data *ch, char *argument, int cmd);
command_return_t do_leadership(char_data *ch, char *argument, int cmd);
command_return_t do_leave(char_data *ch, char *argument, int cmd);
command_return_t do_levels(char_data *ch, char *argument, int cmd);
command_return_t do_lfg_toggle(char_data *ch, char *argument, int cmd);
command_return_t do_linkdead(char_data *ch, char *argument, int cmd);
command_return_t do_linkload(char_data *ch, char *argument, int cmd);
command_return_t do_listAllPaths(char_data *ch, char *argument, int cmd);
command_return_t do_listPathsByZone(char_data *ch, char *argument, int cmd);
command_return_t do_listproc(char_data *ch, char *argument, int cmd);
command_return_t do_load(char_data *ch, char *argument, int cmd);
command_return_t do_medit(char_data *ch, char *argument, int cmd);
command_return_t do_memoryleak(char_data *ch, char *argument, int cmd);
command_return_t do_mortal_set(char_data *ch, char *argument, int cmd);
// command_return_t do_motdload (char_data *ch, char *argument, int cmd);
command_return_t do_msave(char_data *ch, char *argument, int cmd);
command_return_t do_procedit(char_data *ch, char *argument, int cmd);
command_return_t do_mpbestow(char_data *ch, char *argument, int cmd);
command_return_t do_mpstat(char_data *ch, char *argument, int cmd);
command_return_t do_opedit(char_data *ch, char *argument, int cmd);
command_return_t do_eqmax(char_data *ch, char *argument, int cmd);
command_return_t do_opstat(char_data *ch, char *argument, int cmd);
command_return_t do_lock(char_data *ch, char *argument, int cmd);
command_return_t do_log(char_data *ch, char *argument, int cmd);
command_return_t do_look(char_data *ch, char *argument, int cmd);
command_return_t do_botcheck(char_data *ch, char *argument, int cmd);
command_return_t do_make_camp(char_data *ch, char *argument, int cmd);
command_return_t do_matrixinfo(char_data *ch, char *argument, int cmd);
command_return_t do_maxes(char_data *ch, char *argument, int cmd);
command_return_t do_mlocate(char_data *ch, char *argument, int cmd);
command_return_t do_move(char_data *ch, char *argument, int cmd);
command_return_t do_motd(char_data *ch, char *argument, int cmd);
command_return_t do_mpretval(char_data *ch, char *argument, int cmd);
command_return_t do_mpasound(char_data *ch, char *argument, int cmd);
command_return_t do_mpat(char_data *ch, char *argument, int cmd);
command_return_t do_mpdamage(char_data *ch, char *argument, int cmd);
command_return_t do_mpecho(char_data *ch, char *argument, int cmd);
command_return_t do_mpechoaround(char_data *ch, char *argument, int cmd);
command_return_t do_mpechoaroundnotbad(char_data *ch, char *argument, int cmd);
command_return_t do_mpechoat(char_data *ch, char *argument, int cmd);
command_return_t do_mpforce(char_data *ch, char *argument, int cmd);
command_return_t do_mpgoto(char_data *ch, char *argument, int cmd);
command_return_t do_mpjunk(char_data *ch, char *argument, int cmd);
command_return_t do_mpkill(char_data *ch, char *argument, int cmd);
command_return_t do_mphit(char_data *ch, char *argument, int cmd);
command_return_t do_mpsetmath(char_data *ch, char *argument, int cmd);
command_return_t do_mpaddlag(char_data *ch, char *argument, int cmd);
command_return_t do_mpmload(char_data *ch, char *argument, int cmd);
command_return_t do_mpoload(char_data *ch, char *argument, int cmd);
command_return_t do_mppause(char_data *ch, char *argument, int cmd);
command_return_t do_mppeace(char_data *ch, char *argument, int cmd);
command_return_t do_mppurge(char_data *ch, char *argument, int cmd);
command_return_t do_mpteachskill(char_data *ch, char *argument, int cmd);
command_return_t do_mpsetalign(char_data *ch, char *argument, int cmd);
command_return_t do_mpsettemp(char_data *ch, char *argument, int cmd);
command_return_t do_mpthrow(char_data *ch, char *argument, int cmd);
command_return_t do_mpothrow(char_data *ch, char *argument, int cmd);
command_return_t do_mptransfer(char_data *ch, char *argument, int cmd);
command_return_t do_mpxpreward(char_data *ch, char *argument, int cmd);
command_return_t do_mpteleport(char_data *ch, char *argument, int cmd);
command_return_t do_murder(char_data *ch, char *argument, int cmd);
command_return_t do_name(char_data *ch, char *argument, int cmd);
command_return_t do_natural_selection(char_data *ch, char *argument, int cmd);
command_return_t do_newbie(char_data *ch, char *argument, int cmd);
command_return_t do_newPath(char_data *ch, char *argument, int cmd);
command_return_t do_news(char_data *ch, char *argument, int cmd);
command_return_t do_noemote(char_data *ch, char *argument, int cmd);
command_return_t do_nohassle(char_data *ch, char *argument, int cmd);
command_return_t do_noname(char_data *ch, char *argument, int cmd);
command_return_t do_not_here(char_data *ch, char *argument, int cmd);
command_return_t do_notax_toggle(char_data *ch, char *argument, int cmd);
command_return_t do_guide_toggle(char_data *ch, char *argument, int cmd);
command_return_t do_oclone(char_data *ch, char *argument, int cmd);
command_return_t do_mclone(char_data *ch, char *argument, int cmd);
command_return_t do_oedit(char_data *ch, char *argument, int cmd);
command_return_t do_offer(char_data *ch, char *argument, int cmd);
command_return_t do_olocate(char_data *ch, char *argument, int cmd);
command_return_t do_oneway(char_data *ch, char *argument, int cmd);
command_return_t do_onslaught(char_data *ch, char *argument, int cmd);
command_return_t do_open(char_data *ch, char *argument, int cmd);
command_return_t do_order(char_data *ch, char *argument, int cmd);
command_return_t do_orchestrate(char_data *ch, char *argument, int cmd);
command_return_t do_osave(char_data *ch, char *argument, int cmd);
command_return_t do_nodupekeys_toggle(char_data *ch, char *argument, int cmd);
command_return_t do_outcast(char_data *ch, char *argument, int cmd);
command_return_t do_pager(char_data *ch, char *argument, int cmd);
command_return_t do_pardon(char_data *ch, char *argument, int cmd);
command_return_t do_pathpath(char_data *ch, char *argument, int cmd);
command_return_t do_peace(char_data *ch, char *argument, int cmd);
command_return_t do_perseverance(char_data *ch, char *argument, int cmd);
command_return_t do_pick(char_data *ch, char *argument, int cmd);
command_return_t do_plats(char_data *ch, char *argument, int cmd);
command_return_t do_pocket(char_data *ch, char *argument, int cmd);
command_return_t do_poisonmaking(char_data *ch, char *argument, int cmd);
command_return_t do_pour(char_data *ch, char *argument, int cmd);
command_return_t do_poof(char_data *ch, char *argument, int cmd);
command_return_t do_possess(char_data *ch, char *argument, int cmd);
command_return_t do_practice(char_data *ch, char *argument, int cmd);
command_return_t do_pray(char_data *ch, char *argument, int cmd);
command_return_t do_profession(char_data *ch, char *argument, int cmd);
command_return_t do_primalfury(char_data *ch, char *argument, int cmd);
command_return_t do_promote(char_data *ch, char *argument, int cmd);
command_return_t do_prompt(char_data *ch, char *argument, int cmd);
command_return_t do_lastprompt(char_data *ch, char *argument, int cmd);
command_return_t do_processes(char_data *ch, char *argument, int cmd);
command_return_t do_psay(char_data *ch, string argument, int cmd);
// command_return_t do_pshopedit (char_data *ch, char *argument, int cmd);
command_return_t do_pview(char_data *ch, char *argument, int cmd);
command_return_t do_punish(char_data *ch, char *argument, int cmd);
command_return_t do_purge(char_data *ch, char *argument, int cmd);
command_return_t do_purloin(char_data *ch, char *argument, int cmd);
command_return_t do_put(char_data *ch, char *argument, int cmd);
command_return_t do_qedit(char_data *ch, char *argument, int cmd);
command_return_t do_quaff(char_data *ch, char *argument, int cmd);
command_return_t do_quest(char_data *ch, char *argument, int cmd);
command_return_t do_qui(char_data *ch, char *argument, int cmd);
command_return_t do_quivering_palm(char_data *ch, char *argument, int cmd);
command_return_t do_quit(char_data *ch, char *argument, int cmd);
command_return_t do_rage(char_data *ch, char *argument, int cmd);
command_return_t do_random(char_data *ch, char *argument, int cmd);
command_return_t do_range(char_data *ch, char *argument, int cmd);
command_return_t do_rdelete(char_data *ch, char *argument, int cmd);
command_return_t do_read(char_data *ch, char *argument, int cmd);
command_return_t do_recall(char_data *ch, char *argument, int cmd);
command_return_t do_recite(char_data *ch, char *argument, int cmd);
command_return_t do_redirect(char_data *ch, char *argument, int cmd);
command_return_t do_redit(char_data *ch, char *argument, int cmd);
command_return_t do_remove(char_data *ch, char *argument, int cmd);
command_return_t do_rename_char(char_data *ch, char *argument, int cmd);
command_return_t do_rent(char_data *ch, char *argument, int cmd);
command_return_t do_reply(char_data *ch, string argument, int cmd);
command_return_t do_repop(char_data *ch, string argument, int cmd);
command_return_t do_report(char_data *ch, char *argument, int cmd);
command_return_t do_rescue(char_data *ch, char *argument, int cmd);
command_return_t do_rest(char_data *ch, char *argument, int cmd);
command_return_t do_restore(char_data *ch, char *argument, int cmd);
command_return_t do_retreat(char_data *ch, char *argument, int cmd);
command_return_t do_return(char_data *ch, char *argument, int cmd);
command_return_t do_revoke(char_data *ch, char *argument, int cmd);
command_return_t do_rsave(char_data *ch, char *argument, int cmd);
command_return_t do_rstat(char_data *ch, char *argument, int cmd);
command_return_t do_sacrifice(char_data *ch, char *argument, int cmd);
command_return_t do_save(char_data *ch, char *argument, int cmd);
command_return_t do_say(char_data *ch, string argument, int cmd = CMD_SAY);
command_return_t do_scan(char_data *ch, char *argument, int cmd);
command_return_t do_score(char_data *ch, char *argument, int cmd);
command_return_t do_scribe(char_data *ch, char *argument, int cmd);
command_return_t do_sector(char_data *ch, char *argument, int cmd);
command_return_t do_sedit(char_data *ch, char *argument, int cmd);
command_return_t do_send(char_data *ch, char *argument, int cmd);
command_return_t do_set(char_data *ch, char *argument, int cmd);
command_return_t do_search(char_data *ch, char *argument, int cmd);
command_return_t do_shout(char_data *ch, char *argument, int cmd);
command_return_t do_showhunt(char_data *ch, char *argument, int cmd);
command_return_t do_skills(char_data *ch, char *argument, int cmd);
command_return_t do_social(char_data *ch, char *argument, int cmd);
command_return_t do_songs(char_data *ch, char *argument, int cmd);
command_return_t do_stromboli(char_data *ch, char *argument, int cmd);
command_return_t do_summon_toggle(char_data *ch, char *argument, int cmd);
command_return_t do_headbutt(char_data *ch, char *argument, int cmd);
command_return_t do_show(char_data *ch, char *argument, int cmd);
command_return_t do_showbits(char_data *ch, char *argument, int cmd);
command_return_t do_shutdow(char_data *ch, char *argument, int cmd);
command_return_t do_shutdown(char_data *ch, char *argument, int cmd);
command_return_t do_silence(char_data *ch, char *argument, int cmd);
command_return_t do_stupid(char_data *ch, char *argument, int cmd);
command_return_t do_sing(char_data *ch, char *argument, int cmd);
command_return_t do_sip(char_data *ch, char *argument, int cmd);
command_return_t do_sit(char_data *ch, char *argument, int cmd);
command_return_t do_slay(char_data *ch, char *argument, int cmd);
command_return_t do_sleep(char_data *ch, char *argument, int cmd);
command_return_t do_slip(char_data *ch, char *argument, int cmd);
command_return_t do_smite(char_data *ch, char *argument, int cmd);
command_return_t do_sneak(char_data *ch, char *argument, int cmd);
command_return_t do_snoop(char_data *ch, char *argument, int cmd);
command_return_t do_sockets(char_data *ch, char *argument, int cmd);
command_return_t do_spells(char_data *ch, char *argument, int cmd);
command_return_t do_split(char_data *ch, char *argument, int cmd);
command_return_t do_sqedit(char_data *ch, char *argument, int cmd);
command_return_t do_stalk(char_data *ch, char *argument, int cmd);
command_return_t do_stand(char_data *ch, char *argument, int cmd);
command_return_t do_stat(char_data *ch, char *argument, int cmd);
command_return_t do_steal(char_data *ch, char *argument, int cmd);
command_return_t do_stealth(char_data *ch, char *argument, int cmd);
command_return_t do_story(char_data *ch, char *argument, int cmd);
command_return_t do_string(char_data *ch, char *argument, int cmd);
command_return_t do_stun(char_data *ch, char *argument, int cmd);
command_return_t do_suicide(char_data *ch, char *argument, int cmd);
command_return_t do_switch(char_data *ch, char *argument, int cmd);
command_return_t do_tactics(char_data *ch, char *argument, int cmd);
command_return_t do_tame(char_data *ch, char *argument, int cmd);
command_return_t do_taste(char_data *ch, char *argument, int cmd);
command_return_t do_teleport(char_data *ch, char *argument, int cmd);
command_return_t do_tell(char_data *ch, string argument, int cmd);
command_return_t do_tellhistory(char_data *ch, string argument, int cmd);
command_return_t do_testhand(char_data *ch, char *argument, int cmd);
command_return_t do_testhit(char_data *ch, char *argument, int cmd);
command_return_t do_testport(char_data *ch, char *argument, int cmd);
command_return_t do_testuser(char_data *ch, char *argument, int cmd);
command_return_t do_thunder(char_data *ch, char *argument, int cmd);
command_return_t do_tick(char_data *ch, char *argument, int cmd);
command_return_t do_time(char_data *ch, char *argument, int cmd);
command_return_t do_title(char_data *ch, char *argument, int cmd);
command_return_t do_toggle(char_data *ch, char *argument, int cmd);
command_return_t do_track(char_data *ch, char *argument, int cmd);
command_return_t do_transfer(char_data *ch, string argument, int cmd = CMD_DEFAULT);
command_return_t do_triage(char_data *ch, char *argument, int cmd);
command_return_t do_trip(char_data *ch, char *argument, int cmd);
command_return_t do_trivia(char_data *ch, char *argument, int cmd);
command_return_t do_typo(char_data *ch, char *argument, int cmd);
command_return_t do_unarchive(char_data *ch, char *argument, int cmd);
command_return_t do_unlock(char_data *ch, char *argument, int cmd);
command_return_t do_use(char_data *ch, char *argument, int cmd);
command_return_t do_varstat(char_data *ch, char *argument, int cmd);
command_return_t do_vault(char_data *ch, char *argument, int cmd);
command_return_t do_vend(char_data *ch, char *argument, int cmd);
command_return_t do_version(char_data *ch, char *argument, int cmd);
command_return_t do_visible(char_data *ch, char *argument, int cmd);
command_return_t do_vitalstrike(char_data *ch, char *argument, int cmd);
command_return_t do_vt100(char_data *ch, char *argument, int cmd);
command_return_t do_wake(char_data *ch, char *argument, int cmd);
command_return_t do_wear(char_data *ch, char *argument, int cmd);
command_return_t do_weather(char_data *ch, char *argument, int cmd);
command_return_t do_where(char_data *ch, char *argument, int cmd);
command_return_t do_whisper(char_data *ch, char *argument, int cmd);
command_return_t do_who(char_data *ch, char *argument, int cmd);
command_return_t do_whoarena(char_data *ch, char *argument, int cmd);
command_return_t do_whoclan(char_data *ch, char *argument, int cmd);
command_return_t do_whogroup(char_data *ch, char *argument, int cmd);
command_return_t do_whosolo(char_data *ch, char *argument, int cmd);
command_return_t do_wield(char_data *ch, char *argument, int cmd);
command_return_t do_fakelog(char_data *ch, char *argument, int cmd);
command_return_t do_wimpy(char_data *ch, char *argument, int cmd);
command_return_t do_wiz(char_data *ch, string argument, int cmd);
command_return_t do_wizhelp(char_data *ch, char *argument, int cmd);
command_return_t do_wizinvis(char_data *ch, char *argument, int cmd);
command_return_t do_wizlist(char_data *ch, char *argument, int cmd);
command_return_t do_wizlock(char_data *ch, char *argument, int cmd);
command_return_t do_world(char_data *ch, string args, int cmd);
command_return_t do_write_skillquest(char_data *ch, char *argument, int cmd);
command_return_t do_write(char_data *ch, char *argument, int cmd);
command_return_t do_zap(char_data *ch, char *argument, int cmd);
command_return_t do_zedit(char_data *ch, char *argument, int cmd);
command_return_t do_zoneexits(char_data *ch, char *argument, int cmd);
command_return_t do_zsave(char_data *ch, char *argument, int cmd);
command_return_t do_editor(char_data *ch, char *argument, int cmd);
command_return_t do_pursue(char_data *ch, char *argument, int cmd);

#endif
