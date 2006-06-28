/***************************************************************************
 *  file: utils.h, Utility module.                         Part of DIKUMUD *
 *  Usage: Utility macros                                                  *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 *                                                                         *
 *  Revision History                                                       *
 *  10/21/2003   Onager    Changed IS_ANONYMOUS() to handle mobs without   *
 *                         crashing                                        *
 ***************************************************************************/
/* $Id: utility.h,v 1.44 2006/06/28 06:07:48 jhhudso Exp $ */

#ifndef UTILITY_H_
#define UTILITY_H_

extern "C" {
#include <time.h>
#include <stdlib.h>
}

#include "structs.h"
#include "character.h"
#include "weather.h"
#include "memory.h"
#include "player.h"

extern struct weather_data weather_info;
extern char log_buf[MAX_STRING_LENGTH];

//extern struct char_data;
//typedef struct char_data CHAR_DATA;

void check_timer();

#define TRUE  1
#define FALSE 0

#ifdef WIN32
inline int random() { return(rand()); }
char *index(char *buf, char op);
#endif

#define MOB_WAIT_STATE(ch)  ((ch)->deaths)

#define GET_WAIT(ch)  (IS_MOB((ch)) ? (ch)->deaths : ((ch)->desc ? (ch)->desc->wait : 0))

#define WAIT_STATE(czh, cycle)  (((czh)->desc) ? (czh)->desc->wait > (cycle) ? 0 : (czh)->desc->wait = (cycle) :  \
                                  (IS_MOB((czh)) ? MOB_WAIT_STATE((czh)) = (cycle) : 0))


// Defines for gradual skill increase code
// Usage is defined in guild.cpp

#define SKILL_INCREASE_EASY           100
#define SKILL_INCREASE_MEDIUM         200
#define SKILL_INCREASE_HARD           300
void check_timer();

void skill_increase_check(char_data * ch, int skill, int learned, int difficulty);
bool is_hiding(CHAR_DATA *ch, CHAR_DATA *vict);

// End defines for gradual skill increase code


#define REMOVE_FROM_LIST(item, head, next)      \
   if ((item) == (head))                \
      head = (item)->next;              \
   else {                               \
      temp = head;                      \
      while (temp && (temp->next != (item))) \
         temp = temp->next;             \
      if (temp)                         \
         temp->next = (item)->next;     \
   }                                    \

#define MIN(a,b)	((a) < (b) ? (a) : (b))
#define MAX(a,b)	((a) > (b) ? (a) : (b))

#define LOWER(c) (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c) (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c))

//#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r' || (ch) == '|') 
#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')  // replaced to leave off the pipe and put it eclusively in comm.c
                                                   // where we could check to see if we were in an editor first.

#define CAP(st)  (*(st) = UPPER(*(st)), st)

#ifdef LEAK_CHECK
#define CREATE(result, type, number)  do {\
    if (!((result) = (type *) calloc ((number), sizeof(type))))\
	{ perror("calloc failure in CREATE: "); abort(); } } while(0)
#else
#define CREATE(result, type, number)  do {\
    if (!((result) = (type *) dc_alloc ((number), sizeof(type))))\
	{ perror("calloc failure in CREATE: "); abort(); } } while(0)
#endif

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) dc_realloc ((result), sizeof(type) * (number))))\
                { perror("realloc failure in RECREATE"); abort(); } } while(0)

#define FREE(p) do { if((p) != NULL) { dc_free((p)); (p) = 0; } } while (0) 

#define ASIZE 32 //don't change unless you want to be screwed
#define SETBIT(var,bit) ((var)[(bit)/ASIZE] |= (1 << ((bit)-(((bit)/ASIZE)*ASIZE)-1)))
// setting with an OR
#define REMBIT(var,bit) ((var)[(bit)/ASIZE] &= ~(1 << ((bit)-(((bit)/ASIZE)*ASIZE))-1))
// setting with an AND
#define TOGBIT(var,bit) ((var)[(bit)/ASIZE] ^= (1 << ((bit)-(((bit)/ASIZE)*ASIZE))-1))
// setting with an XOR
#define ISSET(var,bit) ((var)[(bit)/ASIZE] & (1 << ((bit)-(((bit)/ASIZE)*ASIZE))-1))
// using an AND

