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
#include <magic.h>
#include <affect.h>
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif
#include <handler.h>
// Locals
void advance_golem_level(CHAR_DATA *golem);

// db.cpp
extern struct index_data *obj_index;
extern struct index_data *mob_index;

// save.cpp
int store_worn_eq(char_data * ch, FILE * fpsave);
struct obj_data *  obj_store_to_char(CHAR_DATA *ch, FILE *fpsave, struct obj_data * last_cont );

// limits.cpp
extern int hit_gain(CHAR_DATA *ch);
extern int mana_gain(CHAR_DATA*ch);
extern int ki_gain(CHAR_DATA *ch);
extern int move_gain(CHAR_DATA *ch);

// const.cpp
extern struct race_shit race_info[];


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
  int special_aff; // Special affect(s). (iron)
  int special_res; // Resists(stone).
  int ac; // armor
  char *creation_message;
  char *shatter_message;
  char *release_message;
};

const struct golem_data golem_list[] = {
  {"iron", "iron golem enchanted", "an enchanted iron golem", "A powerfully enchanted iron golem stands here, guarding its master.\r\n", 
    "The iron golem is bound by its master's magics.  A mindless automaton,\r\nthe iron golem is one of the most powerful forces available in \r\nwizard's aresenal.  Nearly a full 8 feet tall and weighing several\r\ntons, this behemoth of pure iron is absolutely loyal to its master and\r\nsilently follows commands without fail.\r\n",
    1000, 15, 5, 25, 50, {107, 108, 109, 0, 7004}, AFF_LIGHTNINGSHIELD, 0, -100,
    "There is a grinding and shrieking of metal as an iron golem is slowly formed.\r\n",
    "Unable to sustain further damage, the iron golem falls into unrecoverable scrap.",
    "As the magic binding it is released, the iron golem rusts to pieces."
},
 {  "stone","stone enchanted golem", "a enchanted stone golem", "A powerfully enchanted stone golem stands here, guarding its master.\r\n",
    "The stone golem is bound by its caster's magics.  A mindless automaton,\r\nthe stone golem is one of the sturdiest and most resilliant creatures\r\nknown in the realms.  Nearly a full 8 feet tall and weighing several\r\ntons, this mountain of rock is absolutely loyal to its master and\r\nsilently follows orders without fail.\r\n",
    2000, 5, 5, 25, 50, {104,105,106,0,7003}, 0, ISR_PIERCE, -100, "There is a deep rumbling as a stone golem slowly rises from the ground.\r\n",
    "Unable to sustain further damage, the stone golem shaters to pieces.",
    "As the magic binding it is released, the golem crumbles to dust."
   }
};

#define MAX_GOLEMS 2 // amount of golems above +1

void shatter_message(CHAR_DATA *ch)
{
   int golemtype = !IS_AFFECTED2(ch, AFF_GOLEM); // 0 or 1
   act(golem_list[golemtype].shatter_message, ch, 0, 0, TO_ROOM,0);
}
void release_message(CHAR_DATA *ch)
{
   int golemtype = !IS_AFFECTED(ch, AFF_GOLEM);
   act(golem_list[golemtype].release_message, ch, 0, 0, TO_ROOM, 0);
}

void golem_gain_exp(CHAR_DATA *ch)
{
//  extern int exp_table[~];
  int level = 19 + ch->level;
  if (ch->level >= 20) return;
  if (ch->exp > exp_table[level])
  {
     ch->exp = 0;
     ch->level++;
     advance_golem_level(ch);
     do_save(ch->master,"",666);
     do_say(ch, "Errrrrhhgg...",0);     
  }
}

