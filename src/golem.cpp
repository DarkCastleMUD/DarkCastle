/*
  New golem code. Urizen.
   Separated all this from the rest(by putting it in save.cpp,
   magic.cpp, etc) to have all the golem code in one place.
*/
#include <character.h>
#include <structs.h>
#include <spells.h>
#include <utility.h>
#include <levels.h>
#include <player.h>
#include <db.h>
#include <interp.h>
#include <string.h>
#include <returnvals.h>
#include <ki.h>
#include <fileinfo.h>
#include <mobile.h>
#include <race.h>
#include <act.h>
#include <affect.h>
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

// save.cpp
int store_worn_eq(char_data * ch, FILE * fpsave);
struct obj_data *  obj_store_to_char(CHAR_DATA *ch, FILE *fpsave, struct obj_data * last_cont );

struct golem_data
{ // This is how a golem looks.
  char *keyword;
  char *name;
  char *short_desc;
  char *long_desc;
  char *description;
  int max_hp;
  int roll1, roll2; // Damage maxes
  int dam, hit; // bonus maxes
  int components[5]; // Components(vnums)
  int special_aff; // Special affect(s)
};

const struct golem_data golem_list[] = {
  {"iron", "iron golem", "an iron golem", "An iron golem stands here, awaiting its master's commands. ", 
    "The golem looks inanimate, except when it performs some task for its\nmaster. During those periods it moves with suprising speed.",
    1000, 15, 5, 25, 0, {4, 2, 3, 5, 6}, AFF_LIGHTNINGSHIELD},
 {  "stone","stone golem", "a stone golem", "A stone golem stands here, awaiting its master's commands.",
    "The golem looks inanimate, except when it performs some task for its\nmaster. During those periods it moves with suprising speed.",
    2000, 5, 5, 25, 0, {4,2,3,5,6},0}
};
#define MAX_GOLEMS 2 // amount of golems above +1

void save_golem_data(CHAR_DATA *ch)
{
  char file[200];
  FILE *fpfile = NULL;
  int golemtype = 0;
  if (IS_NPC(ch) || GET_CLASS(ch) != CLASS_MAGIC_USER || !ch->pcdata->golem) return;
  golemtype = !IS_AFFECTED2(ch->pcdata->golem, AFF_GOLEM); // 0 or 1
  sprintf(file,"%s/%c/%s.%d",FAMILIAR_DIR, ch->name[0], ch->name,golemtype);
  if (!(fpfile = dc_fopen(file,"w")))
  {
    log("Error while opening file in save_golem_data[golem.cpp].", ANGEL, LOG_BUG);
    return;
  }
  struct char_data *golem = ch->pcdata->golem; // Just to make the code below cleaner.
  fprintf(fpfile, "#GOLEM\n"); // Just 'cause.
  fprintf(fpfile, "%d %d\n", GET_LEVEL(golem), GET_EXP(golem));  // 
  // Use previously defined functions after this.
  obj_to_store(ch->carrying, ch, fpfile, -1);
  store_worn_eq(ch, fpfile);
  dc_fclose(fpfile);
}

void advance_golem_level(CHAR_DATA *golem)
{
  int golemtype = !IS_AFFECTED2(golem, AFF_GOLEM); // 0 or 1
  golem->max_hit = golem->raw_hit = (golem->raw_hit + (golem_list[golemtype].max_hp/20));
  GET_HIT(golem) += golem_list[golemtype].max_hp/20;
  golem->hitroll += golem_list[golemtype].hit / 20;
  golem->damroll += golem_list[golemtype].dam / 20;
}

