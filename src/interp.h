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

char *remove_trailing_spaces(char *arg);
int command_interpreter(CHAR_DATA *ch, char *argument, bool procced = 0);
int search_block(const char *arg, const char **l, bool exact);
int old_search_block(char *argument, int begin, int length, const char **list, int mode);
char lower(char c);
void argument_interpreter(const char *argument, char *first_arg, char *second_arg);
char *one_argument(char *argument, char *first_arg);
const char *one_argument(const char *argument, char *first_arg);
char *one_argument_long(char *argument, char *first_arg);
char *one_argumentnolow(char *argument, char *first_arg);
int fill_word(char *argument);
void half_chop(char *string, char *arg1, char *arg2);
tuple<string, string> half_chop(string arguments);
tuple<string, string> half_chop(string arguments, const char token);
void chop_half(char *string, char *arg1, char *arg2);
void nanny(struct descriptor_data *d, char *arg);
int is_abbrev(char *arg1, char *arg2);
bool is_abbrev(const string &abbrev, const string &word);
int len_cmp(char *s1, char *s2);
void add_command_lag(CHAR_DATA *ch, int cmdnum, int lag);

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
#define CMD_SLIP 87
#define CMD_GIVE 88
#define CMD_DROP 89
#define CMD_DONATE 90
#define CMD_QUIT 91
#define CMD_SACRIFICE 92
#define CMD_PUT 93
#define CMD_OPEN 98
#define CMD_EDITOR 100
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

