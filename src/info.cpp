/***************************************************************************
*	file: act_info.c , Implementation of commands.		 Part of DIKUMUD *
*	Usage : Informative commands. 						*
*	Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*									  	*
*	Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
*	Performance optimization and bug fixes by MERC Industries.		  *
*	You can use our stuff in any way you like whatsoever so long as ths	 *
*	copyright notice remains intact.  If you like it please drop a line	  *
*	to mec@garnet.berkeley.edu.						  *
*										  *
*	This is free software and you are benefitting.	We hope that you	  *
*	share your changes too.  What goes around, comes around. 		  *
***************************************************************************/
/* $Id: info.cpp,v 1.13 2002/08/11 14:53:56 pirahna Exp $ */
extern "C"
{
#include <ctype.h>
#include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <structs.h>
#include <room.h>
#include <character.h>
#include <obj.h>
#include <utility.h>
#include <terminal.h>
#include <player.h>
#include <levels.h>
#include <mobile.h>
#include <clan.h>
#include <handler.h>
#include <db.h> // exp_table
#include <interp.h>
#include <connect.h>
#include <spells.h>
#include <act.h>
#include <returnvals.h>


/* extern variables */

extern CWorld world;


extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern char credits[MAX_STRING_LENGTH];
extern char news[MAX_STRING_LENGTH];
extern char info[MAX_STRING_LENGTH];
extern char story[MAX_STRING_LENGTH];
extern char *dirs[]; 
extern char *where[];
extern char *color_liquid[];
extern char *fullness[];
extern char *sector_types[];
extern char *room_bits[];
extern struct str_app_type str_app[];
extern struct race_shit race_info[];

/* Used for "who" */
extern int max_who;

/* extern functions */

void colorCharSend(char* s, struct char_data* ch);
struct char_data *get_pc_vis(struct char_data *ch, char *name);
struct time_info_data age(struct char_data *ch);
void page_string(struct descriptor_data *d, char *str, int keep_internal);
struct clan_data * get_clan(struct char_data *);
char *str_str(char *first, char *second);

/* intern functions */

void list_obj_to_char(struct obj_data *list,struct char_data *ch, int mode, bool show);

/* Procedures related to 'look' */

void argument_split_3(char *argument, char *first_arg, char *second_arg, char *third_arg) {
   int look_at, begin;
   begin = 0;
   
   /* Find first non blank */
   for ( ;*(argument + begin ) == ' ' ; begin++);
   
   /* Find length of first word */
   for (look_at=0; *(argument+begin+look_at) > ' ' ; look_at++)
      /* Make all letters lower case, AND copy them to first_arg */
      *(first_arg + look_at) = LOWER(*(argument + begin + look_at));

   *(first_arg + look_at) = '\0';
   begin += look_at;
   
   /* Find first non blank */
   for ( ;*(argument + begin ) == ' ' ; begin++);
   
   /* Find length of second word */
   for ( look_at=0; *(argument+begin+look_at)> ' ' ; look_at++)
      /* Make all letters lower case, AND copy them to second_arg */
      *(second_arg + look_at) = LOWER(*(argument + begin + look_at));

   *(second_arg + look_at)='\0';
   begin += look_at;
   
   /* Find first non blank */
   for ( ;*(argument + begin ) == ' ' ; begin++);
   
   /* Find length of second word */
   for ( look_at=0; *(argument+begin+look_at)> ' ' ; look_at++)
      /* Make all letters lower case, AND copy them to second_arg */
      *(third_arg + look_at) = LOWER(*(argument + begin + look_at));

   *(third_arg + look_at)='\0';
   begin += look_at;
}

struct obj_data *get_object_in_equip_vis(struct char_data *ch,
                char *arg, struct obj_data *equipment[], int *j) {
   
   for ((*j) = 0; (*j) < MAX_WEAR ; (*j)++)
      if (equipment[(*j)])
         if (CAN_SEE_OBJ(ch,equipment[(*j)]))
            if (isname(arg, equipment[(*j)]->name))
               return(equipment[(*j)]);
            
   return (0);
}




char *find_ex_description(char *word, struct extra_descr_data *list)
{
   struct extra_descr_data *i;
   
   for (i = list; i; i = i->next)
      if (isname(word, i->keyword))
         return(i->description);
      
   return(0);
}

void show_obj_to_char(struct obj_data *object, struct char_data *ch, int mode)
{
   char buffer[MAX_STRING_LENGTH];
   char flagbuf[MAX_STRING_LENGTH];
   int found = 0;
   int percent;

   // Don't show NO_NOTICE items in a room with "look" unless they have holylite
   if(mode == 0 && IS_SET(object->obj_flags.more_flags, ITEM_NONOTICE) &&
      (ch->pcdata && !ch->pcdata->holyLite))
     return;
   
   buffer[0] = '\0';
   if ((mode == 0) && object->description)
      strcpy(buffer,object->description);
   else	  if (object->short_description && ((mode == 1) ||
      (mode == 2) || (mode==3) || (mode == 4))) 
      strcpy(buffer,object->short_description);
   else if (mode == 5) {
      if (object->obj_flags.type_flag == ITEM_NOTE)
      {
         if (object->action_description)
         {
            strcpy(buffer, "There is something written upon it:\n\r\n\r");
            strcat(buffer, object->action_description);
            page_string(ch->desc, buffer, 1);
         }
         else act("It's blank.", ch,0,0,TO_CHAR, 0);
         return;
      }
      else if((object->obj_flags.type_flag != ITEM_DRINKCON))
      {
         strcpy(buffer,"You see nothing special.");
      }
      else /* ITEM_TYPE == ITEM_DRINKCON */
      {
         strcpy(buffer, "It looks like a drink container.");
      }
   }
   
   if (mode != 3)
   {
      if(mode == 0) // 'look' 
         strcat(buffer, "$R"); // setup color background
      
      strcpy(flagbuf, " $B($R");
      
      if (IS_OBJ_STAT(object,ITEM_INVISIBLE)) {
         strcat(flagbuf,"Invisible");
         found++;
      }
      if (IS_OBJ_STAT(object,ITEM_MAGIC) && IS_AFFECTED(ch,AFF_DETECT_MAGIC)) {
         if(found) strcat(flagbuf, "$B/$R");
         strcat(flagbuf,"Blue Glow");
         found++;
      }
      if (IS_OBJ_STAT(object,ITEM_GLOW)) {
         if(found) strcat(flagbuf, "$B/$R");
         strcat(flagbuf,"Glowing");
         found++;
      }
      if (IS_OBJ_STAT(object,ITEM_HUM)) {
         if(found) strcat(flagbuf, "$B/$R");
         strcat(flagbuf,"Humming");
         found++;
      }
      if(mode == 0 && IS_SET(object->obj_flags.more_flags, ITEM_NONOTICE)) {
         if(found) strcat(flagbuf, "$B/$R");
         strcat(flagbuf,"NO_NOTICE");
         found++;
      }
      if(found) {
         strcat(flagbuf, "$B)$R");
         strcat(buffer, flagbuf);
      }
      
      /* show object's condition if is an armor...  */
      if (object->obj_flags.type_flag == ITEM_ARMOR || 
          object->obj_flags.type_flag == ITEM_WEAPON ||
          object->obj_flags.type_flag == ITEM_FIREWEAPON ||
          object->obj_flags.type_flag == ITEM_CONTAINER ||
          object->obj_flags.type_flag == ITEM_INSTRUMENT
         )
      {
         percent = 100 - (int)(100 * ((float)eq_current_damage(object) / (float)eq_max_damage(object)));
                  
         if (percent >= 100) 
            strcat(buffer, " [$B$2Excellent$R]");
         else if (percent >= 80)
            strcat(buffer, " [$2Good$R]");
         else if (percent >= 60)
            strcat(buffer, " [$3Decent$R]");
         else if (percent >= 40)
            strcat(buffer, " [$B$5Damaged$R]");
         else if (percent >= 20)
            strcat(buffer, " [$4Quite Damaged$R]");
         else if (percent >= 0)
            strcat(buffer, " [$B$4Falling Apart$R]");
         else strcat(buffer, " [$5Pile of Scraps$R]");
      }
      
      if(mode == 0) // 'look' 
         strcat(buffer, "$B$1"); // setup color background
   }
   
   strcat(buffer, "\n\r");
   page_string(ch->desc, buffer, 1);
}

void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, 
                      bool show)
{
   struct obj_data *i;
   bool found = FALSE;
   int number = 1;
   int can_see;
   char buf[50];
   
   for(i = list ; i ; i = i->next_content) { 
      if((can_see = CAN_SEE_OBJ(ch, i)) && i->next_content && 
         i->next_content->item_number == i->item_number && i->item_number != -1) {
         number++;
         continue;
      }
      if(can_see || number > 1) {
         if(number > 1) {
            sprintf(buf, "[%d] ", number);
            send_to_char(buf, ch);
         } 
         show_obj_to_char(i, ch, mode);
         found = TRUE;
         number = 1; 
      }	  
   }	
   
   if((!found) && (show))
      send_to_char("Nothing\n\r", ch);
}



