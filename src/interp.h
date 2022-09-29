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

char *remove_trailing_spaces(char *arg);
int command_interpreter(CHAR_DATA *ch, char *argument, bool procced = 0);
int search_block(const char *arg, const char **l, bool exact);
int old_search_block(const char *argument, int begin, int length, const char **list, int mode);
char lower(char c);
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
int len_cmp(char *s1, char *s2);
void add_command_lag(CHAR_DATA *ch, int cmdnum, int lag);
string ltrim(string str);
string rtrim(string str);

#define CMD_NORTH 1
#define CMD_EAST 2
#define CMD_SOUTH 3
#define CMD_WEST 4
#define CMD_UP 5
#define CMD_DOWN 6

#define CMD_BELLOW 8
#define CMD_DEFAULT 9
#define CMD_TRACK 10
#define CMD_PALM 10
#define CMD_SAY 11
#define CMD_LOOK 12
#define CMD_BACKSTAB 13
#define CMD_SBS 14
#define CMD_ORCHESTRATE 15
#define CMD_REPLY 16
#define CMD_WHISPER 17
#define CMD_GLANCE 20
#define CMD_FLEE 28
#define CMD_ESCAPE 29
#define CMD_PICK 35
#define CMD_STOCK 56
#define CMD_BUY 56
#define CMD_SELL 57
#define CMD_VALUE 58
#define CMD_LIST 59
#define CMD_ENTER 60
#define CMD_CLIMB 60
#define CMD_DESIGN 62
#define CMD_PRICE 65
#define CMD_REPAIR 66
#define CMD_READ 67
#define CMD_REMOVE 69
#define CMD_ERASE 70
#define CMD_ESTIMATE 71
#define CMD_REMORT 80
#define CMD_REROLL 81
#define CMD_CHOOSE 82
#define CMD_CONFIRM 83
#define CMD_CANCEL 84
#define CMD_SLIP 87
#define CMD_GIVE 88
#define CMD_DROP 89
#define CMD_DONATE 90
#define CMD_QUIT 91
#define CMD_SACRIFICE 92
#define CMD_PUT 93
#define CMD_OPEN 98
#define CMD_EDITOR 100
#define CMD_FORCE 123
#define CMD_WRITE 128
#define CMD_WATCH 155
#define CMD_PRACTICE 164
#define CMD_TRAIN 165
#define CMD_PROFESSION 166
#define CMD_GAIN 171
#define CMD_BALANCE 172
#define CMD_DEPOSIT 173
#define CMD_WITHDRAW 174
#define CMD_CLEAN 177
#define CMD_PLAY 178
#define CMD_FINISH 179
#define CMD_VETERNARIAN 180
#define CMD_FEED 181
#define CMD_ASSEMBLE 182
#define CMD_PAY 183
#define CMD_RESTRING 184
#define CMD_PUSH 185
#define CMD_PULL 186
#define CMD_LEAVE 187
#define CMD_TREMOR 188
#define CMD_BET 189
#define CMD_INSURANCE 190
#define CMD_DOUBLE 191
#define CMD_STAY 192
#define CMD_SPLIT 193
#define CMD_HIT 194
#define CMD_LOOT 195
#define CMD_GTELL 200
#define CMD_CTELL 201
#define CMD_SETVOTE 202
#define CMD_VOTE 203
#define CMD_VEND 204
#define CMD_FILTER 205
#define CMD_EXAMINE 206
#define CMD_GAG 207
#define CMD_IMMORT 208
#define CMD_IMPCHAN 209
#define CMD_TELL 210
#define CMD_TELLH 211
#define CMD_PRIZE 999
#define CMD_GAZE 1820

// Temp removal to perfect system. 1/25/06 Eas
// WARNING WARNING WARNING WARNING WARNING
// The command list was modified to account for toggle_hide.
// The last integer will affect a char being removed from hide when they perform the command.

/*
 * Command functions.
 */