#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) = (var) | (bit))
#define REMOVE_BIT(var,bit)  ((var) = (var) & ~(bit) )
#define TOGGLE_BIT(var, bit) ((var) = (var) ^ (bit))

#define IS_AFFECTED(ch,skill) ( ISSET((ch)->affected_by, (skill)) )

int  DARK_AMOUNT( int room );
bool IS_DARK( int room );
#define IS_LIGHT(room)  (!IS_DARK(room))

#define IS_ARENA(room) (IS_SET(world[room].room_flags, ARENA))

#define HSHR(ch) ((ch)->sex ?                    \
    (((ch)->sex == 1) ? "his" : "her") : "its")
#define HSSH(ch) ((ch)->sex ?                    \
    (((ch)->sex == 1) ? "he" : "she") : "it")
#define HMHR(ch) ((ch)->sex ?                    \
    (((ch)->sex == 1) ? "him" : "her") : "it")   
//#define ANA(obj) (index("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")
//#define SANA(obj) (index("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")

#define IS_NPC(ch)  (IS_SET((ch)->misc, MISC_IS_MOB))
#define IS_MOB(ch)  (IS_NPC(ch))
#define IS_FAMILIAR(ch)	(IS_AFFECTED(ch, AFF_FAMILIAR))

#define GET_RDEATHS(ch)      ((ch)->pcdata->rdeaths)
#define GET_PDEATHS(ch)      ((ch)->pcdata->pdeaths)
#define GET_PKILLS(ch)       ((ch)->pcdata->pkills)
#define GET_PKILLS_TOTAL(ch) ((ch)->pcdata->pklvl)

#define GET_PKILLS_LOGIN(ch)        ((ch)->pcdata->totalpkills)
#define GET_PKILLS_TOTAL_LOGIN(ch)  ((ch)->pcdata->totalpkillslv)
#define GET_PDEATHS_LOGIN(ch)       ((ch)->pcdata->pdeathslogin)

#define GET_GROUP_PKILLS(ch)        ((ch)->pcdata->group_kills)
#define GET_GROUP_PKILLSTOTAL(ch)   ((ch)->pcdata->grplvl)

#define GET_HP_METAS(ch)   ((ch)->hpmetas)
#define GET_MANA_METAS(ch) ((ch)->manametas)
#define GET_MOVE_METAS(ch) ((ch)->movemetas)
#define GET_KI_METAS(ch)   ((ch)->pcdata->kimetas)

#define GET_POS(ch)     ((ch)->position)
#define GET_COND(ch, i) ((ch)->conditions[(i)])
#define GET_NAME(ch)    ((ch)->name)
#define GET_SHORT(ch)   ((ch)->short_desc ? (ch)->short_desc : (ch)->name)
#define GET_SHORT_ONLY(ch)     ((ch)->short_desc)
#define GET_TITLE(ch)   ((ch)->title)
#define GET_LEVEL(ch)   ((ch)->level)

#define GET_RANGE(ch)     ((ch)->pcdata->rooms)
#define GET_MOB_RANGE(ch) ((ch)->pcdata->mobiles)
#define GET_OBJ_RANGE(ch) ((ch)->pcdata->objects)

/* mike stuff */
#define GET_OBJ_RNUM(obj)       ((obj)->item_number)
#define GET_OBJ_VAL(obj, val)   ((obj)->obj_flags.value[(val)])
#define GET_OBJ_VROOM(obj)      ((obj)->vroom)
#define GET_OBJ_EXTRA(obj)      ((obj)->obj_flags.extra_flags)
#define GET_OBJ_TIMER(obj)      ((obj)->obj_flags.timer)
#define GET_OBJ_TYPE(obj)       ((obj)->obj_flags.type_flag)
#define GET_OBJ_WEAR(obj)       ((obj)->obj_flags.wear_flags)
#define GET_OBJ_COST(obj)       ((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)       ((obj)->obj_flags.cost_per_day)
#define GET_OBJ_VNUM(obj)       (GET_OBJ_RNUM(obj) >= 0 ? \
                                 obj_index[GET_OBJ_RNUM(obj)].virt : -1)