void show_char_to_char(struct char_data *i, struct char_data *ch, int mode)
{
   char buffer[MAX_STRING_LENGTH];
   char send_buf[MAX_STRING_LENGTH];
   int j, found, percent, has_spells;
   struct obj_data *tmp_obj;
   char buf2[MAX_STRING_LENGTH];
   struct clan_data * clan;
   char buf[200];
   
   if(mode == 0) 
   {
      if (!CAN_SEE(ch,i)) {
         if (IS_AFFECTED(ch, AFF_SENSE_LIFE))
            colorCharSend("$R$7You sense a hidden life form in the room.\n\r", ch);
         return;
      }
      colorCharSend("$B$3", ch);
      
      if (!(i->long_desc)||(IS_MOB(i) && (GET_POS(i) != i->mobdata->default_pos)))
      {
         /* A char without long descr, or not in default pos. */
         if(!IS_NPC(i)) { 
            *buffer = '\0';
            if(!i->desc)
               strcpy(buffer, "*linkdead*  ");
            strcat(buffer, GET_SHORT(i));
            if((GET_LEVEL(i) < OVERSEER) && i->clan && (clan = get_clan(i))) { 
               if(!IS_MOB(i) & !IS_SET(ch->pcdata->toggles, PLR_BRIEF)) { 
                  sprintf(buf, " %s [%s]", GET_TITLE(i), clan->name);
                  strcat(buffer, buf);
               }
               else {
                  sprintf(buf, " the %s [%s]", 
                     race_info[(int)GET_RACE(i)].singular_name, clan->name); 
                  strcat(buffer, buf);
               }
            }
            else {
               if(!IS_MOB(ch) && !IS_SET(ch->pcdata->toggles, PLR_BRIEF)) {
                  strcat(buffer," ");
                  strcat(buffer, GET_TITLE(i));
               }
               else {
                  strcat(buffer, " the ");
                  sprintf(buf2, "%s", race_info[(int)GET_RACE(i)].singular_name);
                  strcat(buffer,buf2);
               }
            } 
            
         }
         if (IS_NPC(i))
         {
            strcat(buffer, " ");
            strcpy(buffer, i->short_desc);
            (void) CAP(buffer);
         }
         
         
         switch(GET_POS(i))
         {
         case POSITION_STUNNED  : 
            strcat(buffer," is on the ground, stunned."); break;
         case POSITION_DEAD	  : 
            strcat(buffer," is lying here, dead."); break;
         case POSITION_STANDING : 
            strcat(buffer," is here."); break;
         case POSITION_SITTING  : 
            strcat(buffer," is sitting here.");  break;
         case POSITION_RESTING  : 
            strcat(buffer," is resting here.");  break;
         case POSITION_SLEEPING : 
            strcat(buffer," is sleeping here."); break;
         case POSITION_FIGHTING :
            if (i->fighting) {
               
               strcat(buffer," is here, fighting ");
               if (i->fighting == ch)
                  strcat(buffer,"YOU!");
               else {
                  if (i->in_room == i->fighting->in_room)
                     strcat(buffer, GET_SHORT(i->fighting));
                  else
                     strcat(buffer, "someone who has already left.");
               }
            } else /* NIL fighting pointer */
               strcat(buffer," is here struggling with thin air.");
            break;
         default : strcat(buffer," is floating here."); break;
         }
         if ( IS_AFFECTED(i,AFF_INVISIBLE))
            strcat(buffer," $1(invisible)");
         if (IS_AFFECTED(ch, AFF_DETECT_EVIL) && IS_EVIL(i)) 
            strcat(buffer, "$R$4(Red Aura)");
         if (IS_AFFECTED(ch, AFF_DETECT_GOOD) && IS_GOOD(i)) 
            strcat(buffer, "$B$5(Halo) ");
         strcat(buffer,"\n\r");
         colorCharSend(buffer, ch);
         
         sprintf(send_buf, "$B$7-$1%s has: ", i->name);
         has_spells = 0;
         
         if (IS_AFFECTED(i,AFF_SANCTUARY)) {
            sprintf(send_buf, "%s$7white aura! ", send_buf);
            has_spells = 1;
         }
         if (IS_AFFECTED(i,AFF_PROTECT_EVIL)) {
            sprintf(send_buf, "%s$R$6pulsing aura! ", send_buf);
            has_spells = 1;
         }
         if (IS_AFFECTED(i,AFF_FIRESHIELD)) {
            sprintf(send_buf, "%s$B$4flames! ", send_buf);
            has_spells = 1;
         }
         if (IS_AFFECTED(i,AFF_LIGHTNINGSHIELD)) {
            sprintf(send_buf, "%s$B$5electricity! ", send_buf);
            has_spells = 1;
         }
         if (IS_AFFECTED(i,AFF_PARALYSIS)) {
            sprintf(send_buf,"%s$R$2paralyze! ", send_buf);
            has_spells = 1;
         }
         if (i->affected_by & AFF_FLYING) {
            sprintf(send_buf, "%s$B$1flying!", send_buf);
            has_spells = 1;
         }
         
         if(has_spells) {
            sprintf(send_buf, "%s$R\r\n", send_buf);
            colorCharSend(send_buf, ch);
         }
         
     } else  /* npc with long */ {
        if (IS_AFFECTED(i,AFF_INVISIBLE))
           strcpy(buffer,"$B$7*$3");
        else
           *buffer = '\0';
        
        if (IS_AFFECTED(ch, AFF_DETECT_EVIL)) {
           if (IS_EVIL(i))
              strcat(buffer, "$R$4(Red Aura)$B$3 ");
        }
        
        if (IS_AFFECTED(ch, AFF_DETECT_GOOD)) {
           if (IS_GOOD(i))
              strcat(buffer, "$B$5(Halo)$3 ");
        }
        
        strcat(buffer, i->long_desc);
        colorCharSend(buffer, ch);
        
        //Code changes to use one-line spells
        
        sprintf(send_buf, "$B$7-$1%s has: ", i->short_desc);
        has_spells = 0;
        
        if (IS_AFFECTED(i,AFF_SANCTUARY)) {
           sprintf(send_buf, "%s$B$7white aura! ", send_buf);
           has_spells = 1;
        }
        if (IS_AFFECTED(i,AFF_PROTECT_EVIL)) {
           sprintf(send_buf, "%s$B$6pulsing aura! ", send_buf);
           has_spells = 1;
        }
        if (IS_AFFECTED(i,AFF_FIRESHIELD)) {
           sprintf(send_buf, "%s$B$4flames! ", send_buf);
           has_spells = 1;
        }
        if (IS_AFFECTED(i, AFF_LIGHTNINGSHIELD)) {
           sprintf(send_buf, "%s$B$5electricity! ", send_buf);
           has_spells = 1;
        }
        if (IS_AFFECTED(i,AFF_PARALYSIS)) {
           sprintf(send_buf,"%s$R$2paralyze! ", send_buf);
           has_spells = 1;
        }
        if (i->affected_by & AFF_FLYING) {
           sprintf(send_buf, "%s$B$1flying!", send_buf);
           has_spells = 1;
        }
        if(has_spells) {
           sprintf(send_buf, "%s\r\n", send_buf);
           colorCharSend(send_buf, ch);
        }
        colorCharSend("$R$7", ch);
        
     }
     
   } else if ((mode == 1) || (mode == 3)) 
   {
       
       if(mode == 1)
       {
          if (i->description)
             send_to_char(i->description, ch);
          else
          {
             act("You see nothing special about $m.", i,0,ch, TO_VICT, 0);
          }
       }
       
       /* Show a character to another */
       
       if (GET_MAX_HIT(i) > 0)
          percent = (100*GET_HIT(i))/GET_MAX_HIT(i);
       else
          percent = -1; /* How could MAX_HIT be < 1?? */
       
       strcpy(buffer, GET_SHORT(i));
       
       if (percent >= 100)
          strcat(buffer, " is in excellent condition.\n\r");
       else if (percent >= 90)
          strcat(buffer, " has a few scratches.\n\r");
       else if (percent >= 75)
          strcat(buffer, " is slightly hurt.\n\r");
       else if (percent >= 50)
          strcat(buffer, " is fairly fucked up.\n\r");
       else if (percent >= 30)
          strcat(buffer, " is bleeding freely.\n\r");
       else if (percent >= 15)
          strcat(buffer, " is covered in blood.\n\r");
       else if (percent >= 0)
          strcat(buffer, " is near death.\n\r");
       else
          strcat(buffer, " is suffering from a slow death.\n\r");
       
       send_to_char(buffer, ch);

       if(mode == 3) // If it was a glance, show spells then get out
       {
          sprintf(send_buf, "-%s has: ", i->short_desc);
          has_spells = 0;
        
          if (IS_AFFECTED(i,AFF_SANCTUARY)) {
             sprintf(send_buf, "%s$B$7white aura! ", send_buf);
             has_spells = 1;
          }
          if (IS_AFFECTED(i,AFF_PROTECT_EVIL)) {
             sprintf(send_buf, "%s$6pulsing aura! ", send_buf);
             has_spells = 1;
          }
          if (IS_AFFECTED(i,AFF_FIRESHIELD)) {
             sprintf(send_buf, "%s$B$4flames! ", send_buf);
             has_spells = 1;
          }
          if (IS_AFFECTED(i, AFF_LIGHTNINGSHIELD)) {
             sprintf(send_buf, "%s$B$5electricity! ", send_buf);
             has_spells = 1;
          }
          if (IS_AFFECTED(i,AFF_PARALYSIS)) {
             sprintf(send_buf,"%s$R$2paralyze! ", send_buf);
             has_spells = 1;
          }
          if (i->affected_by & AFF_FLYING) {
             sprintf(send_buf, "%s$B$1flying!", send_buf);
             has_spells = 1;
          }
          if(has_spells) {
             sprintf(send_buf, "%s$R\r\n", send_buf);
             colorCharSend(send_buf, ch);
          }
          return;
       }

       found = FALSE;
       for (j=0; j< MAX_WEAR; j++)
       {
          if (i->equipment[j])
          {
             if (CAN_SEE_OBJ(ch,i->equipment[j]))
             {
                found = TRUE;
             }
          }
       }
       if (found)
       {
          act("\n\r$n is using:", i, 0, ch, TO_VICT, 0);
          act("<    worn     > Item Description     (Flags) [Item Condition]\r\n", i, 0, ch, TO_VICT, 0);
          for (j=0; j< MAX_WEAR; j++)
          {
             if (i->equipment[j])
             {
                if (CAN_SEE_OBJ(ch,i->equipment[j]))
                {
                   send_to_char(where[j],ch);
                   show_obj_to_char(i->equipment[j],ch,1);
                }
             }
          }
       }
       if ((GET_CLASS(ch) == CLASS_THIEF && ch != i) || GET_LEVEL(ch) > IMMORTAL)
       {
          found = FALSE;
          send_to_char("\n\rYou attempt to peek at the inventory:\n\r", ch);
          for(tmp_obj = i->carrying; tmp_obj;
          tmp_obj = tmp_obj->next_content) {
             if (CAN_SEE_OBJ(ch, tmp_obj) && number(0,MORTAL) < GET_LEVEL(ch))
             {
                show_obj_to_char(tmp_obj, ch, 1);
                found = TRUE;
             }
          }
          if (!found)
             send_to_char("You can't see anything.\n\r", ch);
       }
       
    }
    else if (mode == 2)
    {
       
       /* Lists inventory */
       act("$n is carrying:", i, 0, ch, TO_VICT, 0);
       list_obj_to_char(i->carrying,ch,1,TRUE);
    }
}



