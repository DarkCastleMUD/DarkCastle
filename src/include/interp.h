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



char *remove_trailing_spaces(char *arg);
int command_interpreter(CHAR_DATA *ch, char *argument, bool procced = 0 );
int search_block(char *arg, char **list, bool exact);
int old_search_block(char *argument,int begin,int length,char **list,int mode);
char lower( char c );
void argument_interpreter(const char *argument, char *first_arg, char *second_arg);
char *one_argument(char *argument,char *first_arg);
const char *one_argument(const char *argument, char *first_arg);
char *one_argument_long(char *argument,char *first_arg);
char *one_argumentnolow(char *argument,char *first_arg);
int fill_word(char *argument);
void half_chop(char *string, char *arg1, char *arg2);
void chop_half(char *string, char *arg1, char *arg2);
void nanny(struct descriptor_data *d, char *arg);
int is_abbrev(char *arg1, char *arg2);
int len_cmp(char *s1, char *s2);
void add_command_lag(CHAR_DATA *ch, int cmdnum, int lag);


#define CMD_NORTH	1
#define CMD_EAST	2
#define CMD_SOUTH	3
#define CMD_WEST	4
#define CMD_UP		5
#define CMD_DOWN	6

#define CMD_BELLOW	8
#define CMD_DEFAULT	9
#define CMD_TRACK	10
#define CMD_PALM	10
#define CMD_SAY		11
#define CMD_LOOK	12
#define CMD_BACKSTAB	13
#define CMD_SBS		14
#define CMD_ORCHESTRATE 15
#define CMD_GLANCE	20
#define CMD_FLEE	28
#define CMD_ESCAPE	29
#define CMD_PICK	35
#define CMD_STOCK	56
#define CMD_BUY		56
#define CMD_SELL	57
#define CMD_VALUE	58
#define CMD_LIST	59
#define CMD_ENTER	60
#define CMD_CLIMB	60
#define CMD_DESIGN	62
#define CMD_PRICE	65
#define CMD_REPAIR	66
#define CMD_READ	67
#define CMD_REMOVE	69
#define CMD_ERASE	70
#define CMD_ESTIMATE	71
#define CMD_REMORT	80
#define CMD_SLIP	87
#define CMD_GIVE	88
#define CMD_DROP	89
#define CMD_DONATE	90
#define CMD_QUIT	91
#define CMD_SACRIFICE	92
#define CMD_PUT		93
#define CMD_OPEN	98
#define CMD_EDITOR	100
#define CMD_WRITE	128
#define CMD_WATCH	155
#define CMD_PRACTICE	164
#define CMD_TRAIN	165
#define CMD_PROFESSION 166
#define CMD_GAIN	171
#define CMD_BALANCE	172
#define CMD_DEPOSIT	173
#define CMD_WITHDRAW	174
#define CMD_CLEAN	    177
#define CMD_PLAY        178
#define CMD_FINISH      179
#define CMD_VETERNARIAN 180
#define CMD_FEED        181
#define CMD_ASSEMBLE    182
#define CMD_PAY         183
#define CMD_RESTRING    184
#define CMD_PUSH        185
#define CMD_PULL        186
#define CMD_LEAVE	    187
#define CMD_TREMOR      188
#define CMD_BET         189
#define CMD_INSURANCE   190
#define CMD_DOUBLE      191
#define CMD_STAY        192
#define CMD_SPLIT	    193
#define CMD_HIT		    194
#define CMD_LOOT        195
#define CMD_GTELL       200
#define CMD_CTELL       201
#define CMD_SETVOTE	202
#define CMD_VOTE	203
#define CMD_VEND	204
#define CMD_FILTER	205
#define CMD_PRIZE	999
#define CMD_GAZE	1820



// Temp removal to perfect system. 1/25/06 Eas
// WARNING WARNING WARNING WARNING WARNING
// The command list was modified to account for toggle_hide.
// The last integer will affect a char being removed from hide when they perform the command.













/*
 * Command functions.
 */
typedef int    DO_FUN      (CHAR_DATA *ch, char *argument, int cmd);

struct command_info
{
    char *command_name;             /* Name of ths command             */
    DO_FUN *command_pointer;        /* Function that does it            */
    ubyte minimum_position;          /* Position commander must be in    */
    ubyte minimum_level;             /* Minimum level needed             */
    int command_number;           /* Passed to function as argument   */
    int flags;                      // what flags the skills has 
    ubyte toggle_hide;
};

struct command_lag
{
   struct command_lag *next;
   CHAR_DATA *ch;
   int cmd_number;
   int lag;
};

