#include <structs.h>
#include <db.h>
#include <player.h>
#include <levels.h>
#include <character.h>
#include <spells.h>
#include <utility.h>
#include <fight.h>
#include <mobile.h>
#include <obj.h>
#include <connect.h>
#include <handler.h>
#include <act.h>
#include <interp.h>
#include <returnvals.h>

extern struct index_data *obj_index;

int find_ingredient(CHAR_DATA *ch, char *arg, char *type)
{
  if(!arg[0])
    return 0;

  OBJ_DATA *obj = get_obj_in_list_vis(ch, arg, ch->carrying);
  if (!obj)
  {
    csendf(ch, "You don't seem to have %s\r\n", arg);
    return 0;
  }

  int virt = obj_index[obj->item_number].virt;
  


  return eSUCCESS;
}

int do_scribe_scroll(struct char_data *ch, char *argument, int cmd)
{
  int learned;
  int recipe = 0;
  int ingredient = 0;
  if(IS_MOB(ch))
    ;
  else if(!(learned = has_skill(ch, SKILL_SCRIBE_SCROLL))) 
  {
    send_to_char("You should probably stick with scribing your ABCs....\r\n", ch);
    return eFAILURE;
  }

  if(affected_by_spell(ch, SKILL_SCRIBE_SCROLL)) 
  {
    send_to_char("Your fingers are still too sore from the last time.\r\n", ch);
    return eFAILURE;
  }

//find components here

  if (!skill_success(ch,NULL,SKILL_SCRIBE_SCROLL)) 
  {
    
    return eFAILURE;
  }

  char arg1[MAX_INPUT_LENGTH];
  one_argument(argument, arg1);
  ingredient = find_ingredient(ch, arg1, "paper");
  if(!ingredient)
    return eFAILURE;
  recipe |= ingredient;

  one_argument(argument, arg1);
  ingredient = find_ingredient(ch, arg1, "pen");
  if(!ingredient)
    return eFAILURE;
  recipe |= ingredient;

  one_argument(argument, arg1);
  ingredient = find_ingredient(ch, arg1, "ink");
  if(!ingredient)
    return eFAILURE;
  recipe |= ingredient;
  
  one_argument(argument, arg1);
  ingredient = find_ingredient(ch, arg1, "dust");
  if(!ingredient)
    return eFAILURE;
  recipe |= ingredient;


  return eSUCCESS; 
}