void list_char_to_char(struct char_data *list, struct char_data *ch, int mode)
{
   struct char_data *i;
   int known;

   for (i = list; i ; i = i->next_in_room) {
      if (ch == i)
         continue;
      if(!IS_MOB(i) && (i->pcdata->wizinvis > GET_LEVEL(ch)))
         continue;
      if ( IS_AFFECTED(ch, AFF_SENSE_LIFE) || CAN_SEE(ch, i) )
         show_char_to_char(i, ch, 0);
      else if (IS_DARK(ch->in_room))
      {
         if((known = has_skill(ch, SKILL_BLINDFIGHTING)) && known > number(1, 101))
            send_to_char("Your blindfighting awareness alerts you to a presense in the area.\n\r", ch);
         else if(number(1, 10) == 1)
            send_to_char("$B$4You see a pair of glowing red eyes looking your way.$R$7\n\r", ch);
      }
   }
}

void try_to_peek_into_container(struct char_data *vict, struct char_data *ch, 
                                char * container)
{
   struct obj_data * obj = NULL;
   struct obj_data * cont = NULL;
   int found = FALSE;
   
   if(GET_CLASS(ch) != CLASS_THIEF && GET_LEVEL(ch) < DEITY) {
      send_to_char("They might object to you trying to look in their pockets...", ch);
      return;
   }
   
   if(!(cont = get_obj_in_list_vis(ch, container, vict->carrying)) ||
      number(0,MORTAL+30) > GET_LEVEL(ch))
   {
      send_to_char("You cannot see a container named that to peek into.\r\n", ch);
      return;
   }
   
   if(GET_ITEM_TYPE(cont) != ITEM_CONTAINER)
   {
      send_to_char("It's not a container....\r\n", ch);
      return;
   }
   
   char buf[200];
   sprintf(buf, "You attempt to peek into the %s.\r\n", cont->short_description);
   send_to_char(buf, ch);
   
   if(IS_SET(cont->obj_flags.value[1], CONT_CLOSED)) {
      send_to_char("It is closed.\r\n", ch);
      return;
   }
   
   for(obj = cont->contains; obj; obj = obj->next_content)
      if (CAN_SEE_OBJ(ch, obj) && number(0,MORTAL+30) < GET_LEVEL(ch))
      {
         show_obj_to_char(obj, ch, 1);
         found = TRUE;
      }

   if(!found)
      send_to_char("You don't see anything inside it.\r\n", ch);
}

