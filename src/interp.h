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

class Character;

char *remove_trailing_spaces(char *arg);
int command_interpreter(Character *ch, string argument, bool procced = 0);
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
void nanny(class Connection *d, string arg = "");
bool is_abbrev(QString abbrev, QString word);
// bool is_abbrev(const string &abbrev, const string &word);
// bool is_abbrev(const char *arg1, const char *arg2);
int len_cmp(const char *s1, const char *s2);
void add_command_lag(Character *ch, int cmdnum, int lag);
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
typedef int DO_FUN(Character *ch, char *argument, int cmd);
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
    char *command_name;                                                              /* Name of ths command             */
    int (*command_pointer)(Character *ch, char *argument, int cmd);                  /* Function that does it            */
    command_return_t (*command_pointer2)(Character *ch, string argument, int cmd);   /* Function that does it            */
    command_return_t (Character::*command_pointer3)(QStringList arguments, int cmd); /* Function that does it            */
    uint8_t minimum_position;                                                        /* Position commander must be in    */
    uint8_t minimum_level;                                                           /* Minimum level needed             */
    int command_number;                                                              /* Passed to function as argument   */
    int flags;                                                                       // what flags the skills has
    uint8_t toggle_hide;
    CommandType type;
};

struct command_lag
{
    struct command_lag *next;
    Character *ch;
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

command_return_t do_mscore(Character *ch, char *argument, int cmd);
command_return_t do_huntstart(Character *ch, char *argument, int cmd);
command_return_t do_huntclear(Character *ch, char *argument, int cmd);

command_return_t do_metastat(Character *ch, char *argument, int cmd);
command_return_t do_findfix(Character *ch, char *argument, int cmd);
command_return_t do_reload(Character *ch, char *argument, int cmd);
command_return_t do_abandon(Character *ch, char *argument, int cmd);
command_return_t do_accept(Character *ch, char *argument, int cmd);
command_return_t do_acfinder(Character *ch, char *argument, int cmd);
command_return_t do_action(Character *ch, char *argument, int cmd);
command_return_t do_addnews(Character *ch, char *argument, int cmd);
command_return_t do_addRoom(Character *ch, char *argument, int cmd);
command_return_t do_advance(Character *ch, char *argument, int cmd);
command_return_t do_areastats(Character *ch, char *argument, int cmd);
command_return_t do_awaymsgs(Character *ch, char *argument, int cmd);
command_return_t do_guide(Character *ch, char *argument, int cmd);
command_return_t do_alias(Character *ch, char *argument, int cmd);
command_return_t do_archive(Character *ch, char *argument, int cmd);
command_return_t do_autoeat(Character *ch, char *argument, int cmd);
command_return_t do_autojoin(Character *ch, string argument, int cmd);
command_return_t do_unban(Character *ch, char *argument, int cmd);
command_return_t do_ambush(Character *ch, char *argument, int cmd);
command_return_t do_anonymous(Character *ch, char *argument, int cmd);
command_return_t do_ansi(Character *ch, char *argument, int cmd);
command_return_t do_appraise(Character *ch, char *argument, int cmd);
command_return_t do_assemble(Character *ch, char *argument, int cmd);
command_return_t do_release(Character *ch, char *argument, int cmd);
command_return_t do_jab(Character *ch, char *argument, int cmd);
command_return_t do_areas(Character *ch, char *argument, int cmd);
command_return_t do_arena(Character *ch, char *argument, int cmd);
command_return_t do_ask(Character *ch, char *argument, int cmd);
command_return_t do_at(Character *ch, char *argument, int cmd);
command_return_t do_auction(Character *ch, char *argument, int cmd);
command_return_t do_backstab(Character *ch, char *argument, int cmd);
command_return_t do_ban(Character *ch, char *argument, int cmd);
command_return_t do_bandwidth(Character *ch, char *argument, int cmd);
command_return_t do_bard_song_toggle(Character *ch, char *argument, int cmd);
command_return_t do_bash(Character *ch, char *argument, int cmd);
command_return_t do_batter(Character *ch, char *argument, int cmd);
command_return_t do_battlecry(Character *ch, char *argument, int cmd);
command_return_t do_battlesense(Character *ch, char *argument, int cmd);
command_return_t do_beacon(Character *ch, char *argument, int cmd);
command_return_t do_beep(Character *ch, char *argument, int cmd);
command_return_t do_beep_set(Character *ch, char *argument, int cmd);
command_return_t do_behead(Character *ch, char *argument, int cmd);
command_return_t do_berserk(Character *ch, char *argument, int cmd);
command_return_t do_bestow(Character *ch, string argument, int cmd);
command_return_t do_bladeshield(Character *ch, char *argument, int cmd);
command_return_t do_bloodfury(Character *ch, char *argument, int cmd);
command_return_t do_boot(Character *ch, char *argument, int cmd);
command_return_t do_boss(Character *ch, char *argument, int cmd);
command_return_t do_brace(Character *ch, char *argument, int cmd);
command_return_t do_brief(Character *ch, char *argument, int cmd);
command_return_t do_brew(Character *ch, char *argument, int cmd);
command_return_t do_news_toggle(Character *ch, char *argument, int cmd);
command_return_t do_ascii_toggle(Character *ch, char *argument, int cmd);
command_return_t do_damage_toggle(Character *ch, char *argument, int cmd);
command_return_t do_bug(Character *ch, char *argument, int cmd);
command_return_t do_bullrush(Character *ch, char *argument, int cmd);
command_return_t do_cast(Character *ch, char *argument, int cmd);
command_return_t do_channel(Character *ch, char *argument, int cmd);
command_return_t do_charmiejoin_toggle(Character *ch, char *argument, int cmd);
command_return_t do_check(Character *ch, char *argument, int cmd);
command_return_t do_chpwd(Character *ch, char *argument, int cmd);
command_return_t do_cinfo(Character *ch, char *argument, int cmd);
command_return_t do_circle(Character *ch, char *argument, int cmd);
command_return_t do_clans(Character *ch, char *argument, int cmd);
command_return_t do_clear(Character *ch, char *argument, int cmd);
command_return_t do_clearaff(Character *ch, char *argument, int cmd);
command_return_t do_climb(Character *ch, char *argument, int cmd);
command_return_t do_close(Character *ch, char *argument, int cmd);
command_return_t do_cmotd(Character *ch, char *argument, int cmd);
command_return_t do_ctax(Character *ch, char *argument, int cmd);

command_return_t do_cwithdraw(Character *ch, char *argument, int cmd);
command_return_t do_cbalance(Character *ch, char *argument, int cmd);
command_return_t do_colors(Character *ch, char *argument, int cmd);
command_return_t do_compact(Character *ch, char *argument, int cmd);
command_return_t do_config(Character *ch, QStringList arguments, int cmd);
command_return_t do_consent(Character *ch, char *argument, int cmd);
command_return_t do_consider(Character *ch, char *argument, int cmd);
command_return_t do_count(Character *ch, char *argument, int cmd);
command_return_t do_cpromote(Character *ch, char *argument, int cmd);
command_return_t do_crazedassault(Character *ch, char *argument, int cmd);
command_return_t do_credits(Character *ch, char *argument, int cmd);
command_return_t do_cripple(Character *ch, char *argument, int cmd);
command_return_t do_ctell(Character *ch, char *argument, int cmd);
command_return_t do_deathstroke(Character *ch, char *argument, int cmd);
command_return_t do_debug(Character *ch, char *argument, int cmd);
command_return_t do_deceit(Character *ch, char *argument, int cmd);
command_return_t do_defenders_stance(Character *ch, char *argument, int cmd);
command_return_t do_disarm(Character *ch, char *argument, int cmd);
command_return_t do_disband(Character *ch, char *argument, int cmd);
command_return_t do_disconnect(Character *ch, char *argument, int cmd);
command_return_t do_dmg_eq(Character *ch, char *argument, int cmd);
command_return_t do_donate(Character *ch, char *argument, int cmd);
command_return_t do_dream(Character *ch, char *argument, int cmd);
command_return_t do_drink(Character *ch, char *argument, int cmd);
command_return_t do_drop(Character *ch, char *argument, int cmd);
command_return_t do_eat(Character *ch, char *argument, int cmd);
command_return_t do_eagle_claw(Character *ch, char *argument, int cmd);
command_return_t do_echo(Character *ch, char *argument, int cmd);
command_return_t do_emote(Character *ch, char *argument, int cmd);
command_return_t do_setvote(Character *ch, char *argument, int cmd);
command_return_t do_vote(Character *ch, char *argument, int cmd);
command_return_t do_enter(Character *ch, char *argument, int cmd);
command_return_t do_equipment(Character *ch, char *argument, int cmd);
command_return_t do_eyegouge(Character *ch, char *argument, int cmd);
command_return_t do_examine(Character *ch, char *argument, int cmd);
command_return_t do_exits(Character *ch, char *argument, int cmd);
command_return_t do_experience(Character *ch, QStringList arguments, int cmd);
command_return_t do_export(Character *ch, char *argument, int cmd);
command_return_t do_ferocity(Character *ch, char *argument, int cmd);
command_return_t do_fighting(Character *ch, char *argument, int cmd);
command_return_t do_fill(Character *ch, char *argument, int cmd);
command_return_t do_find(Character *ch, char *argument, int cmd);
command_return_t do_findpath(Character *ch, char *argument, int cmd);
command_return_t do_findPath(Character *ch, char *argument, int cmd);
command_return_t do_fire(Character *ch, char *argument, int cmd);
command_return_t do_flee(Character *ch, char *argument, int cmd);
/* DO_FUN  do_fly; */
command_return_t do_focused_repelance(Character *ch, char *argument, int cmd);
command_return_t do_follow(Character *ch, char *argument, int cmd);
command_return_t do_forage(Character *ch, char *argument, int cmd);
command_return_t do_force(Character *ch, string argument, int cmd);
command_return_t do_found(Character *ch, char *argument, int cmd);
command_return_t do_free_animal(Character *ch, char *argument, int cmd);
command_return_t do_freeze(Character *ch, char *argument, int cmd);
command_return_t do_fsave(Character *ch, string argument, int cmd);
command_return_t do_get(Character *ch, char *argument, int cmd);
command_return_t do_give(Character *ch, char *argument, int cmd);
command_return_t do_global(Character *ch, char *argument, int cmd);
command_return_t do_gossip(Character *ch, char *argument, int cmd);
command_return_t do_golem_score(Character *ch, char *argument, int cmd);
command_return_t do_guild(Character *ch, char *argument, int cmd);
command_return_t do_install(Character *ch, char *argument, int cmd);
command_return_t do_reload_help(Character *ch, char *argument, int cmd);
command_return_t do_hindex(Character *ch, char *argument, int cmd);
command_return_t do_hedit(Character *ch, char *argument, int cmd);
command_return_t do_grab(Character *ch, char *argument, int cmd);
command_return_t do_group(Character *ch, char *argument, int cmd);
command_return_t do_grouptell(Character *ch, char *argument, int cmd);
command_return_t do_gtrans(Character *ch, char *argument, int cmd);
command_return_t do_guard(Character *ch, char *argument, int cmd);
command_return_t do_harmtouch(Character *ch, char *argument, int cmd);
command_return_t do_help(Character *ch, char *argument, int cmd);
command_return_t do_mortal_help(Character *ch, char *argument, int cmd);
command_return_t do_new_help(Character *ch, char *argument, int cmd);
command_return_t do_hide(Character *ch, char *argument, int cmd);
command_return_t do_highfive(Character *ch, char *argument, int cmd);
command_return_t do_hit(Character *ch, char *argument, int cmd);
command_return_t do_hitall(Character *ch, char *argument, int cmd);
command_return_t do_holylite(Character *ch, char *argument, int cmd);
command_return_t do_home(Character *ch, char *argument, int cmd);
command_return_t do_idea(Character *ch, char *argument, int cmd);
command_return_t do_identify(Character *ch, char *argument, int cmd);
command_return_t do_ignore(Character *ch, string argument, int cmd);
command_return_t do_imotd(Character *ch, char *argument, int cmd);
command_return_t do_imbue(Character *ch, char *argument, int cmd);
command_return_t do_incognito(Character *ch, char *argument, int cmd);
command_return_t do_index(Character *ch, char *argument, int cmd);
command_return_t do_info(Character *ch, char *argument, int cmd);
command_return_t do_initiate(Character *ch, char *argument, int cmd);
command_return_t do_innate(Character *ch, char *argument, int cmd);
command_return_t do_instazone(Character *ch, char *argument, int cmd);
command_return_t do_insult(Character *ch, char *argument, int cmd);
command_return_t do_inventory(Character *ch, char *argument, int cmd);
command_return_t do_join(Character *ch, char *argument, int cmd);
command_return_t do_joinarena(Character *ch, char *argument, int cmd);
command_return_t do_ki(Character *ch, char *argument, int cmd);
command_return_t do_kick(Character *ch, char *argument, int cmd);
command_return_t do_kill(Character *ch, char *argument, int cmd);
command_return_t do_knockback(Character *ch, char *argument, int cmd);
// command_return_t do_land (Character *ch, char *argument, int cmd);
command_return_t do_layhands(Character *ch, char *argument, int cmd);
command_return_t do_leaderboard(Character *ch, char *argument, int cmd);
command_return_t do_leadership(Character *ch, char *argument, int cmd);
command_return_t do_leave(Character *ch, char *argument, int cmd);
command_return_t do_levels(Character *ch, char *argument, int cmd);
command_return_t do_lfg_toggle(Character *ch, char *argument, int cmd);
command_return_t do_linkdead(Character *ch, char *argument, int cmd);
command_return_t do_linkload(Character *ch, char *argument, int cmd);
command_return_t do_listAllPaths(Character *ch, char *argument, int cmd);
command_return_t do_listPathsByZone(Character *ch, char *argument, int cmd);
command_return_t do_listproc(Character *ch, char *argument, int cmd);
command_return_t do_load(Character *ch, char *argument, int cmd);
command_return_t do_medit(Character *ch, char *argument, int cmd);
command_return_t do_memoryleak(Character *ch, char *argument, int cmd);
command_return_t do_mortal_set(Character *ch, char *argument, int cmd);
// command_return_t do_motdload (Character *ch, char *argument, int cmd);
command_return_t do_msave(Character *ch, char *argument, int cmd);
command_return_t do_procedit(Character *ch, char *argument, int cmd);
command_return_t do_mpbestow(Character *ch, char *argument, int cmd);
command_return_t do_mpstat(Character *ch, char *argument, int cmd);
command_return_t do_opedit(Character *ch, char *argument, int cmd);
command_return_t do_eqmax(Character *ch, char *argument, int cmd);
command_return_t do_opstat(Character *ch, char *argument, int cmd);
command_return_t do_lock(Character *ch, char *argument, int cmd);
command_return_t do_log(Character *ch, char *argument, int cmd);
command_return_t do_look(Character *ch, char *argument, int cmd);
command_return_t do_botcheck(Character *ch, char *argument, int cmd);
command_return_t do_make_camp(Character *ch, char *argument, int cmd);
command_return_t do_matrixinfo(Character *ch, char *argument, int cmd);
command_return_t do_maxes(Character *ch, char *argument, int cmd);
command_return_t do_mlocate(Character *ch, char *argument, int cmd);
command_return_t do_move(Character *ch, char *argument, int cmd);
command_return_t do_motd(Character *ch, char *argument, int cmd);
command_return_t do_mpretval(Character *ch, char *argument, int cmd);
command_return_t do_mpasound(Character *ch, char *argument, int cmd);
command_return_t do_mpat(Character *ch, char *argument, int cmd);
command_return_t do_mpdamage(Character *ch, char *argument, int cmd);
command_return_t do_mpecho(Character *ch, char *argument, int cmd);
command_return_t do_mpechoaround(Character *ch, char *argument, int cmd);
command_return_t do_mpechoaroundnotbad(Character *ch, char *argument, int cmd);
command_return_t do_mpechoat(Character *ch, char *argument, int cmd);
command_return_t do_mpforce(Character *ch, char *argument, int cmd);
command_return_t do_mpgoto(Character *ch, char *argument, int cmd);
command_return_t do_mpjunk(Character *ch, char *argument, int cmd);
command_return_t do_mpkill(Character *ch, char *argument, int cmd);
command_return_t do_mphit(Character *ch, char *argument, int cmd);
command_return_t do_mpsetmath(Character *ch, char *argument, int cmd);
command_return_t do_mpaddlag(Character *ch, char *argument, int cmd);
command_return_t do_mpmload(Character *ch, char *argument, int cmd);
command_return_t do_mpoload(Character *ch, char *argument, int cmd);
command_return_t do_mppause(Character *ch, char *argument, int cmd);
command_return_t do_mppeace(Character *ch, char *argument, int cmd);
command_return_t do_mppurge(Character *ch, char *argument, int cmd);
command_return_t do_mpteachskill(Character *ch, char *argument, int cmd);
command_return_t do_mpsetalign(Character *ch, char *argument, int cmd);
command_return_t do_mpsettemp(Character *ch, char *argument, int cmd);
command_return_t do_mpthrow(Character *ch, char *argument, int cmd);
command_return_t do_mpothrow(Character *ch, char *argument, int cmd);
command_return_t do_mptransfer(Character *ch, char *argument, int cmd);
command_return_t do_mpxpreward(Character *ch, char *argument, int cmd);
command_return_t do_mpteleport(Character *ch, char *argument, int cmd);
command_return_t do_murder(Character *ch, char *argument, int cmd);
command_return_t do_name(Character *ch, char *argument, int cmd);
command_return_t do_natural_selection(Character *ch, char *argument, int cmd);
command_return_t do_newbie(Character *ch, char *argument, int cmd);
command_return_t do_newPath(Character *ch, char *argument, int cmd);
command_return_t do_news(Character *ch, char *argument, int cmd);
command_return_t do_noemote(Character *ch, char *argument, int cmd);
command_return_t do_nohassle(Character *ch, char *argument, int cmd);
command_return_t do_noname(Character *ch, char *argument, int cmd);
command_return_t generic_command(Character *ch, char *argument, int cmd);
command_return_t do_notax_toggle(Character *ch, char *argument, int cmd);
command_return_t do_guide_toggle(Character *ch, char *argument, int cmd);
command_return_t do_oclone(Character *ch, char *argument, int cmd);
command_return_t do_mclone(Character *ch, char *argument, int cmd);
command_return_t do_oedit(Character *ch, char *argument, int cmd);
command_return_t do_offer(Character *ch, char *argument, int cmd);
command_return_t do_olocate(Character *ch, char *argument, int cmd);
command_return_t do_oneway(Character *ch, char *argument, int cmd);
command_return_t do_onslaught(Character *ch, char *argument, int cmd);
command_return_t do_open(Character *ch, char *argument, int cmd);
command_return_t do_order(Character *ch, char *argument, int cmd);
command_return_t do_orchestrate(Character *ch, char *argument, int cmd);
command_return_t do_osave(Character *ch, char *argument, int cmd);
command_return_t do_nodupekeys_toggle(Character *ch, char *argument, int cmd);
command_return_t do_outcast(Character *ch, char *argument, int cmd);
command_return_t do_pager(Character *ch, char *argument, int cmd);
command_return_t do_pardon(Character *ch, char *argument, int cmd);
command_return_t do_pathpath(Character *ch, char *argument, int cmd);
command_return_t do_peace(Character *ch, char *argument, int cmd);
command_return_t do_perseverance(Character *ch, char *argument, int cmd);
command_return_t do_pick(Character *ch, char *argument, int cmd);
command_return_t do_plats(Character *ch, char *argument, int cmd);
command_return_t do_pocket(Character *ch, char *argument, int cmd);
command_return_t do_poisonmaking(Character *ch, char *argument, int cmd);
command_return_t do_pour(Character *ch, char *argument, int cmd);
command_return_t do_poof(Character *ch, char *argument, int cmd);
command_return_t do_possess(Character *ch, char *argument, int cmd);
command_return_t do_practice(Character *ch, char *argument, int cmd);
command_return_t do_pray(Character *ch, char *argument, int cmd);
command_return_t do_profession(Character *ch, char *argument, int cmd);
command_return_t do_primalfury(Character *ch, char *argument, int cmd);
command_return_t do_promote(Character *ch, char *argument, int cmd);
command_return_t do_prompt(Character *ch, char *argument, int cmd);
command_return_t do_lastprompt(Character *ch, char *argument, int cmd);
command_return_t do_processes(Character *ch, char *argument, int cmd);
command_return_t do_psay(Character *ch, string argument, int cmd);
// command_return_t do_pshopedit (Character *ch, char *argument, int cmd);
command_return_t do_pview(Character *ch, char *argument, int cmd);
command_return_t do_punish(Character *ch, char *argument, int cmd);
command_return_t do_purge(Character *ch, char *argument, int cmd);
command_return_t do_purloin(Character *ch, char *argument, int cmd);
command_return_t do_put(Character *ch, char *argument, int cmd);
command_return_t do_qedit(Character *ch, char *argument, int cmd);
command_return_t do_quaff(Character *ch, char *argument, int cmd);
command_return_t do_quest(Character *ch, char *argument, int cmd);
command_return_t do_qui(Character *ch, char *argument, int cmd);
command_return_t do_quivering_palm(Character *ch, char *argument, int cmd);
command_return_t do_quit(Character *ch, char *argument, int cmd);
command_return_t do_rage(Character *ch, char *argument, int cmd);
command_return_t do_random(Character *ch, char *argument, int cmd);
command_return_t do_range(Character *ch, char *argument, int cmd);
command_return_t do_rdelete(Character *ch, char *argument, int cmd);
command_return_t do_read(Character *ch, char *argument, int cmd);
command_return_t do_recite(Character *ch, char *argument, int cmd);
command_return_t do_redirect(Character *ch, char *argument, int cmd);
command_return_t do_redit(Character *ch, char *argument, int cmd);
command_return_t do_remove(Character *ch, char *argument, int cmd);
command_return_t do_rename_char(Character *ch, char *argument, int cmd);
command_return_t do_rent(Character *ch, char *argument, int cmd);
command_return_t do_reply(Character *ch, string argument, int cmd);
command_return_t do_repop(Character *ch, string argument, int cmd);
command_return_t do_report(Character *ch, char *argument, int cmd);
command_return_t do_rescue(Character *ch, char *argument, int cmd);
command_return_t do_rest(Character *ch, char *argument, int cmd);
command_return_t do_restore(Character *ch, char *argument, int cmd);
command_return_t do_retreat(Character *ch, char *argument, int cmd);
command_return_t do_return(Character *ch, char *argument, int cmd);
command_return_t do_revoke(Character *ch, char *argument, int cmd);
command_return_t do_rsave(Character *ch, char *argument, int cmd);
command_return_t do_rstat(Character *ch, char *argument, int cmd);
command_return_t do_sacrifice(Character *ch, char *argument, int cmd);
command_return_t do_say(Character *ch, string argument, int cmd = CMD_SAY);
command_return_t do_scan(Character *ch, char *argument, int cmd);
command_return_t do_score(Character *ch, char *argument, int cmd);
command_return_t do_scribe(Character *ch, char *argument, int cmd);
command_return_t do_sector(Character *ch, char *argument, int cmd);
command_return_t do_sedit(Character *ch, char *argument, int cmd);
command_return_t do_send(Character *ch, char *argument, int cmd);
command_return_t do_set(Character *ch, char *argument, int cmd);
command_return_t do_shout(Character *ch, char *argument, int cmd);
command_return_t do_showhunt(Character *ch, char *argument, int cmd);
command_return_t do_skills(Character *ch, char *argument, int cmd);
command_return_t do_social(Character *ch, char *argument, int cmd);
command_return_t do_songs(Character *ch, char *argument, int cmd);
command_return_t do_stromboli(Character *ch, char *argument, int cmd);
command_return_t do_summon_toggle(Character *ch, char *argument, int cmd);
command_return_t do_headbutt(Character *ch, char *argument, int cmd);
command_return_t do_show(Character *ch, char *argument, int cmd);
command_return_t do_showbits(Character *ch, char *argument, int cmd);
command_return_t do_shutdow(Character *ch, char *argument, int cmd);
command_return_t do_shutdown(Character *ch, char *argument, int cmd);
command_return_t do_silence(Character *ch, char *argument, int cmd);
command_return_t do_stupid(Character *ch, char *argument, int cmd);
command_return_t do_sing(Character *ch, char *argument, int cmd);
command_return_t do_sip(Character *ch, char *argument, int cmd);
command_return_t do_sit(Character *ch, char *argument, int cmd);
command_return_t do_slay(Character *ch, char *argument, int cmd);
command_return_t do_sleep(Character *ch, char *argument, int cmd);
command_return_t do_slip(Character *ch, char *argument, int cmd);
command_return_t do_smite(Character *ch, char *argument, int cmd);
command_return_t do_sneak(Character *ch, char *argument, int cmd);
command_return_t do_snoop(Character *ch, char *argument, int cmd);
command_return_t do_spells(Character *ch, char *argument, int cmd);
command_return_t do_sqedit(Character *ch, char *argument, int cmd);
command_return_t do_stalk(Character *ch, char *argument, int cmd);
command_return_t do_stand(Character *ch, char *argument, int cmd);
command_return_t do_stat(Character *ch, char *argument, int cmd);
command_return_t do_steal(Character *ch, char *argument, int cmd);
command_return_t do_stealth(Character *ch, char *argument, int cmd);
command_return_t do_story(Character *ch, char *argument, int cmd);
command_return_t do_string(Character *ch, char *argument, int cmd);
command_return_t do_stun(Character *ch, char *argument, int cmd);
command_return_t do_suicide(Character *ch, char *argument, int cmd);
command_return_t do_switch(Character *ch, char *argument, int cmd);
command_return_t do_tactics(Character *ch, char *argument, int cmd);
command_return_t do_tame(Character *ch, char *argument, int cmd);
command_return_t do_taste(Character *ch, char *argument, int cmd);
command_return_t do_teleport(Character *ch, char *argument, int cmd);
command_return_t do_tell(Character *ch, string argument, int cmd);
command_return_t do_tellhistory(Character *ch, string argument, int cmd);
command_return_t do_testhand(Character *ch, char *argument, int cmd);
command_return_t do_testhit(Character *ch, char *argument, int cmd);
command_return_t do_testport(Character *ch, char *argument, int cmd);
command_return_t do_testuser(Character *ch, char *argument, int cmd);
command_return_t do_thunder(Character *ch, char *argument, int cmd);
command_return_t do_tick(Character *ch, char *argument, int cmd);
command_return_t do_time(Character *ch, char *argument, int cmd);
command_return_t do_title(Character *ch, char *argument, int cmd);
command_return_t do_toggle(Character *ch, char *argument, int cmd);
command_return_t do_track(Character *ch, char *argument, int cmd);
command_return_t do_transfer(Character *ch, string argument, int cmd = CMD_DEFAULT);
command_return_t do_triage(Character *ch, char *argument, int cmd);
command_return_t do_trip(Character *ch, char *argument, int cmd);
command_return_t do_trivia(Character *ch, char *argument, int cmd);
command_return_t do_typo(Character *ch, char *argument, int cmd);
command_return_t do_unarchive(Character *ch, char *argument, int cmd);
command_return_t do_unlock(Character *ch, char *argument, int cmd);
command_return_t do_use(Character *ch, char *argument, int cmd);
command_return_t do_varstat(Character *ch, char *argument, int cmd);
command_return_t do_vault(Character *ch, char *argument, int cmd);
command_return_t do_vend(Character *ch, char *argument, int cmd);
command_return_t do_version(Character *ch, char *argument, int cmd);
command_return_t do_visible(Character *ch, char *argument, int cmd);
command_return_t do_vitalstrike(Character *ch, char *argument, int cmd);
command_return_t do_vt100(Character *ch, char *argument, int cmd);
command_return_t do_wake(Character *ch, char *argument, int cmd);
command_return_t do_wear(Character *ch, char *argument, int cmd);
command_return_t do_weather(Character *ch, char *argument, int cmd);
command_return_t do_where(Character *ch, char *argument, int cmd);
command_return_t do_whisper(Character *ch, char *argument, int cmd);
command_return_t do_who(Character *ch, char *argument, int cmd);
command_return_t do_whoarena(Character *ch, char *argument, int cmd);
command_return_t do_whoclan(Character *ch, char *argument, int cmd);
command_return_t do_whogroup(Character *ch, char *argument, int cmd);
command_return_t do_whosolo(Character *ch, char *argument, int cmd);
command_return_t do_wield(Character *ch, char *argument, int cmd);
command_return_t do_fakelog(Character *ch, char *argument, int cmd);
command_return_t do_wimpy(Character *ch, char *argument, int cmd);
command_return_t do_wiz(Character *ch, string argument, int cmd);
command_return_t do_wizhelp(Character *ch, char *argument, int cmd);
command_return_t do_wizinvis(Character *ch, char *argument, int cmd);
command_return_t do_wizlist(Character *ch, char *argument, int cmd);
command_return_t do_wizlock(Character *ch, char *argument, int cmd);
command_return_t do_world(Character *ch, string args, int cmd);
command_return_t do_write_skillquest(Character *ch, char *argument, int cmd);
command_return_t do_write(Character *ch, char *argument, int cmd);
command_return_t do_zap(Character *ch, char *argument, int cmd);
command_return_t do_zedit(Character *ch, char *argument, int cmd);
command_return_t do_zoneexits(Character *ch, char *argument, int cmd);
command_return_t do_editor(Character *ch, char *argument, int cmd);
command_return_t do_pursue(Character *ch, char *argument, int cmd);

#endif