void set_golem(CHAR_DATA *golem, int golemtype)
{ // Set the basics.
        clear_char(golem);
        GET_RACE(golem) = RACE_GOLEM;
        golem->short_desc = str_hsh(golem_list[golemtype].short_desc);
        golem->name = str_hsh(golem_list[golemtype].name);
        golem->long_desc = str_hsh(golem_list[golemtype].long_desc);
	golem->description = str_hsh(golem_list[golemtype].description);
        golem->title = 0;
        #ifdef LEAK_CHECK
           golem->mobdata = (mob_data *) calloc(1, sizeof(mob_data));
        #else
           golem->mobdata = (mob_data *) dc_alloc(1, sizeof(mob_data));
        #endif
        golem->affected_by2 = 0;
        if (!golemtype)
          SET_BIT(golem->affected_by2, AFF_GOLEM);
        golem->mobdata->actflags = ACT_2ND_ATTACK;
        golem->affected_by = 0;
        golem->armor = 0;
        golem->level = 1;
        golem->hitroll = golem_list[golemtype].hit / 20;
        golem->damroll = golem_list[golemtype].dam / 20;
        golem->raw_hit = golem->max_hit = golem->hit = golem_list[golemtype].max_hp / 20;
        golem->mobdata->damnodice = golem_list[golemtype].roll1;
        golem->mobdata->damsizedice = golem_list[golemtype].roll2;
        golem->gold = 0;
        golem->plat = 0;
        golem->move = golem->max_move = golem->mana = golem->max_mana = 100;
        golem->mobdata->last_room = -1;
        golem->position = POSITION_STANDING;
        golem->immune = golem->suscept = golem->resist = 0;
        golem->c_class = 0;
        golem->height = 350;
        golem->weight = 530;
}

void load_golem_data(CHAR_DATA *ch, int golemtype)
{
  char file[200];
  FILE *fpfile = NULL;
  struct char_data *golem; 
  if (IS_NPC(ch) || GET_CLASS(ch) != CLASS_MAGIC_USER || ch->pcdata->golem) return;
  if (golemtype < 0 || golemtype > 1) return; // Say what?
  sprintf(file,"%s/%c/%s.%d",FAMILIAR_DIR, ch->name[0], ch->name, golemtype);
  if (!(fpfile = dc_fopen(file,"r")))
  { // No golem. Create a new one.
	golem = clone_mobile(real_mobile(8));
	set_golem(golem, golemtype);
        golem->alignment = ch->alignment;
	ch->pcdata->golem = golem;
        return;
  }
  golem = clone_mobile(real_mobile(8));
//  fread_newline(fpfile); // #GOLEM
  while ((fread_char(fpfile))!='\n'); // read 'til EOL
  set_golem(golem, golemtype); // Basics  
  ch->pcdata->golem = golem;
  int level = fread_int(fpfile, -1000, 40);
  int exp = fread_int(fpfile, 1, LONG_MAX);
  golem->level = level;
  golem->exp = exp;
  for ( ; level > 0; level--)
     advance_golem_level(golem); // Level it up again.
  struct obj_data *last_cont; // Last container.
  while(!feof(fpfile)) {
    last_cont = obj_store_to_char( ch, fpfile, last_cont );
  }  
  dc_fclose(fpfile);
}

int check_components(CHAR_DATA *ch, int destroy, int item_one = 0,
                     int item_two = 0, int item_three = 0, int item_four = 0); // magic.cpp

int cast_create_golem(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  struct char_data *golem;
  int i;
  char buf[MAX_INPUT_LENGTH];
  arg = one_argument(arg, buf);
  if (IS_NPC(ch)) return eFAILURE;
  if (ch->pcdata->golem)
  {
    send_to_char("You already have a golem.\r\n",ch);
    return eFAILURE;
  }
  for (i = 0; i < MAX_GOLEMS; i++)
  {
    if (!str_cmp(golem_list[i].keyword, buf))
      break;
  }
  if (i >= MAX_GOLEMS) 
  {
    send_to_char("You cannot create any such golem.\r\n",ch);
    return eFAILURE;
  }
  if (!check_components(ch, TRUE, golem_list[i].components[0], golem_list[i].components[1], 
				golem_list[i].components[2]) ||
	!check_components(ch, TRUE, golem_list[i].components[3],0,0))
  {
      send_to_char("Since you do not have the require spell components, the magic fades into nothingness.\r\n",ch);
      return eFAILURE;
  }
  load_golem_data(ch, i); // Load the golem up;
  golem = ch->pcdata->golem;
  if (!golem)
  {
    send_to_char("Something goes wrong, and you fail!",ch);
    return eFAILURE;
  }
  add_follower(golem,ch,0);
  SET_BIT(golem->affected_by, AFF_CHARM);
  struct affected_type af;
  send_to_char("Whow man, you like made a golem.",ch);
  return eSUCCESS;
}