int do_look(struct char_data *ch, char *argument, int cmd)
{
   char buffer[MAX_STRING_LENGTH];
   char arg1[MAX_STRING_LENGTH];
   char arg2[MAX_STRING_LENGTH];
   char arg3[MAX_STRING_LENGTH];
   char tmpbuf[MAX_STRING_LENGTH];
   int keyword_no;
   int j, bits, temp;
   int door, original_loc;
   bool found;
   struct obj_data *tmp_object, *found_object;
   struct char_data *tmp_char;
   char *tmp_desc;
   static char *keywords[]= { 
      "north",
      "east",
      "south",
      "west",
      "up",
      "down",
      "in",
      "at",
      "out",
      "through",
      "",  /* Look at '' case */
      "\n" };
      
      if (!ch->desc)
         return 1;
      if (GET_POS(ch) < POSITION_SLEEPING)
         send_to_char("You can't see anything but stars!\n\r", ch);
      else if (GET_POS(ch) == POSITION_SLEEPING)
         send_to_char("You can't see anything, you're sleeping!\n\r", ch);
      else if ( check_blind(ch) );
      else if ( IS_DARK(ch->in_room) && (!IS_MOB(ch) && !ch->pcdata->holyLite))
      {
         send_to_char("It is pitch black...\n\r", ch);
         list_char_to_char(world[ch->in_room].people, ch, 0);
         send_to_char("$R", ch);
      }
      else
      {
         argument_split_3(argument,arg1,arg2,arg3);
         keyword_no = search_block(arg1, keywords, FALSE); /* Partiel Match */
         
         if ((keyword_no == -1) && *arg1)
         {
            keyword_no = 7;
            strcpy(arg2, arg1); /* Let arg2 become the target object (arg1) */
         }
         
         found = FALSE;
         tmp_object = 0;
         tmp_char 	 = 0;
         tmp_desc 	 = 0;
         
         original_loc = ch->in_room;
         switch(keyword_no)
         {
            /* look <dir> */
         case 0 :
         case 1 :
         case 2 : 
         case 3 : 
         case 4 :
         case 5 :
         {
            if (EXIT(ch, keyword_no)) 
            {
               if (EXIT(ch, keyword_no)->general_description &&
                   strlen(EXIT(ch, keyword_no)->general_description))
               {
                  send_to_char(EXIT(ch, keyword_no)->general_description, ch);
               }
               else
               {
                  send_to_char("You see nothing special.\n\r", ch);
               }
               
               if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED) && 
                  !IS_SET(EXIT(ch, keyword_no)->exit_info, EX_HIDDEN) &&
                  (EXIT(ch, keyword_no)->keyword)) {
                  sprintf(buffer, "The %s is closed.\n\r",
                     fname(EXIT(ch, keyword_no)->keyword));
                  send_to_char(buffer, ch);
               }
               else
               {
                  if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_ISDOOR) &&
                     !IS_SET(EXIT(ch, keyword_no)->exit_info, EX_HIDDEN) &&
                     EXIT(ch, keyword_no)->keyword)
                  {
                     sprintf(buffer, "The %s is open.\n\r",
                        fname(EXIT(ch, keyword_no)->keyword));
                     send_to_char(buffer, ch);
                  }
               }
            }
            else
            {
               send_to_char("You see nothing special.\n\r", ch);
            }
         }
         break;
         
         /* look 'in'	 */
         case 6:
         {
            if (*arg2)
            {
               /* Item carried */
               
               bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM |
                  FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
               
               if (bits)
               { /* Found something */
                  if (GET_ITEM_TYPE(tmp_object)== ITEM_DRINKCON)
                  {
                     if (tmp_object->obj_flags.value[1] <= 0)
                     {
                        act("It is empty.", ch, 0, 0, TO_CHAR, 0);
                     }
                     else
                     {
                        temp=((tmp_object->obj_flags.value[1]*3)
                           /tmp_object->obj_flags.value[0]);
                        sprintf(buffer,
                           "It's %sfull of a %s liquid.\n\r",
                           fullness[temp],
                           color_liquid[tmp_object->
                           obj_flags.value[2]]);
                        send_to_char(buffer, ch);
                     }
                  }
                  else if (GET_ITEM_TYPE(tmp_object) == ITEM_CONTAINER)
                  {
                     if (!IS_SET(tmp_object->obj_flags.value[1],
                        CONT_CLOSED))
                     {
                        send_to_char(fname(tmp_object->name), ch);
                        switch (bits)
                        {
                        case FIND_OBJ_INV :
                           send_to_char(" (carried) : \n\r", ch);
                           break;
                        case FIND_OBJ_ROOM :
                           send_to_char(" (here) : \n\r", ch);
                           break;
                        case FIND_OBJ_EQUIP :
                           send_to_char(" (used) : \n\r", ch);
                           break;
                        }
                        list_obj_to_char(tmp_object->contains,
                           ch, 2, TRUE);
                     }
                     else
                        send_to_char("It is closed.\n\r", ch);
                  }
                  else
                  {
                     send_to_char("That is not a container.\n\r", ch);
                  }
               } else
               { /* wrong argument */
                  send_to_char("You do not see that item here.\n\r", ch);
               }
            }
            else
            { /* no argument */
               send_to_char("Look in what?!\n\r", ch);
            }
         }
         break;
         
         /* look 'at'	 */
         case 7 :
         {
            if (*arg2)
            {
               
               bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM |
                  FIND_OBJ_EQUIP | FIND_CHAR_ROOM,
                  ch, &tmp_char, &found_object);
               
               if (tmp_char)
               {
                  if (GET_LEVEL(tmp_char) == IMP && GET_LEVEL(ch) < IMP && !IS_NPC(tmp_char))
                  {
                     csendf(ch, "%s has a face like thunder.  A terrible, powerful,"
                        " apparition.\n\rYou are frightened by the greatness before "
                        "you!\n\r", GET_SHORT(tmp_char));
                     csendf (tmp_char, "Heh, %s just tried to look at you.\n\r", GET_NAME(ch));
                     act("$n starts shaking and SCREAMS in terror!", ch, 0, 0, TO_ROOM, 0);
                     do_flee(ch, "", 0);
                     return eSUCCESS;
                  }
                  if(*arg3)
                  {
                     try_to_peek_into_container(tmp_char, ch, arg3);
                     return eSUCCESS;
                  }
                  if(cmd == 20) show_char_to_char(tmp_char, ch, 3);
                  else			  show_char_to_char(tmp_char, ch, 1);
                  if (ch != tmp_char)
                  {
                     if (!IS_MOB(ch) && (GET_LEVEL(tmp_char) < ch->pcdata->wizinvis)) {
                        return eSUCCESS;
                     }
                     if((cmd == 20) && !IS_AFFECTED(ch, AFF_HIDE)) { 
                        act("$n glances at you.",
                           ch, 0, tmp_char, TO_VICT, INVIS_NULL);
                        act("$n glances at $N.",
                           ch, 0, tmp_char, TO_ROOM, INVIS_NULL|NOTVICT);
                     }
                     else if(!IS_AFFECTED(ch, AFF_HIDE)) { 
                        act("$n looks at you.",
                           ch, 0, tmp_char, TO_VICT, INVIS_NULL);
                        act("$n looks at $N.",
                           ch, 0, tmp_char, TO_ROOM, INVIS_NULL|NOTVICT);
                     }
                  }
                  return eSUCCESS;
               }
               
               /* Search for Extra Descriptions in room and items */
               
               /* Extra description in room?? */
               if (!found)
               {
                  tmp_desc = find_ex_description(arg2, 
                     world[ch->in_room].ex_description);
                  if (tmp_desc)
                  {
                     page_string(ch->desc, tmp_desc, 0);
                     return eSUCCESS; /* RETURN SINCE IT WAS ROOM DESCRIPTION */
                     /* Old system was: found = TRUE; */
                  }
               }
               
               /* Search for extra descriptions in items */
               
               /* Equipment Used */
               
               if (!found)
               {
                  for (j = 0; j< MAX_WEAR && !found; j++)
                  {
                     if (ch->equipment[j])
                     {
                        if (CAN_SEE_OBJ(ch,ch->equipment[j])) 
                        {
                           tmp_desc = find_ex_description(arg2, 
                              ch->equipment[j]->ex_description);
                           if (tmp_desc)
                           {
                              page_string(ch->desc, tmp_desc, 1);
                              found = TRUE;
                           }
                        }
                     }
                  }
               }
               
               /* In inventory */
               
               if (!found)
               {
                  for(tmp_object = ch->carrying; 
                  tmp_object && !found; 
                  tmp_object = tmp_object->next_content)
                  {
                     if (CAN_SEE_OBJ(ch, tmp_object))
                     {
                        tmp_desc = find_ex_description(arg2, 
                           tmp_object->ex_description);
                        if (tmp_desc)
                        {
                           page_string(ch->desc, tmp_desc, 1);
                           found = TRUE;
                        }
                     }
                  }
               }
               
               /* Object In room */
               
               if (!found)
               {
                  for(tmp_object = world[ch->in_room].contents; 
                  tmp_object && !found; 
                  tmp_object = tmp_object->next_content)
                  {
                     if (CAN_SEE_OBJ(ch, tmp_object))
                     {
                        tmp_desc = find_ex_description(arg2, 
                           tmp_object->ex_description);
                        if (tmp_desc)
                        {
                           page_string(ch->desc, tmp_desc, 1);
                           found = TRUE;
                        }
                     }
                  }
               }
               /* wrong argument */
               
               if (bits)
               { /* If an object was found */
                  if (!found)
                     /* Show no-description */
                     show_obj_to_char(found_object, ch, 5);
                  else
                     /* Find hum, glow etc */
                     show_obj_to_char(found_object, ch, 6);
               }
               else if (!found)
               {
                  send_to_char("You do not see that here.\n\r", ch);
               }
            }
            else
            {
               /* no argument */
               send_to_char("Look at what?\n\r", ch);
            }
         }
         break;
         case 8:
            {
               for(tmp_object = object_list; tmp_object; tmp_object=tmp_object->next)
               {
                  if(((tmp_object->obj_flags.type_flag == ITEM_PORTAL) && 
                     (tmp_object->obj_flags.value[2] == world[ch->in_room].zone) &&
                     (tmp_object->in_room) &&
                     (tmp_object->obj_flags.value[1] == 1))
                     || 
                     ((tmp_object->obj_flags.type_flag == ITEM_PORTAL) &&
                     (tmp_object->obj_flags.value[0] == world[ch->in_room].number) &&
                     (tmp_object->in_room > -1) &&
                     (tmp_object->obj_flags.value[1] == 1)))
                  {
                     ch->in_room = tmp_object->in_room;
                     found = TRUE;
                     break;
                  }
               }
               if (found !=TRUE)
               {
                  send_to_char("Nothing much to see there.\n\r", ch);
                  return eFAILURE;
               }
            }
            
         case 9:
            {
               if(found != TRUE)
               { 
                  // Why is this line here? -pir
                  // tmp_object = get_obj_in_list_vis(ch, arg2, world[ch->in_room].contents);
                  if(*arg2)
                  {  
                     if((tmp_object = get_obj_in_list_vis(ch, arg2, world[ch->in_room].contents))) 
                     {
                        if(tmp_object->obj_flags.type_flag == ITEM_PORTAL)
                        {
                           if(tmp_object->obj_flags.value[1] == 0)
                           {
                              sprintf(tmpbuf,"You look through %s but it seems to be opaque.\n\r",
                                 tmp_object->short_description);
                              send_to_char(tmpbuf, ch);
                              return eFAILURE;
                           }
                           if(-1 == (ch->in_room = real_room(tmp_object->obj_flags.value[0])))
                             ch->in_room = original_loc;
                           else found = TRUE;
                        }
                     } else {
                        send_to_char("Look through what?\n\r", ch);
                        return eFAILURE;
                     }
                  }
               } 
               
               if(found != TRUE)
               { 
                  send_to_char("You can't seem to look through that.\n\r", ch);
                  return eFAILURE; 
               }
            }
            
            
            
            /* look ''		*/
         case 10:
            {
               char sector_buf[50];
               char rflag_buf[150];
               
               ansi_color( GREY, ch );
               ansi_color( BOLD, ch );
               //			send_to_char(world[ch->in_room].name, ch);
               // allow the use of color
               colorCharSend(world[ch->in_room].name, ch);
               ansi_color( NTEXT, ch );
               ansi_color( GREY, ch );
               
               // PUT SECTOR AND ROOMFLAG STUFF HERE
               if(!IS_MOB(ch) && ch->pcdata->holyLite) {
                  sprinttype(world[ch->in_room].sector_type, sector_types, sector_buf);
                  sprintbit((long)world[ch->in_room].room_flags, room_bits, rflag_buf);
                  csendf(ch, " Light[%d] <%s> [ %s]", DARK_AMOUNT(ch->in_room), sector_buf, rflag_buf);
               }
               
               send_to_char("\n\r", ch);
               
               if (!IS_MOB(ch) && !IS_SET(ch->pcdata->toggles, PLR_BRIEF))
                  //		send_to_char(world[ch->in_room].description, ch);
                  // allow room descs to have color
                  colorCharSend(world[ch->in_room].description, ch);
               
               ansi_color( BLUE, ch );
               ansi_color( BOLD, ch );
               list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE);
               
               list_char_to_char(world[ch->in_room].people, ch, 0);
               
               strcpy(buffer, ""); *buffer = '\0';
               for(int doorj = 0; doorj <= 5; doorj++) {

                  int is_closed;
                  int is_hidden;

                  // cheesy way of making it list west before east in 'look'
		  if(doorj == 1)           door = 3;
                  else if(doorj == 3)      door = 1;
                  else                     door = doorj;

                  if(!EXIT(ch, door) || EXIT(ch, door)->to_room == NOWHERE)
                     continue;
                  is_closed = IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED);
                  is_hidden = IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN);               

                  if(IS_MOB(ch) || ch->pcdata->holyLite)
                  {
                    if (is_closed && is_hidden)
                      sprintf(buffer + strlen(buffer),"$B($R%s-closed$B)$R ", keywords[door]);
                    else sprintf(buffer + strlen(buffer),"%s%s ",
                        keywords[door], is_closed ? "-closed" : "");
                  }
                  else if(!(is_closed && is_hidden))
                      sprintf(buffer + strlen(buffer),"%s%s ",
                        keywords[door], is_closed ? "-closed" : "");

               }
               ansi_color( NTEXT, ch );
               send_to_char("Exits: ", ch);
               if (*buffer)
                  send_to_char(buffer, ch);
               else
                  send_to_char("None.", ch);
               send_to_char("\n\r", ch);  
               if(!IS_NPC(ch) && ch->hunting) 
                  do_track(ch, ch->hunting, 10); 
            }
            ch->in_room = original_loc;
            break;
            
            /* wrong arg 	*/
         case -1 : 
            send_to_char("Sorry, I didn't understand that!\n\r", ch);
            break;
   }
   
   ansi_color( NTEXT, ch);}
   ansi_color( GREY, ch );
   return eSUCCESS;
}

