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
int command_interpreter(struct char_data *ch, char *argument, bool procced = 0);
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
void add_command_lag(struct char_data *ch, int cmdnum, int lag);
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
typedef int DO_FUN(struct char_data *ch, char *argument, int cmd);
typedef int command_return_t;

struct command_info
{
    char *command_name;                                                       /* Name of ths command             */
    int (*command_pointer)(struct char_data *ch, char *argument, int cmd);           /* Function that does it            */
    command_return_t (*command_pointer2)(struct char_data *ch, string argument, int cmd); /* Function that does it            */
    uint8_t minimum_position;                                                   /* Position commander must be in    */
    uint8_t minimum_level;                                                      /* Minimum level needed             */
    int command_number;                                                       /* Passed to function as argument   */
    int flags;                                                                // what flags the skills has
    uint8_t toggle_hide;
};

struct command_lag
{
    struct command_lag *next;
    struct char_data *ch;
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

command_return_t do_mscore(struct char_data *ch, char *argument, int cmd);
command_return_t do_clanarea(struct char_data *ch, char *argument, int cmd);
command_return_t do_huntstart(struct char_data *ch, char *argument, int cmd);
command_return_t do_huntclear(struct char_data *ch, char *argument, int cmd);

command_return_t do_metastat(struct char_data *ch, char *argument, int cmd);
command_return_t do_findfix(struct char_data *ch, char *argument, int cmd);
command_return_t do_reload(struct char_data *ch, char *argument, int cmd);
command_return_t do_abandon(struct char_data *ch, char *argument, int cmd);
command_return_t do_accept(struct char_data *ch, char *argument, int cmd);
command_return_t do_acfinder(struct char_data *ch, char *argument, int cmd);
command_return_t do_action(struct char_data *ch, char *argument, int cmd);
command_return_t do_addnews(struct char_data *ch, char *argument, int cmd);
command_return_t do_addRoom(struct char_data *ch, char *argument, int cmd);
command_return_t do_advance(struct char_data *ch, char *argument, int cmd);
command_return_t do_areastats(struct char_data *ch, char *argument, int cmd);
command_return_t do_awaymsgs(struct char_data *ch, char *argument, int cmd);
command_return_t do_guide(struct char_data *ch, char *argument, int cmd);
command_return_t do_alias(struct char_data *ch, char *argument, int cmd);
command_return_t do_archive(struct char_data *ch, char *argument, int cmd);
command_return_t do_autoeat(struct char_data *ch, char *argument, int cmd);
command_return_t do_autojoin(struct char_data *ch, char *argument, int cmd);
command_return_t do_unban(struct char_data *ch, char *argument, int cmd);
command_return_t do_ambush(struct char_data *ch, char *argument, int cmd);
command_return_t do_anonymous(struct char_data *ch, char *argument, int cmd);
command_return_t do_ansi(struct char_data *ch, char *argument, int cmd);
command_return_t do_appraise(struct char_data *ch, char *argument, int cmd);
command_return_t do_assemble(struct char_data *ch, char *argument, int cmd);
command_return_t do_release(struct char_data *ch, char *argument, int cmd);
command_return_t do_jab(struct char_data *ch, char *argument, int cmd);
command_return_t do_areas(struct char_data *ch, char *argument, int cmd);
command_return_t do_arena(struct char_data *ch, char *argument, int cmd);
command_return_t do_ask(struct char_data *ch, char *argument, int cmd);
command_return_t do_at(struct char_data *ch, char *argument, int cmd);
command_return_t do_auction(struct char_data *ch, char *argument, int cmd);
command_return_t do_backstab(struct char_data *ch, char *argument, int cmd);
command_return_t do_ban(struct char_data *ch, char *argument, int cmd);
command_return_t do_bandwidth(struct char_data *ch, char *argument, int cmd);
command_return_t do_bard_song_toggle(struct char_data *ch, char *argument, int cmd);
command_return_t do_bash(struct char_data *ch, char *argument, int cmd);
command_return_t do_batter(struct char_data *ch, char *argument, int cmd);
command_return_t do_battlecry(struct char_data *ch, char *argument, int cmd);
command_return_t do_battlesense(struct char_data *ch, char *argument, int cmd);
command_return_t do_beacon(struct char_data *ch, char *argument, int cmd);
command_return_t do_beep(struct char_data *ch, char *argument, int cmd);
command_return_t do_beep_set(struct char_data *ch, char *argument, int cmd);
command_return_t do_behead(struct char_data *ch, char *argument, int cmd);
command_return_t do_berserk(struct char_data *ch, char *argument, int cmd);
command_return_t do_bestow(struct char_data *ch, string argument, int cmd);
command_return_t do_bladeshield(struct char_data *ch, char *argument, int cmd);
command_return_t do_bloodfury(struct char_data *ch, char *argument, int cmd);
command_return_t do_boot(struct char_data *ch, char *argument, int cmd);
command_return_t do_boss(struct char_data *ch, char *argument, int cmd);
command_return_t do_brace(struct char_data *ch, char *argument, int cmd);
command_return_t do_brief(struct char_data *ch, char *argument, int cmd);
command_return_t do_brew(struct char_data *ch, char *argument, int cmd);
command_return_t do_news_toggle(struct char_data *ch, char *argument, int cmd);
command_return_t do_ascii_toggle(struct char_data *ch, char *argument, int cmd);
command_return_t do_damage_toggle(struct char_data *ch, char *argument, int cmd);
command_return_t do_bug(struct char_data *ch, char *argument, int cmd);
command_return_t do_bullrush(struct char_data *ch, char *argument, int cmd);
command_return_t do_cast(struct char_data *ch, char *argument, int cmd);
command_return_t do_channel(struct char_data *ch, char *argument, int cmd);
command_return_t do_charmiejoin_toggle(struct char_data *ch, char *argument, int cmd);
command_return_t do_check(struct char_data *ch, char *argument, int cmd);
command_return_t do_chpwd(struct char_data *ch, char *argument, int cmd);
command_return_t do_cinfo(struct char_data *ch, char *argument, int cmd);
command_return_t do_circle(struct char_data *ch, char *argument, int cmd);
command_return_t do_clans(struct char_data *ch, char *argument, int cmd);
command_return_t do_clear(struct char_data *ch, char *argument, int cmd);
command_return_t do_clearaff(struct char_data *ch, char *argument, int cmd);
command_return_t do_climb(struct char_data *ch, char *argument, int cmd);
command_return_t do_close(struct char_data *ch, char *argument, int cmd);
command_return_t do_cmotd(struct char_data *ch, char *argument, int cmd);
command_return_t do_ctax(struct char_data *ch, char *argument, int cmd);
command_return_t do_cdeposit(struct char_data *ch, char *argument, int cmd);
command_return_t do_cwithdraw(struct char_data *ch, char *argument, int cmd);
command_return_t do_cbalance(struct char_data *ch, char *argument, int cmd);
command_return_t do_colors(struct char_data *ch, char *argument, int cmd);
command_return_t do_compact(struct char_data *ch, char *argument, int cmd);
command_return_t do_config(struct char_data *ch, char *argument, int cmd);
command_return_t do_consent(struct char_data *ch, char *argument, int cmd);
command_return_t do_consider(struct char_data *ch, char *argument, int cmd);
command_return_t do_count(struct char_data *ch, char *argument, int cmd);
command_return_t do_cpromote(struct char_data *ch, char *argument, int cmd);
command_return_t do_crazedassault(struct char_data *ch, char *argument, int cmd);
command_return_t do_credits(struct char_data *ch, char *argument, int cmd);
command_return_t do_cripple(struct char_data *ch, char *argument, int cmd);
command_return_t do_ctell(struct char_data *ch, char *argument, int cmd);
command_return_t do_deathstroke(struct char_data *ch, char *argument, int cmd);
command_return_t do_debug(struct char_data *ch, char *argument, int cmd);
command_return_t do_deceit(struct char_data *ch, char *argument, int cmd);
command_return_t do_defenders_stance(struct char_data *ch, char *argument, int cmd);
command_return_t do_disarm(struct char_data *ch, char *argument, int cmd);
command_return_t do_disband(struct char_data *ch, char *argument, int cmd);
command_return_t do_disconnect(struct char_data *ch, char *argument, int cmd);
command_return_t do_dmg_eq(struct char_data *ch, char *argument, int cmd);
command_return_t do_donate(struct char_data *ch, char *argument, int cmd);
command_return_t do_dream(struct char_data *ch, char *argument, int cmd);
command_return_t do_drink(struct char_data *ch, char *argument, int cmd);
command_return_t do_drop(struct char_data *ch, char *argument, int cmd);
command_return_t do_eat(struct char_data *ch, char *argument, int cmd);
command_return_t do_eagle_claw(struct char_data *ch, char *argument, int cmd);
command_return_t do_echo(struct char_data *ch, char *argument, int cmd);
command_return_t do_emote(struct char_data *ch, char *argument, int cmd);
command_return_t do_setvote(struct char_data *ch, char *argument, int cmd);
command_return_t do_vote(struct char_data *ch, char *argument, int cmd);
command_return_t do_enter(struct char_data *ch, char *argument, int cmd);
command_return_t do_equipment(struct char_data *ch, char *argument, int cmd);
command_return_t do_eyegouge(struct char_data *ch, char *argument, int cmd);
command_return_t do_examine(struct char_data *ch, char *argument, int cmd);
command_return_t do_exits(struct char_data *ch, char *argument, int cmd);
command_return_t do_export(struct char_data *ch, char *argument, int cmd);
command_return_t do_ferocity(struct char_data *ch, char *argument, int cmd);
command_return_t do_fighting(struct char_data *ch, char *argument, int cmd);
command_return_t do_fill(struct char_data *ch, char *argument, int cmd);
command_return_t do_find(struct char_data *ch, char *argument, int cmd);
command_return_t do_findpath(struct char_data *ch, char *argument, int cmd);
command_return_t do_findPath(struct char_data *ch, char *argument, int cmd);
command_return_t do_fire(struct char_data *ch, char *argument, int cmd);
command_return_t do_flee(struct char_data *ch, char *argument, int cmd);
/* DO_FUN  do_fly; */
command_return_t do_focused_repelance(struct char_data *ch, char *argument, int cmd);
command_return_t do_follow(struct char_data *ch, char *argument, int cmd);
command_return_t do_forage(struct char_data *ch, char *argument, int cmd);
command_return_t do_force(struct char_data *ch, char *argument, int cmd);
command_return_t do_found(struct char_data *ch, char *argument, int cmd);
command_return_t do_free_animal(struct char_data *ch, char *argument, int cmd);
command_return_t do_freeze(struct char_data *ch, char *argument, int cmd);
command_return_t do_fsave(struct char_data *ch, char *argument, int cmd);
command_return_t do_get(struct char_data *ch, char *argument, int cmd);
command_return_t do_give(struct char_data *ch, char *argument, int cmd);
command_return_t do_global(struct char_data *ch, char *argument, int cmd);
command_return_t do_gossip(struct char_data *ch, char *argument, int cmd);
command_return_t do_golem_score(struct char_data *ch, char *argument, int cmd);
command_return_t do_goto(struct char_data *ch, char *argument, int cmd);
command_return_t do_guild(struct char_data *ch, char *argument, int cmd);
command_return_t do_install(struct char_data *ch, char *argument, int cmd);
command_return_t do_reload_help(struct char_data *ch, char *argument, int cmd);
command_return_t do_hindex(struct char_data *ch, char *argument, int cmd);
command_return_t do_hedit(struct char_data *ch, char *argument, int cmd);
command_return_t do_grab(struct char_data *ch, char *argument, int cmd);
command_return_t do_group(struct char_data *ch, char *argument, int cmd);
command_return_t do_grouptell(struct char_data *ch, char *argument, int cmd);
command_return_t do_gtrans(struct char_data *ch, char *argument, int cmd);
command_return_t do_guard(struct char_data *ch, char *argument, int cmd);
command_return_t do_harmtouch(struct char_data *ch, char *argument, int cmd);
command_return_t do_help(struct char_data *ch, char *argument, int cmd);
command_return_t do_mortal_help(struct char_data *ch, char *argument, int cmd);
command_return_t do_new_help(struct char_data *ch, char *argument, int cmd);
command_return_t do_hide(struct char_data *ch, char *argument, int cmd);
command_return_t do_highfive(struct char_data *ch, char *argument, int cmd);
command_return_t do_hit(struct char_data *ch, char *argument, int cmd);
command_return_t do_hitall(struct char_data *ch, char *argument, int cmd);
command_return_t do_holylite(struct char_data *ch, char *argument, int cmd);
command_return_t do_home(struct char_data *ch, char *argument, int cmd);
command_return_t do_idea(struct char_data *ch, char *argument, int cmd);
command_return_t do_identify(struct char_data *ch, char *argument, int cmd);
command_return_t do_ignore(struct char_data *ch, string argument, int cmd);
command_return_t do_imotd(struct char_data *ch, char *argument, int cmd);
command_return_t do_imbue(struct char_data *ch, char *argument, int cmd);
command_return_t do_incognito(struct char_data *ch, char *argument, int cmd);
command_return_t do_index(struct char_data *ch, char *argument, int cmd);
command_return_t do_info(struct char_data *ch, char *argument, int cmd);
command_return_t do_initiate(struct char_data *ch, char *argument, int cmd);
command_return_t do_innate(struct char_data *ch, char *argument, int cmd);
command_return_t do_instazone(struct char_data *ch, char *argument, int cmd);
command_return_t do_insult(struct char_data *ch, char *argument, int cmd);
command_return_t do_inventory(struct char_data *ch, char *argument, int cmd);
command_return_t do_join(struct char_data *ch, char *argument, int cmd);
command_return_t do_joinarena(struct char_data *ch, char *argument, int cmd);
command_return_t do_ki(struct char_data *ch, char *argument, int cmd);
command_return_t do_kick(struct char_data *ch, char *argument, int cmd);
command_return_t do_kill(struct char_data *ch, char *argument, int cmd);
command_return_t do_knockback(struct char_data *ch, char *argument, int cmd);
// command_return_t do_land (struct char_data *ch, char *argument, int cmd);
command_return_t do_layhands(struct char_data *ch, char *argument, int cmd);
command_return_t do_leaderboard(struct char_data *ch, char *argument, int cmd);
command_return_t do_leadership(struct char_data *ch, char *argument, int cmd);
command_return_t do_leave(struct char_data *ch, char *argument, int cmd);
command_return_t do_levels(struct char_data *ch, char *argument, int cmd);
command_return_t do_lfg_toggle(struct char_data *ch, char *argument, int cmd);
command_return_t do_linkdead(struct char_data *ch, char *argument, int cmd);
command_return_t do_linkload(struct char_data *ch, char *argument, int cmd);
command_return_t do_listAllPaths(struct char_data *ch, char *argument, int cmd);
command_return_t do_listPathsByZone(struct char_data *ch, char *argument, int cmd);
command_return_t do_listproc(struct char_data *ch, char *argument, int cmd);
command_return_t do_load(struct char_data *ch, char *argument, int cmd);
command_return_t do_medit(struct char_data *ch, char *argument, int cmd);
command_return_t do_memoryleak(struct char_data *ch, char *argument, int cmd);
command_return_t do_mortal_set(struct char_data *ch, char *argument, int cmd);
// command_return_t do_motdload (struct char_data *ch, char *argument, int cmd);
command_return_t do_msave(struct char_data *ch, char *argument, int cmd);
command_return_t do_procedit(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpbestow(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpstat(struct char_data *ch, char *argument, int cmd);
command_return_t do_opedit(struct char_data *ch, char *argument, int cmd);
command_return_t do_eqmax(struct char_data *ch, char *argument, int cmd);
command_return_t do_opstat(struct char_data *ch, char *argument, int cmd);
command_return_t do_lock(struct char_data *ch, char *argument, int cmd);
command_return_t do_log(struct char_data *ch, char *argument, int cmd);
command_return_t do_look(struct char_data *ch, char *argument, int cmd);
command_return_t do_botcheck(struct char_data *ch, char *argument, int cmd);
command_return_t do_make_camp(struct char_data *ch, char *argument, int cmd);
command_return_t do_matrixinfo(struct char_data *ch, char *argument, int cmd);
command_return_t do_maxes(struct char_data *ch, char *argument, int cmd);
command_return_t do_mlocate(struct char_data *ch, char *argument, int cmd);
command_return_t do_move(struct char_data *ch, char *argument, int cmd);
command_return_t do_motd(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpretval(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpasound(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpat(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpdamage(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpecho(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpechoaround(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpechoaroundnotbad(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpechoat(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpforce(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpgoto(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpjunk(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpkill(struct char_data *ch, char *argument, int cmd);
command_return_t do_mphit(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpsetmath(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpaddlag(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpmload(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpoload(struct char_data *ch, char *argument, int cmd);
command_return_t do_mppause(struct char_data *ch, char *argument, int cmd);
command_return_t do_mppeace(struct char_data *ch, char *argument, int cmd);
command_return_t do_mppurge(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpteachskill(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpsetalign(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpsettemp(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpthrow(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpothrow(struct char_data *ch, char *argument, int cmd);
command_return_t do_mptransfer(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpxpreward(struct char_data *ch, char *argument, int cmd);
command_return_t do_mpteleport(struct char_data *ch, char *argument, int cmd);
command_return_t do_murder(struct char_data *ch, char *argument, int cmd);
command_return_t do_name(struct char_data *ch, char *argument, int cmd);
command_return_t do_natural_selection(struct char_data *ch, char *argument, int cmd);
command_return_t do_newbie(struct char_data *ch, char *argument, int cmd);
command_return_t do_newPath(struct char_data *ch, char *argument, int cmd);
command_return_t do_news(struct char_data *ch, char *argument, int cmd);
command_return_t do_noemote(struct char_data *ch, char *argument, int cmd);
command_return_t do_nohassle(struct char_data *ch, char *argument, int cmd);
command_return_t do_noname(struct char_data *ch, char *argument, int cmd);
command_return_t do_not_here(struct char_data *ch, char *argument, int cmd);
command_return_t do_notax_toggle(struct char_data *ch, char *argument, int cmd);
command_return_t do_guide_toggle(struct char_data *ch, char *argument, int cmd);
command_return_t do_oclone(struct char_data *ch, char *argument, int cmd);
command_return_t do_mclone(struct char_data *ch, char *argument, int cmd);
command_return_t do_oedit(struct char_data *ch, char *argument, int cmd);
command_return_t do_offer(struct char_data *ch, char *argument, int cmd);
command_return_t do_olocate(struct char_data *ch, char *argument, int cmd);
command_return_t do_oneway(struct char_data *ch, char *argument, int cmd);
command_return_t do_onslaught(struct char_data *ch, char *argument, int cmd);
command_return_t do_open(struct char_data *ch, char *argument, int cmd);
command_return_t do_order(struct char_data *ch, char *argument, int cmd);
command_return_t do_orchestrate(struct char_data *ch, char *argument, int cmd);
command_return_t do_osave(struct char_data *ch, char *argument, int cmd);
command_return_t do_nodupekeys_toggle(struct char_data *ch, char *argument, int cmd);
command_return_t do_outcast(struct char_data *ch, char *argument, int cmd);
command_return_t do_pager(struct char_data *ch, char *argument, int cmd);
command_return_t do_pardon(struct char_data *ch, char *argument, int cmd);
command_return_t do_pathpath(struct char_data *ch, char *argument, int cmd);
command_return_t do_peace(struct char_data *ch, char *argument, int cmd);
command_return_t do_perseverance(struct char_data *ch, char *argument, int cmd);
command_return_t do_pick(struct char_data *ch, char *argument, int cmd);
command_return_t do_plats(struct char_data *ch, char *argument, int cmd);
command_return_t do_pocket(struct char_data *ch, char *argument, int cmd);
command_return_t do_poisonmaking(struct char_data *ch, char *argument, int cmd);
command_return_t do_pour(struct char_data *ch, char *argument, int cmd);
command_return_t do_poof(struct char_data *ch, char *argument, int cmd);
command_return_t do_possess(struct char_data *ch, char *argument, int cmd);
command_return_t do_practice(struct char_data *ch, char *argument, int cmd);
command_return_t do_pray(struct char_data *ch, char *argument, int cmd);
command_return_t do_profession(struct char_data *ch, char *argument, int cmd);
command_return_t do_primalfury(struct char_data *ch, char *argument, int cmd);
command_return_t do_promote(struct char_data *ch, char *argument, int cmd);
command_return_t do_prompt(struct char_data *ch, char *argument, int cmd);
command_return_t do_lastprompt(struct char_data *ch, char *argument, int cmd);
command_return_t do_processes(struct char_data *ch, char *argument, int cmd);
command_return_t do_psay(struct char_data *ch, string argument, int cmd);
// command_return_t do_pshopedit (struct char_data *ch, char *argument, int cmd);
command_return_t do_pview(struct char_data *ch, char *argument, int cmd);
command_return_t do_punish(struct char_data *ch, char *argument, int cmd);
command_return_t do_purge(struct char_data *ch, char *argument, int cmd);
command_return_t do_purloin(struct char_data *ch, char *argument, int cmd);
command_return_t do_put(struct char_data *ch, char *argument, int cmd);
command_return_t do_qedit(struct char_data *ch, char *argument, int cmd);
command_return_t do_quaff(struct char_data *ch, char *argument, int cmd);
command_return_t do_quest(struct char_data *ch, char *argument, int cmd);
command_return_t do_qui(struct char_data *ch, char *argument, int cmd);
command_return_t do_quivering_palm(struct char_data *ch, char *argument, int cmd);
command_return_t do_quit(struct char_data *ch, char *argument, int cmd);
command_return_t do_rage(struct char_data *ch, char *argument, int cmd);
command_return_t do_random(struct char_data *ch, char *argument, int cmd);
command_return_t do_range(struct char_data *ch, char *argument, int cmd);
command_return_t do_rdelete(struct char_data *ch, char *argument, int cmd);
command_return_t do_read(struct char_data *ch, char *argument, int cmd);
command_return_t do_recall(struct char_data *ch, char *argument, int cmd);
command_return_t do_recite(struct char_data *ch, char *argument, int cmd);
command_return_t do_redirect(struct char_data *ch, char *argument, int cmd);
command_return_t do_redit(struct char_data *ch, char *argument, int cmd);
command_return_t do_remove(struct char_data *ch, char *argument, int cmd);
command_return_t do_rename_char(struct char_data *ch, char *argument, int cmd);
command_return_t do_rent(struct char_data *ch, char *argument, int cmd);
command_return_t do_reply(struct char_data *ch, string argument, int cmd);
command_return_t do_repop(struct char_data *ch, char *argument, int cmd);
command_return_t do_report(struct char_data *ch, char *argument, int cmd);
command_return_t do_rescue(struct char_data *ch, char *argument, int cmd);
command_return_t do_rest(struct char_data *ch, char *argument, int cmd);
command_return_t do_restore(struct char_data *ch, char *argument, int cmd);
command_return_t do_retreat(struct char_data *ch, char *argument, int cmd);
command_return_t do_return(struct char_data *ch, char *argument, int cmd);
command_return_t do_revoke(struct char_data *ch, char *argument, int cmd);
command_return_t do_rsave(struct char_data *ch, char *argument, int cmd);
command_return_t do_rstat(struct char_data *ch, char *argument, int cmd);
command_return_t do_sacrifice(struct char_data *ch, char *argument, int cmd);
command_return_t do_save(struct char_data *ch, char *argument, int cmd);
command_return_t do_say(struct char_data *ch, string argument, int cmd = CMD_SAY);
command_return_t do_scan(struct char_data *ch, char *argument, int cmd);
command_return_t do_score(struct char_data *ch, char *argument, int cmd);
command_return_t do_scribe(struct char_data *ch, char *argument, int cmd);
command_return_t do_sector(struct char_data *ch, char *argument, int cmd);
command_return_t do_sedit(struct char_data *ch, char *argument, int cmd);
command_return_t do_send(struct char_data *ch, char *argument, int cmd);
command_return_t do_set(struct char_data *ch, char *argument, int cmd);
command_return_t do_search(struct char_data *ch, char *argument, int cmd);
command_return_t do_shout(struct char_data *ch, char *argument, int cmd);
command_return_t do_showhunt(struct char_data *ch, char *argument, int cmd);
command_return_t do_skills(struct char_data *ch, char *argument, int cmd);
command_return_t do_social(struct char_data *ch, char *argument, int cmd);
command_return_t do_songs(struct char_data *ch, char *argument, int cmd);
command_return_t do_stromboli(struct char_data *ch, char *argument, int cmd);
command_return_t do_summon_toggle(struct char_data *ch, char *argument, int cmd);
command_return_t do_headbutt(struct char_data *ch, char *argument, int cmd);
command_return_t do_show(struct char_data *ch, char *argument, int cmd);
command_return_t do_show_exp(struct char_data *ch, char *argument, int cmd);
command_return_t do_showbits(struct char_data *ch, char *argument, int cmd);
command_return_t do_shutdow(struct char_data *ch, char *argument, int cmd);
command_return_t do_shutdown(struct char_data *ch, char *argument, int cmd);
command_return_t do_silence(struct char_data *ch, char *argument, int cmd);
command_return_t do_stupid(struct char_data *ch, char *argument, int cmd);
command_return_t do_sing(struct char_data *ch, char *argument, int cmd);
command_return_t do_sip(struct char_data *ch, char *argument, int cmd);
command_return_t do_sit(struct char_data *ch, char *argument, int cmd);
command_return_t do_slay(struct char_data *ch, char *argument, int cmd);
command_return_t do_sleep(struct char_data *ch, char *argument, int cmd);
command_return_t do_slip(struct char_data *ch, char *argument, int cmd);
command_return_t do_smite(struct char_data *ch, char *argument, int cmd);
command_return_t do_sneak(struct char_data *ch, char *argument, int cmd);
command_return_t do_snoop(struct char_data *ch, char *argument, int cmd);
command_return_t do_sockets(struct char_data *ch, char *argument, int cmd);
command_return_t do_spells(struct char_data *ch, char *argument, int cmd);
command_return_t do_split(struct char_data *ch, char *argument, int cmd);
command_return_t do_sqedit(struct char_data *ch, char *argument, int cmd);
command_return_t do_stalk(struct char_data *ch, char *argument, int cmd);
command_return_t do_stand(struct char_data *ch, char *argument, int cmd);
command_return_t do_stat(struct char_data *ch, char *argument, int cmd);
command_return_t do_steal(struct char_data *ch, char *argument, int cmd);
command_return_t do_stealth(struct char_data *ch, char *argument, int cmd);
command_return_t do_story(struct char_data *ch, char *argument, int cmd);
command_return_t do_string(struct char_data *ch, char *argument, int cmd);
command_return_t do_stun(struct char_data *ch, char *argument, int cmd);
command_return_t do_suicide(struct char_data *ch, char *argument, int cmd);
command_return_t do_switch(struct char_data *ch, char *argument, int cmd);
command_return_t do_tactics(struct char_data *ch, char *argument, int cmd);
command_return_t do_tame(struct char_data *ch, char *argument, int cmd);
command_return_t do_taste(struct char_data *ch, char *argument, int cmd);
command_return_t do_teleport(struct char_data *ch, char *argument, int cmd);
command_return_t do_tell(struct char_data *ch, string argument, int cmd);
command_return_t do_tellhistory(struct char_data *ch, string argument, int cmd);
command_return_t do_testhand(struct char_data *ch, char *argument, int cmd);
command_return_t do_testhit(struct char_data *ch, char *argument, int cmd);
command_return_t do_testport(struct char_data *ch, char *argument, int cmd);
command_return_t do_testuser(struct char_data *ch, char *argument, int cmd);
command_return_t do_thunder(struct char_data *ch, char *argument, int cmd);
command_return_t do_tick(struct char_data *ch, char *argument, int cmd);
command_return_t do_time(struct char_data *ch, char *argument, int cmd);
command_return_t do_title(struct char_data *ch, char *argument, int cmd);
command_return_t do_toggle(struct char_data *ch, char *argument, int cmd);
command_return_t do_track(struct char_data *ch, char *argument, int cmd);
command_return_t do_trans(struct char_data *ch, char *argument, int cmd);
command_return_t do_triage(struct char_data *ch, char *argument, int cmd);
command_return_t do_trip(struct char_data *ch, char *argument, int cmd);
command_return_t do_trivia(struct char_data *ch, char *argument, int cmd);
command_return_t do_typo(struct char_data *ch, char *argument, int cmd);
command_return_t do_unarchive(struct char_data *ch, char *argument, int cmd);
command_return_t do_unlock(struct char_data *ch, char *argument, int cmd);
command_return_t do_use(struct char_data *ch, char *argument, int cmd);
command_return_t do_varstat(struct char_data *ch, char *argument, int cmd);
command_return_t do_vault(struct char_data *ch, char *argument, int cmd);
command_return_t do_vend(struct char_data *ch, char *argument, int cmd);
command_return_t do_version(struct char_data *ch, char *argument, int cmd);
command_return_t do_visible(struct char_data *ch, char *argument, int cmd);
command_return_t do_vitalstrike(struct char_data *ch, char *argument, int cmd);
command_return_t do_vt100(struct char_data *ch, char *argument, int cmd);
command_return_t do_wake(struct char_data *ch, char *argument, int cmd);
command_return_t do_wear(struct char_data *ch, char *argument, int cmd);
command_return_t do_weather(struct char_data *ch, char *argument, int cmd);
command_return_t do_where(struct char_data *ch, char *argument, int cmd);
command_return_t do_whisper(struct char_data *ch, char *argument, int cmd);
command_return_t do_who(struct char_data *ch, char *argument, int cmd);
command_return_t do_whoarena(struct char_data *ch, char *argument, int cmd);
command_return_t do_whoclan(struct char_data *ch, char *argument, int cmd);
command_return_t do_whogroup(struct char_data *ch, char *argument, int cmd);
command_return_t do_whosolo(struct char_data *ch, char *argument, int cmd);
command_return_t do_wield(struct char_data *ch, char *argument, int cmd);
command_return_t do_fakelog(struct char_data *ch, char *argument, int cmd);
command_return_t do_wimpy(struct char_data *ch, char *argument, int cmd);
command_return_t do_wiz(struct char_data *ch, string argument, int cmd);
command_return_t do_wizhelp(struct char_data *ch, char *argument, int cmd);
command_return_t do_wizinvis(struct char_data *ch, char *argument, int cmd);
command_return_t do_wizlist(struct char_data *ch, char *argument, int cmd);
command_return_t do_wizlock(struct char_data *ch, char *argument, int cmd);
command_return_t do_world(struct char_data* ch, string args, int cmd);
command_return_t do_write_skillquest(struct char_data *ch, char *argument, int cmd);
command_return_t do_write(struct char_data *ch, char *argument, int cmd);
command_return_t do_zap(struct char_data *ch, char *argument, int cmd);
command_return_t do_zedit(struct char_data *ch, char *argument, int cmd);
command_return_t do_zoneexits(struct char_data *ch, char *argument, int cmd);
command_return_t do_zsave(struct char_data *ch, char *argument, int cmd);
command_return_t do_editor(struct char_data *ch, char *argument, int cmd);
command_return_t do_pursue(struct char_data *ch, char *argument, int cmd);

#endif