typedef int DO_FUN(CHAR_DATA *ch, char *argument, int cmd);
typedef int command_return_t;

struct command_info
{
    char *command_name;                                                       /* Name of ths command             */
    int (*command_pointer)(CHAR_DATA *ch, char *argument, int cmd);           /* Function that does it            */
    command_return_t (*command_pointer2)(char_data *ch, string argument, int cmd); /* Function that does it            */
    ubyte minimum_position;                                                   /* Position commander must be in    */
    ubyte minimum_level;                                                      /* Minimum level needed             */
    int command_number;                                                       /* Passed to function as argument   */
    int flags;                                                                // what flags the skills has
    ubyte toggle_hide;
};

struct command_lag
{
    struct command_lag *next;
    CHAR_DATA *ch;
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

command_return_t do_mscore(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_clanarea(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_huntstart(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_huntclear(CHAR_DATA *ch, char *argument, int cmd);

command_return_t do_metastat(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_findfix(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_reload(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_abandon(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_accept(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_acfinder(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_action(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_addnews(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_addRoom(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_advance(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_areastats(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_awaymsgs(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_guide(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_alias(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_archive(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_autoeat(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_autojoin(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_unban(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_ambush(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_anonymous(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_ansi(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_appraise(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_assemble(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_release(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_jab(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_areas(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_arena(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_ask(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_at(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_auction(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_backstab(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_ban(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_bandwidth(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_bard_song_toggle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_bash(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_batter(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_battlecry(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_battlesense(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_beacon(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_beep(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_beep_set(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_behead(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_berserk(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_bestow(CHAR_DATA *ch, string argument, int cmd);
command_return_t do_bladeshield(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_bloodfury(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_boot(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_boss(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_brace(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_brief(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_brew(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_news_toggle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_ascii_toggle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_damage_toggle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_bug(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_bullrush(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_cast(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_channel(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_charmiejoin_toggle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_check(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_chpwd(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_cinfo(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_circle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_clans(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_clear(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_clearaff(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_climb(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_close(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_cmotd(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_ctax(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_cdeposit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_cwithdraw(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_cbalance(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_colors(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_compact(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_config(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_consent(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_consider(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_count(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_cpromote(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_crazedassault(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_credits(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_cripple(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_ctell(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_deathstroke(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_debug(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_deceit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_defenders_stance(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_disarm(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_disband(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_disconnect(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_dmg_eq(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_donate(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_dream(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_drink(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_drop(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_eat(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_eagle_claw(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_echo(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_emote(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_setvote(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_vote(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_enter(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_equipment(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_eyegouge(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_examine(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_exits(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_export(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_ferocity(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_fighting(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_fill(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_find(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_findpath(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_findPath(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_fire(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_flee(CHAR_DATA *ch, char *argument, int cmd);
/* DO_FUN  do_fly; */
command_return_t do_focused_repelance(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_follow(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_forage(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_force(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_found(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_free_animal(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_freeze(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_fsave(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_get(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_give(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_global(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_gossip(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_golem_score(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_goto(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_guild(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_install(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_reload_help(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_hindex(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_hedit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_grab(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_group(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_grouptell(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_gtrans(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_guard(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_harmtouch(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_help(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mortal_help(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_new_help(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_hide(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_highfive(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_hit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_hitall(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_holylite(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_home(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_idea(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_identify(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_ignore(char_data *ch, string argument, int cmd);
command_return_t do_imotd(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_imbue(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_incognito(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_index(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_info(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_initiate(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_innate(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_instazone(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_insult(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_inventory(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_join(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_joinarena(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_ki(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_kick(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_kill(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_knockback(CHAR_DATA *ch, char *argument, int cmd);
// command_return_t do_land (CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_layhands(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_leaderboard(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_leadership(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_leave(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_levels(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_lfg_toggle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_linkdead(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_linkload(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_listAllPaths(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_listPathsByZone(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_listproc(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_load(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_medit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_memoryleak(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mortal_set(CHAR_DATA *ch, char *argument, int cmd);
// command_return_t do_motdload (CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_msave(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_procedit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpbestow(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpstat(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_opedit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_eqmax(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_opstat(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_lock(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_log(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_look(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_botcheck(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_make_camp(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_matrixinfo(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_maxes(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mlocate(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_move(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_motd(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpretval(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpasound(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpat(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpdamage(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpecho(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpechoaround(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpechoaroundnotbad(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpechoat(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpforce(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpgoto(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpjunk(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpkill(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mphit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpsetmath(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpaddlag(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpmload(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpoload(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mppause(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mppeace(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mppurge(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpteachskill(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpsetalign(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpsettemp(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpthrow(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpothrow(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mptransfer(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpxpreward(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mpteleport(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_murder(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_name(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_natural_selection(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_newbie(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_newPath(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_news(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_noemote(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_nohassle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_noname(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_not_here(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_notax_toggle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_guide_toggle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_oclone(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_mclone(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_oedit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_offer(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_olocate(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_oneway(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_onslaught(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_open(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_order(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_orchestrate(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_osave(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_nodupekeys_toggle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_outcast(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_pager(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_pardon(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_pathpath(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_peace(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_perseverance(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_pick(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_plats(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_pocket(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_poisonmaking(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_pour(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_poof(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_possess(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_practice(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_pray(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_profession(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_primalfury(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_promote(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_prompt(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_lastprompt(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_processes(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_psay(char_data *ch, string argument, int cmd);
// command_return_t do_pshopedit (CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_pview(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_punish(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_purge(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_purloin(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_put(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_qedit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_quaff(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_quest(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_qui(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_quivering_palm(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_quit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_rage(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_random(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_range(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_rdelete(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_read(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_recall(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_recite(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_redirect(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_redit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_remove(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_rename_char(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_rent(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_reply(char_data *ch, string argument, int cmd);
command_return_t do_repop(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_report(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_rescue(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_rest(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_restore(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_retreat(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_return(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_revoke(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_rsave(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_rstat(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_sacrifice(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_save(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_say(char_data *ch, string argument, int cmd = CMD_SAY);
command_return_t do_scan(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_score(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_scribe(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_sector(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_sedit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_send(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_set(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_search(char_data *ch, char *argument, int cmd);
command_return_t do_shout(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_showhunt(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_skills(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_social(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_songs(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_stromboli(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_summon_toggle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_headbutt(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_show(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_show_exp(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_showbits(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_shutdow(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_shutdown(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_silence(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_stupid(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_sing(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_sip(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_sit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_slay(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_sleep(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_slip(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_smite(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_sneak(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_snoop(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_sockets(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_spells(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_split(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_sqedit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_stalk(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_stand(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_stat(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_steal(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_stealth(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_story(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_string(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_stun(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_suicide(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_switch(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_tactics(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_tame(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_taste(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_teleport(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_tell(char_data *ch, string argument, int cmd);
command_return_t do_tellhistory(char_data *ch, string argument, int cmd);
command_return_t do_testhand(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_testhit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_testport(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_testuser(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_thunder(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_tick(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_time(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_title(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_toggle(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_track(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_trans(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_triage(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_trip(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_trivia(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_typo(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_unarchive(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_unlock(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_use(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_varstat(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_vault(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_vend(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_version(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_visible(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_vitalstrike(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_vt100(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_wake(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_wear(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_weather(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_where(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_whisper(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_who(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_whoarena(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_whoclan(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_whogroup(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_whosolo(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_wield(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_fakelog(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_wimpy(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_wiz(char_data *ch, string argument, int cmd);
command_return_t do_wizhelp(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_wizinvis(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_wizlist(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_wizlock(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_world(char_data* ch, string args, int cmd);
command_return_t do_write_skillquest(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_write(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_zap(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_zedit(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_zoneexits(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_zsave(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_editor(CHAR_DATA *ch, char *argument, int cmd);
command_return_t do_pursue(CHAR_DATA *ch, char *argument, int cmd);

#endif