#define VALID_ROOM_RNUM(rnum)   ((rnum) != NOWHERE && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
        ((long)(VALID_ROOM_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
/* end mike */

#define GET_PROMPT(ch)  ((ch)->pcdata->prompt)
#define GET_TOGGLES(ch) ((ch)->pcdata->toggles)

#define GET_CLASS(ch)   ((ch)->c_class)
#define GET_HOME(ch)    ((ch)->hometown)
#define GET_AGE(ch)     (age(ch).year)

#define GET_STR(ch)     ((ch)->str)
#define GET_DEX(ch)     ((ch)->dex)
#define GET_INT(ch)     ((ch)->intel)
#define GET_WIS(ch)     ((ch)->wis)
#define GET_CON(ch)     ((ch)->con)

#define GET_STR_BONUS(ch)     ((ch)->str_bonus)
#define GET_DEX_BONUS(ch)     ((ch)->dex_bonus)
#define GET_INT_BONUS(ch)     ((ch)->intel_bonus)
#define GET_WIS_BONUS(ch)     ((ch)->wis_bonus)
#define GET_CON_BONUS(ch)     ((ch)->con_bonus)

#define GET_RAW_STR(ch) ((ch)->raw_str)
#define GET_RAW_DEX(ch) ((ch)->raw_dex)
#define GET_RAW_INT(ch) ((ch)->raw_intel)
#define GET_RAW_WIS(ch) ((ch)->raw_wis)
#define GET_RAW_CON(ch) ((ch)->raw_con)

#define GET_POISON_AMOUNT(ch) ((ch)->poison_amount)

#define STRENGTH_APPLY_INDEX(ch) \
			(GET_STR(ch))

#define GET_AC(ch)       ((ch)->armor)
#define GET_ARMOR(ch)    ((ch)->armor + dex_app[GET_DEX((ch))].ac_mod)
#define GET_HIT(ch)      ((ch)->hit)
#define GET_RAW_HIT(ch)  ((ch)->raw_hit)
#define GET_MAX_HIT(ch)  (hit_limit(ch))
#define GET_MOVE(ch)     ((ch)->move)
#define GET_RAW_MOVE(ch) ((ch)->raw_move)
#define GET_MAX_MOVE(ch) (move_limit(ch))
#define GET_MANA(ch)     ((ch)->mana)
#define GET_RAW_MANA(ch) ((ch)->raw_mana)
#define GET_MAX_MANA(ch) (mana_limit(ch))
#define GET_KI(ch)       ((ch)->ki)
#define GET_RAW_KI(ch)   ((ch)->raw_ki)
#define GET_MAX_KI(ch)   ((ch)->max_ki)
#define GET_GOLD(ch)     ((ch)->gold)
#define GET_PLATINUM(ch) ((ch)->plat)
#define GET_BANK(ch)     ((ch)->pcdata->bank)
#define GET_CLAN(ch)     (GET_LEVEL((ch)) >= IMMORTAL ? 31337 : (ch)->clan)
#define GET_EXP(ch)      ((ch)->exp)
#define GET_HEIGHT(ch)   ((ch)->height)
#define GET_WEIGHT(ch)   ((ch)->weight)
#define GET_SEX(ch)      ((ch)->sex)
#define GET_HITROLL(ch)  ((ch)->hitroll)
#define GET_REAL_HITROLL(ch) ((ch)->hitroll + dex_app[GET_DEX((ch))].tohit)
#define GET_DAMROLL(ch)  ((ch)->damroll)
#define GET_REAL_DAMROLL(ch) ((ch)->damroll + str_app[GET_STR((ch))].todam)
#define GET_QPOINTS(ch)  ((ch)->pcdata->quest_points)

#define GET_RACE(ch)     ((ch)->race)
#define GET_BITV(ch)     ((ch)->race==1?1:(1<<(((ch)->race)-1)))
#define IS_UNDEAD(ch)    ((GET_RACE(ch)==RACE_UNDEAD) || (GET_RACE(ch)==RACE_GHOST))

#define AWAKE(ch) (GET_POS(ch)  != POSITION_SLEEPING)

#define IS_ANONYMOUS(ch) (IS_MOB(ch) ? 1 : ( (GET_LEVEL(ch) >= 101) ? 0 : IS_SET((ch)->pcdata->toggles, PLR_ANONYMOUS)))
/*
inline const short IS_ANONYMOUS(CHAR_DATA *ch)
{
  if (IS_MOB(ch))
     // this should really never be called on mobs
     return 1;
  else if (GET_LEVEL(ch) >= 101)
     return 0;
  else
     return (IS_SET(ch->pcdata->toggles, PLR_ANONYMOUS) != 0);
}
*/
/* Object And Carry related macros */

#define GET_ITEM_TYPE(obj) ((obj)->obj_flags.type_flag)
#define GET_OBJ_WEIGHT(obj) ((obj)->obj_flags.weight)

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags,part))