struct command_info
{
    char *command_name;      /* Name of ths command             */
    DO_FUN *command_pointer; /* Function that does it            */
    ubyte minimum_position;  /* Position commander must be in    */
    ubyte minimum_level;     /* Minimum level needed             */
    int command_number;      /* Passed to function as argument   */
    int flags;               // what flags the skills has
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

int do_mscore(CHAR_DATA *ch, char *argument, int cmd);
int do_clanarea(CHAR_DATA *ch, char *argument, int cmd);
int do_huntstart(CHAR_DATA *ch, char *argument, int cmd);
int do_huntclear(CHAR_DATA *ch, char *argument, int cmd);

int do_metastat(CHAR_DATA *ch, char *argument, int cmd);
int do_findfix(CHAR_DATA *ch, char *argument, int cmd);
int do_reload(CHAR_DATA *ch, char *argument, int cmd);
int do_abandon(CHAR_DATA *ch, char *argument, int cmd);
int do_accept(CHAR_DATA *ch, char *argument, int cmd);
int do_acfinder(CHAR_DATA *ch, char *argument, int cmd);
int do_action(CHAR_DATA *ch, char *argument, int cmd);
int do_addnews(CHAR_DATA *ch, char *argument, int cmd);
int do_addRoom(CHAR_DATA *ch, char *argument, int cmd);
int do_advance(CHAR_DATA *ch, char *argument, int cmd);
int do_areastats(CHAR_DATA *ch, char *argument, int cmd);
int do_awaymsgs(CHAR_DATA *ch, char *argument, int cmd);
int do_guide(CHAR_DATA *ch, char *argument, int cmd);
int do_alias(CHAR_DATA *ch, char *argument, int cmd);
int do_archive(CHAR_DATA *ch, char *argument, int cmd);
int do_autoeat(CHAR_DATA *ch, char *argument, int cmd);
int do_autojoin(CHAR_DATA *ch, char *argument, int cmd);
int do_unban(CHAR_DATA *ch, char *argument, int cmd);
int do_ambush(CHAR_DATA *ch, char *argument, int cmd);
int do_anonymous(CHAR_DATA *ch, char *argument, int cmd);
int do_ansi(CHAR_DATA *ch, char *argument, int cmd);
int do_appraise(CHAR_DATA *ch, char *argument, int cmd);
int do_assemble(CHAR_DATA *ch, char *argument, int cmd);
int do_release(CHAR_DATA *ch, char *argument, int cmd);
int do_jab(CHAR_DATA *ch, char *argument, int cmd);
int do_areas(CHAR_DATA *ch, char *argument, int cmd);
int do_arena(CHAR_DATA *ch, char *argument, int cmd);
int do_ask(CHAR_DATA *ch, char *argument, int cmd);
int do_at(CHAR_DATA *ch, char *argument, int cmd);
int do_auction(CHAR_DATA *ch, char *argument, int cmd);
int do_backstab(CHAR_DATA *ch, char *argument, int cmd);
int do_ban(CHAR_DATA *ch, char *argument, int cmd);
int do_bandwidth(CHAR_DATA *ch, char *argument, int cmd);
int do_bard_song_toggle(CHAR_DATA *ch, char *argument, int cmd);
int do_bash(CHAR_DATA *ch, char *argument, int cmd);
int do_batter(CHAR_DATA *ch, char *argument, int cmd);
int do_battlecry(CHAR_DATA *ch, char *argument, int cmd);
int do_battlesense(CHAR_DATA *ch, char *argument, int cmd);
int do_beacon(CHAR_DATA *ch, char *argument, int cmd);
int do_beep(CHAR_DATA *ch, char *argument, int cmd);
int do_beep_set(CHAR_DATA *ch, char *argument, int cmd);
int do_behead(CHAR_DATA *ch, char *argument, int cmd);
int do_berserk(CHAR_DATA *ch, char *argument, int cmd);
int do_bestow(CHAR_DATA *ch, char *argument, int cmd);
int do_bladeshield(CHAR_DATA *ch, char *argument, int cmd);
int do_bloodfury(CHAR_DATA *ch, char *argument, int cmd);
int do_boot(CHAR_DATA *ch, char *argument, int cmd);
int do_boss(CHAR_DATA *ch, char *argument, int cmd);
int do_brace(CHAR_DATA *ch, char *argument, int cmd);
int do_brief(CHAR_DATA *ch, char *argument, int cmd);
int do_brew(CHAR_DATA *ch, char *argument, int cmd);
int do_news_toggle(CHAR_DATA *ch, char *argument, int cmd);
int do_ascii_toggle(CHAR_DATA *ch, char *argument, int cmd);
int do_damage_toggle(CHAR_DATA *ch, char *argument, int cmd);
int do_bug(CHAR_DATA *ch, char *argument, int cmd);
int do_bullrush(CHAR_DATA *ch, char *argument, int cmd);
int do_cast(CHAR_DATA *ch, char *argument, int cmd);
int do_channel(CHAR_DATA *ch, char *argument, int cmd);
int do_charmiejoin_toggle(CHAR_DATA *ch, char *argument, int cmd);
int do_check(CHAR_DATA *ch, char *argument, int cmd);
int do_chpwd(CHAR_DATA *ch, char *argument, int cmd);
int do_cinfo(CHAR_DATA *ch, char *argument, int cmd);
int do_circle(CHAR_DATA *ch, char *argument, int cmd);
int do_clans(CHAR_DATA *ch, char *argument, int cmd);
int do_clear(CHAR_DATA *ch, char *argument, int cmd);
int do_clearaff(CHAR_DATA *ch, char *argument, int cmd);
int do_climb(CHAR_DATA *ch, char *argument, int cmd);
int do_close(CHAR_DATA *ch, char *argument, int cmd);
int do_cmotd(CHAR_DATA *ch, char *argument, int cmd);
int do_ctax(CHAR_DATA *ch, char *argument, int cmd);
int do_cdeposit(CHAR_DATA *ch, char *argument, int cmd);
int do_cwithdraw(CHAR_DATA *ch, char *argument, int cmd);
int do_cbalance(CHAR_DATA *ch, char *argument, int cmd);
int do_colors(CHAR_DATA *ch, char *argument, int cmd);
int do_compact(CHAR_DATA *ch, char *argument, int cmd);
int do_config(CHAR_DATA *ch, char *argument, int cmd);
int do_consent(CHAR_DATA *ch, char *argument, int cmd);
int do_consider(CHAR_DATA *ch, char *argument, int cmd);
int do_count(CHAR_DATA *ch, char *argument, int cmd);
int do_cpromote(CHAR_DATA *ch, char *argument, int cmd);
int do_crazedassault(CHAR_DATA *ch, char *argument, int cmd);
int do_credits(CHAR_DATA *ch, char *argument, int cmd);
int do_cripple(CHAR_DATA *ch, char *argument, int cmd);
int do_ctell(CHAR_DATA *ch, char *argument, int cmd);
int do_deathstroke(CHAR_DATA *ch, char *argument, int cmd);
int do_debug(CHAR_DATA *ch, char *argument, int cmd);
int do_deceit(CHAR_DATA *ch, char *argument, int cmd);
int do_defenders_stance(CHAR_DATA *ch, char *argument, int cmd);
int do_disarm(CHAR_DATA *ch, char *argument, int cmd);
int do_disband(CHAR_DATA *ch, char *argument, int cmd);
int do_disconnect(CHAR_DATA *ch, char *argument, int cmd);
int do_dmg_eq(CHAR_DATA *ch, char *argument, int cmd);
int do_donate(CHAR_DATA *ch, char *argument, int cmd);
int do_dream(CHAR_DATA *ch, char *argument, int cmd);
int do_drink(CHAR_DATA *ch, char *argument, int cmd);
int do_drop(CHAR_DATA *ch, char *argument, int cmd);
int do_eat(CHAR_DATA *ch, char *argument, int cmd);
int do_eagle_claw(CHAR_DATA *ch, char *argument, int cmd);
int do_echo(CHAR_DATA *ch, char *argument, int cmd);
int do_emote(CHAR_DATA *ch, char *argument, int cmd);
int do_setvote(CHAR_DATA *ch, char *argument, int cmd);
int do_vote(CHAR_DATA *ch, char *argument, int cmd);
int do_enter(CHAR_DATA *ch, char *argument, int cmd);
int do_equipment(CHAR_DATA *ch, char *argument, int cmd);
int do_eyegouge(CHAR_DATA *ch, char *argument, int cmd);
int do_examine(CHAR_DATA *ch, char *argument, int cmd);
int do_exits(CHAR_DATA *ch, char *argument, int cmd);
int do_export(CHAR_DATA *ch, char *argument, int cmd);
int do_ferocity(CHAR_DATA *ch, char *argument, int cmd);
int do_fighting(CHAR_DATA *ch, char *argument, int cmd);
int do_fill(CHAR_DATA *ch, char *argument, int cmd);
int do_find(CHAR_DATA *ch, char *argument, int cmd);
int do_findpath(CHAR_DATA *ch, char *argument, int cmd);
int do_findPath(CHAR_DATA *ch, char *argument, int cmd);
int do_fire(CHAR_DATA *ch, char *argument, int cmd);
int do_flee(CHAR_DATA *ch, char *argument, int cmd);
/* DO_FUN  do_fly; */
int do_focused_repelance(CHAR_DATA *ch, char *argument, int cmd);
int do_follow(CHAR_DATA *ch, char *argument, int cmd);
int do_forage(CHAR_DATA *ch, char *argument, int cmd);
int do_force(CHAR_DATA *ch, char *argument, int cmd);
int do_found(CHAR_DATA *ch, char *argument, int cmd);
int do_free_animal(CHAR_DATA *ch, char *argument, int cmd);
int do_freeze(CHAR_DATA *ch, char *argument, int cmd);
int do_fsave(CHAR_DATA *ch, char *argument, int cmd);
int do_get(CHAR_DATA *ch, char *argument, int cmd);
int do_give(CHAR_DATA *ch, char *argument, int cmd);
int do_global(CHAR_DATA *ch, char *argument, int cmd);
int do_gossip(CHAR_DATA *ch, char *argument, int cmd);
int do_golem_score(CHAR_DATA *ch, char *argument, int cmd);
int do_goto(CHAR_DATA *ch, char *argument, int cmd);
int do_guild(CHAR_DATA *ch, char *argument, int cmd);
int do_install(CHAR_DATA *ch, char *argument, int cmd);
int do_reload_help(CHAR_DATA *ch, char *argument, int cmd);
int do_hindex(CHAR_DATA *ch, char *argument, int cmd);
int do_hedit(CHAR_DATA *ch, char *argument, int cmd);
int do_grab(CHAR_DATA *ch, char *argument, int cmd);
int do_group(CHAR_DATA *ch, char *argument, int cmd);
int do_grouptell(CHAR_DATA *ch, char *argument, int cmd);
int do_gtrans(CHAR_DATA *ch, char *argument, int cmd);
int do_guard(CHAR_DATA *ch, char *argument, int cmd);
int do_harmtouch(CHAR_DATA *ch, char *argument, int cmd);
int do_help(CHAR_DATA *ch, char *argument, int cmd);
int do_mortal_help(CHAR_DATA *ch, char *argument, int cmd);
int do_new_help(CHAR_DATA *ch, char *argument, int cmd);
int do_hide(CHAR_DATA *ch, char *argument, int cmd);
int do_highfive(CHAR_DATA *ch, char *argument, int cmd);
int do_hit(CHAR_DATA *ch, char *argument, int cmd);
int do_hitall(CHAR_DATA *ch, char *argument, int cmd);
int do_holylite(CHAR_DATA *ch, char *argument, int cmd);
int do_home(CHAR_DATA *ch, char *argument, int cmd);
int do_idea(CHAR_DATA *ch, char *argument, int cmd);
int do_identify(CHAR_DATA *ch, char *argument, int cmd);
int do_ignore(CHAR_DATA *ch, char *argument, int cmd);
int do_imotd(CHAR_DATA *ch, char *argument, int cmd);
int do_imbue(CHAR_DATA *ch, char *argument, int cmd);
int do_incognito(CHAR_DATA *ch, char *argument, int cmd);
int do_index(CHAR_DATA *ch, char *argument, int cmd);
int do_info(CHAR_DATA *ch, char *argument, int cmd);
int do_initiate(CHAR_DATA *ch, char *argument, int cmd);
int do_innate(CHAR_DATA *ch, char *argument, int cmd);
int do_instazone(CHAR_DATA *ch, char *argument, int cmd);
int do_insult(CHAR_DATA *ch, char *argument, int cmd);
int do_inventory(CHAR_DATA *ch, char *argument, int cmd);
int do_join(CHAR_DATA *ch, char *argument, int cmd);
int do_joinarena(CHAR_DATA *ch, char *argument, int cmd);
int do_ki(CHAR_DATA *ch, char *argument, int cmd);
int do_kick(CHAR_DATA *ch, char *argument, int cmd);
int do_kill(CHAR_DATA *ch, char *argument, int cmd);
int do_knockback(CHAR_DATA *ch, char *argument, int cmd);
// int do_land (CHAR_DATA *ch, char *argument, int cmd);
int do_layhands(CHAR_DATA *ch, char *argument, int cmd);
int do_leaderboard(CHAR_DATA *ch, char *argument, int cmd);
int do_leadership(CHAR_DATA *ch, char *argument, int cmd);
int do_leave(CHAR_DATA *ch, char *argument, int cmd);
int do_levels(CHAR_DATA *ch, char *argument, int cmd);
int do_lfg_toggle(CHAR_DATA *ch, char *argument, int cmd);
int do_linkdead(CHAR_DATA *ch, char *argument, int cmd);
int do_linkload(CHAR_DATA *ch, char *argument, int cmd);
int do_listAllPaths(CHAR_DATA *ch, char *argument, int cmd);
int do_listPathsByZone(CHAR_DATA *ch, char *argument, int cmd);
int do_listproc(CHAR_DATA *ch, char *argument, int cmd);
int do_load(CHAR_DATA *ch, char *argument, int cmd);
int do_medit(CHAR_DATA *ch, char *argument, int cmd);
int do_memoryleak(CHAR_DATA *ch, char *argument, int cmd);
int do_mortal_set(CHAR_DATA *ch, char *argument, int cmd);
//int do_motdload (CHAR_DATA *ch, char *argument, int cmd);
int do_msave(CHAR_DATA *ch, char *argument, int cmd);
int do_procedit(CHAR_DATA *ch, char *argument, int cmd);
int do_mpbestow(CHAR_DATA *ch, char *argument, int cmd);
int do_mpstat(CHAR_DATA *ch, char *argument, int cmd);
int do_opedit(CHAR_DATA *ch, char *argument, int cmd);
int do_eqmax(CHAR_DATA *ch, char *argument, int cmd);
int do_opstat(CHAR_DATA *ch, char *argument, int cmd);
int do_lock(CHAR_DATA *ch, char *argument, int cmd);
int do_log(CHAR_DATA *ch, char *argument, int cmd);
int do_look(CHAR_DATA *ch, char *argument, int cmd);
int do_botcheck(CHAR_DATA *ch, char *argument, int cmd);
int do_make_camp(CHAR_DATA *ch, char *argument, int cmd);
int do_matrixinfo(CHAR_DATA *ch, char *argument, int cmd);
int do_maxes(CHAR_DATA *ch, char *argument, int cmd);
int do_mlocate(CHAR_DATA *ch, char *argument, int cmd);
int do_move(CHAR_DATA *ch, char *argument, int cmd);
int do_motd(CHAR_DATA *ch, char *argument, int cmd);
int do_mpretval(CHAR_DATA *ch, char *argument, int cmd);
int do_mpasound(CHAR_DATA *ch, char *argument, int cmd);
int do_mpat(CHAR_DATA *ch, char *argument, int cmd);
int do_mpdamage(CHAR_DATA *ch, char *argument, int cmd);
int do_mpecho(CHAR_DATA *ch, char *argument, int cmd);
int do_mpechoaround(CHAR_DATA *ch, char *argument, int cmd);
int do_mpechoaroundnotbad(CHAR_DATA *ch, char *argument, int cmd);
int do_mpechoat(CHAR_DATA *ch, char *argument, int cmd);
int do_mpforce(CHAR_DATA *ch, char *argument, int cmd);
int do_mpgoto(CHAR_DATA *ch, char *argument, int cmd);
int do_mpjunk(CHAR_DATA *ch, char *argument, int cmd);
int do_mpkill(CHAR_DATA *ch, char *argument, int cmd);
int do_mphit(CHAR_DATA *ch, char *argument, int cmd);
int do_mpsetmath(CHAR_DATA *ch, char *argument, int cmd);
int do_mpaddlag(CHAR_DATA *ch, char *argument, int cmd);
int do_mpmload(CHAR_DATA *ch, char *argument, int cmd);
int do_mpoload(CHAR_DATA *ch, char *argument, int cmd);
int do_mppause(CHAR_DATA *ch, char *argument, int cmd);
int do_mppeace(CHAR_DATA *ch, char *argument, int cmd);
int do_mppurge(CHAR_DATA *ch, char *argument, int cmd);
int do_mpteachskill(CHAR_DATA *ch, char *argument, int cmd);
int do_mpsetalign(CHAR_DATA *ch, char *argument, int cmd);
int do_mpsettemp(CHAR_DATA *ch, char *argument, int cmd);
int do_mpthrow(CHAR_DATA *ch, char *argument, int cmd);
int do_mpothrow(CHAR_DATA *ch, char *argument, int cmd);
int do_mptransfer(CHAR_DATA *ch, char *argument, int cmd);
int do_mpxpreward(CHAR_DATA *ch, char *argument, int cmd);
int do_mpteleport(CHAR_DATA *ch, char *argument, int cmd);
int do_murder(CHAR_DATA *ch, char *argument, int cmd);
int do_name(CHAR_DATA *ch, char *argument, int cmd);
int do_natural_selection(CHAR_DATA *ch, char *argument, int cmd);
int do_newbie(CHAR_DATA *ch, char *argument, int cmd);
int do_newPath(CHAR_DATA *ch, char *argument, int cmd);
int do_news(CHAR_DATA *ch, char *argument, int cmd);
int do_noemote(CHAR_DATA *ch, char *argument, int cmd);
int do_nohassle(CHAR_DATA *ch, char *argument, int cmd);
int do_noname(CHAR_DATA *ch, char *argument, int cmd);
int do_not_here(CHAR_DATA *ch, char *argument, int cmd);
int do_notax_toggle(CHAR_DATA *ch, char *argument, int cmd);
int do_guide_toggle(CHAR_DATA *ch, char *argument, int cmd);
int do_oclone(CHAR_DATA *ch, char *argument, int cmd);
int do_mclone(CHAR_DATA *ch, char *argument, int cmd);
int do_oedit(CHAR_DATA *ch, char *argument, int cmd);
int do_offer(CHAR_DATA *ch, char *argument, int cmd);
int do_olocate(CHAR_DATA *ch, char *argument, int cmd);
int do_oneway(CHAR_DATA *ch, char *argument, int cmd);
int do_onslaught(CHAR_DATA *ch, char *argument, int cmd);
int do_open(CHAR_DATA *ch, char *argument, int cmd);
int do_order(CHAR_DATA *ch, char *argument, int cmd);
int do_orchestrate(CHAR_DATA *ch, char *argument, int cmd);
int do_osave(CHAR_DATA *ch, char *argument, int cmd);
int do_nodupekeys_toggle(CHAR_DATA *ch, char *argument, int cmd);
int do_outcast(CHAR_DATA *ch, char *argument, int cmd);
int do_pager(CHAR_DATA *ch, char *argument, int cmd);
int do_pardon(CHAR_DATA *ch, char *argument, int cmd);
int do_pathpath(CHAR_DATA *ch, char *argument, int cmd);
int do_peace(CHAR_DATA *ch, char *argument, int cmd);
int do_perseverance(CHAR_DATA *ch, char *argument, int cmd);
int do_pick(CHAR_DATA *ch, char *argument, int cmd);
int do_plats(CHAR_DATA *ch, char *argument, int cmd);
int do_pocket(CHAR_DATA *ch, char *argument, int cmd);
int do_poisonmaking(CHAR_DATA *ch, char *argument, int cmd);
int do_pour(CHAR_DATA *ch, char *argument, int cmd);
int do_poof(CHAR_DATA *ch, char *argument, int cmd);
int do_possess(CHAR_DATA *ch, char *argument, int cmd);
int do_practice(CHAR_DATA *ch, char *argument, int cmd);
int do_pray(CHAR_DATA *ch, char *argument, int cmd);
int do_profession(CHAR_DATA *ch, char *argument, int cmd);
int do_primalfury(CHAR_DATA *ch, char *argument, int cmd);
int do_promote(CHAR_DATA *ch, char *argument, int cmd);
int do_prompt(CHAR_DATA *ch, char *argument, int cmd);
int do_lastprompt(CHAR_DATA *ch, char *argument, int cmd);
int do_processes(CHAR_DATA *ch, char *argument, int cmd);
int do_psay(CHAR_DATA *ch, char *argument, int cmd);
//int do_pshopedit (CHAR_DATA *ch, char *argument, int cmd);
int do_pview(CHAR_DATA *ch, char *argument, int cmd);
int do_punish(CHAR_DATA *ch, char *argument, int cmd);
int do_purge(CHAR_DATA *ch, char *argument, int cmd);
int do_purloin(CHAR_DATA *ch, char *argument, int cmd);
int do_put(CHAR_DATA *ch, char *argument, int cmd);
int do_qedit(CHAR_DATA *ch, char *argument, int cmd);
int do_quaff(CHAR_DATA *ch, char *argument, int cmd);
int do_quest(CHAR_DATA *ch, char *argument, int cmd);
int do_qui(CHAR_DATA *ch, char *argument, int cmd);
int do_quivering_palm(CHAR_DATA *ch, char *argument, int cmd);
int do_quit(CHAR_DATA *ch, char *argument, int cmd);
int do_rage(CHAR_DATA *ch, char *argument, int cmd);
int do_range(CHAR_DATA *ch, char *argument, int cmd);
int do_rdelete(CHAR_DATA *ch, char *argument, int cmd);
int do_read(CHAR_DATA *ch, char *argument, int cmd);
int do_recall(CHAR_DATA *ch, char *argument, int cmd);
int do_recite(CHAR_DATA *ch, char *argument, int cmd);
int do_redirect(CHAR_DATA *ch, char *argument, int cmd);
int do_redit(CHAR_DATA *ch, char *argument, int cmd);
int do_remove(CHAR_DATA *ch, char *argument, int cmd);
int do_rename_char(CHAR_DATA *ch, char *argument, int cmd);
int do_rent(CHAR_DATA *ch, char *argument, int cmd);
int do_reply(CHAR_DATA *ch, char *argument, int cmd);
int do_repop(CHAR_DATA *ch, char *argument, int cmd);
int do_report(CHAR_DATA *ch, char *argument, int cmd);
int do_rescue(CHAR_DATA *ch, char *argument, int cmd);
int do_rest(CHAR_DATA *ch, char *argument, int cmd);
int do_restore(CHAR_DATA *ch, char *argument, int cmd);
int do_retreat(CHAR_DATA *ch, char *argument, int cmd);
int do_return(CHAR_DATA *ch, char *argument, int cmd);
int do_revoke(CHAR_DATA *ch, char *argument, int cmd);
int do_rsave(CHAR_DATA *ch, char *argument, int cmd);
int do_rstat(CHAR_DATA *ch, char *argument, int cmd);
int do_save(CHAR_DATA *ch, char *argument, int cmd);
int do_say(CHAR_DATA *ch, char *argument, int cmd);
int do_scan(CHAR_DATA *ch, char *argument, int cmd);
int do_score(CHAR_DATA *ch, char *argument, int cmd);
int do_scribe(CHAR_DATA *ch, char *argument, int cmd);
int do_sector(CHAR_DATA *ch, char *argument, int cmd);
int do_sedit(CHAR_DATA *ch, char *argument, int cmd);
int do_send(CHAR_DATA *ch, char *argument, int cmd);
int do_set(CHAR_DATA *ch, char *argument, int cmd);
int do_shout(CHAR_DATA *ch, char *argument, int cmd);
int do_showhunt(CHAR_DATA *ch, char *argument, int cmd);
int do_skills(CHAR_DATA *ch, char *argument, int cmd);
int do_social(CHAR_DATA *ch, char *argument, int cmd);
int do_songs(CHAR_DATA *ch, char *argument, int cmd);
int do_stromboli(CHAR_DATA *ch, char *argument, int cmd);
int do_summon_toggle(CHAR_DATA *ch, char *argument, int cmd);
int do_headbutt(CHAR_DATA *ch, char *argument, int cmd);
int do_show(CHAR_DATA *ch, char *argument, int cmd);
int do_show_exp(CHAR_DATA *ch, char *argument, int cmd);
int do_showbits(CHAR_DATA *ch, char *argument, int cmd);
int do_shutdow(CHAR_DATA *ch, char *argument, int cmd);
int do_shutdown(CHAR_DATA *ch, char *argument, int cmd);
int do_silence(CHAR_DATA *ch, char *argument, int cmd);
int do_stupid(CHAR_DATA *ch, char *argument, int cmd);
int do_sing(CHAR_DATA *ch, char *argument, int cmd);
int do_sip(CHAR_DATA *ch, char *argument, int cmd);
int do_sit(CHAR_DATA *ch, char *argument, int cmd);
int do_slay(CHAR_DATA *ch, char *argument, int cmd);
int do_sleep(CHAR_DATA *ch, char *argument, int cmd);
int do_slip(CHAR_DATA *ch, char *argument, int cmd);
int do_smite(CHAR_DATA *ch, char *argument, int cmd);
int do_sneak(CHAR_DATA *ch, char *argument, int cmd);
int do_snoop(CHAR_DATA *ch, char *argument, int cmd);
int do_sockets(CHAR_DATA *ch, char *argument, int cmd);
int do_spells(CHAR_DATA *ch, char *argument, int cmd);
int do_split(CHAR_DATA *ch, char *argument, int cmd);
int do_sqedit(CHAR_DATA *ch, char *argument, int cmd);
int do_stalk(CHAR_DATA *ch, char *argument, int cmd);
int do_stand(CHAR_DATA *ch, char *argument, int cmd);
int do_stat(CHAR_DATA *ch, char *argument, int cmd);
int do_steal(CHAR_DATA *ch, char *argument, int cmd);
int do_stealth(CHAR_DATA *ch, char *argument, int cmd);
int do_story(CHAR_DATA *ch, char *argument, int cmd);
int do_string(CHAR_DATA *ch, char *argument, int cmd);
int do_stun(CHAR_DATA *ch, char *argument, int cmd);
int do_suicide(CHAR_DATA *ch, char *argument, int cmd);
int do_switch(CHAR_DATA *ch, char *argument, int cmd);
int do_tactics(CHAR_DATA *ch, char *argument, int cmd);
int do_tame(CHAR_DATA *ch, char *argument, int cmd);
int do_tap(CHAR_DATA *ch, char *argument, int cmd);
int do_taste(CHAR_DATA *ch, char *argument, int cmd);
int do_teleport(CHAR_DATA *ch, char *argument, int cmd);
int do_tell(CHAR_DATA *ch, char *argument, int cmd);
int do_testhand(CHAR_DATA *ch, char *argument, int cmd);
int do_testhit(CHAR_DATA *ch, char *argument, int cmd);
int do_testport(CHAR_DATA *ch, char *argument, int cmd);
int do_testuser(CHAR_DATA *ch, char *argument, int cmd);
int do_thunder(CHAR_DATA *ch, char *argument, int cmd);
int do_tick(CHAR_DATA *ch, char *argument, int cmd);
int do_time(CHAR_DATA *ch, char *argument, int cmd);
int do_title(CHAR_DATA *ch, char *argument, int cmd);
int do_toggle(CHAR_DATA *ch, char *argument, int cmd);
int do_track(CHAR_DATA *ch, char *argument, int cmd);
int do_trans(CHAR_DATA *ch, char *argument, int cmd);
int do_triage(CHAR_DATA *ch, char *argument, int cmd);
int do_trip(CHAR_DATA *ch, char *argument, int cmd);
int do_trivia(CHAR_DATA *ch, char *argument, int cmd);
int do_typo(CHAR_DATA *ch, char *argument, int cmd);
int do_unarchive(CHAR_DATA *ch, char *argument, int cmd);
int do_unlock(CHAR_DATA *ch, char *argument, int cmd);
int do_use(CHAR_DATA *ch, char *argument, int cmd);
int do_varstat(CHAR_DATA *ch, char *argument, int cmd);
int do_vault(CHAR_DATA *ch, char *argument, int cmd);
int do_vend(CHAR_DATA *ch, char *argument, int cmd);
int do_version(CHAR_DATA *ch, char *argument, int cmd);
int do_visible(CHAR_DATA *ch, char *argument, int cmd);
int do_vitalstrike(CHAR_DATA *ch, char *argument, int cmd);
int do_vt100(CHAR_DATA *ch, char *argument, int cmd);
int do_wake(CHAR_DATA *ch, char *argument, int cmd);
int do_wear(CHAR_DATA *ch, char *argument, int cmd);
int do_weather(CHAR_DATA *ch, char *argument, int cmd);
int do_where(CHAR_DATA *ch, char *argument, int cmd);
int do_whisper(CHAR_DATA *ch, char *argument, int cmd);
int do_who(CHAR_DATA *ch, char *argument, int cmd);
int do_whoarena(CHAR_DATA *ch, char *argument, int cmd);
int do_whoclan(CHAR_DATA *ch, char *argument, int cmd);
int do_whogroup(CHAR_DATA *ch, char *argument, int cmd);
int do_whosolo(CHAR_DATA *ch, char *argument, int cmd);
int do_wield(CHAR_DATA *ch, char *argument, int cmd);
int do_fakelog(CHAR_DATA *ch, char *argument, int cmd);
int do_wimpy(CHAR_DATA *ch, char *argument, int cmd);
int do_wiz(CHAR_DATA *ch, char *argument, int cmd);
int do_wizhelp(CHAR_DATA *ch, char *argument, int cmd);
int do_wizinvis(CHAR_DATA *ch, char *argument, int cmd);
int do_wizlist(CHAR_DATA *ch, char *argument, int cmd);
int do_wizlock(CHAR_DATA *ch, char *argument, int cmd);
int do_write_skillquest(CHAR_DATA *ch, char *argument, int cmd);
int do_write(CHAR_DATA *ch, char *argument, int cmd);
int do_zap(CHAR_DATA *ch, char *argument, int cmd);
int do_zedit(CHAR_DATA *ch, char *argument, int cmd);
int do_zoneexits(CHAR_DATA *ch, char *argument, int cmd);
int do_zsave(CHAR_DATA *ch, char *argument, int cmd);
int do_editor(CHAR_DATA *ch, char *argument, int cmd);
int do_pursue(CHAR_DATA *ch, char *argument, int cmd);

#endif