struct cmd_hash_info {
	struct command_info *command;
	struct cmd_hash_info *left;
	struct cmd_hash_info *right;
};

#define COM_CHARMIE_OK       1

DO_FUN do_mscore;
DO_FUN do_clanarea;
DO_FUN do_huntstart;
DO_FUN do_huntclear;

DO_FUN do_metastat;
DO_FUN do_findfix;
DO_FUN do_reload;
DO_FUN do_abandon;
DO_FUN do_accept;
DO_FUN do_acfinder;
DO_FUN do_action;
DO_FUN do_addnews;
DO_FUN do_addRoom;
DO_FUN do_advance;
DO_FUN do_areastats;
DO_FUN do_awaymsgs;
DO_FUN do_guide;
DO_FUN do_alias;
DO_FUN do_archive;
DO_FUN do_autoeat;
DO_FUN do_autojoin;
DO_FUN do_unban;
DO_FUN do_ambush;
DO_FUN do_anonymous;
DO_FUN do_ansi;
DO_FUN do_appraise;
DO_FUN do_assemble;
DO_FUN do_release;
DO_FUN do_jab;
DO_FUN do_areas;
DO_FUN do_arena;
DO_FUN do_ask;
DO_FUN do_at;
DO_FUN do_auction;
DO_FUN do_backstab;
DO_FUN do_ban;
DO_FUN do_bandwidth;
DO_FUN do_bard_song_toggle;
DO_FUN do_bash;
DO_FUN do_batter;
DO_FUN do_battlecry;
DO_FUN do_battlesense;
DO_FUN do_beacon;
DO_FUN do_beep;
DO_FUN do_beep_set;
DO_FUN do_behead;
DO_FUN do_berserk;
DO_FUN do_bestow;
DO_FUN do_bladeshield;
DO_FUN do_bloodfury;
DO_FUN do_boot;
DO_FUN do_boss;
DO_FUN do_brace;
DO_FUN do_brief;
DO_FUN do_brew;
DO_FUN do_news_toggle;
DO_FUN do_ascii_toggle;
DO_FUN do_damage_toggle;
DO_FUN do_bug;
DO_FUN do_bullrush;
DO_FUN do_cast;
DO_FUN do_channel;
DO_FUN do_charmiejoin_toggle;
DO_FUN do_check;
DO_FUN do_chpwd;
DO_FUN do_cinfo;
DO_FUN do_circle;
DO_FUN do_clans; 
DO_FUN do_clear;
DO_FUN do_clearaff;
DO_FUN do_climb;
DO_FUN do_close;
DO_FUN do_cmotd;
DO_FUN do_ctax;
DO_FUN do_cdeposit;
DO_FUN do_cwithdraw;
DO_FUN do_cbalance;
DO_FUN do_colors;
DO_FUN do_compact;
DO_FUN do_consent;
DO_FUN do_consider;
DO_FUN do_count;
DO_FUN do_cpromote;
DO_FUN do_crazedassault;
DO_FUN do_credits;
DO_FUN do_cripple;
DO_FUN do_ctell; 
DO_FUN do_deathstroke;
DO_FUN do_deceit;
DO_FUN do_defenders_stance;
DO_FUN do_disarm;
DO_FUN do_disband;
DO_FUN do_disconnect;
DO_FUN do_dmg_eq;
DO_FUN do_donate;
DO_FUN do_dream;
DO_FUN do_drink;
DO_FUN do_drop;
DO_FUN do_eat;
DO_FUN do_eagle_claw;
DO_FUN do_echo;
DO_FUN do_emote;
DO_FUN do_setvote;
DO_FUN do_vote;
DO_FUN do_enter;
DO_FUN do_equipment;
DO_FUN do_eyegouge;
DO_FUN do_examine;
DO_FUN do_exits;
DO_FUN do_export;
DO_FUN do_ferocity;
DO_FUN do_fighting;
DO_FUN do_fill;
DO_FUN do_find;
DO_FUN do_findpath;
DO_FUN do_findPath;
DO_FUN do_fire;
DO_FUN do_flee;
/* DO_FUN  do_fly; */
DO_FUN do_focused_repelance;
DO_FUN do_follow;
DO_FUN do_forage;
DO_FUN do_force;
DO_FUN do_found;
DO_FUN do_free_animal;
DO_FUN do_freeze;
DO_FUN do_fsave;
DO_FUN do_get;
DO_FUN do_give;
DO_FUN do_global;
DO_FUN do_gossip; 
DO_FUN do_golem_score;
DO_FUN do_goto;
DO_FUN do_guild;
DO_FUN do_install;
DO_FUN do_reload_help;
DO_FUN do_hindex;
DO_FUN do_hedit;
DO_FUN do_grab;
DO_FUN do_group;
DO_FUN do_grouptell;
DO_FUN do_gtrans;
DO_FUN do_guard;
DO_FUN do_harmtouch;
DO_FUN do_help;
DO_FUN do_mortal_help;
DO_FUN do_new_help;
DO_FUN do_hide;
DO_FUN do_highfive;
DO_FUN do_hit;
DO_FUN do_hitall;
DO_FUN do_holylite;
DO_FUN do_home;
DO_FUN do_idea;
DO_FUN do_ignore;
DO_FUN do_imotd;
DO_FUN do_imbue;
DO_FUN do_incognito;
DO_FUN do_index;
DO_FUN do_info;
DO_FUN do_initiate;
DO_FUN do_innate;
DO_FUN do_instazone;
DO_FUN do_insult;
DO_FUN do_inventory;
DO_FUN do_join;
DO_FUN do_joinarena;
DO_FUN do_ki;
DO_FUN do_kick;
DO_FUN do_kill;
DO_FUN do_knockback;
// DO_FUN do_land;
DO_FUN do_layhands;
DO_FUN do_leaderboard;
DO_FUN do_leadership;
DO_FUN do_leave;
DO_FUN do_levels;
DO_FUN do_lfg_toggle;
DO_FUN do_linkdead;
DO_FUN do_linkload;
DO_FUN do_listAllPaths;
DO_FUN do_listPathsByZone;
DO_FUN do_listproc;
DO_FUN do_load;
DO_FUN do_medit;
DO_FUN do_memoryleak;
DO_FUN do_mortal_set;
//DO_FUN do_motdload;
DO_FUN do_msave;
DO_FUN do_mpedit;
DO_FUN do_mpbestow;
DO_FUN do_mpstat;
DO_FUN do_opedit;
DO_FUN do_eqmax;
DO_FUN do_opstat;
DO_FUN do_lock;
DO_FUN do_log;
DO_FUN do_look;
DO_FUN do_botcheck;
DO_FUN do_make_camp;
DO_FUN do_matrixinfo;
DO_FUN do_maxes;
DO_FUN do_mlocate;
DO_FUN do_move;
DO_FUN do_motd;
DO_FUN do_mpretval     ;
DO_FUN do_mpasound     ;
DO_FUN do_mpat         ;
DO_FUN do_mpdamage     ;
DO_FUN do_mpecho       ;
DO_FUN do_mpechoaround ;
DO_FUN do_mpechoaroundnotbad;
DO_FUN do_mpechoat     ;
DO_FUN do_mpforce      ;
DO_FUN do_mpgoto       ;
DO_FUN do_mpjunk       ;
DO_FUN do_mpkill       ;
DO_FUN do_mphit;
DO_FUN do_mpsetmath;
DO_FUN do_mpaddlag;
DO_FUN do_mpmload;
DO_FUN do_mpoload;
DO_FUN do_mppause;
DO_FUN do_mppeace;
DO_FUN do_mppurge;
DO_FUN do_mpteachskill;
DO_FUN do_mpsetalign;
DO_FUN do_mpsettemp;
DO_FUN do_mpthrow;
DO_FUN do_mpothrow;
DO_FUN do_mptransfer;
DO_FUN do_mpxpreward;
DO_FUN do_mpteleport;
DO_FUN do_murder;
DO_FUN do_name;
DO_FUN do_natural_selection;
DO_FUN do_newbie;
DO_FUN do_newPath;
DO_FUN do_news;
DO_FUN do_noemote;
DO_FUN do_nohassle;
DO_FUN do_noname;
DO_FUN do_not_here;
DO_FUN do_notax_toggle;
DO_FUN do_guide_toggle;
DO_FUN do_oclone;
DO_FUN do_mclone;
DO_FUN do_oedit;
DO_FUN do_offer;
DO_FUN do_olocate;
DO_FUN do_oneway;
DO_FUN do_onslaught;
DO_FUN do_open;
DO_FUN do_order;
DO_FUN do_orchestrate;
DO_FUN do_osave;
DO_FUN do_outcast;
DO_FUN do_pager;
DO_FUN do_pardon;
DO_FUN do_pathpath;
DO_FUN do_peace;
DO_FUN do_perseverance;
DO_FUN do_pick;
DO_FUN do_plats;
DO_FUN do_pocket;
DO_FUN do_poisonmaking;
DO_FUN do_pour;
DO_FUN do_poof;
DO_FUN do_possess;
DO_FUN do_practice;
DO_FUN do_pray;
DO_FUN do_profession;
DO_FUN do_primalfury;
DO_FUN do_promote;
DO_FUN do_prompt;
DO_FUN do_lastprompt;
DO_FUN do_processes;
DO_FUN do_psay;
//DO_FUN do_pshopedit;
DO_FUN do_pview;
DO_FUN do_punish;
DO_FUN do_purge;
DO_FUN do_purloin;
DO_FUN do_put;
DO_FUN do_qedit;
DO_FUN do_quaff;
DO_FUN do_quest;
DO_FUN do_qui;
DO_FUN do_quivering_palm;
DO_FUN do_quit;
DO_FUN do_rage;
DO_FUN do_range;
DO_FUN do_rdelete;
DO_FUN do_read;
DO_FUN do_recall;
DO_FUN do_recite;
DO_FUN do_redirect;
DO_FUN do_redit;
DO_FUN do_remove;
DO_FUN do_rename_char;
DO_FUN do_rent;
DO_FUN do_reply;
DO_FUN do_repop;
DO_FUN do_report;
DO_FUN do_rescue;
DO_FUN do_rest;
DO_FUN do_restore;
DO_FUN do_retreat;
DO_FUN do_return;
DO_FUN do_revoke;
//DO_FUN do_rload;
DO_FUN do_rsave;
DO_FUN do_rstat;
DO_FUN do_save;
DO_FUN do_say;
DO_FUN do_scan;
DO_FUN do_score;
DO_FUN do_scribe;
DO_FUN do_sector;
DO_FUN do_sedit;
DO_FUN do_send;
DO_FUN do_set;
DO_FUN do_shout;
DO_FUN do_showhunt;
DO_FUN do_skills;
DO_FUN do_social;
DO_FUN do_songs;
DO_FUN do_stromboli;
DO_FUN do_summon_toggle;
DO_FUN do_headbutt;
DO_FUN do_show;
DO_FUN do_show_exp;
DO_FUN do_showbits;
DO_FUN do_shutdow;
DO_FUN do_shutdown;
DO_FUN do_silence;
DO_FUN do_stupid;
DO_FUN do_sing;
DO_FUN do_sip;
DO_FUN do_sit;
DO_FUN do_slay;
DO_FUN do_sleep;
DO_FUN do_slip;
DO_FUN do_smite;
DO_FUN do_sneak;
DO_FUN do_snoop;
DO_FUN do_sockets;
DO_FUN do_spells;
DO_FUN do_split;
DO_FUN do_sqedit;
DO_FUN do_stalk;
DO_FUN do_stand;
DO_FUN do_stat;
DO_FUN do_steal;
DO_FUN do_stealth;
DO_FUN do_story;
DO_FUN do_string;
DO_FUN do_stun;
DO_FUN do_suicide;
DO_FUN do_switch;
DO_FUN do_tactics;
DO_FUN do_tame;
DO_FUN do_tap;
DO_FUN do_taste;
DO_FUN do_teleport;
DO_FUN do_tell;
DO_FUN do_testhand;
DO_FUN do_testhit;
DO_FUN do_testport;
DO_FUN do_testuser;
DO_FUN do_thunder;
DO_FUN do_tick;
DO_FUN do_time;
DO_FUN do_title;
DO_FUN do_toggle;
DO_FUN do_track;
DO_FUN do_trans;
DO_FUN do_triage;
DO_FUN do_trip;
DO_FUN do_trivia;
DO_FUN do_typo;
DO_FUN do_unarchive;
DO_FUN do_unlock;
DO_FUN do_use;
DO_FUN do_varstat;
DO_FUN do_vault;
DO_FUN do_vend;
DO_FUN do_version;
DO_FUN do_visible;
DO_FUN do_vitalstrike;
DO_FUN do_vt100;
DO_FUN do_wake;
DO_FUN do_wear;
DO_FUN do_weather;
DO_FUN do_where;
DO_FUN do_whisper;
DO_FUN do_who;
DO_FUN do_whoarena;
DO_FUN do_whoclan;
DO_FUN do_whogroup;
DO_FUN do_whosolo;
DO_FUN do_wield;
DO_FUN do_fakelog;
DO_FUN do_wimpy;
DO_FUN do_wiz;
DO_FUN do_wizhelp;
DO_FUN do_wizinvis;
DO_FUN do_wizlist;
DO_FUN do_wizlock;
DO_FUN do_write_skillquest;
DO_FUN do_write;
DO_FUN do_zap;
DO_FUN do_zedit;
DO_FUN do_zoneexits;
DO_FUN do_zsave;
DO_FUN do_editor;
DO_FUN do_pursue;


#endif