#define CAN_CARRY_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w)
#define CAN_CARRY_N(ch) (5+GET_DEX(ch))
#define IS_CARRYING_W(ch) ((ch)->carry_weight)
#define IS_CARRYING_N(ch) ((ch)->carry_items)

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))
#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_TAKE) && CAN_CARRY_OBJ((ch),(obj)) &&          \
    CAN_SEE_OBJ((ch),(obj)))

#define IS_OBJ_STAT(obj,stat) (IS_SET((obj)->obj_flags.extra_flags,stat))


/* char name/short_desc(for mobs) or someone?  */

#define PERS(ch, vict)   (                                          \
    GET_LEVEL(ch) > MIN_GOD ?                                       \
    ( CAN_SEE(vict, ch) ?                                           \
       GET_SHORT(ch) : "an immortal presence" ) :                   \
    ( CAN_SEE(vict, ch) ?                                           \
       GET_SHORT(ch) : "someone" ) )

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
    (obj)->short_description  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
    fname((obj)->name) : "something")

#define IS_EXIT(room, door) (world[(room)].dir_option[(door)])
#define EXIT_TO(room, door) (world[(room)].dir_option[(door)]->to_room)
#define IS_OPEN(room, door) (!IS_SET(world[(room)].dir_option[(door)]->exit_info, EX_CLOSED))

#define OUTSIDE(ch) (!IS_SET(world[(ch)->in_room].room_flags,INDOORS))
#define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door])
#define CAN_GO(ch, door) (EXIT(ch,door) \
			  && (EXIT(ch,door)->to_room != NOWHERE) \
			  && !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))

#define GET_ALIGNMENT(ch) ((ch)->alignment)
#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_SINGING(ch) (ch->song_timer > 0)

char *str_hsh(char *);
void double_dollars(char * destination, char * source);

void    clan_death      (char* b, char_data *ch);

int     move_char       (CHAR_DATA *ch, int dest);
int	number		(int from, int to);
int 	dice		(int number, int size);
int	str_cmp		(char *arg1, char *arg2);
int	str_nosp_cmp	(char *arg1, char *arg2);
int     str_n_nosp_cmp  (char *arg1, char *arg2, int size);
char *  str_nospace     (char *stri);
char *	str_dup		(const char *str);
void    log		(char * str, int god_level, long type);
void    logf            (int level, long type, char *arg, ...);
int     send_to_gods    (char * str, int god_level, long type);
void	sprintbit	(uint value[], char *names[], char *result);
void    sprintbit	(unsigned long vektor, char *names[], char *result);
void    sprinttype	(int type, char *names[], char *result);
int     consttype       (char * search_str, char *names[]);
char *  constindex      (int index, char *names[]);
struct time_info_data
	mud_time_passed	(time_t t2, time_t t1);
struct time_info_data
	age		(CHAR_DATA *ch);