int  verify_existing_components(CHAR_DATA *ch, int golemtype)
{ // check_components didn't suit me.
  int i;
  int retval = 0;
  struct obj_data *curr, *next_content;
  char buf[MAX_STRING_LENGTH];
  SET_BIT(retval, eSUCCESS);
  for (i = 0; i < 5; i++)
  {
    if (golem_list[golemtype].components[i] == 0) continue;
    bool found = FALSE;
    for (curr = ch->carrying; curr; curr = next_content)
    {
       next_content = curr->next_content;
       int vnum = obj_index[curr->item_number].virt;
       if (vnum == golem_list[golemtype].components[i])
	{
	  found = TRUE;
	  if (i == 4) SET_BIT(retval, eEXTRA_VALUE); // Special effect.
	}
     }
     if (!found && i != 4) { return eFAILURE; }
  }
  for (i = 0; i<5; i++)
  {
    for (curr = ch->carrying; curr; curr = next_content)
    {
      next_content = curr->next_content;
      if (golem_list[golemtype].components[i] == obj_index[curr->item_number].virt)
      {
	if (number(0,2) || !spellcraft(ch, SPELL_CREATE_GOLEM)) {
          sprintf(buf, "%s explodes, releasing a stream of magical energies!\r\n", curr->short_description);
          send_to_char(buf,ch);
          obj_from_char(curr);
          extract_obj(curr);
	  break; // Only remove ONE of the components of that type.
	} else {
	  sprintf(buf, "%s glows bright red, but you manage to retain it by only extracting part of its magic.\r\n", curr->short_description);
	  send_to_char(buf,ch);
  	  break;
	}
      }
    }
  }
  return retval;
}

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
  fwrite(&(golem->level), sizeof(golem->level), 1, fpfile);
  fwrite(&(golem->exp),   sizeof(golem->exp), 1, fpfile);
  // Use previously defined functions after this.
  obj_to_store(golem->carrying, golem, fpfile, -1);
  store_worn_eq(golem, fpfile);
  dc_fclose(fpfile);
}

void advance_golem_level(CHAR_DATA *golem)
{
  int golemtype = !IS_AFFECTED2(golem, AFF_GOLEM); // 0 or 1
  golem->max_hit = golem->raw_hit = (golem->raw_hit + (golem_list[golemtype].max_hp/20));
  GET_HIT(golem) += golem_list[golemtype].max_hp/20;
  golem->hitroll += golem_list[golemtype].hit / 20;
  golem->damroll += golem_list[golemtype].dam / 20;
  golem->armor += golem_list[golemtype].ac / 20;
  golem->exp = 0;
}

void set_golem(CHAR_DATA *golem, int golemtype)
{ // Set the basics.
        GET_RACE(golem) = RACE_GOLEM;
        golem->short_desc = str_hsh(golem_list[golemtype].short_desc);
        golem->name = str_hsh(golem_list[golemtype].name);
        golem->long_desc = str_hsh(golem_list[golemtype].long_desc);
	golem->description = str_hsh(golem_list[golemtype].description);
        golem->title = 0;
        golem->affected_by2 = 0;
        if (!golemtype)
          SET_BIT(golem->affected_by2, AFF_GOLEM);
        SET_BIT(golem->affected_by, AFF_INFRARED);
        SET_BIT(golem->affected_by2, AFF_STABILITY);
        golem->mobdata->actflags = ACT_2ND_ATTACK;
	golem->misc = MISC_IS_MOB;
        golem->affected_by = 0;
        golem->armor = 0;
        golem->level = 1;
        golem->hitroll = golem_list[golemtype].hit / 20;
        golem->damroll = golem_list[golemtype].dam / 20;
	golem->armor = golem_list[golemtype].ac / 20;
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
  set_golem(golem, golemtype); // Basics  
  ch->pcdata->golem = golem;
  fread(&(golem->level), sizeof(golem->level), 1, fpfile);
  int level = golem->level;
  for ( ; level > 1; level--)
     advance_golem_level(golem); // Level it up again.
  fread(&(golem->exp),   sizeof(golem->exp),  1, fpfile);
   struct obj_data *last_cont; // Last container.
  while(!feof(fpfile)) {
    last_cont = obj_store_to_char( golem, fpfile, last_cont );
  }
  dc_fclose(fpfile);
}

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
  int retval = verify_existing_components(ch,i);
  if (IS_SET(retval, eFAILURE))
  {
      send_to_char("Since you do not have the require spell components, the magic fades into nothingness.\r\n",ch);
      return eFAILURE;
  }
  load_golem_data(ch, i); // Load the golem up;
  skill_increase_check(ch, SPELL_CREATE_GOLEM, skill, SKILL_INCREASE_EASY);
  golem = ch->pcdata->golem;
  if (!golem)
  { // Returns false if something goes wrong. (Not a mage, etc).
    send_to_char("Something goes wrong, and you fail!",ch);
    return eFAILURE;
  }
  if (IS_SET(retval, eEXTRA_VALUE))
  {
    send_to_char("Adding in the final ingredient, your golem increases in strength!\r\n",ch);
    SET_BIT(golem->affected_by, golem_list[i].special_aff);
    SET_BIT(golem->resist, golem_list[i].special_res);
  }
  char_to_room(golem, ch->in_room);
  add_follower(golem, ch, 0);
  SET_BIT(golem->affected_by, AFF_CHARM);
//  struct affected_type af;
  send_to_char(golem_list[i].creation_message,ch);
  return eSUCCESS;
}

extern char frills[];

