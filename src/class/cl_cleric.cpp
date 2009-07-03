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
#include <map>

using namespace std;

extern struct index_data *obj_index;
extern map<int, int> scribe_recipes;

OBJ_DATA * find_ingredient(CHAR_DATA *ch, char *arg, char *type)
{
  if(!arg[0])
    return NULL;

  OBJ_DATA *obj = get_obj_in_list_vis(ch, arg, ch->carrying);
  if (!obj)
    csendf(ch, "You don't seem to have any %s\r\n", arg);

  //int virt = obj_index[obj->item_number].virt;
  
  return obj;
}

int do_scribe_scroll(struct char_data *ch, char *argument, int cmd)
{
  int learned;
  int recipe = 0;
  OBJ_DATA * paper, *pen, *ink, *dust;
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


  char arg1[MAX_INPUT_LENGTH];
  one_argument(argument, arg1);
  paper = find_ingredient(ch, arg1, "paper");
  if(!paper)
    return eFAILURE;
  ingredient = obj_index[paper->item_number].virt;
  if(!ingredient)
    return eFAILURE;
  recipe |= ingredient;

  one_argument(argument, arg1);
  pen = find_ingredient(ch, arg1, "pen");
  if(!pen)
    return eFAILURE;
  ingredient = obj_index[pen->item_number].virt;
  if(!ingredient)
    return eFAILURE;
  recipe |= ingredient;

  one_argument(argument, arg1);
  ink = find_ingredient(ch, arg1, "ink");
  if(!ink)
    return eFAILURE;
  ingredient = obj_index[ink->item_number].virt;
  if(!ingredient)
    return eFAILURE;
  recipe |= ingredient;
  
  one_argument(argument, arg1);
  dust = find_ingredient(ch, arg1, "dust");
  if(!dust)
    return eFAILURE;
  ingredient = obj_index[dust->item_number].virt;
  if(!ingredient)
    return eFAILURE;
  recipe |= ingredient;

  extract_obj(paper);
  extract_obj(pen);
  extract_obj(ink);
  extract_obj(dust);

  if (!skill_success(ch,NULL,SKILL_SCRIBE_SCROLL)) 
  {
    send_to_char("Your attempt to scribe the scroll has failed!\r\n", ch);
    //TODO: if(!number(0,1)) cast_wild_magic_at_yourself 
    return eFAILURE;
  }


  csendf(ch, "You get a scroll, yippee. recipe %d, spell %d\r\n", recipe, scribe_recipes[recipe]);

  return eSUCCESS; 
}