bool	circle_follow   (CHAR_DATA *ch, CHAR_DATA *victim);
bool ARE_GROUPED( struct char_data *sub, struct char_data *obj); 
bool ARE_CLANNED( struct char_data *sub, struct char_data *obj); 
int	is_number	(char *str);
void	gain_condition	(CHAR_DATA *ch, int condition, int value);
void	set_fighting	(CHAR_DATA *ch, CHAR_DATA *vict);
void	stop_fighting	(CHAR_DATA *ch, int clearlag = 1);
int	do_simple_move	(CHAR_DATA *ch, int cmd, int following);
// int	attempt_move	(CHAR_DATA *ch, int cmd, int is_retreat = 0);
int32	move_limit	(CHAR_DATA *ch);
int32	mana_limit	(CHAR_DATA *ch);
int32	ki_limit	(CHAR_DATA *ch);
int32	hit_limit	(CHAR_DATA *ch);
int	has_skill	(CHAR_DATA *ch, int16 skill);
char *  get_skill_name  (int skillnum);
void	gain_exp_regardless	(CHAR_DATA *ch, int gain);
void	advance_level	(CHAR_DATA *ch, int is_conversion);
int	close_socket	(struct descriptor_data *d);
char *	one_argument	(char *argument, char *first_arg);
int	isname		(char *arg, char *arg2);
void	page_string	(struct descriptor_data *d, char *str,
			    int keep_internal);
void	gain_exp	(CHAR_DATA *ch, int64 gain);
void	redo_hitpoints  (CHAR_DATA *ch);       /* Rua's put in  */
void	redo_mana	(CHAR_DATA *ch);       /* Rua's put in  */
void    redo_ki		(CHAR_DATA *ch);	/* And Urizen*/
void assign_rooms(void);
void assign_objects(void);
void assign_mobiles(void);
int search_block( char *arg, char **list, bool exact );
void free_obj(struct obj_data *obj);

int char_to_room(CHAR_DATA *ch, int room);
int char_from_room(CHAR_DATA *ch);
void do_start(CHAR_DATA *ch);

void update_pos( CHAR_DATA *victim );
void clear_object(struct obj_data *obj);
void death_cry( CHAR_DATA *ch );

void add_follower( CHAR_DATA *ch, CHAR_DATA *leader, int cmd);
void send_to_outdoor( char *messg );
void send_to_zone(char *messg, int zone);
void weather_and_time(int mode);
void night_watchman( void );
int special(CHAR_DATA *ch, int cmd, char *arg);
int process_output(struct descriptor_data *t);
int file_to_string(const char *name, char *buf);
bool load_char_obj( struct descriptor_data *d, char *name );
void save_char_obj( CHAR_DATA *ch );
void char_to_store(CHAR_DATA *ch, struct char_file_u *st, struct time_data & tmpage);
bool obj_to_store( struct obj_data *obj, CHAR_DATA *ch, FILE *fpsave, int wear_pos );
void check_idling(CHAR_DATA *ch);
void stop_follower(CHAR_DATA *ch, int cmd);
bool CAN_SEE( CHAR_DATA *sub, CHAR_DATA *obj );
int  SWAP_CH_VICT(int value);
bool SOMEONE_DIED(int value);
bool CAN_SEE_OBJ( CHAR_DATA *sub, struct obj_data *obj, bool bf = FALSE);
bool check_blind( CHAR_DATA *ch );
void raw_kill(CHAR_DATA *ch, CHAR_DATA *victim);
void check_killer( CHAR_DATA *ch, CHAR_DATA *victim );
int map_eq_level( CHAR_DATA *mob );
void disarm( CHAR_DATA *ch, CHAR_DATA *victim );
int shop_keeper( CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA * invoker );
void send_to_all(char *messg);
void ansi_color(char *txt, CHAR_DATA *ch);
void send_to_char(char *messg, CHAR_DATA *ch);
void send_to_char_nosp(char *messg, CHAR_DATA *ch);
void send_to_room(char *messg, int room, bool awakeonly = FALSE, CHAR_DATA *nta = NULL);
void record_track_data(CHAR_DATA *ch, int cmd); 
int write_to_descriptor(int desc, char *txt);
int write_to_descriptor_fd(int desc, char *txt);
void write_to_q(char *txt, struct txt_q *queue);
int use_mana( CHAR_DATA *ch, int sn );
void automail(char *name);
bool file_exists(char *);
void util_archive(char *, CHAR_DATA *);
void util_unarchive(char *, CHAR_DATA *);
int is_busy(CHAR_DATA *ch);
int is_ignoring(struct char_data *ch, struct char_data *i);
void colorCharSend(char* s, struct char_data* ch);
void send_to_char_regardless(char *messg, struct char_data *ch);
int csendf(struct char_data *ch, char *arg, ...);
bool check_range_valid_and_convert(int & value, char * buf, int begin, int end);
bool check_valid_and_convert(int & value, char * buf);
void parse_bitstrings_into_int(char * bits[], char * strings, char_data * ch, uint32 value[]);
void parse_bitstrings_into_int(char * bits[], char * strings, char_data * ch, uint32 & value);
void parse_bitstrings_into_int(char * bits[], char * strings, char_data * ch, uint16 & value);
void display_string_list(char * list[], char_data *ch);
int contains_no_trade_item(obj_data * obj);
int contents_cause_unique_problem(obj_data * obj, char_data * vict);