int do_golem_score(struct char_data *ch, char *argument, int cmd)
{ /* Pretty much a rip of score */
   char race[100];
   char buf[MAX_STRING_LENGTH], scratch;
   int  level = 0;
   int to_dam, to_hit;
   struct char_data *master = ch;
   if (IS_NPC(ch)) return eFAILURE;
   if (!ch->pcdata->golem) 
   {
      send_to_char("But you don't have a golem!\r\n",ch);
      return eFAILURE;
   }
   ch = ch->pcdata->golem;
   struct affected_type *aff;
   extern char *apply_types[];
   extern char *pc_clss_types[];

   int64 exp_needed;

   sprintf(race, "%s", race_info[(int)GET_RACE(ch)].singular_name);
   exp_needed = (int)(exp_table[(int)GET_LEVEL(ch) + 19] - (int64)GET_EXP(ch));

   to_hit = GET_REAL_HITROLL(ch);
   to_dam = GET_REAL_DAMROLL(ch);

   sprintf(buf,
      "$7($5:$7)================================================="
      "=======================($5:$7)\n\r"
      "|=|  %-29s -- Character Attributes (DarkCastleMUD) |=|\n\r"
      "($5:$7)===========================($5:$7)==================="
      "=======================($5:$7)\n\r", GET_SHORT(ch));

   send_to_char(buf, master);

   sprintf(buf,
      "|\\|  $4Strength$7:      %4d (%2d) |/|  $1Race$7:   %-10s $1HitPts$7:%5d$1/$7(%5d) |~|\n\r"
      "|~|  $4Dexterity$7:     %4d (%2d) |o|  $1Class$7:  %-11s$1Mana$7:   %4d$1/$7(%5d) |\\|\n\r"
      "|/|  $4Constitution$7:  %4d (%2d) |\\|  $1Lvl$7:    %-8d   $1Fatigue$7:%4d$1/$7(%5d) |o|\n\r"
      "|o|  $4Intelligence$7:  %4d (%2d) |~|  $1Height$7: %3d        $1Ki$7:     %4d$1/$7(%5d) |/|\n\r"
      "|\\|  $4Wisdom$7:        %4d (%2d) |/|  $1Weight$7: %3d                             |~|\n\r"
      "|~|  $3Rgn$7: $4H$7:%2d $4M$7:%2d $4V$7:%2d $4K$7:%2d |o|  $1Age$7:    %3d yrs    $1Align$7: %+5d         |\\|\n\r",
       GET_STR(ch), GET_RAW_STR(ch), race, GET_HIT(ch), GET_MAX_HIT(ch),
      GET_DEX(ch), GET_RAW_DEX(ch), pc_clss_types[(int)GET_CLASS(ch)], GET_MANA(ch), GET_MAX_MANA(ch),
      GET_CON(ch), GET_RAW_CON(ch), GET_LEVEL(ch), GET_MOVE(ch), GET_MAX_MOVE(ch),
      GET_INT(ch), GET_RAW_INT(ch), GET_HEIGHT(ch), GET_KI(ch),  GET_MAX_KI(ch),
      GET_WIS(ch), GET_RAW_WIS(ch), GET_WEIGHT(ch),
      hit_gain(ch), mana_gain(ch), move_gain(ch), ki_gain(ch), GET_AGE(ch), GET_ALIGNMENT(ch));
   send_to_char(buf, master);

     sprintf(buf,
      "($5:$7)===========================($5:$7)===($5:$7)====================================($5:$7)\n\r"
      "|/|  $2Combat Statistics:$7             |\\|   $2Equipment and Valuables:$7         |o|\n\r"
      "|o|   $3Armor$7:   %5d $3Pkills$7:  %5d |~|    $3Items Carried$7:  %-3d/(%-3d)       |/|\n\r"
      "|\\|   $3BonusHit$7: %+4d $3PDeaths$7: %5d |/|    $3Weight Carried$7: %-3d/(%-4d)      |~|\n\r"
      "|~|   $3BonusDam$7: %+4d $3RDeaths$7: %5d |o|    $3Experience$7:     %-10lld      |\\|\n\r"
      "|/|   $B$4FIRE$R[%+3d] $BCOLD$R[%+3d] $B$5NRGY$R[%+3d] |\\|    $3ExpTillLevel$7:   %-10lld      |o|\n\r"
      "|o|   $B$2ACID$R[%+3d] $B$3MAGK$R[%+3d] $2POIS$7[%+3d] |~|    $3Gold$7: %-9d $3Platinum$7: %-5d |/|\n\r"
      "|\\|   $3MELE$R[%+3d] $3SPEL$R[%+3d] $3SONG$R[%+3d] |/|    $3Bank$7: %-10d                |-|\n\r"
      "($5:$7)=================================($5:$7)====================================($5:$7)\n\r",
   GET_ARMOR(ch),         0,   IS_CARRYING_N(ch), CAN_CARRY_N(ch),
   to_hit, 0, IS_CARRYING_W(ch), CAN_CARRY_W(ch),
   to_dam, 0, GET_EXP(ch),
   get_saves(ch,SAVE_TYPE_FIRE), get_saves(ch, SAVE_TYPE_COLD), get_saves(ch, SAVE_TYPE_ENERGY), GET_LEVEL(ch) == 50 ? 0 : exp_needed,
   get_saves(ch, SAVE_TYPE_ACID), get_saves(ch, SAVE_TYPE_MAGIC), get_saves(ch, SAVE_TYPE_POISON), (int)GET_GOLD(ch), (int)GET_PLATINUM(ch),
   ch->melee_mitigation, ch->spell_mitigation, ch->song_mitigation, 0);
     send_to_char(buf, master);

   if((aff = ch->affected))
   {
      int found = FALSE;

      for( ; aff; aff = aff->next) {
         if(aff->type == SKILL_SNEAK)
            continue;
         scratch = frills[level];
         // figure out the name of the affect (if any)
         char * aff_name = get_skill_name(aff->type);
         switch(aff->type) {
           case FUCK_CANTQUIT:
             aff_name = "CANT_QUIT";
             break;
           case FUCK_PTHIEF:
             aff_name = "DIRTY_DIRTY_THIEF";
             break;
           case SKILL_HARM_TOUCH:
             aff_name = "harmtouch reuse timer";
             break;
           case SKILL_LAY_HANDS:
             aff_name = "layhands reuse timer";
             break;
           case SKILL_QUIVERING_PALM:
             aff_name = "quiver reuse timer";
             break;
           case SKILL_BLOOD_FURY:
             aff_name = "blood fury reuse timer";
             break;
           case SKILL_CRAZED_ASSAULT:
             if (strcmp(apply_types[(int)aff->location], "HITROLL"))
               aff_name = "crazed assault reuse timer";
             break;
           case SPELL_HOLY_AURA_TIMER:
             aff_name = "holy aura timer";
             break;
           default: break;
         }
         if(!aff_name) // not one we want displayed
           continue;

         sprintf(buf, "|%c| Affected by %-22s %s Modifier %-16s  |%c|\n\r",
               scratch, aff_name,
               ((IS_AFFECTED(ch, AFF_DETECT_MAGIC) && aff->duration < 3) ?
                          "$2(fading)$7" : "        "),
               apply_types[(int)aff->location], scratch);
         send_to_char(buf, master);
         found = TRUE;
         if(++level == 4)
            level = 0;
      }
       sprintf(buf, "|%c| Affected by %-22s          Modifier %-16s  |%c|\n\r",
               frills[level],"STABILITY","NONE",frills[level]);
	send_to_char(buf,master);
       sprintf(buf, "|%c| Affected by %-22s          Modifier %-16s  |%c|\n\r",
               frills[level],"INFRARED","NONE",frills[level]);
        send_to_char(buf,master);
       if (IS_SET(ch->affected_by, AFF_LIGHTNINGSHIELD))
       sprintf(buf, "|%c| Affected by %-22s          Modifier %-16s  |%c|\n\r",
               frills[level],"LIGHTNING SHIELD","NONE",frills[level]);
        send_to_char(buf,master);
	if (IS_SET(ch->resist, ISR_PIERCE))
       sprintf(buf, "|%c| Affected by %-22s          Modifier %-16s  |%c|\n\r",
               frills[level],"STONE SKIN","NONE",frills[level]);
        found = TRUE;
     if(found)
        send_to_char(
         "($5:$7)========================================================================($5:$7)\n\r", master);
   }
   return eSUCCESS;
}

int spell_release_golem(byte level, CHAR_DATA *ch, char *arg, int type, CHAR_DATA *tar_ch, struct obj_data *tar_obj, int skill)
{
  struct follow_type *fol;
  for (fol = ch->followers; fol; fol = fol->next)
    if (IS_NPC(fol->follower) && mob_index[fol->follower->mobdata->nr].virt == 8)
    {
      release_message(fol->follower);
      extract_char(fol->follower, FALSE);
      return eSUCCESS;
    }
  send_to_char("You don't have a golem.\r\n",ch);
  return eSUCCESS;
}