/* end of look */




int do_read(struct char_data *ch, char *arg, int cmd)
{
   char buf[200];
   
   // This is just for now - To be changed later.!
   
   // yeah right.  -Sadus
   sprintf(buf, "at %s", arg);
   do_look(ch, buf, 15);
   return eSUCCESS;
}



int do_examine(struct char_data *ch, char *argument, int cmd)
{
   char name[200], buf[200];
   int bits;
   struct char_data *tmp_char;
   struct obj_data *tmp_object;
   
   sprintf(buf,"at %s",argument);
   do_look(ch,buf,15);
   
   one_argument(argument, name);
   
   if (!*name)
   {
      send_to_char("Examine what?\n\r", ch);
      return eFAILURE;
   }
   
   bits = generic_find(name, FIND_OBJ_INV | FIND_OBJ_ROOM |
      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
   
   if (tmp_object) {
      if ((GET_ITEM_TYPE(tmp_object) == ITEM_DRINKCON) ||
         (GET_ITEM_TYPE(tmp_object) == ITEM_CONTAINER)) {
         send_to_char("When you look inside, you see:\n\r", ch);
         sprintf(buf, "in %s", argument);
         do_look(ch, buf, 15);
      }
   }
   return eSUCCESS;
}



int do_exits(struct char_data *ch, char *argument, int cmd)
{
   int door;
   char buf[MAX_STRING_LENGTH];
   char *exits[] = {   
      "North",
         "East ",
         "South",
         "West ",
         "Up   ",
         "Down "
   };
   
   *buf = '\0';
   
   if ( check_blind(ch) )
      return eFAILURE;
   
   for (door = 0; door <= 5; door++) {
      if (!EXIT(ch, door) || EXIT(ch, door)->to_room == NOWHERE)
         continue;
      
      if(!IS_MOB(ch) && ch->pcdata->holyLite)
         sprintf(buf + strlen(buf), "%s - %s [%d]\n\r", exits[door],
         world[EXIT(ch, door)->to_room].name,
         world[EXIT(ch, door)->to_room].number);
      else if(IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
         if(IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN))
            continue;
         else
            sprintf(buf + strlen(buf), "%s - (Closed)\n\r", exits[door]);
      }
      else if (IS_DARK(EXIT(ch, door)->to_room))
         sprintf(buf + strlen(buf), "%s - Too dark to tell\n\r", exits[door]);
      else
         sprintf(buf + strlen(buf), "%s leads to %s.\n\r", exits[door],
         world[EXIT(ch, door)->to_room].name);
   }
   
   send_to_char("You scan around the exits to see where they lead.\n\r", ch);
   
   if(buf)
      send_to_char(buf, ch);
   else send_to_char("None.\n\r", ch);

   return eSUCCESS;
}

char frills[] = {
   '\\',
   'o',
   '/',
   '~'
};

int do_score(struct char_data *ch, char *argument, int cmd)
{
   char race[100];
   char buf[MAX_STRING_LENGTH], scratch;
   int  level = 0;
   int to_dam, to_hit;
   
   extern struct str_app_type str_app[];
   
   struct affected_type *aff;
   extern char *apply_types[];
   extern char *pc_clss_types[];
   
   int exp_needed;
   
   sprintf(race, "%s", race_info[(int)GET_RACE(ch)].singular_name);
   exp_needed = (int)(exp_table[(int)GET_LEVEL(ch) + 1] - (long)GET_EXP(ch));
   
   to_hit = ( str_app[STRENGTH_APPLY_INDEX(ch)].tohit ) + ( GET_HITROLL(ch));
   to_dam = ( str_app[STRENGTH_APPLY_INDEX(ch)].todam ) + ( GET_DAMROLL(ch));
   
   
   sprintf(buf,
      "$7($5:$7)================================================="
      "=======================($5:$7)\n\r"
      "|=|  %-28s -- Character Attributes (DarkCastleMUD)  |=|\n\r"
      "($5:$7)===========================($5:$7)==================="
      "=======================($5:$7)\n\r", GET_SHORT(ch));
   
   colorCharSend(buf, ch);
   
   sprintf(buf,
      "|\\|  $4Strength$7:     %9d  |/|  $1Race$7:   %-10s $1HitPts$7:%5d$1/$7(%5d) |~|\n\r"
      "|~|  $4Dexterity$7:    %9d  |o|  $1Class$7:  %-11s$1Mana$7:   %4d$1/$7(%5d) |\\|\n\r"
      "|/|  $4Constitution$7: %9d  |\\|  $1Lvl$7:    %-8d   $1Fatigue$7:%4d$1/$7(%5d) |o|\n\r"
      "|o|  $4Intelligence$7: %9d  |~|  $1Height$7: %3d        $1Ki$7:     %4d$1/$7(%5d) |/|\n\r"
      "|\\|  $4Wisdom$7:       %9d  |/|  $1Weight$7: %3d        $1Alignment$7: %-9d |~|\n\r"
      "|~|                           |o|  $1Age$7:    %3d years                       |\\|\n\r",
      GET_STR(ch), race, GET_HIT(ch), GET_MAX_HIT(ch),
      GET_DEX(ch), pc_clss_types[(int)GET_CLASS(ch)], GET_MANA(ch), GET_MAX_MANA(ch),
      GET_CON(ch), GET_LEVEL(ch), GET_MOVE(ch), GET_MAX_MOVE(ch),
      GET_INT(ch), GET_HEIGHT(ch), GET_KI(ch),  GET_MAX_KI(ch), 
      GET_WIS(ch), GET_WEIGHT(ch), GET_ALIGNMENT(ch),
      GET_AGE(ch));
   colorCharSend(buf, ch);

   if(!IS_NPC(ch)) // mobs can't view this part
   {
     sprintf(buf,
      "($5:$7)===========================($5:$7)===($5:$7)====================================($5:$7)\n\r"
      "|/|  $2Combat Statistics...$7           |\\|   $2Equipment and Valuables$7          |o|\n\r"
      "|o|   $3Armor$7:   %5d $3Pkills$7:  %5d |~|    $3Items Carried$7:  %-3d/(%-3d)       |/|\n\r"
      "|\\|   $3RDeaths$7: %5d $3PDeaths$7: %5d |/|    $3Weight Carried$7: %-3d/(%-4d)      |~|\n\r"
      "|~|   $3BonusHit$7: %+4d $3BonusDam$7: %+4d |o|    $3Experience$7:     %-10d      |\\|\n\r"
      "|/|   $B$4FIRE$R[%+3d] $BCOLD$R[%+3d] $B$5NRGY$R[%+3d] |\\|    $3ExpTillLevel$7:   %-10d      |o|\n\r"
      "|o|   $B$2ACID$R[%+3d] $B$3MAGK$R[%+3d] $2POIS$7[%+3d] |~|    $3Gold$7: %-9d $3Platinum$7: %-5d |/|\n\r"
      "($5:$7)=================================($5:$7)====================================($5:$7)\n\r",
   GET_AC(ch), 	  GET_PKILLS(ch),   IS_CARRYING_N(ch), CAN_CARRY_N(ch),
   GET_RDEATHS(ch), GET_PDEATHS(ch),  IS_CARRYING_W(ch), CAN_CARRY_W(ch),
   to_hit, to_dam, GET_EXP(ch),
   ch->saves[SAVE_TYPE_FIRE], ch->saves[SAVE_TYPE_COLD], ch->saves[SAVE_TYPE_ENERGY], GET_LEVEL(ch) == IMP ? 0 : exp_needed, 
   ch->saves[SAVE_TYPE_ACID], ch->saves[SAVE_TYPE_MAGIC], ch->saves[SAVE_TYPE_POISON], (int)GET_GOLD(ch), (int)GET_PLATINUM(ch));
   
     colorCharSend(buf, ch);
   }
   else send_to_char(
      "($5:$7)=================================($5:$7)===================================($5:$7)\n\r", ch);

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
           case SKILL_HARM_TOUCH:
             aff_name = "harmtouch reuse timer";
             break;
           case SKILL_LAY_HANDS:
             aff_name = "layhands reuse timer";
             break;
           case SKILL_BLOOD_FURY:
             aff_name = "blood fury reuse timer";
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
         colorCharSend(buf, ch);
         found = TRUE;
         if(++level == 4)
            level = 0;
      }
      if(found)
        colorCharSend(
         "($5:$7)========================================================================($5:$7)\n\r", ch);
   }
   if(!IS_NPC(ch)) // mob can't view this part
   {
      if(GET_RANGE(ch) && GET_LEVEL(ch) > IMMORTAL) {
         sprintf(buf, "R RANGE: %s\n\r", GET_RANGE(ch));
         send_to_char(buf, ch);
      }
      if(GET_MOB_RANGE(ch) && GET_LEVEL(ch) > IMMORTAL) {
         sprintf(buf, "M RANGE: %s\n\r", GET_MOB_RANGE(ch));
         send_to_char(buf, ch);
      }
      if(GET_OBJ_RANGE(ch) && GET_LEVEL(ch) > IMMORTAL) {
         sprintf(buf, "O RANGE: %s\n\r", GET_OBJ_RANGE(ch));
         send_to_char(buf, ch);
      }
   }
   return eSUCCESS;
}