void mob_suprised_sayings(char_data * ch, char_data * aggressor);

// MOBProgs prototypes
int     mprog_wordlist_check    ( char * arg, CHAR_DATA *mob,
                			CHAR_DATA* actor, OBJ_DATA* object,
					void* vo, int type );
void    mprog_percent_check     ( CHAR_DATA *mob, CHAR_DATA* actor,
					OBJ_DATA* object, void* vo,
					int type );
void    mprog_act_trigger       ( char* buf, CHAR_DATA* mob,
		                        CHAR_DATA* ch, OBJ_DATA* obj,
					void* vo );
int     mprog_bribe_trigger     ( CHAR_DATA* mob, CHAR_DATA* ch,
		                        int amount );
int     mprog_entry_trigger     ( CHAR_DATA* mob );
int     mprog_give_trigger      ( CHAR_DATA* mob, CHAR_DATA* ch,
                		        OBJ_DATA* obj );
int     mprog_greet_trigger     ( CHAR_DATA* mob );
int     mprog_fight_trigger     ( CHAR_DATA* mob, CHAR_DATA* ch );
int     mprog_hitprcnt_trigger  ( CHAR_DATA* mob, CHAR_DATA* ch );
int    mprog_death_trigger     ( CHAR_DATA* mob, CHAR_DATA* killer );
int     mprog_random_trigger    ( CHAR_DATA* mob );
int     mprog_arandom_trigger   ( CHAR_DATA *mob);
int     mprog_speech_trigger    ( char* txt, CHAR_DATA* mob );
int 	mprog_catch_trigger	(char_data * mob, int catch_num, char 
*var, int opt, char_data *actor,obj_data *obj, void *vo);
int 	mprog_attack_trigger	(char_data * mob, CHAR_DATA* ch);
int     mprog_load_trigger      (CHAR_DATA *mob);
int oprog_catch_trigger(obj_data *obj, int catch_num, char *var, int opt, char_data *actor, obj_data *obj, void *vo);

#define MAX_THROW_NAME     60
#define MPROG_CATCH_MIN    1
#define MPROG_CATCH_MAX    50

struct mprog_throw_type {
   int target_mob_num;         // num of mob to recieve
   char target_mob_name[MAX_THROW_NAME]; // string used to find target name
   
   int data_num;               // number of catch call to activate on target
   int delay;                  // how long until the mob gets it

   int pitcher;                // vnum of mob that threw the call
   int opt;
   mprog_throw_type * next;
   bool mob;			// Mob or object.
   char *var;			// temporary variable
   CHAR_DATA *actor;
   obj_data *obj;
   void *vo;
};

struct mprog_variable_data {
  char *invoker;
  char *object;
  char *rndm;
  char *voi;
  int nested; // amount of nested ifs, at time of pause
  char *program;
};

int handle_poisoned_weapon_attack(char_data * ch, char_data * vict, int percent);

#endif /* UTILITY_H_ */