int do_time(struct char_data *ch, char *argument, int cmd)
{
   char buf[100], *suf;
   int weekday, day;
   long ct;
   long h,m;
   // long s;
   extern struct time_info_data time_info;
   extern char *weekdays[];
   extern char *month_name[];
   struct tm *pTime = NULL;
   
   /* 35 days in a month */
   weekday = ((35*time_info.month)+time_info.day+1) % 7;
   
   sprintf(buf, "It is %d o'clock %s, on %s.\n\r",
      ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
      ((time_info.hours >= 12) ? "pm" : "am"),
      weekdays[weekday]);
   
   send_to_char(buf,ch);
   
   day = time_info.day + 1;	/* day in [1..35] */
   
   if (day == 1)
      suf = "st";
   else if (day == 2)
      suf = "nd";
   else if (day == 3)
      suf = "rd";
   else if (day < 20)
      suf = "th";
   else if ((day % 10) == 1)
      suf = "st";
   else if ((day % 10) == 2)
      suf = "nd";
   else if ((day % 10) == 3)
      suf = "rd";
   else
      suf = "th";
   
   sprintf(buf, "The %d%s Day of the %s, Year %d.  (game time)\n\r",
      day,
      suf,
      month_name[time_info.month],
      time_info.year);
   
   send_to_char(buf,ch);
   
   // Changed to the below code without seconds in an attempt to stop
   // the timing of bingos... - pir 2/7/1999
   ct = time(0);
   // 	sprintf( buf, "The system time is %s", ctime(&ct) );
   
   // 	send_to_char(buf, ch);
   
   pTime = localtime(&ct);
   if(!pTime)
      return eFAILURE;
   
   sprintf(buf, "The system time is %d/%d/%02d (%d:%02d)\n\r",
      (pTime->tm_mon)+1,
      (pTime->tm_mday),
      ((pTime->tm_year) % 100), // mod 100 fixes yXk bug -pir
      (pTime->tm_hour),
      (pTime->tm_min));
   send_to_char(buf, ch);
   
   ct -= start_time;
   h = ct / 3600;
   m = (ct % 3600) / 60;
   // 	s = ct % 60;
   // 	sprintf (buf, "The mud has been running for: %02li:%02li:%02li \n\r", 
   // 			h,m,s);
   sprintf (buf, "The mud has been running for: %02li:%02li \n\r", h,m);
   send_to_char (buf, ch);
   return eSUCCESS;
}


int do_weather(struct char_data *ch, char *argument, int cmd)
{
   extern struct weather_data weather_info;
   char buf[256];
   static char *sky_look[4]= {
      "cloudless",
      "cloudy",
      "rainy",
      "lit by flashes of lightning"};
      
   if (OUTSIDE(ch)) {
      sprintf(buf, 
         "The sky is %s and %s.\n\r",
         sky_look[weather_info.sky],
         (weather_info.change >=0 ? "you feel a warm wind from south" :
      "your foot tells you bad weather is due"));
      send_to_char(buf,ch);
   } else
      send_to_char("You have no feeling about the weather at all.\n\r", ch);
   return eSUCCESS;
}


int do_help(struct char_data *ch, char *argument, int cmd)
{
   extern int top_of_helpt;
   extern struct help_index_element *help_index;
   extern FILE *help_fl;
   extern char help[MAX_STRING_LENGTH];
   
   int chk, bot, top, mid;
   char buf[90], buffer[MAX_STRING_LENGTH];
   
   if (!ch->desc)
      return eFAILURE;
   
   for(;isspace(*argument); argument++)  ;
   
   
   if (*argument)
   {
      if (!help_index)
      {
         send_to_char("No help available.\n\r", ch);
         return eSUCCESS;
      }
      bot = 0;
      top = top_of_helpt;
      
      for (;;)
      {
         mid = (bot + top) / 2;
         
         if (!(chk = str_cmp(argument, help_index[mid].keyword)))
         {
            fseek(help_fl, help_index[mid].pos, 0);
            *buffer = '\0';
            for (;;)
            {
               fgets(buf, 80, help_fl);
               if (*buf == '#')
                  break;
               buf[80] = 0;
               if((strlen(buffer) + strlen(buf)) >= MAX_STRING_LENGTH)
                  break;
               strcat(buffer, buf);
               strcat(buffer, "\r");
            }
            page_string(ch->desc, buffer, 1);
            return eSUCCESS;
         }
         else if (bot >= top)
         {
            send_to_char("There is no help on that word.\n\r", ch);
            return 1;
         }
         else if (chk > 0)
            bot = ++mid;
         else
            top = --mid;
      }
   }   
   
   send_to_char(help, ch);
   return eSUCCESS;
}

int do_count(struct char_data *ch, char *arg, int cmd)
{
   struct descriptor_data *d;
   struct char_data *i;
   int clss[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   int immortal = 0;
   int total = 0;
   
   for(d = descriptor_list; d; d = d->next) {
      if(d->connected || !d->character)
         continue;
      if(!(i = d->original))
         i = d->character;
      if(!CAN_SEE(ch, i))
         continue;
      if(GET_LEVEL(i) > MORTAL) {
         immortal++;
         total++;
         continue;
      }
      clss[(int)GET_CLASS(i)]++;
      total++;
   }
   
   if(total > max_who)
      max_who = total;
   
   csendf(ch, "There are %d visible players connected, %d of which are immortals.\n\r", total, immortal);
   csendf(ch, "%d warriors, %d clerics, %d mages, %d thieves, %d barbarians, %d monks,\n\r", clss[CLASS_WARRIOR], clss[CLASS_CLERIC], clss[CLASS_MAGIC_USER], clss[CLASS_THIEF], clss[CLASS_BARBARIAN], clss[CLASS_MONK]);
   csendf(ch, "%d paladins, %d antipaladins, %d bards, %d druids, and %d rangers.\n\r",
      clss[CLASS_PALADIN], clss[CLASS_ANTI_PAL], clss[CLASS_BARD], clss[CLASS_DRUID], clss[CLASS_RANGER]);
   csendf(ch, "The maximum number of players since "
      "last reboot was %d.\n\r", max_who); 
   return eSUCCESS;
}


int do_inventory(struct char_data *ch, char *argument, int cmd)
{
   send_to_char("You are carrying:\n\r", ch);
   list_obj_to_char(ch->carrying, ch, 1, TRUE);
   return eSUCCESS;
}


int do_equipment(struct char_data *ch, char *argument, int cmd)
{
   int j;
   bool found;
   
   send_to_char("You are using:\n\r", ch);
   found = FALSE;
   for (j=0; j< MAX_WEAR; j++) {
      if (ch->equipment[j]) {
         if (CAN_SEE_OBJ(ch,ch->equipment[j])) {
            if(!found) {
               act("<    worn     > Item Description     (Flags) [Item Condition]\r\n", ch, 0, 0, TO_CHAR, 0);
               found = TRUE;
            }
            send_to_char(where[j],ch);
            show_obj_to_char(ch->equipment[j],ch,1);
         } else {
            send_to_char(where[j],ch);
            send_to_char("something\n\r",ch);
            found = TRUE;
         }
      }
   }
   if(!found) {
      send_to_char(" Nothing.\n\r", ch);
   }
   return eSUCCESS;
}


int do_credits(struct char_data *ch, char *argument, int cmd)
{
   page_string(ch->desc, credits, 0);
   return eSUCCESS;
}


int do_story(struct char_data *ch, char *argument, int cmd)
{
   page_string(ch->desc, story, 0);
   return eSUCCESS;
}

int do_news(struct char_data *ch, char *argument, int cmd) 
{   
   page_string(ch->desc, news, 0);
   return eSUCCESS;
}


int do_info(struct char_data *ch, char *argument, int cmd) 
{
   page_string(ch->desc, info, 0);
   return eSUCCESS;
}


/*********------------ locate objects -----------------***************/
int do_olocate(struct char_data *ch, char *name, int cmd)
{
   char buf[300], buf2[MAX_STRING_LENGTH];
   struct obj_data *k;
   int in_room = -1, count = 0;
   int vnum = 0;
   int searchnum;
   
   buf2[0] = '\0';
   if(isdigit(*name))
   {
      vnum = atoi(name);
      searchnum = real_object(vnum);
   }

   send_to_char("-#-- Short Description ------- Room Number\n\n\r", ch);
   
   for(k = object_list; k; k = k->next) {
      
      // allow search by vnum
      if(vnum)
      {
         if(k->item_number != searchnum)
            continue;
      }
      else if (!(isname(name, k->name)))
         continue;
      
      if (!CAN_SEE_OBJ(ch, k))
         continue;
      
      buf[0] = '\0';
      
      if(k->in_obj) {
         if(k->in_obj->in_room > -1)
           in_room = world[k->in_obj->in_room].number;
         else if(k->in_obj->carried_by) { 
            if(!CAN_SEE(ch, k->in_obj->carried_by))
               continue;
            in_room = world[k->in_obj->carried_by->in_room].number;
         }
         else if(k->in_obj->equipped_by) { 
            if(!CAN_SEE(ch, k->in_obj->equipped_by))
               continue;
            in_room = world[k->in_obj->equipped_by->in_room].number;
         }
      }
      else if(k->carried_by) { 
         if(!CAN_SEE(ch, k->carried_by))
            continue;
         in_room = world[k->carried_by->in_room].number;
      }
      else if(k->equipped_by) { 
         if(!CAN_SEE(ch, k->equipped_by))
            continue;
         in_room = world[k->equipped_by->in_room].number;
      }
      else if(k->in_room > (-1))
         in_room = world[k->in_room].number;
      else
         in_room = -1;
      
      count++;
      
      if(in_room != -1)
         sprintf(buf, "[%2d] %-26s %d", count, k->short_description, in_room);
      else
         sprintf(buf, "[%2d] %-26s %s", count, k->short_description,
         "(Item at NOWHERE.)");
      
      if(k->in_obj) {
         strcat(buf, " in ");
         strcat(buf, k->in_obj->short_description);
         if(k->in_obj->carried_by) {
            strcat(buf, " carried by ");
            strcat(buf, GET_NAME(k->in_obj->carried_by));
         }
         else if(k->in_obj->equipped_by) {
            strcat(buf, " equipped by ");
            strcat(buf, GET_NAME(k->in_obj->equipped_by));
         }
      }
      if(k->carried_by) {
         strcat(buf, " carried by ");
         strcat(buf, GET_NAME(k->carried_by));
      }
      else if(k->equipped_by) {
         strcat(buf, " equipped by ");
         strcat(buf, GET_NAME(k->equipped_by));
      }
      if(strlen(buf2) + strlen(buf) + 3 >= MAX_STRING_LENGTH) {
         send_to_char("LIST TRUNCATED...TOO LONG\n\r", ch);
         break;
      }
      strcat(buf2, buf);
      strcat(buf2, "\n\r");
   }
   
   if(!*buf2)
      send_to_char("Couldn't find any such OBJECT.\n\r", ch);
   else
      page_string(ch->desc, buf2, 1);
   return eSUCCESS;
}
/*********--------- end of locate objects -----------------************/



/* -----------------   MOB LOCATE FUNCTION ---------------------------- */
// locates ONLY mobiles.  If cmd == 18, it locates pc's AND mobiles
int do_mlocate(struct char_data *ch, char *name, int cmd)
{
   char buf[300], buf2[MAX_STRING_LENGTH];
   struct char_data * i;
   int count = 0;
   int vnum = 0;
   int searchnum;

   if(isdigit(*name))
   {
      vnum = atoi(name);
      searchnum = real_mobile(vnum);
   }
   
   *buf2 = '\0';
   send_to_char(" #   Short description          Room Number\n\n\r", ch);
   
   for(i = character_list; i; i = i->next) {
      if((!IS_NPC(i) &&
         (cmd != 18 || !CAN_SEE(ch, i))))
         continue;
      
      // allow find by vnum
      if(vnum)
      {
         if(searchnum != i->mobdata->nr)
            continue;
      }
      else if(!(isname(name, i->name)))
         continue;
      
      count++;
      *buf = '\0';
      sprintf(buf, "[%2d] %-26s %d\n\r", 
         count,
         i->short_desc,
         world[i->in_room].number);
      if(strlen(buf) + strlen(buf2) + 3 >= MAX_STRING_LENGTH) {
         send_to_char("LIST TRUNCATED...TOO LONG\n\r", ch);
         break;
      }
      strcat(buf2, buf);
   }
   
   if(!*buf2)
      send_to_char("Couldn't find any MOBS by that NAME.\n\r", ch);
   else
      page_string(ch->desc, buf2, 1);
   return eSUCCESS;
}
/* --------------------- End of Mob locate function -------------------- */



int do_consider(struct char_data *ch, char *argument, int cmd)
{
   struct char_data *victim;
   char name[256];
   int mod = 0;
   int percent, x, y;
   int Learned;
   
   extern struct str_app_type str_app[];
   
   char *level_messages[] = {
      "You can kill %s naked and weaponless.\n\r",
         "%s is no match for you.\n\r",
         "%s looks like an easy kill.\n\r",	 
         "%s wouldn't be all that hard.\n\r",	
         "%s is perfect for you!\n\r",  
         "You would need some luck and good equipment to kill %s.\n\r", 
         "%s says 'Do you feel lucky, punk?'.\n\r", 
         "%s laughs at you mercilessly.\n\r",  
         "%s will tear your head off and piss on your dead skull.\n\r"			 
   };
   
   char *ac_messages[] = {
      "looks impenetrable.",
         "is heavily armored.",
         "is very well armored.",	
         "looks quite durable.",  
         "looks pretty durable.", 
         "is well protected.",  
         "is protected.",			 
         "is pretty well protected.",
         "is slightly protected.", 
         "is enticingly dressed.", 
         "is pretty much naked." 
   };
   
   char *hplow_messages[] = {
      "wouldn't be worth your time",
         "wouldn't stand a snowball's chance in hell against you",
         "definitely wouldn't last too long against you",	
         "probably wouldn't last too long against you",	
         "can handle almost half the damage you can", 
         "can take half the damage you can",  
         "can handle just over half the damage you can", 		  
         "can take two-thirds the damage you can",
         "can handle almost as much damage as you", 
         "can handle nearly as much damage as you", 
         "can handle just as much damage as you" 
   };
   
   char *hphigh_messages[] = {
      "can definitely take anything you can dish out",
         "can probably take anything you can dish out",
         "takes a licking and keeps on ticking",  
         "can take some punishment",	
         "can handle more than twice the damage that you can", 
         "can handle twice as much damage as you",  
         "can handle quite a bit more damage than you",
         "can handle a lot more damage than you",
         "can handle more damage than you", 
         "can handle a bit more damage than you",
         "can handle just as much damage as you" 
   };
   
   char *dam_messages[] = {
      "hits like my grandmother",
         "will probably graze you preetty good",	
         "can hit pretty hard",	
         "can pack a pretty damn good punch",	
         "can massacre on a good day", 		  
         "can massacre even on a bad day",
         "could make Darth Vader cry like a baby",
         "can eat the Terminator for lunch and not have to burp",
         "will beat the living shit out of you",
         "will pound the fuck out of you", 
         "could make Sylvester Stallone cry for his mommy",
         "is a *very* tough mobile.  Be careful" 
   };
   
   char *thief_messages[] = {
      "At least they'll hang you quickly.",
         "Bards will sing of your bravery, rogues will snicker at"
         "\n\ryour stupidity.",
         "Don't plan on sending your kids to college.",
         "I'd bet against you.",
         "The odds aren't quite in your favor.",
         "I'd give you about 50-50.",
         "The odds are slightly in your favor.",
         "I'd place my money on you. (Not ALL of my money.)",
         "Pretty damn good...80-90 percent.",
         "If you fail THIS steal, you're a loser.",
         "You can't miss."
   };
   
   one_argument(argument, name);
   
   if (!(victim = get_char_room_vis(ch, name))) {
      send_to_char("Who was that you're scoping out?\n\r", ch);
      return eFAILURE;
   }
   
   if (victim == ch) {
      send_to_char("Looks like a WIMP! (Used to be \"Looks like a PUSSY!\" but we got complaints.)\n\r", ch);
      return eFAILURE;
   }

   Learned = has_skill(ch, SKILL_CONSIDER);
   
   if (Learned > 0) {
      /* ARMOR CLASS */
      x = GET_AC(victim)/20+5;
      if(x > 10) x = 10;
      if(x < 0)  x = 0;
      
      percent = number(1,101);
      if (percent > Learned) {
         if ( number(0,1)==0 ) {
            x -= number(1,3);
            if (x < 0) x = 0;
         } else {
            x += number(1,3);
            if (x > 10) x = 10;
         }
      }
      
      csendf(ch, "As far as armor goes, %s %s\n\r",
         GET_SHORT(victim), ac_messages[x]);
   }
   
   
   
   /* HIT POINTS */
   
   if (Learned > 0) {
      
      if(!IS_NPC(victim) && GET_LEVEL(victim) > IMMORTAL) {
         csendf(ch, "Compared to your hps, %s can definitely take anything you can dish out.\n\r",
            GET_SHORT(victim));
         csendf(ch, "Average damage: %s will pound the living shit out of you.\n\r",
            GET_SHORT(victim));
      }
      else {
         
         if(GET_HIT(ch) >= GET_HIT(victim) || GET_LEVEL(ch) > MORTAL) {
            x = GET_HIT(victim)/GET_HIT(ch)*100;
            x /= 10;
            if(x < 0)  x = 0;
            if(x > 10) x = 10;
            percent = number(1,101);
            if (percent > Learned) {
               if ( number(0,1)==0 ) {
                  x -= number(1,3);
                  if (x < 0) x = 0;
               } else {
                  x += number(1,3);
                  if (x > 10) x = 10;
               }
            }
            
            csendf(ch, "Compared to your hps, %s %s.\n\r", GET_SHORT(victim), hplow_messages[x]);
         }
         else {
            x = GET_HIT(ch)/GET_HIT(victim)*100;
            x /= 10;
            if(x < 0)  x = 0;
            if(x > 10) x = 10;
            percent = number(1,101);
            if (percent > Learned) {
               if ( number(0,1)==0 ) {
                  x -= number(1,3);
                  if (x < 0) x = 0;
               } else {
                  x += number(1,3);
                  if (x > 10) x = 10;
               }
            }
            
            csendf(ch, "Compared to your hps, %s %s.\n\r", GET_SHORT(victim), hphigh_messages[x]);
         }
      }
      
      if (Learned > 0) {
         
         /* Average Damage */
         
         if(victim->equipment[WIELD]) {
            x = victim->equipment[WIELD]->obj_flags.value[1];
            y = victim->equipment[WIELD]->obj_flags.value[2];
            x = (((x*y-x)/2) + x);
         }
         else {
            if(IS_NPC(victim)) {
               x = victim->mobdata->damnodice;
               y = victim->mobdata->damsizedice;
               x = (((x*y-x)/2) + x);
            }
            else 
               x = number(0,2);
         }
         x += str_app[STRENGTH_APPLY_INDEX(victim)].todam;
         x += GET_DAMROLL(victim);
         
         if(x <= 5)	x = 0;
         else if(x <= 10)	x = 1;
         else if(x <= 15)	x = 2;
         else if(x <= 23)	x = 3;
         else if(x <= 30)	x = 4;
         else if(x <= 40)	x = 5;
         else if(x <= 50)	x = 6;
         else if(x <= 75)	x = 7;
         else if(x <= 100)  x = 8;
         else if(x <= 125)  x = 9;
         else if(x <= 150)  x = 10;
         else					 x = 11;
         percent = number(1,101);
         if (percent > Learned) {
            if ( number(0,1)==0 ) {
               x -= number(1,4);
               if (x < 0) x = 0;
            } else {
               x += number(1,4);
               if (x > 11) x = 11;
            }
         }
         
         csendf(ch, "Average damage: %s %s.\n\r", GET_SHORT(victim),
            dam_messages[x]);
      }
   }
   
   if (Learned > 0) {
      /* CHANCES TO STEAL */
      if((GET_CLASS(ch) == CLASS_THIEF) || (GET_LEVEL(ch) > IMMORTAL)) {
         
         percent = Learned;
         
         mod += AWAKE(victim) ? 10 : -50;
         mod += ((GET_LEVEL(victim) - GET_LEVEL(ch)) / 2);
         mod += 5;  /* average item is 5 lbs, steal takes ths into acct */
         if(GET_DEX(ch) < 10) 	  mod += ((10 - GET_DEX(ch)) * 5);
         else if(GET_DEX(ch) > 15) mod -= ((GET_DEX(ch) - 10) * 2);
         
         percent -= mod;
         
         if (GET_POS(victim) <= POSITION_SLEEPING)
            percent = 100;
         if (GET_LEVEL(victim) > IMMORTAL)
            percent = 0;
         if (percent < 0)	 percent = 0;
         else if (percent > 100) percent = 100;
         percent /= 10;
         x = percent;
         
         percent = number(1,101);
         if (percent > Learned) {
            if ( number(0,1)==0 ) {
               x -= number(1,3);
               if (x < 0) x = 0;
            } else {
               x += number(1,3);
               if (x > 10) x = 10;
            }
         }
         
         csendf(ch, "Chances of stealing: %s\n\r", thief_messages[x]);
      }
   }
   /* Level Comparison */
   x = GET_LEVEL(victim) - GET_LEVEL(ch);
   if ( x <= -15 ) y = 0;
   else if ( x <= -10 ) y = 1;
   else if ( x <=  -5 ) y = 2;
   else if ( x <=  -2 ) y = 3;
   else if ( x <=   1 ) y = 4;
   else if ( x <=   2 ) y = 5;
   else if ( x <=   4 ) y = 6;
   else if ( x <=   9 ) y = 6;
   else						y = 8;
   
   send_to_char("Level comparison: ", ch);
   csendf(ch, level_messages[y], GET_SHORT(victim));
   return eSUCCESS;
}



/* Shows characters in adjacent rooms -- Sadus */
int do_scan(struct char_data *ch, char *argument, int cmd)
{
   int i;
   struct char_data *vict;
   struct room_data *room;
   long was_in;
   int percent;
   int Learned;
   
   char *possibilities[] =
   {
      "to the North",
         "to the East",
         "to the South",
         "to the West",
         "above you",
         "below you",
         "\n",
   };
   
   Learned = has_skill(ch, SKILL_SCAN);

   act("$n carefully searches the surroundings...", ch, 0, 0, TO_ROOM,
      INVIS_NULL|STAYHIDE);
   send_to_char("You carefully search the surroundings...\n\r\n\r", ch);
   
   for(vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
      if(CAN_SEE(ch, vict) && ch != vict) {
         csendf(ch,"%35s -- %s\n\r", GET_SHORT(vict), "Right Here");
      }
   }
   
   for(i = 0; i < 6; i++) {
      if(CAN_GO(ch, i)) {
         room = &world[world[ch->in_room].dir_option[i]->to_room];
         if(room == &world[ch->in_room])
            continue;
         if(IS_SET(room->room_flags, NO_SCAN))
         {
            csendf(ch, "%35s -- a little bit %s\n\r", "It's too hard to see!",
               possibilities[i]);
         }
         else for(vict = room->people; vict; vict = vict->next_in_room) {
            if(CAN_SEE(ch, vict)  && !IS_AFFECTED2(vict, AFF_CAMOUFLAGUE)) { 
               percent = number(1,101);
               if(percent < 75) {
                  csendf(ch,"%35s -- a little bit %s\n\r", GET_SHORT(vict),
                     possibilities[i]);
               }
            }
         }
         
         // Now we go one room further (reach out and touch someone)
         
         was_in = ch->in_room;
         ch->in_room = world[ch->in_room].dir_option[i]->to_room;
         
         if(CAN_GO(ch, i)) {
            room = &world[world[ch->in_room].dir_option[i]->to_room];
            if(IS_SET(room->room_flags, NO_SCAN))
            {
               csendf(ch, "%35s -- a ways off %s\n\r", "It's too hard to see!",
                  possibilities[i]);
            }
            else for(vict = room->people; vict; vict = vict->next_in_room) {
               if(CAN_SEE(ch, vict) && !IS_AFFECTED2(vict, AFF_CAMOUFLAGUE)){ 
                  percent = number(1,101);
                  if(percent < 50) {
                     csendf(ch,"%35s -- a ways off %s\n\r",
                        GET_SHORT(vict),
                        possibilities[i]);
                  }
               }
            }
            // Now if we have the farsight spell we go another room out
            if(IS_AFFECTED2(ch, AFF_FARSIGHT)) {
               ch->in_room = world[ch->in_room].dir_option[i]->to_room;
               if(CAN_GO(ch, i)) {
                  room = &world[world[ch->in_room].dir_option[i]->to_room];
                  if(IS_SET(room->room_flags, NO_SCAN))
                  {
                     csendf(ch, "%35s -- extremely far off %s\n\r", "It's too hard to see!",
                        possibilities[i]);
                  }
                  else for(vict = room->people; vict; vict = vict->next_in_room) {
                     if(CAN_SEE(ch, vict) && !IS_AFFECTED2(vict, AFF_CAMOUFLAGUE)) {
                        percent = number(1,101);
                        if(percent < 40) {
                           csendf(ch, "%35s -- extremely far off %s\n\r",
                              GET_SHORT(vict),
                              possibilities[i]);
                        }
                     }
                  }
               }
            }
         }
         ch->in_room = was_in;
      }
   }
   return eSUCCESS;
}

int do_tick( struct char_data *ch, char *argument, int cmd )
{
  int ntick;
  char buf[256];  

  if (IS_SET(world[ch->in_room].room_flags, QUIET)) {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return 1;
  }

  if ( IS_NPC(ch) ) {
    send_to_char( "Monsters don't wait for anything.\n\r", ch );
    return eFAILURE;
  }

  if ( ch->desc == NULL )
    return eFAILURE;

  while ( *argument == ' ' )
    argument++;
    
  if ( *argument == '\0' )
    ntick = 1;
  else
    ntick = atoi( argument );
      
  if ( ntick == 1 )
    sprintf( buf, "$n is waiting for one tick." );
  else
    sprintf( buf, "$n is waiting for %d ticks.", ntick );
     
  act(buf, ch, 0, 0, TO_CHAR, INVIS_NULL);
  act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL);
     
  // TODO - figure out if this ever had any purpose.  It's still fun though:)
  ch->desc->tick_wait = ntick;
  return eSUCCESS;
}
