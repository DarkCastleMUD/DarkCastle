
/****************************************************************************
 * file: act_info.c , Implementation of commands.	 Part of DIKUMUD    *
 * Usage : Informative commands. 					    *
 * Copyright (C) 1990, 1991 - see 'license.doc' for complete information.   *
 *                                                                          *
 * Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse    *
 * Performance optimization and bug fixes by MERC Industries.		    *
 * You can use our stuff in any way you like whatsoever so long as ths	    *
 * copyright notice remains intact.  If you like it please drop a line	    *
 * to mec@garnet.berkeley.edu.						    *
 * 									    *
 * This is free software and you are benefitting.	We hope that you    *
 * share your changes too.  What goes around, comes around. 		    *
 ****************************************************************************/
/* $Id: info.cpp,v 1.210 2015/06/14 02:38:12 pirahna Exp $ */
#include <sys/time.h>

#include <cctype>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <map>
#include <sstream>
#include <fstream>
#include <fmt/format.h>

#include <algorithm>
#include <QRegularExpression>

#include "DC/DC.h"
#include "DC/structs.h"
#include "DC/character.h"
#include "DC/utility.h"
#include "DC/terminal.h"
#include "DC/player.h"
#include "DC/mobile.h"
#include "DC/clan.h"
#include "DC/handler.h"
#include "DC/db.h" // exp_table
#include "DC/interp.h"
#include "DC/connect.h"
#include "DC/spells.h"
#include "DC/race.h"
#include "DC/act.h"
#include "DC/set.h"
#include "DC/returnvals.h"
#include "DC/fileinfo.h"
#include "DC/utility.h"
#include "DC/isr.h"
#include "DC/Leaderboard.h"
#include "DC/handler.h"
#include "DC/const.h"
#include "DC/vault.h"
#include "DC/sing.h"

/* extern variables */

extern char credits[MAX_STRING_LENGTH];
extern char info[MAX_STRING_LENGTH];
extern char story[MAX_STRING_LENGTH];
extern char *where[];
extern char *color_liquid[];
extern char *fullness[];
extern char *sky_look[];
extern const char *temp_room_bits[];

/* Used for "who" */
extern int max_who;

/* extern functions */

void page_string(class Connection *d, const char *str, int keep_internal);
clan_data *get_clan(Character *);
extern int getRealSpellDamage(Character *ch);

/* intern functions */

void showStatDiff(Character *ch, int base, int random, bool swapcolors = false);

int get_saves(Character *ch, int savetype)
{
   int save = ch->saves[savetype];
   switch (savetype)
   {
   case SAVE_TYPE_MAGIC:
      save += int_app[GET_INT(ch)].magic_resistance;
      break;
   case SAVE_TYPE_COLD:
      save += str_app[GET_STR(ch)].cold_resistance;
      break;
   case SAVE_TYPE_ENERGY:
      save += wis_app[GET_WIS(ch)].energy_resistance;
      break;
   case SAVE_TYPE_FIRE:
      save += dex_app[GET_DEX(ch)].fire_resistance;
      break;
   case SAVE_TYPE_POISON:
      save += con_app[GET_CON(ch)].poison_resistance;
      break;
   default:
      break;
   }
   return save;
}

/* Procedures related to 'look' */

void argument_split_3(const char *argument, char *first_arg, char *second_arg, char *third_arg)
{
   int look_at, begin;
   begin = 0;

   /* Find first non blank */
   for (; *(argument + begin) == ' '; begin++)
      ;

   /* Find length of first word */
   for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /* Make all letters lower case, AND copy them to first_arg */
      *(first_arg + look_at) = LOWER(*(argument + begin + look_at));

   *(first_arg + look_at) = '\0';
   begin += look_at;

   /* Find first non blank */
   for (; *(argument + begin) == ' '; begin++)
      ;

   /* Find length of second word */
   for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /* Make all letters lower case, AND copy them to second_arg */
      *(second_arg + look_at) = LOWER(*(argument + begin + look_at));

   *(second_arg + look_at) = '\0';
   begin += look_at;

   /* Find first non blank */
   for (; *(argument + begin) == ' '; begin++)
      ;

   /* Find length of second word */
   for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
      /* Make all letters lower case, AND copy them to second_arg */
      *(third_arg + look_at) = LOWER(*(argument + begin + look_at));

   *(third_arg + look_at) = '\0';
   begin += look_at;
}

class Object *Character::get_object_in_equip_vis(char *arg, class Object *equipment[], int *j, bool blindfighting)
{
   int k, num;
   char tmpname[MAX_STRING_LENGTH];
   char *tmp;

   strcpy(tmpname, arg);
   tmp = tmpname;
   if ((num = get_number(&tmp)) < 0)
      return (0);

   for ((*j) = 0, k = 1; ((*j) < MAX_WEAR) && (k <= num); (*j)++)
      if (equipment[(*j)])
         if (CAN_SEE_OBJ(this, equipment[(*j)], blindfighting))
            if (isexact(tmp, equipment[(*j)]->name))
            {
               if (k == num)
                  return (equipment[(*j)]);
               k++;
            }

   return (0);
}

char *find_ex_description(char *word, struct extra_descr_data *list)
{
   struct extra_descr_data *i;

   for (i = list; i; i = i->next)
      if (isexact(word, i->keyword))
         return (i->description);

   return (0);
}

const char *item_condition(class Object *object)
{
   int percent = 100 - (int)(100 * ((float)eq_current_damage(object) / (float)eq_max_damage(object)));

   if (percent >= 100)
      return " [$B$2Excellent$R]";
   else if (percent >= 80)
      return " [$2Good$R]";
   else if (percent >= 60)
      return " [$3Decent$R]";
   else if (percent >= 40)
      return " [$B$5Damaged$R]";
   else if (percent >= 20)
      return " [$4Quite Damaged$R]";
   else if (percent >= 0)
      return " [$B$4Falling Apart$R]";
   else
      return " [$5Pile of Scraps$R]";
}

void Character::show_obj_to_char(class Object *object, int mode)
{
   char buffer[MAX_STRING_LENGTH];
   char flagbuf[MAX_STRING_LENGTH];
   int found = 0;
   //   int percent;

   // Don't show NO_NOTICE items in a room with "look" unless they have holylite
   if (mode == 0 && isSet(object->obj_flags.more_flags, ITEM_NONOTICE) &&
       (this->player && !this->player->holyLite))
      return;

   buffer[0] = '\0';
   if ((mode == 0) && object->description)
      strcpy(buffer, object->description);
   else if (object->short_description && ((mode == 1) ||
                                          (mode == 2) || (mode == 3) || (mode == 4)))
      strcpy(buffer, object->short_description);
   else if (mode == 5)
   {
      if (object->obj_flags.type_flag == ITEM_NOTE)
      {
         if (!object->ActionDescription().isEmpty())
         {
            strncpy(buffer, "There is something written upon it:\n\r\n\r", sizeof(buffer) - 1);
            strncat(buffer, object->ActionDescription().toStdString().c_str(), sizeof(buffer) - 1);
            page_string(this->desc, buffer, 1);
         }
         else
            act("It's blank.", this, 0, 0, TO_CHAR, 0);
         return;
      }
      else if ((object->obj_flags.type_flag != ITEM_DRINKCON))
      {
         strcpy(buffer, "You see nothing special.");
      }
      else /* ITEM_TYPE == ITEM_DRINKCON */
      {
         strcpy(buffer, "It looks like a drink container.");
      }
   }

   if (mode != 3)
   {
      if (mode == 0)           // 'look'
         strcat(buffer, "$R"); // setup color background

      strcpy(flagbuf, " $B($R");

      if (IS_OBJ_STAT(object, ITEM_INVISIBLE))
      {
         strcat(flagbuf, "Invisible");
         found++;
      }
      if (IS_OBJ_STAT(object, ITEM_MAGIC) && IS_AFFECTED(this, AFF_DETECT_MAGIC))
      {
         if (found)
            strcat(flagbuf, "$B/$R");
         strcat(flagbuf, "Blue Glow");
         found++;
      }
      if (IS_OBJ_STAT(object, ITEM_GLOW))
      {
         if (found)
            strcat(flagbuf, "$B/$R");
         strcat(flagbuf, "Glowing");
         found++;
      }
      if (IS_OBJ_STAT(object, ITEM_HUM))
      {
         if (found)
            strcat(flagbuf, "$B/$R");
         strcat(flagbuf, "Humming");
         found++;
      }
      if (mode == 0 && isSet(object->obj_flags.more_flags, ITEM_NONOTICE))
      {
         if (found)
            strcat(flagbuf, "$B/$R");
         strcat(flagbuf, "NO_NOTICE");
         found++;
      }
      if (mode == 0 && isSet(object->obj_flags.more_flags, ITEM_NOSEE))
      {
         if (found)
            strcat(flagbuf, "$B/$R");
         strcat(flagbuf, "NO_SEE");
         found++;
      }
      if (found)
      {
         strcat(flagbuf, "$B)$R");
         strcat(buffer, flagbuf);
      }

      /* show object's condition if is an armor...  */
      if (object->obj_flags.type_flag == ITEM_ARMOR ||
          object->obj_flags.type_flag == ITEM_WEAPON ||
          object->obj_flags.type_flag == ITEM_FIREWEAPON ||
          object->obj_flags.type_flag == ITEM_CONTAINER ||
          IS_KEYRING(object) ||
          object->obj_flags.type_flag == ITEM_INSTRUMENT ||
          object->obj_flags.type_flag == ITEM_WAND ||
          object->obj_flags.type_flag == ITEM_STAFF ||
          object->obj_flags.type_flag == ITEM_LIGHT)
      {
         strcat(buffer, item_condition(object)); /*
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
      */
      }
      if (isSet(object->obj_flags.more_flags, ITEM_24H_SAVE) && !isSet(object->obj_flags.extra_flags, ITEM_NOSAVE))
      {
         time_t now = time(nullptr);
         time_t expires = object->save_expiration;
         if (expires == 0)
         {
            strcat(buffer, " $R($B$0unsaved$R)");
         }
         else if (now >= expires)
         {
            strcat(buffer, " $R($B$0expired$R)");
         }
         else
         {
            char timebuffer[100];
#if __x86_64__
            snprintf(timebuffer, 100, " $R($B$0%lu secs left$R)", expires - now);
#else
            snprintf(timebuffer, 100, " $R($B$0%lu secs left$R)", expires - now);
#endif
            strcat(buffer, timebuffer);
         }
      }

      if (isSet(object->obj_flags.more_flags, ITEM_24H_NO_SELL))
      {
         time_t now = time(nullptr);
         time_t expires = object->no_sell_expiration;
         if (now >= expires || expires == 0)
         {
            strcat(buffer, " $R($B$0sellable$R)");
         }
         else
         {
            char timebuffer[101] = {};
#if __x86_64__
            snprintf(timebuffer, 100, " $R($B$0No sell for %lu secs$R)", expires - now);
#else
            snprintf(timebuffer, 100, " $R($B$0No sell for %lu secs$R)", expires - now);
#endif
            strcat(buffer, timebuffer);
         }
      }

      if (isSet(object->obj_flags.more_flags, ITEM_POOF_AFTER_24H))
      {
         if (object->obj_flags.timer)
         {
            char timebuffer[101] = {};
            snprintf(timebuffer, 100, " $R($B$0%lu ticks left$R)", object->obj_flags.timer);
            strcat(buffer, timebuffer);
         }
      }

      if (mode == 0)             // 'look'
         strcat(buffer, "$B$1"); // setup color background
   }

   strcat(buffer, "\n\r");
   page_string(this->desc, buffer, 1);
}

void Character::list_obj_to_char(class Object *list, int mode, bool show)
{
   class Object *i;
   bool found = false;
   int number = 1;
   int can_see;
   char buf[50];

   for (i = list; i; i = i->next_content)
   {
      if ((can_see = CAN_SEE_OBJ(this, i)) && i->next_content &&
          i->next_content->item_number == i->item_number && i->item_number != -1 && !isSet(i->obj_flags.more_flags, ITEM_NONOTICE))
      {
         number++;
         continue;
      }
      if (can_see || number > 1)
      {
         if (number > 1)
         {
            sprintf(buf, "[%d] ", number);
            this->send(buf);
         }
         show_obj_to_char(i, mode);
         found = true;
         number = 1;
      }
   }

   if ((!found) && (show))
      this->sendln("Nothing");
}

void show_spells(Character *i, Character *ch)
{
   std::string strbuf;

   if (IS_AFFECTED(i, AFF_SANCTUARY))
   {
      strbuf = strbuf + "$7aura! ";
   }

   if (i->affected_by_spell(SPELL_PROTECT_FROM_EVIL))
   {
      strbuf = strbuf + "$R$6pulsing! ";
   }
   else if (i->affected_by_spell(SPELL_PROTECT_FROM_GOOD))
   {
      strbuf = strbuf + "$R$6$Bpulsing! ";
   }

   if (IS_AFFECTED(i, AFF_FIRESHIELD))
   {
      strbuf = strbuf + "$B$4flames! ";
   }

   if (IS_AFFECTED(i, AFF_FROSTSHIELD))
   {
      strbuf = strbuf + "$B$3frost! ";
   }

   if (IS_AFFECTED(i, AFF_ACID_SHIELD))
   {
      strbuf = strbuf + "$B$2acid! ";
   }

   if (i->affected_by_spell(SPELL_BARKSKIN))
   {
      strbuf = strbuf + "$R$5woody! ";
   }

   if (IS_AFFECTED(i, AFF_LIGHTNINGSHIELD))
   {
      strbuf = strbuf + "$B$5energy! ";
   }

   if (IS_AFFECTED(i, AFF_PARALYSIS))
   {
      strbuf = strbuf + "$R$2paralyze! ";
   }

   if (i->affected_by_spell(SPELL_STONE_SHIELD) || i->affected_by_spell(SPELL_GREATER_STONE_SHIELD))
   {
      strbuf = strbuf + "$B$0stones! ";
   }

   if (IS_AFFECTED(i, AFF_FLYING))
   {
      strbuf = strbuf + "$B$1flying!";
   }

   if (!strbuf.empty())
   {
      if (IS_NPC(i))
         strbuf = std::string("$B$7-$1") + GET_SHORT(i) + " has: " + strbuf + "$R\r\n";
      else
         strbuf = std::string("$B$7-$1") + GET_NAME(i) + " has: " + strbuf + "$R\r\n";

      send_to_char(strbuf.c_str(), ch);
   }
}

void show_char_to_char(Character *i, Character *ch, int mode)
{
   std::string buffer;
   int j, found, percent;
   class Object *tmp_obj;
   char buf2[MAX_STRING_LENGTH];
   clan_data *clan;
   char buf[200];

   if (mode == 0)
   {
      if (!CAN_SEE(ch, i))
      {
         if (IS_AFFECTED(ch, AFF_SENSE_LIFE))
         {
            ch->sendln("$R$7You sense a hidden life form in the room.");
         }

         return;
      }
      ch->send("$B$3");

      if (!(i->long_desc) || (IS_NPC(i) && (GET_POS(i) != i->mobdata->default_pos)))
      {
         /* A char without long descr, or not in default pos. */
         if (IS_PC(i))
         {
            buffer = "";

            if (!i->desc)
               buffer = "*linkdead*  ";
            if (isSet(i->player->toggles, Player::PLR_GUIDE_TOG))
               buffer.append("$B$7(Guide)$B$3 ");

            buffer.append(GET_SHORT(i));
            if ((i->getLevel() < OVERSEER) && i->clan && (clan = get_clan(i)))
            {
               if (IS_PC(ch) && !isSet(ch->player->toggles, Player::PLR_BRIEF))
               {
                  sprintf(buf, " %s [%s]", GET_TITLE(i), clan->name);
                  buffer.append(buf);
               }
               else
               {
                  sprintf(buf, " the %s [%s]",
                          races[(int)GET_RACE(i)].singular_name, clan->name);
                  buffer.append(buf);
               }
            }
            else
            {
               if (!IS_NPC(ch) && !isSet(ch->player->toggles, Player::PLR_BRIEF))
               {
                  buffer.append(" ");
                  buffer.append(GET_TITLE(i));
               }
               else
               {
                  buffer.append(" the ");
                  sprintf(buf2, "%s", races[(int)GET_RACE(i)].singular_name);
                  buffer.append(buf2);
               }
            }
         }

         if (IS_NPC(i))
         {
            buffer = i->short_desc;
            buffer[0] = toupper(buffer[0]);
         }

         switch (GET_POS(i))
         {
         case position_t::STUNNED:
            buffer.append(" is on the ground, stunned.");
            break;
         case position_t::DEAD:
            buffer.append(" is lying here, dead.");
            break;
         case position_t::STANDING:
            buffer.append(" is here.");
            break;
         case position_t::SITTING:
            buffer.append(" is sitting here.");
            break;
         case position_t::RESTING:
            buffer.append(" is resting here.");
            break;
         case position_t::SLEEPING:
            buffer.append(" is sleeping here.");
            break;
         case position_t::FIGHTING:
            if (i->fighting)
            {
               buffer.append(" is here, fighting ");

               if (i->fighting == ch)
               {
                  buffer.append("YOU!");
               }
               else
               {
                  if (i->in_room == i->fighting->in_room)
                  {
                     buffer.append(GET_SHORT(i->fighting));
                  }
                  else
                  {
                     buffer.append("someone who has already left.");
                  }
               }
            }
            else
            { /* NIL fighting pointer */
               buffer.append(" is here struggling with thin air.");
            }
            break;
         default:
            buffer.append(" is floating here.");
            break;
         }

         if (IS_AFFECTED(i, AFF_INVISIBLE))
            buffer.append(" $1(invisible) ");
         if (IS_AFFECTED(i, AFF_HIDE) && ((IS_AFFECTED(ch, AFF_true_SIGHT) && ch->has_skill(SPELL_true_SIGHT) > 80) || ch->getLevel() > IMMORTAL || ARE_GROUPED(i, ch)))
            buffer.append(" $4(hidden) ");
         if ((IS_AFFECTED(ch, AFF_DETECT_EVIL) || IS_AFFECTED(ch, AFF_KNOW_ALIGN)) && IS_EVIL(i))
            buffer.append(" $B$4(red halo) ");
         if ((IS_AFFECTED(ch, AFF_DETECT_GOOD) || IS_AFFECTED(ch, AFF_KNOW_ALIGN)) && IS_GOOD(i))
            buffer.append(" $B$1(blue halo) ");
         if (IS_AFFECTED(ch, AFF_KNOW_ALIGN) && !IS_GOOD(i) && !IS_EVIL(i))
            buffer.append(" $B$5(yellow halo) ");
         if (IS_AFFECTED(i, AFF_CHAMPION))
            buffer.append(" $B$4(Champion) ");

         buffer.append("$R\n\r");
         send_to_char(buffer.c_str(), ch);

         show_spells(i, ch);
      }
      else /* npc with long */
      {
         if (IS_AFFECTED(i, AFF_INVISIBLE))
         {
            buffer = "$B$7*$3";
         }
         else
         {
            buffer = "";
         }

         if (IS_AFFECTED(i, AFF_HIDE) && IS_AFFECTED(ch, AFF_true_SIGHT) && ch->has_skill(SPELL_true_SIGHT) > 80)
            buffer.append(" $4(hidden) $3");
         if ((IS_AFFECTED(ch, AFF_DETECT_EVIL) || IS_AFFECTED(ch, AFF_KNOW_ALIGN)) && IS_EVIL(i))
            buffer.append(" $B$4(red halo)$3 ");
         if ((IS_AFFECTED(ch, AFF_DETECT_GOOD) || IS_AFFECTED(ch, AFF_KNOW_ALIGN)) && IS_GOOD(i))
            buffer.append(" $B$1(blue halo)$3 ");
         if (IS_AFFECTED(ch, AFF_KNOW_ALIGN) && !IS_GOOD(i) && !IS_EVIL(i))
            buffer.append(" $B$5(yellow halo)$3 ");

         buffer.append(i->long_desc);

         send_to_char(buffer.c_str(), ch);

         show_spells(i, ch);
         ch->send("$R$7");
      }
   }
   else if ((mode == 1) || (mode == 3))
   {
      if (mode == 1)
      {
         if (i->description)
         {
            ch->send(i->description);
         }
         else
         {
            act("You see nothing special about $m.", i, 0, ch, TO_VICT, 0);
         }
      }

      /* Show a character to another */

      if (GET_MAX_HIT(i) > 0)
      {
         percent = (100 * i->getHP()) / GET_MAX_HIT(i);
      }
      else
      {
         percent = -1; /* How could MAX_HIT be < 1?? */
      }

      buffer = GET_SHORT(i);

      sprintf(buf, " the %s",
              races[(int)GET_RACE(i)].singular_name);

      buffer.append(buf);

      if (percent >= 100)
         buffer.append(" is in excellent condition.\r\n");
      else if (percent >= 90)
         buffer.append(" has a few scratches.\r\n");
      else if (percent >= 75)
         buffer.append(" is slightly hurt.\r\n");
      else if (percent >= 50)
         buffer.append(" is fairly fucked up.\r\n");
      else if (percent >= 30)
         buffer.append(" is bleeding freely.\r\n");
      else if (percent >= 15)
         buffer.append(" is covered in blood.\r\n");
      else if (percent >= 0)
         buffer.append(" is near death.\r\n");
      else
         buffer.append(" is suffering from a slow death.\r\n");

      send_to_char(buffer.c_str(), ch);

      if (mode == 3)
      { // If it was a glance, show spells then get out
         show_spells(i, ch);
         return;
      }

      found = false;
      for (j = 0; j < MAX_WEAR; j++)
      {
         if (i->equipment[j])
         {
            if (CAN_SEE_OBJ(ch, i->equipment[j]))
            {
               found = true;
            }
         }
      }

      if (found)
      {
         act("\n\r$n is using:", i, 0, ch, TO_VICT, 0);
         act("<    worn     > Item Description     (Flags) [Item Condition]\r\n", i, 0, ch, TO_VICT, 0);

         for (j = 0; j < MAX_WEAR; j++)
         {
            if (i->equipment[j])
            {
               if (CAN_SEE_OBJ(ch, i->equipment[j]))
               {
                  send_to_char(where[j], ch);
                  ch->show_obj_to_char(i->equipment[j], 1);
               }
            }
         }
      }

      if ((GET_CLASS(ch) == CLASS_THIEF && ch != i) || ch->getLevel() > IMMORTAL)
      {
         found = false;
         ch->sendln("\n\rYou attempt to peek at the inventory:");
         for (tmp_obj = i->carrying; tmp_obj;
              tmp_obj = tmp_obj->next_content)
         {
            if ((isSet(tmp_obj->obj_flags.extra_flags, ITEM_QUEST) == false ||
                 ch->getLevel() > IMMORTAL) &&
                CAN_SEE_OBJ(ch, tmp_obj) &&
                number(0, MORTAL) < ch->getLevel())
            {
               ch->show_obj_to_char(tmp_obj, 1);
               found = true;
            }
         }
         if (!found)
            ch->sendln("You can't see anything.");
      }
   }
   else if (mode == 2)
   {
      /* Lists inventory */
      act("$n is carrying:", i, 0, ch, TO_VICT, 0);
      ch->list_obj_to_char(i->carrying, 1, true);
   }
}

command_return_t Character::do_botcheck(QStringList arguments, cmd_t cmd)
{
   QString name = arguments.value(0);
   if (name.isEmpty())
   {
      this->sendln("botcheck <player> or all\n\r");
      return eFAILURE;
   }

   QString name2 = "0." + name;
   Character *victim = get_char(name2);

   if (!victim && name == "all")
   {
      Connection *d;
      Character *i;

      for (d = DC::getInstance()->descriptor_list; d; d = d->next)
      {
         if (d->connected || !d->character)
            continue;
         if (!(i = d->original))
            i = d->character;
         if (!CAN_SEE(this, i))
            continue;
         sendln(QStringLiteral("\n\r%1").arg(i->getName()));
         sendln("----------");
         do_botcheck(i->getName().split(' '));
      }
      return eSUCCESS;
   }

   if (victim == nullptr)
   {
      sendln(QStringLiteral("Unable to find %1.").arg(name));
      return eFAILURE;
   }

   if (victim->getLevel() > this->getLevel())
   {
      this->sendln("Unable to show information.");
      csendf(this, "%s is a higher level than you.\r\n", victim->getNameC());
      return eFAILURE;
   }

   if (IS_NPC(victim))
   {
      this->sendln("Unable to show information.");
      csendf(this, "%s is a mob.\r\n", victim->getNameC());
      return eFAILURE;
   }

   if (victim->player->lastseen == 0)
      victim->player->lastseen = new std::multimap<int, std::pair<timeval, timeval>>;

   if (victim->player->lastseen->size() == 0)
   {
      csendf(this, "%s has not seen any mobs recently.\r\n", victim->getNameC());
      return eFAILURE;
   }

   int nr, ms;
   timeval seen, targeted;
   double ts1, ts2;
   for (std::multimap<int, std::pair<timeval, timeval>>::iterator i = victim->player->lastseen->begin(); i != victim->player->lastseen->end(); ++i)
   {
      nr = (*i).first;
      seen = (*i).second.first;
      targeted = (*i).second.second;

      ts1 = seen.tv_sec + ((double)seen.tv_usec / 1000000.0);
      ts2 = targeted.tv_sec + ((double)targeted.tv_usec / 1000000.0);

      if (ts2 > ts1)
      {
         ms = (int)((ts2 - ts1) * 1000.0);
      }
      else
      {
         ms = 0;
      }

      if (nr >= 0)
      {
         csendf(this, "[%4dms] [%5d] [%s]\n\r", ms, DC::getInstance()->mob_index[nr].virt,
                ((Character *)(DC::getInstance()->mob_index[nr].item))->short_desc);
      }
   }

   return eSUCCESS;
}

void Character::list_char_to_char(Character *list, int mode)
{
   bool clear_lastseen = false;
   Character *i;
   int known = this->has_skill(SKILL_BLINDFIGHTING);
   timeval tv, tv_zero = {0, 0};

   for (i = list; i; i = i->next_in_room)
   {
      if (this == i)
         continue;
      if (!IS_NPC(i) && (i->player->wizinvis > this->getLevel()))
         if (!i->player->incognito || !(this->in_room == i->in_room))
            continue;
      if (IS_AFFECTED(this, AFF_SENSE_LIFE) || CAN_SEE(this, i))
      {
         show_char_to_char(i, this, 0);

         if (IS_PC(this) && IS_NPC(i))
         {
            if (this->player->lastseen == 0)
               this->player->lastseen = new std::multimap<int, std::pair<timeval, timeval>>;

            if (clear_lastseen == false)
            {
               this->player->lastseen->clear();
               clear_lastseen = true;
            }

            gettimeofday(&tv, nullptr);
            this->player->lastseen->insert(std::pair<int, std::pair<timeval, timeval>>(i->mobdata->nr, std::pair<timeval, timeval>(tv, tv_zero)));
         }
      }
      else if (IS_DARK(this->in_room))
      {
         if (known && skill_success(nullptr, SKILL_BLINDFIGHTING))
            this->sendln("Your blindfighting awareness alerts you to a presense in the area.");
         else if (number(1, 10) == 1)
            this->sendln("$B$4You see a pair of glowing red eyes looking your way.$R$7");
      }
   }
}

void try_to_peek_into_container(Character *vict, Character *ch,
                                char *container)
{
   class Object *obj = nullptr;
   class Object *cont = nullptr;
   int found = false;

   if (GET_CLASS(ch) != CLASS_THIEF && ch->getLevel() < DEITY)
   {
      ch->send("They might object to you trying to look in their pockets...");
      return;
   }

   if (!(cont = get_obj_in_list_vis(ch, container, vict->carrying)) ||
       number(0, MORTAL + 30) > ch->getLevel())
   {
      ch->sendln("You cannot see a container named that to peek into.");
      return;
   }

   if (NOT_CONTAINERS(cont))
   {
      ch->sendln("It's not a container....");
      return;
   }

   char buf[200];
   sprintf(buf, "You attempt to peek into the %s.\r\n", cont->short_description);
   ch->send(buf);

   if (isSet(cont->obj_flags.value[1], CONT_CLOSED))
   {
      ch->sendln("It is closed.");
      return;
   }

   for (obj = cont->contains; obj; obj = obj->next_content)
      if (CAN_SEE_OBJ(ch, obj) && number(0, MORTAL + 30) < ch->getLevel())
      {
         ch->show_obj_to_char(obj, 1);
         found = true;
      }

   if (!found)
      ch->sendln("You don't see anything inside it.");
}

QString Character::getStatDiff(int base, int random, bool swapcolors)
{
   QString buf, buf2;
   QString color_good = "$2";
   QString color_bad = "$4";

   if (this->player)
   {
      color_good = this->getSettingAsColor("color.good");
      color_bad = this->getSettingAsColor("color.bad");
   }
   else
   {
      return QString();
   }

   // original value
   buf2 = QStringLiteral("%1").arg(base);
   buf += buf2;

   if (random - base > 0)
   {
      // if postive show "+ difference"
      if (swapcolors)
      {
         buf2 = QStringLiteral("%1+%2$R").arg(color_bad).arg(random - base);
      }
      else
      {
         buf2 = QStringLiteral("%1+%2$R").arg(color_good).arg(random - base);
      }
      buf += buf2;
   }
   else if (random - base < 0)
   {
      // if negative show "- difference"
      if (swapcolors)
      {
         buf2 = QStringLiteral("%s%d$R").arg(color_good).arg(random - base);
      }
      else
      {
         buf2 = QStringLiteral("%1%2$R").arg(color_bad).arg(random - base);
      }
      buf += buf2;
   }
   buf += "$R";

   return buf;
}
void showStatDiff(Character *ch, int base, int random, bool swapcolors)
{
   ch->send(ch->getStatDiff(base, random, swapcolors));
}

bool identify(Character *ch, Object *obj)
{
   if (ch == nullptr || obj == nullptr)
   {
      return false;
   }

   char buf[MAX_STRING_LENGTH] = {0}, buf2[256] = {0};
   int i = 0, value = 0, bits = 0;
   bool found = false;

   if (obj->isDark() && !ch->isImmortalPlayer())
   {
      ch->sendln("A magical aura around the item attempts to conceal its secrets.");
      return false;
   }

   if (obj->short_description)
   {
      ch->send(QStringLiteral("$3Short description: $R%1\r\n").arg(obj->short_description));
   }
   else
   {
      ch->sendln("$3Short description: $R");
   }

   ch->send(QStringLiteral("$3Keywords: '$R%1$3'$R\r\n").arg(obj->name));

   sprinttype(GET_ITEM_TYPE(obj), item_types, buf2);
   csendf(ch, "$3Item type: $R%s\r\n", buf2);

   sprintbit(obj->obj_flags.extra_flags, Object::extra_bits, buf);
   ch->send(QStringLiteral("$3Extra flags: $R%1\r\n").arg(buf));

   sprintbit(obj->obj_flags.more_flags, Object::more_obj_bits, buf2);
   csendf(ch, "$3More flags: $R%s\r\n", buf2);

   if (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) || isSet(obj->obj_flags.more_flags, ITEM_NPC_CORPSE) || isSet(obj->obj_flags.more_flags, ITEM_PC_CORPSE) || isSet(obj->obj_flags.more_flags, ITEM_PC_CORPSE_LOOTED))
   {
      csendf(ch, "$3Owner: $R%s\r\n", obj->getOwner().toStdString().c_str());
   }

   sprintbit(obj->obj_flags.wear_flags, Object::wear_bits, buf);
   ch->send(QStringLiteral("$3Worn on: $R%1\r\n").arg(buf));

   sprintbit(obj->obj_flags.size, Object::size_bits, buf);
   ch->send(QStringLiteral("$3Worn by: $R%1\r\n").arg(buf));

   ch->send(QStringLiteral("$3Level: $R%1\n\r").arg(obj->obj_flags.eq_level));
   ch->send(QStringLiteral("$3Weight: $R%1\r\n").arg(obj->obj_flags.weight));
   ch->send(QStringLiteral("$3Value: $R%1\r\n").arg(obj->obj_flags.cost));

   const Object *vobj = nullptr;
   if (obj->item_number >= 0)
   {
      const int vnum = DC::getInstance()->obj_index[obj->item_number].virt;
      if (vnum >= 0)
      {
         const int rn_of_vnum = real_object(vnum);
         if (rn_of_vnum >= 0)
         {
            vobj = (Object *)DC::getInstance()->obj_index[rn_of_vnum].item;
         }
      }
   }

   switch (GET_ITEM_TYPE(obj))
   {

   case ITEM_SCROLL:
   case ITEM_POTION:
      csendf(ch, "$3Level $R%d ", obj->obj_flags.value[0]);

      if (vobj != nullptr)
      {
         csendf(ch, "(");
         showStatDiff(ch, vobj->obj_flags.value[0], obj->obj_flags.value[0]);
         csendf(ch, ") ");
      }
      ch->sendln("$3spells of:$R");

      if (obj->obj_flags.value[1] >= 1)
      {
         sprinttype(obj->obj_flags.value[1] - 1, spells, buf);
         strcat(buf, "\n\r");
         ch->send(buf);
      }
      if (obj->obj_flags.value[2] >= 1)
      {
         sprinttype(obj->obj_flags.value[2] - 1, spells, buf);
         strcat(buf, "\n\r");
         ch->send(buf);
      }
      if (obj->obj_flags.value[3] >= 1)
      {
         sprinttype(obj->obj_flags.value[3] - 1, spells, buf);
         strcat(buf, "\n\r");
         ch->send(buf);
      }
      break;

   case ITEM_WAND:
   case ITEM_STAFF:
      sprintf(buf, "$3Has $R%d$3 charges, with $R%d$3 charges left.$R\n\r",
              obj->obj_flags.value[1],
              obj->obj_flags.value[2]);
      ch->send(buf);

      sprintf(buf, "$3Level $R%d$3 spell of:$R\n\r", obj->obj_flags.value[0]);
      ch->send(buf);

      if (obj->obj_flags.value[3] >= 1)
      {
         sprinttype(obj->obj_flags.value[3] - 1, spells, buf);
         strcat(buf, "\n\r");
         ch->send(buf);
      }
      break;

   case ITEM_WEAPON:
      csendf(ch, "$3Damage Dice are '$R%dD%d$3'$R",
             obj->obj_flags.value[1],
             obj->obj_flags.value[2]);

      if (vobj != nullptr)
      {
         csendf(ch, " (");
         showStatDiff(ch, vobj->obj_flags.value[1], obj->obj_flags.value[1]);
         csendf(ch, "D");
         showStatDiff(ch, vobj->obj_flags.value[2], obj->obj_flags.value[2]);
         csendf(ch, ")");
      }
      ch->sendln("");

      int get_weapon_damage_type(Object * wielded);
      bits = get_weapon_damage_type(obj) - 1000;
      extern char *strs_damage_types[];
      csendf(ch, "$3Damage type$R: %s\r\n", strs_damage_types[bits]);
      break;

   case ITEM_INSTRUMENT:
      sprintf(buf, "$3Affects non-combat singing by '$R%d$3'$R\r\n$3Affects combat singing by '$R%d$3'$R\r\n",
              obj->obj_flags.value[0],
              obj->obj_flags.value[1]);
      ch->send(buf);
      break;

   case ITEM_MISSILE:
      sprintf(buf, "$3Damage Dice are '$R%dD%d$3'$R\n\rIt is +%d to arrow hit and +%d to arrow damage\r\n",
              obj->obj_flags.value[0],
              obj->obj_flags.value[1],
              obj->obj_flags.value[2],
              obj->obj_flags.value[3]);
      ch->send(buf);
      break;

   case ITEM_FIREWEAPON:
      sprintf(buf, "$3Bow is +$R%d$3 to arrow hit and +$R%d$3 to arrow damage.$R\r\n",
              obj->obj_flags.value[0],
              obj->obj_flags.value[1]);
      ch->send(buf);
      break;

   case ITEM_ARMOR:

      if (isSet(obj->obj_flags.extra_flags, ITEM_ENCHANTED))
      {
         value = (obj->obj_flags.value[0]) - (obj->obj_flags.value[1]);
      }
      else
      {
         value = obj->obj_flags.value[0];
      }

      sprintf(buf, "$3AC-apply is $R%d (", value);
      ch->send(buf);
      if (vobj != nullptr)
      {
         showStatDiff(ch, vobj->obj_flags.value[0], obj->obj_flags.value[0]);
      }
      if (isSet(obj->obj_flags.extra_flags, ITEM_ENCHANTED))
      {
         csendf(ch, "-%d", obj->obj_flags.value[1]);
      }
      csendf(ch, ")$3     Resistance to damage is $R%d\n\r", obj->obj_flags.value[2]);
      break;
   }

   found = false;

   for (i = 0; i < obj->num_affects; i++)
   {
      if ((obj->affected[i].location != APPLY_NONE) &&
          (obj->affected[i].modifier != 0 ||
           (vobj != nullptr &&
            i < vobj->num_affects &&
            vobj->affected != nullptr &&
            vobj->affected[i].location == obj->affected[i].location)))
      {
         if (!found)
         {
            ch->sendln("$3Can affect you as:$R");
            found = true;
         }

         if (obj->affected[i].location < 1000)
            sprinttype(obj->affected[i].location, apply_types, buf2);
         else if (!get_skill_name(obj->affected[i].location / 1000).isEmpty())
            strcpy(buf2, get_skill_name(obj->affected[i].location / 1000).toStdString().c_str());
         else
            strcpy(buf2, "Invalid");
         csendf(ch, "    $3Affects : $R%s$3 By $R%d", buf2, obj->affected[i].modifier);

         if (vobj != nullptr &&
             i < vobj->num_affects &&
             vobj->affected != nullptr &&
             vobj->affected[i].location == obj->affected[i].location)
         {
            csendf(ch, " (");
            // Swap color for ARMOR so lower values use "good" color
            if (vobj->affected[i].location == 17)
            {
               showStatDiff(ch, vobj->affected[i].modifier, obj->affected[i].modifier, true);
            }
            else
            {
               showStatDiff(ch, vobj->affected[i].modifier, obj->affected[i].modifier);
            }

            csendf(ch, ")");
         }

         csendf(ch, "\r\n", buf);
      }
   }

   return true;
}

command_return_t Character::do_identify(QStringList arguments, cmd_t cmd)
{
   if (arguments.isEmpty())
   {
      send("What object do you want to identify?\r\n");
      return eFAILURE;
   }
   QString arg1 = arguments.at(0);

   QRegularExpression re("^v(?<vnum>\\d+)$");
   QRegularExpressionMatch match = re.match(arg1);
   Object *obj = nullptr;
   if (match.hasMatch())
   {
      QString buffer = match.captured("vnum");

      vnum_t vnum = buffer.toULongLong();
      vnum_t rnum = real_object(vnum);
      if (rnum == -1)
      {
         send("Invalid VNUM.\r\n");
         return eFAILURE;
      }
      obj = (Object *)DC::getInstance()->obj_index[rnum].item;

      if (obj->isDark() && !isImmortalPlayer())
      {
         send("This object cannot be identified by mortals.\r\n");
         return eFAILURE;
      }
      return identify(this, obj);
   }
   else
   {
      Character *tmp_char;
      int bits = generic_find(arg1.toStdString().c_str(), FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, this, &tmp_char, &obj, true);
      if (bits && obj)
      {
         if (identify(this, obj))
         {
            return eSUCCESS;
         }
      }
      else
      {
         send(QStringLiteral("You could not find %1 in your inventory, among your equipment or in this room.\r\n").arg(arg1));
      }
   }

   return eFAILURE;
}

int do_look(Character *ch, const char *argument, cmd_t cmd)
{
   char buffer[MAX_STRING_LENGTH] = {0};
   char arg1[MAX_STRING_LENGTH] = {0};
   char arg2[MAX_STRING_LENGTH] = {0};
   char arg3[MAX_STRING_LENGTH] = {0};
   char tmpbuf[MAX_STRING_LENGTH] = {0};
   int keyword_no = 0;
   int j = 0, bits = 0, temp = 0;
   int door = 0, original_loc = 0;
   bool found = 0;
   class Object *tmp_object = nullptr, *found_object = nullptr;
   Character *tmp_char = nullptr;
   char *tmp_desc = nullptr;
   static const char *keywords[] = {"north", "east", "south", "west", "up", "down",
                                    "in", "at", "out", "through", "", /* Look at '' case */
                                    "\n"};

   int weight_in(class Object * obj);
   if (!ch->desc)
      return 1;
   if (GET_POS(ch) < position_t::SLEEPING)
      ch->sendln("You can't see anything but stars!");
   else if (GET_POS(ch) == position_t::SLEEPING)
      ch->sendln("You can't see anything, you're sleeping!");
   else if (check_blind(ch))
   {
      ansi_color(GREY, ch);
      return eSUCCESS;
   }
   else if (IS_DARK(ch->in_room) && (!IS_NPC(ch) && !ch->player->holyLite))
   {
      ch->sendln("It is pitch black...");
      ch->list_char_to_char(DC::getInstance()->world[ch->in_room].people, 0);
      ch->send("$R");
      // TODO - if have blindfighting, list some of the room exits sometimes
   }
   else
   {
      argument_split_3(argument, arg1, arg2, arg3);
      keyword_no = search_block(arg1, keywords, false); /* Partial Match */

      if ((keyword_no == -1) && *arg1)
      {
         keyword_no = 7;
         strcpy(arg2, arg1); /* Let arg2 become the target object (arg1) */
      }

      found = false;
      tmp_object = 0;
      tmp_char = 0;
      tmp_desc = 0;

      original_loc = ch->in_room;
      switch (keyword_no)
      {
      /* look <dir> */
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      {
         /* Check if there is an extra-desc with "up"(or whatever) and use that instead */
         tmp_desc = find_ex_description(arg1,
                                        DC::getInstance()->world[ch->in_room].ex_description);
         if (tmp_desc)
         {
            page_string(ch->desc, tmp_desc, 0);
            return eSUCCESS;
         }

         if (EXIT(ch, keyword_no))
         {
            if (EXIT(ch, keyword_no)->general_description && strlen(EXIT(ch, keyword_no)->general_description))
            {
               send_to_char(EXIT(ch, keyword_no)->general_description, ch);
            }
            else
            {
               ch->sendln("You see nothing special.");
            }

            if (isSet(EXIT(ch, keyword_no)->exit_info, EX_CLOSED) && !isSet(EXIT(ch, keyword_no)->exit_info, EX_HIDDEN) && (EXIT(ch, keyword_no)->keyword))
            {
               sprintf(buffer, "The %s is closed.\r\n", fname(EXIT(ch, keyword_no)->keyword).toStdString().c_str());
               ch->send(buffer);
            }
            else
            {
               if (isSet(EXIT(ch, keyword_no)->exit_info, EX_ISDOOR) && !isSet(EXIT(ch, keyword_no)->exit_info, EX_HIDDEN) &&
                   EXIT(ch, keyword_no)->keyword)
               {
                  sprintf(buffer, "The %s is open.\r\n", fname(EXIT(ch, keyword_no)->keyword).toStdString().c_str());
                  ch->send(buffer);
               }
            }
         }
         else
         {
            ch->sendln("You see nothing special.");
         }
      }
      break;

         /* look 'in'	 */
      case 6:
      {
         if (*arg2)
         {
            /* Item carried */

            bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

            if (bits)
            { /* Found something */
               if (GET_ITEM_TYPE(tmp_object) == ITEM_DRINKCON)
               {
                  if (tmp_object->obj_flags.value[1] <= 0)
                  {
                     act("It is empty.", ch, 0, 0, TO_CHAR, 0);
                  }
                  else
                  {
                     temp = ((tmp_object->obj_flags.value[1] * 3) / tmp_object->obj_flags.value[0]);
                     if (temp > 3)
                     {
                        logf(IMMORTAL, DC::LogChannel::LOG_WORLD,
                             "Bug in object %d. v2: %d > v1: %d. Resetting.",
                             DC::getInstance()->obj_index[tmp_object->item_number].virt,
                             tmp_object->obj_flags.value[1],
                             tmp_object->obj_flags.value[0]);
                        tmp_object->obj_flags.value[1] =
                            tmp_object->obj_flags.value[0];
                        temp = 3;
                     }

                     sprintf(buffer, "It's %sfull of a %s liquid.\r\n",
                             fullness[temp],
                             color_liquid[tmp_object->obj_flags.value[2]]);
                     ch->send(buffer);
                  }
               }
               else if (ARE_CONTAINERS(tmp_object))
               {
                  if (!isSet(tmp_object->obj_flags.value[1], CONT_CLOSED))
                  {
                     send_to_char(fname(tmp_object->name), ch);
                     switch (bits)
                     {
                     case FIND_OBJ_INV:
                        ch->send(" (carried) ");
                        break;
                     case FIND_OBJ_ROOM:
                        ch->send(" (in room) ");
                        break;
                     case FIND_OBJ_EQUIP:
                        ch->send(" (equipped) ");
                        break;
                     }

                     if (tmp_object->obj_flags.value[0] && tmp_object->obj_flags.weight)
                     {

                        int weight_in(class Object * obj);
                        if (DC::getInstance()->obj_index[tmp_object->item_number].virt == 536)
                           temp = (3 * weight_in(tmp_object)) / tmp_object->obj_flags.value[0];
                        else
                           temp = ((tmp_object->obj_flags.weight * 3) / tmp_object->obj_flags.value[0]);
                     }
                     else
                     {
                        temp = 3;
                     }

                     if (temp < 0)
                     {
                        temp = 0;
                     }
                     else if (temp > 3)
                     {
                        temp = 3;
                        logf(IMMORTAL, DC::LogChannel::LOG_WORLD,
                             "Bug in object %d. Weight: %d v1: %d",
                             DC::getInstance()->obj_index[tmp_object->item_number].virt,
                             tmp_object->obj_flags.weight,
                             tmp_object->obj_flags.value[0]);
                     }

                     if (NOT_KEYRING(tmp_object))
                     {
                        csendf(ch, "(%sfull) : \r\n", fullness[temp]);
                     }
                     else
                     {
                        ch->sendln(": ");
                     }

                     ch->list_obj_to_char(tmp_object->contains, 2, true);
                  }
                  else
                     ch->sendln("It is closed.");
               }
               else
               {
                  ch->sendln("That is not a container.");
               }
            }
            else
            { /* wrong argument */
               ch->sendln("You do not see that item here.");
            }
         }
         else
         { /* no argument */
            ch->sendln("Look in what?!");
         }
      }
      break;

         /* look 'at'	 */
      case 7:
      {
         if (*arg2)
         {

            bits = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &tmp_char, &found_object);

            if (tmp_char)
            {
               if (*arg3)
               {
                  try_to_peek_into_container(tmp_char, ch, arg3);
                  return eSUCCESS;
               }
               if (cmd == cmd_t::GLANCE)
                  show_char_to_char(tmp_char, ch, 3);
               else
                  show_char_to_char(tmp_char, ch, 1);
               if (ch != tmp_char)
               {
                  if (!IS_NPC(ch) && (tmp_char->getLevel() < ch->player->wizinvis))
                  {
                     return eSUCCESS;
                  }
                  if ((cmd == cmd_t::GLANCE) && !IS_AFFECTED(ch, AFF_HIDE))
                  {
                     act("$n glances at you.", ch, 0, tmp_char, TO_VICT,
                         INVIS_NULL);
                     act("$n glances at $N.", ch, 0, tmp_char, TO_ROOM,
                         INVIS_NULL | NOTVICT);
                  }
                  else if (!IS_AFFECTED(ch, AFF_HIDE))
                  {
                     act("$n looks at you.", ch, 0, tmp_char, TO_VICT,
                         INVIS_NULL);
                     act("$n looks at $N.", ch, 0, tmp_char, TO_ROOM,
                         INVIS_NULL | NOTVICT);
                  }
               }
               return eSUCCESS;
            }

            /* Search for Extra Descriptions in room and items */

            /* Extra description in room?? */
            if (!found)
            {
               tmp_desc = find_ex_description(arg2,
                                              DC::getInstance()->world[ch->in_room].ex_description);
               if (tmp_desc)
               {
                  page_string(ch->desc, tmp_desc, 0);
                  return eSUCCESS; /* RETURN SINCE IT WAS ROOM DESCRIPTION */
                                   /* Old system was: found = true; */
               }
            }

            /* Search for extra descriptions in items */

            /* Equipment Used */

            if (!found)
            {
               for (j = 0; j < MAX_WEAR && !found; j++)
               {
                  if (ch->equipment[j])
                  {
                     if (CAN_SEE_OBJ(ch, ch->equipment[j]))
                     {
                        tmp_desc = find_ex_description(arg2,
                                                       ch->equipment[j]->ex_description);
                        if (tmp_desc)
                        {
                           page_string(ch->desc, tmp_desc, 1);
                           return eSUCCESS;
                           //                              found = true;
                        }
                     }
                  }
               }
            }

            /* In inventory */

            if (!found)
            {
               for (tmp_object = ch->carrying; tmp_object && !found;
                    tmp_object = tmp_object->next_content)
               {
                  if (CAN_SEE_OBJ(ch, tmp_object))
                  {
                     tmp_desc = find_ex_description(arg2,
                                                    tmp_object->ex_description);
                     if (tmp_desc)
                     {
                        page_string(ch->desc, tmp_desc, 1);
                        return eSUCCESS;
                        //                           found = true;
                     }
                  }
               }
            }

            /* Object In room */

            if (!found)
            {
               for (tmp_object = DC::getInstance()->world[ch->in_room].contents;
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
                        return eSUCCESS;
                        //                           found = true;
                     }
                  }
               }
            }
            /* wrong argument */

            if (bits)
            { /* If an object was found */
               if (!found)
                  /* Show no-description */
                  ch->show_obj_to_char(found_object, 5);
               else
                  /* Find hum, glow etc */
                  ch->show_obj_to_char(found_object, 6);
            }
            else if (!found)
            {
               ch->sendln("You do not see that here.");
            }
         }
         else
         {
            /* no argument */
            ch->sendln("Look at what?");
         }
      }
      break;
      case 8:
      { // look out
         for (tmp_object = DC::getInstance()->object_list; tmp_object;
              tmp_object = tmp_object->next)
         {
            if (tmp_object->isPortal() && tmp_object->getPortalDestinationRoom() == ch->in_room && tmp_object->in_room != DC::NOWHERE && tmp_object->isPortalTypePermanent())
            {
               ch->in_room = tmp_object->in_room;
               found = true;
               break;
            }
         }
         if (found != true)
         {
            ch->sendln("Nothing much to see there.");
            return eFAILURE;
         }
      }
         /* no break */
      case 9: // look through
         if (found != true)
         {
            if (*arg2)
            {
               if ((tmp_object = get_obj_in_list_vis(ch, arg2,
                                                     DC::getInstance()->world[ch->in_room].contents)))
               {
                  if (tmp_object->isPortal())
                  {
                     if (tmp_object->isPortalTypePlayer() || tmp_object->isPortalTypePermanentNoLook())
                     {
                        sprintf(tmpbuf,
                                "You look through %s but it seems to be opaque.\r\n",
                                tmp_object->short_description);
                        ch->send(tmpbuf);
                        return eFAILURE;
                     }
                     if ((ch->in_room = real_room(tmp_object->getPortalDestinationRoom())) == DC::NOWHERE)
                        ch->in_room = original_loc;
                     else
                        found = true;
                  }
               }
               else
               {
                  ch->sendln("Look through what?");
                  return eFAILURE;
               }
            }
         }

         if (found != true)
         {
            ch->sendln("You can't seem to look through that.");
            return eFAILURE;
         }
         /* no break */
         /* no break */
         /* look ''		*/
      case 10:
      {
         char sector_buf[50];
         char rflag_buf[MAX_STRING_LENGTH];
         char tempflag_buf[MAX_STRING_LENGTH];

         ansi_color(GREY, ch);
         ansi_color(BOLD, ch);
         send_to_char(DC::getInstance()->world[ch->in_room].name, ch);
         ansi_color(NTEXT, ch);
         ansi_color(GREY, ch);

         // PUT SECTOR AND ROOMFLAG STUFF HERE
         if (!IS_NPC(ch) && ch->player->holyLite)
         {
            sprinttype(DC::getInstance()->world[ch->in_room].sector_type, sector_types,
                       sector_buf);
            sprintbit((int32_t)DC::getInstance()->world[ch->in_room].room_flags, room_bits,
                      rflag_buf);
            csendf(ch, "\r\nLight[%d] <%s> [ %s]", DARK_AMOUNT(ch->in_room), sector_buf, rflag_buf);
            if (DC::getInstance()->world[ch->in_room].temp_room_flags)
            {
               sprintbit((int32_t)DC::getInstance()->world[ch->in_room].temp_room_flags, temp_room_bits,
                         tempflag_buf);
               ch->send(QStringLiteral(" [ %1]").arg(tempflag_buf));
            }
         }

         ch->sendln("");

         if (!IS_NPC(ch) && !isSet(ch->player->toggles, Player::PLR_BRIEF))
            send_to_char(DC::getInstance()->world[ch->in_room].description, ch);

         ansi_color(BLUE, ch);
         ansi_color(BOLD, ch);
         ch->list_obj_to_char(DC::getInstance()->world[ch->in_room].contents, 0, false);
         ch->list_char_to_char(DC::getInstance()->world[ch->in_room].people, 0);

         strcpy(buffer, "");
         *buffer = '\0';
         for (int doorj = 0; doorj <= 5; doorj++)
         {

            int is_closed;
            int is_hidden;

            // cheesy way of making it list west before east in 'look'
            if (doorj == 1)
               door = 3;
            else if (doorj == 3)
               door = 1;
            else
               door = doorj;

            if (!EXIT(ch, door) || EXIT(ch, door)->to_room == DC::NOWHERE)
               continue;
            is_closed = isSet(EXIT(ch, door)->exit_info, EX_CLOSED);
            is_hidden = isSet(EXIT(ch, door)->exit_info, EX_HIDDEN);

            if (IS_NPC(ch) || ch->player->holyLite)
            {
               if (is_closed && is_hidden)
                  sprintf(buffer + strlen(buffer), "$B($R%s-closed$B)$R ",
                          keywords[door]);
               else
                  sprintf(buffer + strlen(buffer), "%s%s ",
                          keywords[door], is_closed ? "-closed" : "");
            }
            else if (!(is_closed && is_hidden))
               sprintf(buffer + strlen(buffer), "%s%s ", keywords[door],
                       is_closed ? "-closed" : "");
         }
         ansi_color(NTEXT, ch);
         ch->send("Exits: ");
         if (*buffer)
            ch->send(buffer);
         else
            ch->send("None.");
         ch->sendln("");
         if (IS_PC(ch) && !ch->hunting.isEmpty())
            ch->do_track(QString(ch->hunting).split(' '), cmd_t::TRACK);
      }
         ch->in_room = original_loc;
         break;

         /* wrong arg 	*/
      case -1:
         ch->sendln("Sorry, I didn't understand that!");
         break;
      }

      ansi_color(NTEXT, ch);
   }
   ansi_color(GREY, ch);
   return eSUCCESS;
}

/* end of look */

int do_read(Character *ch, char *arg, cmd_t cmd)
{
   char buf[200];

   // This is just for now - To be changed later.!

   // yeah right.  -Sadus
   sprintf(buf, "at %s", arg);
   do_look(ch, buf);
   return eSUCCESS;
}

int do_examine(Character *ch, char *argument, cmd_t cmd)
{
   char name[200], buf[200];
   Character *tmp_char;
   class Object *tmp_object;

   sprintf(buf, "at %s", argument);
   do_look(ch, buf);

   one_argument(argument, name);

   if (!*name)
   {
      ch->sendln("Examine what?");
      return eFAILURE;
   }

   generic_find(name, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &tmp_char, &tmp_object, true);

   if (tmp_object)
   {
      if (GET_ITEM_TYPE(tmp_object) == ITEM_DRINKCON || ARE_CONTAINERS(tmp_object))
      {
         ch->sendln("When you look inside, you see:");
         sprintf(buf, "in %s", argument);
         do_look(ch, buf);
      }
   }
   return eSUCCESS;
}

int do_exits(Character *ch, char *argument, cmd_t cmd)
{
   int door;
   char buf[MAX_STRING_LENGTH];
   char *exits[] = {
       "North",
       "East ",
       "South",
       "West ",
       "Up   ",
       "Down "};

   *buf = '\0';

   if (check_blind(ch))
      return eFAILURE;

   for (door = 0; door <= 5; door++)
   {
      if (!EXIT(ch, door) || EXIT(ch, door)->to_room == DC::NOWHERE)
         continue;

      if (!IS_NPC(ch) && ch->player->holyLite)
         sprintf(buf + strlen(buf), "%s - %s [%d]\n\r", exits[door],
                 DC::getInstance()->world[EXIT(ch, door)->to_room].name,
                 DC::getInstance()->world[EXIT(ch, door)->to_room].number);
      else if (isSet(EXIT(ch, door)->exit_info, EX_CLOSED))
      {
         if (isSet(EXIT(ch, door)->exit_info, EX_HIDDEN))
            continue;
         else
            sprintf(buf + strlen(buf), "%s - (Closed)\n\r", exits[door]);
      }
      else if (IS_DARK(EXIT(ch, door)->to_room))
         sprintf(buf + strlen(buf), "%s - Too dark to tell\n\r", exits[door]);
      else
         sprintf(buf + strlen(buf), "%s leads to %s.\r\n", exits[door],
                 DC::getInstance()->world[EXIT(ch, door)->to_room].name);
   }

   ch->sendln("You scan around the exits to see where they lead.");

   if (buf[0])
      ch->send(buf);
   else
      ch->sendln("None.");

   return eSUCCESS;
}

char frills[] = {
    'o',
    '/',
    '~',
    '\\'};

int do_score(Character *ch, char *argument, cmd_t cmd)
{
   char race[100];
   char buf[MAX_STRING_LENGTH], scratch;
   int level = 0;
   int to_dam, to_hit, spell_dam;
   // int flying = 0;
   bool affect_found[AFF_MAX + 1] = {false};
   bool modifyOutput;

   struct affected_type *aff;

   int64_t exp_needed;
   uint32_t immune = 0, suscept = 0, resist = 0;
   std::string isrString;
   // int i;

   sprintf(race, "%s", races[(int)GET_RACE(ch)].singular_name);
   exp_needed = (exp_table[(int)ch->getLevel() + 1] - GET_EXP(ch));

   to_hit = GET_REAL_HITROLL(ch);
   to_dam = GET_REAL_DAMROLL(ch);
   spell_dam = getRealSpellDamage(ch);

   sprintf(buf,
           "$7($5:$7)================================================="
           "========================($5:$7)\n\r"
           "|=| %-30s  -- Character Attributes (DarkCastleMUD) |=|\n\r"
           "($5:$7)=============================($5:$7)================="
           "========================($5:$7)\n\r",
           GET_SHORT(ch));

   ch->send(buf);

   sprintf(buf,
           "|\\| $4Strength$7:        %4d  (%2d) |/| $1Race$7:  %-10s  $1HitPts$7:%5d$1/$7(%5d) |~|\n\r"
           "|~| $4Dexterity$7:       %4d  (%2d) |o| $1Class$7: %-11s $1Mana$7:   %4d$1/$7(%5d) |\\|\n\r"
           "|/| $4Constitution$7:    %4d  (%2d) |\\| $1Level$7:  %3d        $1Fatigue$7:%4d$1/$7(%5d) |o|\n\r"
           "|o| $4Intelligence$7:    %4d  (%2d) |~| $1Height$7: %3d        $1Ki$7:     %4d$1/$7(%5d) |/|\n\r"
           "|\\| $4Wisdom$7:          %4d  (%2d) |/| $1Weight$7: %3d        $1Rdeaths$7:   %-5d     |~|\n\r"
           "|~| $3Rgn$7: $4H$7:%3d $4M$7:%3d $4V$7:%3d $4K$7:%2d |o| $1Age$7:    %3d yrs    $1Align$7: %+5d         |\\|\n\r",
           GET_STR(ch), GET_RAW_STR(ch), race, ch->getHP(), GET_MAX_HIT(ch),
           GET_DEX(ch), GET_RAW_DEX(ch), pc_clss_types[(int)GET_CLASS(ch)], GET_MANA(ch), GET_MAX_MANA(ch),
           GET_CON(ch), GET_RAW_CON(ch), ch->getLevel(), GET_MOVE(ch), GET_MAX_MOVE(ch),
           GET_INT(ch), GET_RAW_INT(ch), GET_HEIGHT(ch), GET_KI(ch), GET_MAX_KI(ch),
           GET_WIS(ch), GET_RAW_WIS(ch), GET_WEIGHT(ch), IS_NPC(ch) ? 0 : GET_RDEATHS(ch), ch->hit_gain_lookup(),
           ch->mana_gain_lookup(), ch->move_gain_lookup(), ch->ki_gain_lookup(), GET_AGE(ch),
           GET_ALIGNMENT(ch));
   ch->send(buf);

   if (IS_PC(ch)) // mobs can't view this part
   {
      QString experience_needed;
      if (ch->isImmortalPlayer())
      {
         experience_needed = "0";
      }
      else
      {
         experience_needed = QStringLiteral("%L1").arg(exp_needed);
      }

      int instrument_combat{}, instrument_non_combat{};
      get_instrument_bonus(ch, instrument_combat, instrument_non_combat);
      ch->sendln(QStringLiteral("($5:$7)=============================($5:$7)===($5:$7)===================================($5:$7)\n\r"
                                "|/| $2Combat Statistics:$7                |\\| $2Equipment and Valuables:$7          |o|\n\r"
                                "|o|  $3Armor$7:   %1   $3Pkills$7:  %2  |~|  $3Items Carried$7:  %3 |/|\n\r"
                                "|\\|  $3BonusHit$7: %4   $3PDeaths$7: %5  |/|  $3Weight Carried$7: %6 |~|\n\r"
                                "|~|  $3BonusDam$7: %7   $3SplDam$7:  %8  |o|  $3Experience$7:   %L9 |\\|\n\r"
                                "|/|  $3InstrCom$7: %10   $3InstrNon$7:%11  |\\|  $3ExpTillLevel$7: %L12 |/|\r\n"
                                "|o|  $B$4FIRE$R[%13]  $B$3COLD$R[%14]  $B$5NRGY$R[%15]  |\\|  $3Gold$7: %L16 |o|\n\r"
                                "|\\|  $B$2ACID$R[%17]  $B$7MAGK$R[%18]  $2POIS$7[%19]  |~|  $3Bank$7: %L20 |/|\n\r"
                                "|-|  $3MELE$R[%21]  $3SPEL$R[%22]   $3KI$R [%23]  |/|  $3QPoints$7: %L24 $3Platinum$7: %L25 |-|\n\r"
                                "|/|                                   |o|                                   |/|\r\n"
                                "($5:$7)===================================($5:$7)===================================($5:$7)")
                     .arg(GET_ARMOR(ch), 5)
                     .arg(GET_PKILLS(ch), 5)
                     .arg(QStringLiteral("%1/%2").arg(IS_CARRYING_N(ch)).arg(CAN_CARRY_N(ch)), 16)
                     .arg(QStringLiteral("%1%2").arg(to_hit > 0 ? "+" : "").arg(to_hit), 4)
                     .arg(QString::number(GET_PDEATHS(ch)), 5)
                     .arg(QStringLiteral("%1/%2").arg(IS_CARRYING_W(ch)).arg(CAN_CARRY_W(ch)), 16)
                     .arg(QStringLiteral("%1%2").arg(to_dam > 0 ? "+" : "").arg(to_dam), 4)                                                   // +4d
                     .arg(QStringLiteral("%1%2").arg(spell_dam > 0 ? "+" : "").arg(spell_dam), 5)                                             //+5d
                     .arg(GET_EXP(ch), 18)                                                                                                    // -18s
                     .arg(QStringLiteral("%1%2").arg(instrument_combat > 0 ? "+" : "").arg(instrument_combat), 4)                             //+4
                     .arg(QStringLiteral("%1%2").arg(instrument_non_combat > 0 ? "+" : "").arg(instrument_non_combat), 5)                     //+5
                     .arg(experience_needed, 18)                                                                                              // -18s
                     .arg(QStringLiteral("%1%2").arg(get_saves(ch, SAVE_TYPE_FIRE) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_FIRE)), 3)     //+3
                     .arg(QStringLiteral("%1%2").arg(get_saves(ch, SAVE_TYPE_COLD) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_COLD)), 3)     //+3
                     .arg(QStringLiteral("%1%2").arg(get_saves(ch, SAVE_TYPE_ENERGY) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_ENERGY)), 3) //+3
                     .arg(ch->getGold(), 26)
                     .arg(QStringLiteral("%1%2").arg(get_saves(ch, SAVE_TYPE_ACID) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_ACID)), 3)     //+3
                     .arg(QStringLiteral("%1%2").arg(get_saves(ch, SAVE_TYPE_MAGIC) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_MAGIC)), 3)   //+3
                     .arg(QStringLiteral("%1%2").arg(get_saves(ch, SAVE_TYPE_POISON) > 0 ? "+" : "").arg(get_saves(ch, SAVE_TYPE_POISON)), 3) //+3
                     .arg(GET_BANK(ch), 26)                                                                                                   // -25s
                     .arg(QStringLiteral("%1%2").arg(ch->melee_mitigation > 0 ? "+" : "").arg(ch->melee_mitigation), 3)                       // +3
                     .arg(QStringLiteral("%1%2").arg(ch->spell_mitigation > 0 ? "+" : "").arg(ch->spell_mitigation), 3)                       // +3
                     .arg(QStringLiteral("%1%2").arg(ch->song_mitigation > 0 ? "+" : "").arg(ch->song_mitigation), 3)                         // +3
                     .arg(GET_QPOINTS(ch), 5)                                                                                                 // -25s
                     .arg(GET_PLATINUM(ch), 7));                                                                                              // -6d
   }
   else
      ch->sendln("($5:$7)===================================($5:$7)==================================($5:$7)");
   int found = false;

   if ((immune = ch->immune))
   {
      for (int i = 0; i <= ISR_MAX; i++)
      {
         isrString = get_isr_string(immune, i);
         if (!isrString.empty())
         {
            scratch = frills[level];
            sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\n\r",
                    scratch, "Immunity", isrString.c_str(), scratch);
            ch->send(buf);
            found = true;
            isrString = std::string();
            if (++level == 4)
               level = 0;
         }
      }
   }
   if ((suscept = ch->suscept))
   {
      for (int i = 0; i <= ISR_MAX; i++)
      {
         isrString = get_isr_string(suscept, i);
         if (!isrString.empty())
         {
            scratch = frills[level];
            sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\n\r",
                    scratch, "Susceptibility", isrString.c_str(), scratch);
            ch->send(buf);
            found = true;
            isrString = std::string();
            if (++level == 4)
               level = 0;
         }
      }
   }
   if ((resist = ch->resist))
   {
      for (int i = 0; i <= ISR_MAX; i++)
      {
         isrString = get_isr_string(resist, i);
         if (!isrString.empty())
         {
            scratch = frills[level];
            sprintf(buf, "|%c| Affected by %-25s          Modifier %-13s   |%c|\n\r",
                    scratch, "Resistibility", isrString.c_str(), scratch);
            ch->send(buf);
            found = true;
            isrString = std::string();
            if (++level == 4)
               level = 0;
         }
      }
   }

   if ((aff = ch->affected))
   {

      for (; aff; aff = aff->next)
      {
         if (aff->bitvector >= 0 && aff->bitvector <= AFF_MAX)
         {
            assert(aff->bitvector >= 0 && aff->bitvector <= AFF_MAX);
            affect_found[aff->bitvector] = true;
         }
         scratch = frills[level];
         modifyOutput = false;

         // figure out the name of the affect (if any)
         QString aff_name = get_skill_name(aff->type);
         //	 if (aff_name)
         //      if (*aff_name && !str_cmp(aff_name, "fly")) flying = 1;
         switch (aff->type)
         {
         case BASE_SETS + SET_RAGER:
            if (aff->location == 0)
            {
               aff_name = QStringLiteral("Battlerager's Fury");
            }
            break;
         case BASE_SETS + SET_RAGER2:
            if (aff->location == 0)
            {
               aff_name = QStringLiteral("Battlerager's Fury");
            }
            break;
         case BASE_SETS + SET_MOSS:
            if (aff->location == 0)
               aff_name = QStringLiteral("infravision");
            break;
         case Character::PLAYER_CANTQUIT:
            aff_name = QStringLiteral("CANTQUIT");
            break;
         case Character::PLAYER_OBJECT_THIEF:
            aff_name = QStringLiteral("DIRTY_THIEF/CANT_QUIT");
            break;
         case Character::PLAYER_GOLD_THIEF:
            aff_name = QStringLiteral("GOLD_THIEF/CANT_QUIT");
            break;
         case SKILL_HARM_TOUCH:
            aff_name = QStringLiteral("harmtouch reuse timer");
            break;
         case SKILL_LAY_HANDS:
            aff_name = QStringLiteral("layhands reuse timer");
            break;
         case SKILL_QUIVERING_PALM:
            aff_name = QStringLiteral("quiver reuse timer");
            break;
         case SKILL_BLOOD_FURY:
            aff_name = QStringLiteral("blood fury reuse timer");
            break;
         case SKILL_FEROCITY_TIMER:
            aff_name = QStringLiteral("ferocity reuse timer");
            break;
         case SKILL_DECEIT_TIMER:
            aff_name = QStringLiteral("deceit reuse timer");
            break;
         case SKILL_TACTICS_TIMER:
            aff_name = QStringLiteral("tactics reuse timer");
            break;
         case SKILL_CLANAREA_CLAIM:
            aff_name = QStringLiteral("clanarea claim timer");
            break;
         case SKILL_CLANAREA_CHALLENGE:
            aff_name = QStringLiteral("clanarea challenge timer");
            break;
         case SKILL_CRAZED_ASSAULT:
            if (strcmp(apply_types[(int)aff->location], "HITROLL"))
               aff_name = QStringLiteral("crazed assault reuse timer");
            break;
         case SPELL_IMMUNITY:
            aff_name = QStringLiteral("immunity");
            modifyOutput = true;
            break;
         case SKILL_NAT_SELECT:
            aff_name = QStringLiteral("natural selection");
            modifyOutput = true;
            break;
         case SKILL_BREW_TIMER:
            aff_name = QStringLiteral("brew timer");
            break;
         case SKILL_SCRIBE_TIMER:
            aff_name = QStringLiteral("scribe timer");
            break;
         case CONC_LOSS_FIXER:
            aff_name = {}; // We don't want this showing up in score
            break;
         default:
            break;
         }
         if (aff_name.isEmpty()) // not one we want displayed
            continue;

         std::string fading;
         if (IS_AFFECTED(ch, AFF_DETECT_MAGIC))
         {
            if (aff->duration < 3)
            {
               fading = "$2(fading)$7";
            }
         }

         std::string modified = apply_types[(int)aff->location];
         if (modifyOutput)
         {
            if (ch->affected_by_spell(SKILL_NAT_SELECT))
            {
               modified = races[aff->modifier].singular_name;
            }
            else if (ch->affected_by_spell(SPELL_IMMUNITY))
            {
               modified = spells[aff->modifier];
            }
         }

         QString format;
         if (aff->type == Character::PLAYER_CANTQUIT)
         {
            format = "|%1| Affected by %2 from %3%4 Modifier %5 |%1|\r\n";
            format = format.arg(scratch);
            format = format.arg(QString::fromStdString("CANTQUIT"), 8);
            format = format.arg(QString::fromStdString(aff->caster), -12);
            format = format.arg(QString::fromStdString(fading), 8);
            format = format.arg(QString::fromStdString(modified), -15);
         }
         else
         {
            format = "|%1| Affected by %2 %3 Modifier %4 |%1|\r\n";
            format = format.arg(scratch);
            format = format.arg(aff_name, -25);
            format = format.arg(QString::fromStdString(fading), 8);
            format = format.arg(QString::fromStdString(modified), -15);
         }
         ch->send(format.toStdString());

         found = true;
         if (++level == 4)
            level = 0;
      }
   }
   /*  if (flying == 0 && IS_AFFECTED(ch, AFF_FLYING)) {
       scratch = frills[level];
       sprintf(buf, "|%c| Affected by fly                                Modifier NONE            |%c|\n\r",
               scratch, scratch);
       ch->send(buf);
       found = true;
       if(++level == 4)
         level = 0;
     }*/
   extern bool elemental_score(Character * ch, int level);
   if (!found)
      found = elemental_score(ch, level);
   else
      elemental_score(ch, level);

   if (found)
      ch->sendln("($5:$7)=========================================================================($5:$7)");

   found = false;

   for (int aff_idx = 1; aff_idx < (AFF_MAX + 1); aff_idx++)
   {
      if ((!affect_found[aff_idx]) && IS_AFFECTED(ch, aff_idx))
      {
         if (aff_idx == AFF_HIDE || aff_idx == AFF_GROUP)
            continue;

         found = true;
         if (++level == 4)
            level = 0;
         scratch = frills[level];

         if (aff_idx != AFF_REFLECT)
            sprintf(buf, "|%c| Affected by %-25s          Modifier NONE            |%c|\n\r",
                    scratch, affected_bits[aff_idx - 1], scratch);
         else
            sprintf(buf, "|%c| Affected by %-25s          Modifier (%3d)           |%c|\n\r",
                    scratch, affected_bits[aff_idx - 1], ch->spell_reflect, scratch);
         ch->send(buf);
      }
   }

   if (found)
      ch->sendln("($5:$7)=========================================================================($5:$7)");

   if (IS_PC(ch)) // mob can't view this part
   {
      if (ch->getLevel() > IMMORTAL && ch->player->buildLowVnum && ch->player->buildHighVnum)
      {
         if (ch->player->buildLowVnum == ch->player->buildOLowVnum &&
             ch->player->buildLowVnum == ch->player->buildMLowVnum)
         {
            sprintf(buf, "CREATION RANGE: %d-%d\n\r", ch->player->buildLowVnum, ch->player->buildHighVnum);
            ch->send(buf);
         }
         else
         {
            sprintf(buf, "ROOM RANGE: %d-%d\n\r", ch->player->buildLowVnum, ch->player->buildHighVnum);
            ch->send(buf);
            sprintf(buf, "MOB RANGE: %d-%d\n\r", ch->player->buildMLowVnum, ch->player->buildMHighVnum);
            ch->send(buf);
            sprintf(buf, "OBJ RANGE: %d-%d\n\r", ch->player->buildOLowVnum, ch->player->buildOHighVnum);
            ch->send(buf);
         }
      }
   }
   return eSUCCESS;
}

int do_time(Character *ch, char *argument, cmd_t cmd)
{
   char buf[100];
   char const *suf;
   int weekday, day;
   time_t timep;
   int32_t h, m;
   // int32_t s;
   extern struct time_info_data time_info;
   extern char *weekdays[];
   extern char *month_name[];
   struct tm *pTime = nullptr;

   /* 35 days in a month */
   weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

   sprintf(buf, "It is %d o'clock %s, on %s.\r\n",
           ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
           ((time_info.hours >= 12) ? "pm" : "am"),
           weekdays[weekday]);

   ch->send(buf);

   day = time_info.day + 1; /* day in [1..35] */

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

   ch->send(buf);

   // Changed to the below code without seconds in an attempt to stop
   // the timing of bingos... - pir 2/7/1999
   timep = time(0);
   if (ch->getLevel() > IMMORTAL)
   {
      sprintf(buf, "The system time is %ld.\r\n", timep);

      ch->send(buf);
   }

   pTime = localtime(&timep);
   if (!pTime)
      return eFAILURE;

#ifdef __CYGWIN__
   sprintf(buf, "The system time is %d/%d/%d (%d:%02d)\n\r",
           pTime->tm_mon + 1,
           pTime->tm_mday,
           pTime->tm_year + 1900,
           pTime->tm_hour,
           pTime->tm_min);
#else
   sprintf(buf, "The system time is %d/%d/%d (%d:%02d) %s\n\r",
           pTime->tm_mon + 1,
           pTime->tm_mday,
           pTime->tm_year + 1900,
           pTime->tm_hour,
           pTime->tm_min,
           pTime->tm_zone);
#endif
   ch->send(buf);

   timep -= start_time;
   h = timep / 3600;
   m = (timep % 3600) / 60;
   // 	s = timep % 60;
   // 	sprintf (buf, "The mud has been running for: %02li:%02li:%02li \n\r",
   // 			h,m,s);
   sprintf(buf, "The mud has been running for: %02li:%02li \n\r", h, m);
   ch->send(buf);
   return eSUCCESS;
}

int do_weather(Character *ch, char *argument, cmd_t cmd)
{
   extern struct weather_data weather_info;
   char buf[256];

   if (GET_POS(ch) <= position_t::SLEEPING)
   {
      ch->sendln("You dream of being on a tropical island surrounded by beautiful members of the attractive sex.");
      return eSUCCESS;
   }
   if (OUTSIDE(ch))
   {
      sprintf(buf,
              "The sky is %s and %s.\r\n",
              sky_look[weather_info.sky],
              (weather_info.change >= 0 ? "you feel a warm wind from south" : "your foot tells you bad weather is due"));
      act(buf, ch, 0, 0, TO_CHAR, 0);
   }
   else
      ch->sendln("You have no feeling about the weather at all.");

   if (ch->isImmortalPlayer())
   {
      csendf(ch, "Pressure: %4d  Change: %d (- = worse)\r\n",
             weather_info.pressure, weather_info.change);
      csendf(ch, "Sky: %9s  Sunlight: %d\r\n",
             sky_look[weather_info.sky], weather_info.sunlight);
   }
   return eSUCCESS;
}

int do_help(Character *ch, char *argument, cmd_t cmd)
{
   extern struct help_index_element *help_index;
   extern FILE *help_fl;
   extern char help[MAX_STRING_LENGTH];

   int chk, bot, top, mid;
   char buf[90], buffer[MAX_STRING_LENGTH];

   if (!ch->desc)
      return eFAILURE;

   for (; isspace(*argument); argument++)
      ;

   if (*argument)
   {
      if (!help_index)
      {
         ch->sendln("No help available.");
         return eSUCCESS;
      }
      bot = 0;
      top = DC::getInstance()->top_of_helpt;

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
               if ((strlen(buffer) + strlen(buf)) >= MAX_STRING_LENGTH)
                  break;
               strcat(buffer, buf);
               strcat(buffer, "\r");
            }
            page_string(ch->desc, buffer, 1);
            return eSUCCESS;
         }
         else if (bot >= top)
         {
            ch->sendln("There is no help on that word.");
            return 1;
         }
         else if (chk > 0)
            bot = ++mid;
         else
            top = --mid;
      }
   }

   ch->send(help);
   return eSUCCESS;
}

int do_count(Character *ch, char *arg, cmd_t cmd)
{
   class Connection *d;
   Character *i;
   int clss[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   int race[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   int immortal = 0;
   int total = 0;

   for (d = DC::getInstance()->descriptor_list; d; d = d->next)
   {
      if (d->connected || !d->character)
         continue;
      if (!(i = d->original))
         i = d->character;
      if (!CAN_SEE(ch, i))
         continue;
      if (i->getLevel() > MORTAL)
      {
         immortal++;
         total++;
         continue;
      }
      clss[(int)GET_CLASS(i)]++;
      race[(int)GET_RACE(i)]++;
      total++;
   }

   csendf(ch, "There are %d visible players connected, %d of which are immortals.\r\n", total, immortal);
   csendf(ch, "%d warriors, %d clerics, %d mages, %d thieves, %d barbarians, %d monks,\n\r", clss[CLASS_WARRIOR], clss[CLASS_CLERIC], clss[CLASS_MAGIC_USER], clss[CLASS_THIEF], clss[CLASS_BARBARIAN], clss[CLASS_MONK]);
   csendf(ch, "%d paladins, %d antipaladins, %d bards, %d druids, and %d rangers.\r\n",
          clss[CLASS_PALADIN], clss[CLASS_ANTI_PAL], clss[CLASS_BARD], clss[CLASS_DRUID], clss[CLASS_RANGER]);
   csendf(ch, "%d humans, %d elves, %d dwarves, %d hobbits, %d pixies,\n\r", race[RACE_HUMAN], race[RACE_ELVEN], race[RACE_DWARVEN], race[RACE_HOBBIT], race[RACE_PIXIE]);
   csendf(ch, "%d ogres, %d gnomes, %d orcs, %d trolls.\r\n", race[RACE_GIANT], race[RACE_GNOME], race[RACE_ORC], race[RACE_TROLL]);
   csendf(ch, "The maximum number of players since "
              "last reboot was %d.\r\n",
          max_who);
   return eSUCCESS;
}

int do_inventory(Character *ch, char *argument, cmd_t cmd)
{
   ch->sendln("You are carrying:");
   ch->list_obj_to_char(ch->carrying, 1, true);
   return eSUCCESS;
}

int do_equipment(Character *ch, char *argument, cmd_t cmd)
{
   int j;
   bool found;

   ch->sendln("You are using:");
   found = false;
   for (j = 0; j < MAX_WEAR; j++)
   {
      if (ch->equipment[j])
      {
         if (CAN_SEE_OBJ(ch, ch->equipment[j]))
         {
            if (!found)
            {
               act("<    worn     > Item Description     (Flags) [Item Condition]\r\n", ch, 0, 0, TO_CHAR, 0);
               found = true;
            }
            send_to_char(where[j], ch);
            ch->show_obj_to_char(ch->equipment[j], 1);
         }
         else
         {
            send_to_char(where[j], ch);
            ch->sendln("something");
            found = true;
         }
      }
   }
   if (!found)
   {
      ch->sendln("Nothing.");
   }
   return eSUCCESS;
}

int do_credits(Character *ch, char *argument, cmd_t cmd)
{
   page_string(ch->desc, credits, 0);
   return eSUCCESS;
}

int do_story(Character *ch, char *argument, cmd_t cmd)
{
   page_string(ch->desc, story, 0);
   return eSUCCESS;
}
/*
int do_news(Character *ch, char *argument, cmd_t cmd)
{
   page_string(ch->desc, news, 0);
   return eSUCCESS;
}

*/
int do_info(Character *ch, char *argument, cmd_t cmd)
{
   page_string(ch->desc, info, 0);
   return eSUCCESS;
}

/*********------------ locate objects -----------------***************/
int do_olocate(Character *ch, char *name, cmd_t cmd)
{
   char buf[300], buf2[MAX_STRING_LENGTH];
   class Object *k;
   int in_room = 0, count = 0;
   int vnum = 0;
   int searchnum = 0;

   buf2[0] = '\0';
   if (isdigit(*name))
   {
      vnum = atoi(name);
      searchnum = real_object(vnum);
   }

   ch->sendln("-#-- Short Description ------- Room Number\n");

   for (k = DC::getInstance()->object_list; k; k = k->next)
   {

      // allow search by vnum
      if (vnum)
      {
         if (k->item_number != searchnum)
            continue;
      }
      else if (!(isexact(name, k->name)))
         continue;

      if (!CAN_SEE_OBJ(ch, k))
         continue;

      buf[0] = '\0';

      if (k->in_obj)
      {
         if (k->in_obj->in_room > DC::NOWHERE)
            in_room = DC::getInstance()->world[k->in_obj->in_room].number;
         else if (k->in_obj->carried_by)
         {
            if (!CAN_SEE(ch, k->in_obj->carried_by))
               continue;
            in_room = DC::getInstance()->world[k->in_obj->carried_by->in_room].number;
         }
         else if (k->in_obj->equipped_by)
         {
            if (!CAN_SEE(ch, k->in_obj->equipped_by))
               continue;
            in_room = DC::getInstance()->world[k->in_obj->equipped_by->in_room].number;
         }
      }
      else if (k->carried_by)
      {
         if (!CAN_SEE(ch, k->carried_by))
            continue;
         in_room = DC::getInstance()->world[k->carried_by->in_room].number;
      }
      else if (k->equipped_by)
      {
         if (!CAN_SEE(ch, k->equipped_by))
            continue;
         in_room = DC::getInstance()->world[k->equipped_by->in_room].number;
      }
      else if (k->in_room > 0)
         in_room = DC::getInstance()->world[k->in_room].number;
      else
         in_room = 0;

      count++;

      if (in_room != DC::NOWHERE)
         sprintf(buf, "[%2d] %-26s %d", count, k->short_description, in_room);
      else
         sprintf(buf, "[%2d] %-26s %s", count, k->short_description,
                 "(Item at DC::NOWHERE.)");

      if (k->in_obj)
      {
         strcat(buf, " in ");
         strcat(buf, k->in_obj->short_description);
         if (k->in_obj->carried_by)
         {
            strcat(buf, " carried by ");
            strcat(buf, GET_NAME(k->in_obj->carried_by));
         }
         else if (k->in_obj->equipped_by)
         {
            strcat(buf, " equipped by ");
            strcat(buf, GET_NAME(k->in_obj->equipped_by));
         }
      }
      if (k->carried_by)
      {
         strcat(buf, " carried by ");
         strcat(buf, GET_NAME(k->carried_by));
      }
      else if (k->equipped_by)
      {
         strcat(buf, " equipped by ");
         strcat(buf, GET_NAME(k->equipped_by));
      }
      if (strlen(buf2) + strlen(buf) + 3 >= MAX_STRING_LENGTH)
      {
         ch->sendln("LIST TRUNCATED...TOO LONG");
         break;
      }
      strcat(buf2, buf);
      strcat(buf2, "\n\r");
   }

   if (!*buf2)
      ch->sendln("Couldn't find any such OBJECT.");
   else
      page_string(ch->desc, buf2, 1);
   return eSUCCESS;
}
/*********--------- end of locate objects -----------------************/

/* -----------------   MOB LOCATE FUNCTION ---------------------------- */
// locates ONLY mobiles.  If cmd == 18, it locates pc's AND mobiles
int do_mlocate(Character *ch, char *name, cmd_t cmd)
{
   char buf[300], buf2[MAX_STRING_LENGTH];
   int count = 0;
   int vnum = 0;
   int searchnum = 0;

   if (isdigit(*name))
   {
      vnum = atoi(name);
      searchnum = real_mobile(vnum);
   }

   *buf2 = '\0';
   ch->sendln(" #   Short description          Room Number\n");

   const auto &character_list = DC::getInstance()->character_list;
   for (const auto &i : character_list)
   {

      if ((IS_PC(i) &&
           (cmd != cmd_t::MLOCATE_CHARACTER || !CAN_SEE(ch, i))))
         continue;

      // allow find by vnum
      if (vnum)
      {
         if (searchnum != i->mobdata->nr)
            continue;
      }
      else if (!(isexact(name, i->getName())))
         continue;

      if (i->in_room == DC::NOWHERE)
      {
         continue;
      }

      count++;
      *buf = '\0';
      sprintf(buf, "[%2d] %-26s %d\n\r",
              count,
              i->short_desc,
              DC::getInstance()->world[i->in_room].number);
      if (strlen(buf) + strlen(buf2) + 3 >= MAX_STRING_LENGTH)
      {
         ch->sendln("LIST TRUNCATED...TOO LONG");
         break;
      }
      strcat(buf2, buf);
   }

   if (!*buf2)
      ch->sendln("Couldn't find any MOBS by that NAME.");
   else
      page_string(ch->desc, buf2, 1);
   return eSUCCESS;
}
/* --------------------- End of Mob locate function -------------------- */

int do_consider(Character *ch, char *argument, cmd_t cmd)
{
   Character *victim;
   char name[256];
   int mod = 0;
   int percent, x, y;
   int Learned;

   char *level_messages[] = {
       "You can kill %s naked and weaponless.\r\n",
       "%s is no match for you.\r\n",
       "%s looks like an easy kill.\r\n",
       "%s wouldn't be all that hard.\r\n",
       "%s is perfect for you!\n\r",
       "You would need some luck and good equipment to kill %s.\r\n",
       "%s says 'Do you feel lucky, punk?'.\r\n",
       "%s laughs at you mercilessly.\r\n",
       "%s will tear your head off and piss on your dead skull.\r\n"};

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
       "is pretty much naked."};

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
       "can handle just as much damage as you"};

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
       "can handle just as much damage as you"};

   char *dam_messages[] = {
       "hits like my grandmother",
       "will probably graze you pretty good",
       "can hit pretty hard",
       "can pack a pretty damn good punch",
       "can massacre on a good day",
       "can massacre even on a bad day",
       "could make Darth Vader cry like a baby",
       "can eat the Terminator for lunch and not have to burp",
       "will beat the living shit out of you",
       "will pound the fuck out of you",
       "could make Sylvester Stallone cry for his mommy",
       "is a *very* tough mobile.  Be careful"};

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
       "You can't miss."};

   one_argument(argument, name);

   if (!(victim = ch->get_char_room_vis(name)))
   {
      ch->sendln("Who was that you're scoping out?");
      return eFAILURE;
   }

   if (victim == ch)
   {
      ch->sendln("Looks like a WIMP! (Used to be \"Looks like a PUSSY!\" but we got complaints.)");
      return eFAILURE;
   }

   if (!ch->decrementMove(5, "You are too tired to consider much of anything at the moment."))
   {
      return eFAILURE;
   }

   if (!skill_success(ch, nullptr, SKILL_CONSIDER))
   {
      ch->sendln("You try really hard, but you really have no idea about their capabilties!");
      return eFAILURE;
   }

   Learned = ch->has_skill(SKILL_CONSIDER);

   if (Learned > 20)
   {
      /* ARMOR CLASS */
      x = GET_ARMOR(victim) / 20 + 5;
      if (x > 10)
         x = 10;
      if (x < 0)
         x = 0;

      percent = number(1, 101);
      if (percent > Learned)
      {
         if (number(0, 1) == 0)
         {
            x -= number(1, 3);
            if (x < 0)
               x = 0;
         }
         else
         {
            x += number(1, 3);
            if (x > 10)
               x = 10;
         }
      }

      csendf(ch, "As far as armor goes, %s %s\n\r",
             GET_SHORT(victim), ac_messages[x]);
   }

   /* HIT POINTS */

   if (Learned > 40)
   {

      if (IS_PC(victim) && victim->getLevel() > IMMORTAL)
      {
         csendf(ch, "Compared to your hps, %s can definitely take anything you can dish out.\r\n",
                GET_SHORT(victim));
      }
      else
      {

         if (ch->getHP() >= victim->getHP() || ch->getLevel() > MORTAL)
         {
            x = victim->getHP() / ch->getHP() * 100;
            x /= 10;
            if (x < 0)
               x = 0;
            if (x > 10)
               x = 10;
            percent = number(1, 101);
            if (percent > Learned)
            {
               if (number(0, 1) == 0)
               {
                  x -= number(1, 3);
                  if (x < 0)
                     x = 0;
               }
               else
               {
                  x += number(1, 3);
                  if (x > 10)
                     x = 10;
               }
            }

            csendf(ch, "Compared to your hps, %s %s.\r\n", GET_SHORT(victim), hplow_messages[x]);
         }
         else
         {
            x = ch->getHP() / victim->getHP() * 100;
            x /= 10;
            if (x < 0)
               x = 0;
            if (x > 10)
               x = 10;
            percent = number(1, 101);
            if (percent > Learned)
            {
               if (number(0, 1) == 0)
               {
                  x -= number(1, 3);
                  if (x < 0)
                     x = 0;
               }
               else
               {
                  x += number(1, 3);
                  if (x > 10)
                     x = 10;
               }
            }

            csendf(ch, "Compared to your hps, %s %s.\r\n", GET_SHORT(victim), hphigh_messages[x]);
         }
      }

      if (Learned > 60)
      {

         /* Average Damage */

         if (victim->equipment[WIELD])
         {
            x = victim->equipment[WIELD]->obj_flags.value[1];
            y = victim->equipment[WIELD]->obj_flags.value[2];
            x = (((x * y - x) / 2) + x);
         }
         else
         {
            if (IS_NPC(victim))
            {
               x = victim->mobdata->damnodice;
               y = victim->mobdata->damsizedice;
               x = (((x * y - x) / 2) + x);
            }
            else
               x = number(0, 2);
         }
         x += GET_DAMROLL(victim);

         if (x <= 5)
            x = 0;
         else if (x <= 10)
            x = 1;
         else if (x <= 15)
            x = 2;
         else if (x <= 23)
            x = 3;
         else if (x <= 30)
            x = 4;
         else if (x <= 40)
            x = 5;
         else if (x <= 50)
            x = 6;
         else if (x <= 75)
            x = 7;
         else if (x <= 100)
            x = 8;
         else if (x <= 125)
            x = 9;
         else if (x <= 150)
            x = 10;
         else
            x = 11;
         percent = number(1, 101);
         if (percent > Learned)
         {
            if (number(0, 1) == 0)
            {
               x -= number(1, 4);
               if (x < 0)
                  x = 0;
            }
            else
            {
               x += number(1, 4);
               if (x > 11)
                  x = 11;
            }
         }

         csendf(ch, "Average damage: %s %s.\r\n", GET_SHORT(victim),
                dam_messages[x]);
      }
   }

   if (Learned > 80)
   {
      /* CHANCES TO STEAL */
      if ((GET_CLASS(ch) == CLASS_THIEF) || (ch->getLevel() > IMMORTAL))
      {

         percent = Learned;

         mod += AWAKE(victim) ? 10 : -50;
         level_diff_t level_difference = victim->getLevel() - ch->getLevel();

         mod += level_difference / 2;
         mod += 5; /* average item is 5 lbs, steal takes ths into acct */
         if (GET_DEX(ch) < 10)
            mod += ((10 - GET_DEX(ch)) * 5);
         else if (GET_DEX(ch) > 15)
            mod -= ((GET_DEX(ch) - 10) * 2);

         percent -= mod;

         if (GET_POS(victim) <= position_t::SLEEPING)
            percent = 100;
         if (victim->getLevel() > IMMORTAL)
            percent = 0;
         if (percent < 0)
            percent = 0;
         else if (percent > 100)
            percent = 100;
         percent /= 10;
         x = percent;

         percent = number(1, 101);
         if (percent > Learned)
         {
            if (number(0, 1) == 0)
            {
               x -= number(1, 3);
               if (x < 0)
                  x = 0;
            }
            else
            {
               x += number(1, 3);
               if (x > 10)
                  x = 10;
            }
         }

         csendf(ch, "Chances of stealing: %s\n\r", thief_messages[x]);
      }
   }
   /* Level Comparison */
   level_diff_t level_difference = victim->getLevel() - ch->getLevel();
   if (level_difference <= -15)
      y = 0;
   else if (level_difference <= -10)
      y = 1;
   else if (level_difference <= -5)
      y = 2;
   else if (level_difference <= -2)
      y = 3;
   else if (level_difference <= 1)
      y = 4;
   else if (level_difference <= 2)
      y = 5;
   else if (level_difference <= 4)
      y = 6;
   else if (level_difference <= 9)
      y = 7;
   else
      y = 8;

   ch->send("Level comparison: ");
   csendf(ch, level_messages[y], GET_SHORT(victim));

   if (Learned > 89)
   {
      ch->send("Training: ");

      if (GET_CLASS(victim) == CLASS_WARRIOR ||
          GET_CLASS(victim) == CLASS_THIEF ||
          GET_CLASS(victim) == CLASS_BARBARIAN ||
          GET_CLASS(victim) == CLASS_MONK ||
          GET_CLASS(victim) == CLASS_BARD)
         csendf(ch, "%s appears to be a trained fighter.\r\n", GET_SHORT(victim));
      else if (GET_CLASS(victim) == CLASS_MAGIC_USER ||
               GET_CLASS(victim) == CLASS_CLERIC ||
               GET_CLASS(victim) == CLASS_DRUID ||
               GET_CLASS(victim) == CLASS_PSIONIC ||
               GET_CLASS(victim) == CLASS_NECROMANCER)
         csendf(ch, "%s appears to be trained in mystical arts.\r\n", GET_SHORT(victim));
      else if (GET_CLASS(victim) == CLASS_ANTI_PAL ||
               GET_CLASS(victim) == CLASS_PALADIN ||
               GET_CLASS(victim) == CLASS_RANGER)
         csendf(ch, "%s appears to have training in both combat and magic.\r\n", GET_SHORT(victim));
      else if (GET_CLASS(victim))
         csendf(ch, "%s appears to have training, but you are unfamiliar with what.\r\n", GET_SHORT(victim));
      else
         ch->sendln("You've seen stray dogs that were better trained.");
   }

   return eSUCCESS;
}

/* Shows characters in adjacent rooms -- Sadus */
int do_scan(Character *ch, char *argument, cmd_t cmd)
{
   int i;
   Character *vict;
   Room *room;
   int32_t was_in;

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

   if (!ch->decrementMove(2, "You are to tired to scan right now."))
   {
      return eFAILURE;
   }

   act("$n carefully searches the surroundings...", ch, 0, 0, TO_ROOM,
       INVIS_NULL | STAYHIDE);
   ch->sendln("You carefully search the surroundings...\n\r");

   for (vict = DC::getInstance()->world[ch->in_room].people; vict; vict = vict->next_in_room)
   {
      if (CAN_SEE(ch, vict) && ch != vict)
      {
         csendf(ch, "%35s -- %s\n\r", GET_SHORT(vict), "Right Here");
      }
   }

   for (i = 0; i < 6; i++)
   {
      if (CAN_GO(ch, i))
      {
         room = &DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[i]->to_room];
         if (room == &DC::getInstance()->world[ch->in_room])
            continue;
         if (isSet(room->room_flags, NO_SCAN))
         {
            csendf(ch, "%35s -- a little bit %s\n\r", "It's too hard to see!",
                   possibilities[i]);
         }
         else
            for (vict = room->people; vict; vict = vict->next_in_room)
            {
               if (CAN_SEE(ch, vict))
               {
                  if (IS_AFFECTED(vict, AFF_CAMOUFLAGUE) &&
                      DC::getInstance()->world[vict->in_room].sector_type != SECT_INSIDE &&
                      DC::getInstance()->world[vict->in_room].sector_type != SECT_CITY &&
                      DC::getInstance()->world[vict->in_room].sector_type != SECT_AIR)
                     continue;

                  if (skill_success(ch, nullptr, SKILL_SCAN))
                  {
                     csendf(ch, "%35s -- a little bit %s\n\r", GET_SHORT(vict),
                            possibilities[i]);
                  }
               }
            }

         // Now we go one room further (reach out and touch someone)

         was_in = ch->in_room;
         ch->in_room = DC::getInstance()->world[ch->in_room].dir_option[i]->to_room;

         if (CAN_GO(ch, i))
         {
            room = &DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[i]->to_room];
            if (isSet(room->room_flags, NO_SCAN))
            {
               csendf(ch, "%35s -- a ways off %s\n\r", "It's too hard to see!",
                      possibilities[i]);
            }
            else
               for (vict = room->people; vict; vict = vict->next_in_room)
               {
                  if (CAN_SEE(ch, vict))
                  {
                     if (IS_AFFECTED(vict, AFF_CAMOUFLAGUE) &&
                         DC::getInstance()->world[vict->in_room].sector_type != SECT_INSIDE &&
                         DC::getInstance()->world[vict->in_room].sector_type != SECT_CITY &&
                         DC::getInstance()->world[vict->in_room].sector_type != SECT_AIR)
                        continue;

                     if (skill_success(ch, nullptr, SKILL_SCAN, -10))
                     {
                        csendf(ch, "%35s -- a ways off %s\n\r",
                               GET_SHORT(vict),
                               possibilities[i]);
                     }
                  }
               }
            // Now if we have the farsight spell we go another room out
            if (IS_AFFECTED(ch, AFF_FARSIGHT))
            {
               ch->in_room = DC::getInstance()->world[ch->in_room].dir_option[i]->to_room;
               if (CAN_GO(ch, i))
               {
                  room = &DC::getInstance()->world[DC::getInstance()->world[ch->in_room].dir_option[i]->to_room];
                  if (isSet(room->room_flags, NO_SCAN))
                  {
                     csendf(ch, "%35s -- extremely far off %s\n\r", "It's too hard to see!",
                            possibilities[i]);
                  }
                  else
                     for (vict = room->people; vict; vict = vict->next_in_room)
                     {
                        if (CAN_SEE(ch, vict))
                        {
                           if (IS_AFFECTED(vict, AFF_CAMOUFLAGUE) &&
                               DC::getInstance()->world[vict->in_room].sector_type != SECT_INSIDE &&
                               DC::getInstance()->world[vict->in_room].sector_type != SECT_CITY &&
                               DC::getInstance()->world[vict->in_room].sector_type != SECT_AIR)
                              continue;

                           if (skill_success(ch, nullptr, SKILL_SCAN, -20))
                           {
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

int do_tick(Character *ch, char *argument, cmd_t cmd)
{
   int ntick;
   char buf[256];

   if (isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET))
   {
      ch->sendln("SHHHHHH!! Can't you see people are trying to read?");
      return 1;
   }

   if (IS_NPC(ch))
   {
      ch->sendln("Monsters don't wait for anything.");
      return eFAILURE;
   }

   if (ch->desc == nullptr)
      return eFAILURE;

   while (*argument == ' ')
      argument++;

   if (*argument == '\0')
      ntick = 1;
   else
      ntick = atoi(argument);

   if (ntick == 1)
      sprintf(buf, "$n is waiting for one tick.");
   else
      sprintf(buf, "$n is waiting for %d ticks.", ntick);

   act(buf, ch, 0, 0, TO_CHAR, INVIS_NULL);
   act(buf, ch, 0, 0, TO_ROOM, INVIS_NULL);

   // TODO - figure out if this ever had any purpose.  It's still fun though:)
   ch->desc->tick_wait = ntick;
   return eSUCCESS;
}

command_return_t Character::do_experience(QStringList arguments, cmd_t cmd)
{
   if (level_ >= IMMORTAL)
   {
      send("Immortals cannot gain levels by gaining experience.\r\n");
      return eSUCCESS;
   }

   level_t next_level = level_;
   qint64 experience_remaining = 0;

   do
   {
      next_level += 1;
      quint64 experience_next_level = exp_table[next_level];
      quint64 current_experience = exp;
      experience_remaining = experience_next_level - current_experience;

      if (experience_remaining < 0)
      {
         send(QStringLiteral("You have enough experience to advance to level %L1.\r\n").arg(next_level));
      }
      else
      {
         send(QStringLiteral("You require %L1 experience to advance to level %L2.\r\n").arg(experience_remaining).arg(next_level));
      }
   } while (experience_remaining < 0);

   return eSUCCESS;
}

void check_champion_and_website_who_list()
{
   Object *obj;
   std::stringstream buf, buf2;
   int addminute = 0;
   std::string name;

   const auto &character_list = DC::getInstance()->character_list;
   for (const auto &ch : character_list)
   {

      if (IS_PC(ch) && ch->desc && ch->player && ch->player->wizinvis <= 0)
      {
         buf << GET_SHORT(ch) << std::endl;
      }

      if ((IS_NPC(ch) || !ch->desc) && (obj = get_obj_in_list_num(real_object(CHAMPION_ITEM), ch->carrying)))
      {
         obj_from_char(obj);
         obj_to_room(obj, CFLAG_HOME);
      }

      if (IS_AFFECTED(ch, AFF_CHAMPION) && !(obj = get_obj_in_list_num(real_object(CHAMPION_ITEM), ch->carrying)))
      {
         REMBIT(ch->affected_by, AFF_CHAMPION);
      }
   }

   buf << "endminutenobodywillhavethisnameever" << std::endl;
   addminute++;

   if (!(obj = get_obj_num(real_object(CHAMPION_ITEM))))
   {
      if ((obj = clone_object(real_object(CHAMPION_ITEM))))
      {
         obj_to_room(obj, CFLAG_HOME);
      }
      else
      {
         logentry(QStringLiteral("CHAMPION_ITEM obj not found. Please create one."), 0, DC::LogChannel::LOG_MISC);
      }
   }

   std::ifstream fl(LOCAL_WHO_FILE);

   while (getline(fl, name))
   {
      if (addminute <= 9)
         buf << name << std::endl;
      else
         buf2 << name << std::endl;
      if (name == "endminutenobodywillhavethisnameever")
         addminute++;
   }

   fl.close();

   std::ofstream flo(LOCAL_WHO_FILE);
   flo << buf.str();
   flo.close();

   std::ofstream flwo(WEB_WHO_FILE);
   flwo << buf2.str();
   flwo.close();
}

int do_sector(Character *ch, char *arg, cmd_t cmd)
{
   std::string art = "a";

   if (ch->desc && ch->in_room)
   {
      int sector = DC::getInstance()->world[ch->in_room].sector_type;
      switch (sector)
      {
      case SECT_INSIDE:
      case SECT_UNDERWATER:
      case SECT_AIR:
      case SECT_ARCTIC:
         art = "an";
         break;
      }

      csendf(ch, "You are currently in %s %s area.\r\n", art.c_str(), sector_types[sector]);
   }

   return eSUCCESS;
}

int do_version(Character *ch, char *arg, cmd_t cmd)
{
   if (ch)
   {
      ch->sendln(QStringLiteral("Version: %1 Build time: %2").arg(DC::getBuildVersion()).arg(DC::getBuildTime()));
   }
   return eSUCCESS;
}

class Search
{
public:
   enum types
   {
      O_NAME, // name=
      O_DESCRIPTION,
      O_SHORT_DESCRIPTION,
      O_ACTION_DESCRIPTION,
      O_TYPE_FLAG,   // type=
      O_WEAR_FLAGS,  // wear=
      O_SIZE,        // size=
      O_EXTRA_FLAGS, // extra=
      O_WEIGHT,
      O_COST,
      O_MORE_FLAGS, // more=
      O_EQ_LEVEL,   // level
      O_V1,         // v1
      O_V2,         // v2
      O_V3,         // v3
      O_V4,         // v4
      O_AFFECTED,
      O_EDD_KEYWORD,
      O_EDD_DESCRIPTION,
      O_CARRIED_BY,
      O_EQUIPPED_BY,
      LIMIT
   };
   enum locations
   {
      in_inventory,
      in_equipment,
      in_room,
      in_vault,
      in_clan_vault,
      in_object_database
   };
   bool operator==(const Object *obj);
   void setType(types type) { type_ = type; }
   types getType(void) const { return type_; }

   // name=
   void setObjectName(QString name) { o_name_ = name; }

   // description
   // short description
   // action description

   // type=
   void setObjectType(object_type_t object_type) { obj_flags_.type_flag = object_type; }

   // wear=
   void setObjectWearLocation(uint32_t location) { obj_flags_.wear_flags = location; }

   // size=
   void setObjectSize(uint16_t size) { obj_flags_.size = size; }

   // extra=
   void setObjectExtra(uint32_t flags) { obj_flags_.extra_flags = flags; }

   // weight
   void setObjectMinimumWeight(uint64_t weight) { o_min_weight_ = weight; }
   void setObjectMaximumWeight(uint64_t weight) { o_max_weight_ = weight; }

   // cost
   void setObjectMinimumCost(uint64_t cost) { o_min_cost_ = cost; }
   void setObjectMaximumCost(uint64_t cost) { o_max_cost_ = cost; }

   // more flags
   void setObjectMore(uint32_t flags) { obj_flags_.more_flags = flags; }

   // level=
   void setObjectMinimumLevel(uint64_t level) { o_min_level_ = level; }
   void setObjectMaximumLevel(uint64_t level) { o_max_level_ = level; }

   // v1
   // v2
   // v3
   // v4
   // affected

   // edd keyword
   void setObjectEDDKeyword(QString keyword) { o_edd_keyword_ = keyword; }

   // edd description

   // carried by
   void setObjectCarriedBy(QString name) { o_carried_by_ = name; }

   // equired by
   void setObjectEquippedBy(QString name) { o_equipped_by_ = name; }

   // limit=
   void setLimitOutput(uint64_t limit_output) { limit_output_ = limit_output; }
   uint64_t getLimitOutput(void) const { return limit_output_; }

   void enableShowRange(void) { show_range_ = true; }
   bool isShowRange(void) const { return show_range_; }

private:
   types type_ = {};

   uint64_t o_min_level_ = {};
   uint64_t o_max_level_ = {};
   uint64_t o_min_cost_ = {};
   uint64_t o_max_cost_ = {};
   uint64_t o_min_weight_ = {};
   uint64_t o_max_weight_ = {};

   uint64_t o_value[4] = {};

   uint64_t o_item_number_ = {};         /* Where in data-base               */
   uint64_t o_in_room_ = {};             /* In what room -1 when conta/carr  */
   uint64_t o_vroo_ = {};                /* for corpse saving */
   struct obj_flag_data obj_flags_ = {}; /* Object information               */
   int16_t o_num_affects_ = {};
   obj_affected_type o_affected_ = {}; /* Which abilities in PC to change  */

   QString o_name_ = {};               /* Title of object :get etc.        */
   QString o_description_ = {};        /* When in room                     */
   QString o_short_description_ = {};  /* when worn/carry/in cont.         */
   QString o_action_description_ = {}; /* What to write when used          */

   QString o_edd_keyword_ = {};     /* Keyword in look/examine          */
   QString o_edd_description_ = {}; /* What to see                      */
   QString o_carried_by_ = {};
   QString o_equipped_by_ = {};
   uint64_t limit_output_ = {};
   bool show_range_ = false;
};

class Result
{
   Search::locations location_ = {};
   Object *object_ = {};
   QString name_;

public:
   Result(Search::locations location, Object *object)
       : location_(location), object_(object)
   {
   }

   Result(Search::locations location, Object *object, QString name)
       : location_(location), object_(object), name_(name)
   {
   }

   Search::locations getLocation() const { return location_; }
   Object *getObject() const { return object_; }
   QString getName() const { return name_; }
};

bool Search::operator==(const Object *obj)
{
   if (obj == nullptr)
   {
      return false;
   }

   if (show_range_)
   {
      return true;
   }

   switch (type_)
   {
   case O_NAME:
      if (o_name_ == obj->name || isexact(o_name_, obj->name))
      {
         return true;
      }
      break;

   case O_DESCRIPTION:
      break;

   case O_SHORT_DESCRIPTION:
      break;

   case O_ACTION_DESCRIPTION:
      break;

   case O_TYPE_FLAG:
      if (obj->obj_flags.type_flag == obj_flags_.type_flag)
      {
         return true;
      }
      else
      {
         return false;
      }
      break;

   case O_WEAR_FLAGS:
      if (obj->obj_flags.wear_flags == obj_flags_.wear_flags || isSet(obj->obj_flags.wear_flags, obj_flags_.wear_flags))
      {
         return true;
      }
      else
      {
         return false;
      }
      break;
   case O_SIZE:
      if (obj->obj_flags.size == obj_flags_.size || isSet(obj->obj_flags.size, obj_flags_.size))
      {
         return true;
      }
      else
      {
         return false;
      }
      break;

   case O_EDD_KEYWORD:
      break;

   case O_EDD_DESCRIPTION:
      break;

   case O_WEIGHT:
      if (o_max_weight_ == -1 && obj->obj_flags.weight >= o_min_weight_)
      {
         return true;
      }
      else if (obj->obj_flags.weight >= o_min_weight_ && obj->obj_flags.weight <= o_max_weight_)
      {
         return true;
      }
      break;

   case O_COST:
      if (o_max_cost_ == -1 && obj->obj_flags.cost >= o_min_cost_)
      {
         return true;
      }
      else if (obj->obj_flags.cost >= o_min_cost_ && obj->obj_flags.cost <= o_max_cost_)
      {
         return true;
      }
      break;

   case O_MORE_FLAGS:
      if (obj_flags_.more_flags == obj->obj_flags.more_flags || isSet(obj->obj_flags.more_flags, obj_flags_.more_flags))
      {
         return true;
      }
      else
      {
         return false;
      }
      break;
   case O_EXTRA_FLAGS:
      if (obj->obj_flags.extra_flags == obj_flags_.extra_flags || isSet(obj->obj_flags.extra_flags, obj_flags_.extra_flags))
      {
         return true;
      }
      else
      {
         return false;
      }
      break;
   case O_EQ_LEVEL:
      if (o_max_level_ == -1 && obj->obj_flags.eq_level >= o_min_level_)
      {
         return true;
      }
      else if (obj->obj_flags.eq_level >= o_min_level_ && obj->obj_flags.eq_level <= o_max_level_)
      {
         return true;
      }
      break;

   case O_V1:
      if (o_value[0] == obj->obj_flags.value[0])
      {
         return true;
      }
      break;

   case O_V2:
      if (o_value[1] == obj->obj_flags.value[1])
      {
         return true;
      }
      break;

   case O_V3:
      if (o_value[2] == obj->obj_flags.value[2])
      {
         return true;
      }
      break;

   case O_V4:
      if (o_value[3] == obj->obj_flags.value[3])
      {
         return true;
      }
      break;

   case O_AFFECTED:
      break;
   }
   return false;
}

bool search_object(Object *obj, QList<Search> sl)
{
   bool matches = false;
   for (auto i = 0; i < sl.size(); ++i)
   {
      if (sl[i].getType() == Search::types::LIMIT)
      {
         continue;
      }

      if (sl[i] == obj)
      {
         matches = true;
      }
      else
      {
         matches = false;
         break;
      }
   }

   return matches;
}

command_return_t Character::do_search(QStringList arguments, cmd_t cmd)
{
   if (arguments.empty())
   {
      arguments.append("help");
   }

   // Create search object based on parameters

   QList<Search> sl;
   QString arg1, arg2;
   bool equals = false, greater = false, greater_equals = false, lesser = false, lesser_equals = false, search_world = false, show_affects = false, show_details = false;
   for (auto i = 0; i < arguments.size(); ++i)
   {
      Search so = {};
      // ch->send(fmt::format("{} [{}]\r\n", i, parsables[i]));
      if (arguments[i].contains('=') && !arguments[i].contains(">=") && !arguments[i].contains("<="))
      {
         QStringList equal_buffer = arguments[i].split('=');
         arg1 = equal_buffer.at(0);
         arg2 = equal_buffer.at(1);
         equals = true;
      }
      else if (arguments[i].contains('>') && !arguments[i].contains(">="))
      {
         QStringList equal_buffer = arguments[i].split('>');
         arg1 = equal_buffer.at(0);
         arg2 = equal_buffer.at(1);
         greater = true;
      }
      else if (arguments[i].contains(">="))
      {
         QStringList equal_buffer = arguments[i].split(">=");
         arg1 = equal_buffer.at(0);
         arg2 = equal_buffer.at(1);
         greater_equals = true;
      }
      else if (arguments[i].contains('<') && !arguments[i].contains("<="))
      {
         QStringList equal_buffer = arguments[i].split('<');
         arg1 = equal_buffer.at(0);
         arg2 = equal_buffer.at(1);
         lesser = true;
      }
      else if (arguments[i].contains("<="))
      {
         QStringList equal_buffer = arguments[i].split("<=");
         arg1 = equal_buffer.at(0);
         arg2 = equal_buffer.at(1);
         lesser_equals = true;
      }
      else
      {
         arg1 = arguments[i];
      }

      if (arg1 == "help" || arg1 == "?")
      {
         send("Usage: search <search terms> <other options>\r\n");
         send("       Searches object database.\r\n\r\n");
         send("Usage: search world <search terms> <other options>\r\n");
         send("       Searches your reachable world.\r\n\r\n");
         send("Search terms:\r\n");
         send("level=50      show objects that are level 50.\r\n");
         send("level=50-60   show objects with level including and between 50 to 60.\r\n");
         send("level=?       show objects with level including and between 50 to 60.\r\n");
         send("type=weapon   show objects of type weapon.\r\n");
         send("type=?        show available object types.\r\n");
         send("wear=neck     show objects that can be worn on the neck.\r\n");
         send("wear=?        show available wear locations.\r\n");
         send("size=small    show objects that can be worn on the neck.\r\n");
         send("size=?        show available sizes.\r\n");
         send("extra=mage    show objects that have the extra flag for mage set.\r\n");
         send("extra=?       show available extra flags.\r\n");
         send("more=unique   show objects that have the extra flag for mage set.\r\n");
         send("more=?        show available extra flags.\r\n");
         send("name=moss     show objects matching keyword moss.\r\n");
         send("xyz           show objects matching keyword xyz.\r\n");
         send("Search terms can be combined.\r\n");
         send("Example: search level=1-10 type=ARMOR golden\r\n");
         send("\r\n");
         send("Other options:\r\n");
         send("limit=10       limit output to only 10 results.\r\n");
         send("affects        shows affects.\r\n");
         send("details        shows certain details depending on object type.\r\n");
         return eSUCCESS;
      }
      else if (arg1 == "world")
      {
         search_world = true;
      }
      else if (arg1 == "affects")
      {
         show_affects = true;
      }
      else if (arg1 == "details")
      {
         show_details = true;
      }
      else if (arg1 == "level")
      {
         so.setType(Search::types::O_EQ_LEVEL);

         if (arg2.isEmpty() || arg2 == "?")
         {
            so.enableShowRange();
            sl.push_back(so);
         }
         else
         {

            // #-#
            // #-
            if (arg2.contains('-'))
            {
               QStringList equal_buffer = arg2.split('-');
               arg1 = equal_buffer.at(0);
               arg2 = equal_buffer.at(1);
               if (!arg1.isEmpty())
               {
                  so.setObjectMinimumLevel(arg1.toULongLong());
                  if (!arg2.isEmpty())
                  {
                     so.setObjectMaximumLevel(arg2.toULongLong());
                  }
                  else
                  {
                     so.setObjectMaximumLevel(-1);
                  }

                  sl.push_back(so);
               }
            }
            else // #
            {
               if (!arg2.isEmpty())
               {
                  if (greater)
                  {
                     so.setObjectMinimumLevel(arg2.toULongLong() + 1);
                     so.setObjectMaximumLevel(-1);
                  }
                  else if (greater_equals)
                  {
                     so.setObjectMinimumLevel(arg2.toULongLong());
                     so.setObjectMaximumLevel(-1);
                  }
                  else if (lesser)
                  {
                     so.setObjectMinimumLevel(0);
                     so.setObjectMaximumLevel(arg2.toULongLong() - 1);
                  }
                  else if (lesser_equals)
                  {
                     so.setObjectMinimumLevel(0);
                     so.setObjectMaximumLevel(arg2.toULongLong());
                  }
                  else
                  {
                     so.setObjectMinimumLevel(arg2.toULongLong());
                     so.setObjectMaximumLevel(arg2.toULongLong());
                  }

                  sl.push_back(so);
               }
            }
         }
      }
      else if (arg1 == "name")
      {
         if (arg2.isEmpty())
         {
            send("What name are you searching for? You can use name=value multiple times.\r\n");
            return eFAILURE;
         }
         else
         {
            qsizetype quote;
            while ((quote = arg2.indexOf('"')) != -1)
            {
               arg2 = arg2.remove(quote, 1);
            }

            // Ex. name=woodbey
            so.setObjectName(arg2);
            so.setType(Search::types::O_NAME);
            sl.push_back(so);
         }
      }
      else if (arg1 == "type")
      {
         bool found = false;
         if (!arg2.isEmpty())
         {
            arg2 = arg2.toUpper();

            for (auto i = 0; i < item_types.size(); ++i)
            {
               if (item_types[i].indexOf(arg2) == 0)
               {
                  found = true;
                  so.setObjectType(i);
                  so.setType(Search::types::O_TYPE_FLAG);
                  sl.push_back(so);
                  break;
               }
            }
         }

         if (!found)
         {
            send("What type are you searching for?\r\n");
            send("Here are some valid types:\r\n");
            for (const auto &i : item_types)
            {
               send(i + "\r\n");
            }
            return eFAILURE;
         }
      }
      else if (arg1 == "wear")
      {
         bool found = false;
         if (!arg2.isEmpty())
         {
            uint32_t location = 0;
            parse_bitstrings_into_int(Object::wear_bits, arg2, nullptr, location);
            if (location)
            {
               found = true;
               so.setObjectWearLocation(location);
               so.setType(Search::types::O_WEAR_FLAGS);
               sl.push_back(so);
            }
         }
         if (!found)
         {
            send("What type are you searching for?\r\n");
            send("Here are some valid wear locations:\r\n");
            for (const auto &w : Object::wear_bits)
            {
               send(w + "\r\n");
            }
            return eFAILURE;
         }
      }
      else if (arg1 == "size")
      {
         bool found = false;
         if (!arg2.isEmpty())
         {
            uint32_t size = 0;
            parse_bitstrings_into_int(Object::size_bits, arg2, nullptr, size);
            if (size)
            {
               found = true;
               so.setObjectSize(size);
               so.setType(Search::types::O_SIZE);
               sl.push_back(so);
            }
         }
         if (!found)
         {
            send("What size are you searching for?\r\n");
            send("Here are some valid sizes:\r\n");
            for (const auto &s : Object::size_bits)
            {
               send(s + "\r\n");
            }
            return eFAILURE;
         }
      }
      else if (arg1 == "more")
      {
         bool found = false;
         if (!arg2.isEmpty())
         {
            uint32_t more_flag = 0;
            parse_bitstrings_into_int(Object::more_obj_bits, arg2, nullptr, more_flag);
            if (more_flag)
            {
               found = true;
               so.setObjectMore(more_flag);
               so.setType(Search::types::O_MORE_FLAGS);
               sl.push_back(so);
            }
         }
         if (!found)
         {
            send("What more flag are you searching for?\r\n");
            send("Here are some valid more flags:\r\n");
            for (const auto &s : Object::more_obj_bits)
            {
               send(s + "\r\n");
            }
            return eFAILURE;
         }
      }
      else if (arg1 == "extra")
      {
         bool found = false;
         if (!arg2.isEmpty())
         {
            uint32_t extra_flag = 0;
            parse_bitstrings_into_int(Object::extra_bits, arg2, nullptr, extra_flag);
            if (extra_flag)
            {
               found = true;
               so.setObjectExtra(extra_flag);
               so.setType(Search::types::O_EXTRA_FLAGS);
               sl.push_back(so);
            }
         }
         if (!found)
         {
            send("What extra flag are you searching for?\r\n");
            send("Here are some valid extra flags:\r\n");
            for (const auto &s : Object::extra_bits)
            {
               send(s + "\r\n");
            }
            return eFAILURE;
         }
      }
      else if (arg1 == "limit")
      {
         if (!arg2.isEmpty())
         {
            so.setType(Search::types::LIMIT);
            so.setLimitOutput(arg2.toULongLong());
            sl.push_back(so);
         }
      }
      else
      {
         so.setObjectName(arg1);
         so.setType(Search::types::O_NAME);
         sl.push_back(so);
      }
   }

   bool header_shown = false;
   size_t objects_found = 0;

   QList<Result> obj_results;
   uint64_t limit_output = 0;

   if (search_world)
   {
      uint64_t old_count = 0;

      // search inventory
      for (auto obj = carrying; obj; obj = obj->next_content)
      {
         if (CAN_SEE_OBJ(this, obj))
         {
            if (search_object(obj, sl))
            {
               obj_results.push_back({Search::locations::in_inventory, obj});
            }

            // containers must be open to search them
            if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !isSet(obj->obj_flags.value[1], CONT_CLOSED))
            {
               // search inventory containers
               for (auto obj_in_container = obj->contains; obj_in_container != nullptr; obj_in_container = obj_in_container->next_content)
               {
                  if (CAN_SEE_OBJ(this, obj_in_container))
                  {
                     if (search_object(obj_in_container, sl))
                     {
                        obj_results.push_back({Search::locations::in_inventory, obj_in_container});
                     }
                  }
               }
            }
         }
      }
      send(QStringLiteral("%1 matches found in inventory.\r\n").arg(obj_results.size() - old_count));
      old_count = obj_results.size();

      // Search equipment
      for (auto k = 0; k < MAX_WEAR; k++)
      {
         auto *obj = equipment[k];
         if (obj != nullptr)
         {
            if (CAN_SEE_OBJ(this, obj))
            {
               if (search_object(obj, sl))
               {
                  obj_results.push_back({Search::locations::in_equipment, obj});
               }

               if (GET_ITEM_TYPE(obj) == ITEM_CONTAINER && !isSet(obj->obj_flags.value[1], CONT_CLOSED))
               {
                  for (auto obj_in_container = obj->contains; obj_in_container != nullptr; obj_in_container = obj_in_container->next_content)
                  {
                     if (CAN_SEE_OBJ(this, obj_in_container))
                     {
                        if (search_object(obj_in_container, sl))
                        {
                           obj_results.push_back({Search::locations::in_equipment, obj_in_container});
                        }
                     }
                  }
               }
            }
         }
      }
      send(QStringLiteral("%1 matches found among worn equipment.\r\n").arg(obj_results.size() - old_count));
      old_count = obj_results.size();

      // search room
      for (auto obj = DC::getInstance()->world[in_room].contents; obj; obj = obj->next_content)
      {
         if (CAN_SEE_OBJ(this, obj))
         {
            if (search_object(obj, sl))
            {
               obj_results.push_back({Search::locations::in_room, obj});
            }

            if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && !isSet(obj->obj_flags.value[1], CONT_CLOSED))
            {
               // search inventory containers
               for (auto obj_in_container = obj->contains; obj_in_container != nullptr; obj_in_container = obj_in_container->next_content)
               {
                  if (CAN_SEE_OBJ(this, obj_in_container))
                  {
                     if (search_object(obj_in_container, sl))
                     {
                        obj_results.push_back({Search::locations::in_room, obj_in_container});
                     }
                  }
               }
            }
         }
      }
      send(QStringLiteral("%1 matches found in room.\r\n").arg(obj_results.size() - old_count));
      old_count = obj_results.size();

      // Search all vaults player has access to
      uint64_t vaults_searched = 0;
      for (auto vault = vault_table; vault; vault = vault->next)
      {
         if (vault && !vault->owner.isEmpty() && has_vault_access(GET_NAME(this), vault))
         {
            vaults_searched++;
            struct vault_items_data *items;
            struct sorted_vault sv;
            sort_vault(*vault, sv);
            if (!sv.vault_contents.empty())
            {
               for (const auto &o_short_description : sv.vault_contents)
               {
                  const auto &o = sv.vault_content_qty[o_short_description];
                  const auto &obj = o.first;
                  const auto &count = o.second;

                  bool matches = false;

                  for (auto i = 0; i < sl.size(); ++i)
                  {
                     if (sl[i].getType() == Search::types::LIMIT)
                     {
                        limit_output = sl[i].getLimitOutput();
                        continue;
                     }

                     if (sl[i] == obj)
                     {
                        matches = true;
                     }
                     else
                     {
                        matches = false;
                        break;
                     }
                  }

                  if (matches)
                  {
                     for (auto counter = 0; counter < count; counter++)
                     {
                        obj_results.push_back({Search::locations::in_vault, obj, vault->owner});
                     }
                  }
               }
            }
         }
      }

      if (vaults_searched == 1)
      {
         send(QStringLiteral("%1 matches found within 1 vault.\r\n").arg(obj_results.size() - old_count));
      }
      else
      {
         send(QStringLiteral("%1 matches found within %2 vaults.\r\n").arg(obj_results.size() - old_count).arg(vaults_searched));
      }
      old_count = obj_results.size();

      if (clan)
      {
         // search clan vault is able

         QString vault_name = QStringLiteral("clan%1").arg(clan);
         auto vault = has_vault(vault_name.toStdString().c_str());
         // search vault if able
         if (vault)
         {
            struct vault_items_data *items;
            struct sorted_vault sv;
            sort_vault(*vault, sv);
            if (!sv.vault_contents.empty())
            {
               for (const auto &o_short_description : sv.vault_contents)
               {
                  const auto &o = sv.vault_content_qty[o_short_description];
                  const auto &obj = o.first;
                  const auto &count = o.second;

                  bool matches = false;

                  for (auto i = 0; i < sl.size(); ++i)
                  {
                     if (sl[i].getType() == Search::types::LIMIT)
                     {
                        limit_output = sl[i].getLimitOutput();
                        continue;
                     }

                     if (sl[i] == obj)
                     {
                        matches = true;
                     }
                     else
                     {
                        matches = false;
                        break;
                     }
                  }

                  if (matches)
                  {
                     for (auto counter = 0; counter < count; counter++)
                     {
                        obj_results.push_back({Search::locations::in_clan_vault, obj, vault_name});
                     }
                  }
               }
            }
         }
      }

      bool showed_ranges = false;

      for (const auto &s : sl)
      {
         if (s.isShowRange())
         {
            if (s.getType() == Search::types::O_EQ_LEVEL)
            {
               uint64_t min_level = 100, max_level = 0;
               for (const auto &result : obj_results)
               {
                  auto obj = result.getObject();
                  if (obj->getLevel() > max_level)
                  {
                     max_level = obj->getLevel();
                  }
                  else if (obj->getLevel() < min_level)
                  {
                     min_level = obj->getLevel();
                  }
               }

               send(QStringLiteral("Within %1 results the levels were %2-%3\r\n").arg(obj_results.size()).arg(min_level).arg(max_level));

               showed_ranges = true;
            }
         }
      }

      if (showed_ranges)
      {
         return eSUCCESS;
      }
   }
   else
   {
      for (int vnum = 0; vnum < DC::getInstance()->obj_index[top_of_objt].virt; ++vnum)
      {
         int rnum = 0;
         // real_object returns -1 for missing VNUMs
         if ((rnum = real_object(vnum)) < 0)
         {
            continue;
         }
         Object *obj = static_cast<Object *>(DC::getInstance()->obj_index[rnum].item);
         if (obj == nullptr)
         {
            continue;
         }

         if (obj->isDark() && !isImmortalPlayer())
         {
            continue;
         }

         bool matches = false;

         for (auto i = 0; i < sl.size(); ++i)
         {
            if (sl[i].getType() == Search::types::LIMIT)
            {
               limit_output = sl[i].getLimitOutput();
               continue;
            }

            if (sl[i] == obj)
            {
               matches = true;
            }
            else
            {
               matches = false;
               break;
            }
         }

         if (matches)
         {
            obj_results.push_back({Search::locations::in_object_database, obj});
         }
      }
      if (obj_results.empty())
      {
         send(QStringLiteral("Searching $B%1$R objects...No results found.\r\n").arg(top_of_objt));
         return eFAILURE;
      }

      bool showed_ranges = false;

      for (const auto &s : sl)
      {
         if (s.isShowRange())
         {
            if (s.getType() == Search::types::O_EQ_LEVEL)
            {
               uint64_t min_level = 100, max_level = 0;
               for (const auto &result : obj_results)
               {
                  auto obj = result.getObject();
                  if (obj->getLevel() > max_level)
                  {
                     max_level = obj->getLevel();
                  }
                  else if (obj->getLevel() < min_level)
                  {
                     min_level = obj->getLevel();
                  }
               }

               send(QStringLiteral("Within %1 results the levels were %2-%3\r\n").arg(obj_results.size()).arg(min_level).arg(max_level));

               showed_ranges = true;
            }
         }
      }

      if (showed_ranges)
      {
         return eSUCCESS;
      }
   }

   if (!search_world)
   {
      if (limit_output)
      {
         send(QStringLiteral("Searching %1 objects...%2 matches found. Limiting output to %3 matches.\r\n").arg(top_of_objt).arg(obj_results.size()).arg(limit_output));
      }
      else
      {
         send(QStringLiteral("Searching %1 objects...%2 matches found.\r\n").arg(top_of_objt).arg(obj_results.size()));
      }
   }

   QString header;
   qsizetype max_keyword_size = 0, max_short_description_size = 0;

   uint64_t result_nr = {};
   for (const auto &result : obj_results)
   {
      auto obj = result.getObject();
      if (limit_output > 0 && ++result_nr > limit_output)
      {
         break;
      }
      if (QString(obj->name).size() > max_keyword_size)
      {
         max_keyword_size = QString(obj->name).size();
      }

      if (nocolor_strlen(obj->short_description) > max_short_description_size)
      {
         max_short_description_size = nocolor_strlen(obj->short_description);
         max_short_description_size = MAX(20, max_short_description_size);
      }
   }

   if (std::count_if(sl.begin(), sl.end(), [](Search search_item)
                     { return (search_item.getType() == Search::types::O_NAME); }))
   {
      header += QStringLiteral(" [%1]").arg("Keywords", -max_keyword_size);
   }

   if (search_world)
   {
      header += QStringLiteral(" [%1]").arg("Location", 19);
   }

   if (true || std::count_if(sl.begin(), sl.end(), [](Search search_item)
                             { return (search_item.getType() == Search::types::O_SHORT_DESCRIPTION); }))
   {
      header += QStringLiteral(" [%1]").arg(QStringLiteral("Short Description"), -max_short_description_size);
   }

   if (show_details)
   {
      if (search_world)
      {
         header += QStringLiteral(" [%1]").arg(QStringLiteral("Details"), -21);
      }
      else
      {
         header += QStringLiteral(" [%1]").arg(QStringLiteral("Details"));
      }
   }

   if (show_affects)
   {
      header += QStringLiteral(" [%1]").arg(QStringLiteral("Affects"));
   }

   send(QStringLiteral("$7$B[ VNUM] [ LV]%1$R\r\n").arg(header));

   result_nr = 0;
   for (const auto &result : obj_results)
   {
      auto obj = result.getObject();
      if (limit_output > 0 && ++result_nr > limit_output)
      {
         break;
      }
      QString custom_columns;

      if (std::count_if(sl.begin(), sl.end(), [](Search search_item)
                        { return (search_item.getType() == Search::types::O_NAME); }))
      {
         custom_columns += QStringLiteral(" [%1]").arg(obj->name, -max_keyword_size);
      }

      if (search_world)
      {
         switch (result.getLocation())
         {
         case Search::locations::in_inventory:
            custom_columns += QStringLiteral(" [%1]").arg("inventory", 19);
            break;
         case Search::locations::in_equipment:
            custom_columns += QStringLiteral(" [%1]").arg("equipped", 19);
            break;
         case Search::locations::in_room:
            custom_columns += QStringLiteral(" [%1]").arg("in room", 19);
            break;
         case Search::locations::in_vault:
         case Search::locations::in_clan_vault:
            custom_columns += QStringLiteral(" [%1 vault]").arg(result.getName(), 13);
            break;
         }
      }

      // For now short description is always shown
      if (true ||
          std::count_if(sl.begin(), sl.end(), [](Search search_item)
                        { return (search_item.getType() == Search::types::O_SHORT_DESCRIPTION); }))
      {
         // Because the color codes make the std::string longer then it visually appears, we calculate that color code difference and add it to our max_short_description_size to get alignment right
         custom_columns += QStringLiteral(" [%1]").arg(obj->short_description, -(strlen(obj->short_description) - nocolor_strlen(obj->short_description) + max_short_description_size));
      }

      // Needed to show details or affects below
      const Object *vobj = nullptr;
      if (obj->item_number >= 0)
      {
         const int vnum = DC::getInstance()->obj_index[obj->item_number].virt;
         if (vnum >= 0)
         {
            const int rn_of_vnum = real_object(vnum);
            if (rn_of_vnum >= 0)
            {
               vobj = (Object *)DC::getInstance()->obj_index[rn_of_vnum].item;
            }
         }
      }

      QString buffer;
      if (show_details)
      {
         switch (GET_ITEM_TYPE(obj))
         {
         case ITEM_WEAPON:
            buffer = QStringLiteral("%1D%2").arg(obj->obj_flags.value[1]).arg(obj->obj_flags.value[2]);

            if (search_world && vobj != nullptr)
            {
               buffer += " (";
               buffer += getStatDiff(vobj->obj_flags.value[1], obj->obj_flags.value[1]);
               buffer += "D";
               buffer += getStatDiff(vobj->obj_flags.value[2], obj->obj_flags.value[2]);
               buffer += ")";
            }
            break;
         default:
            buffer = "";
            break;
         }
         custom_columns += QStringLiteral(" [%1]").arg(buffer, -21);
         /*
                  int get_weapon_damage_type(Object * wielded);
                  bits = get_weapon_damage_type(obj) - 1000;
                  extern char *strs_damage_types[];
                  csendf(ch, "$3Damage type$R: %s\r\n", strs_damage_types[bits]);
         */
      }

      if (show_affects)
      {
         uint64_t affects_found = 0;
         for (int16_t i = 0; i < obj->num_affects; i++)
         {
            if ((obj->affected[i].location != APPLY_NONE) &&
                (obj->affected[i].modifier != 0 ||
                 (vobj != nullptr &&
                  i < vobj->num_affects &&
                  vobj->affected != nullptr &&
                  vobj->affected[i].location == obj->affected[i].location)))
            {
               QString buffer;
               if (obj->affected[i].location < 1000)
               {
                  buffer = sprinttype(obj->affected[i].location, Object::apply_types);
               }
               else if (!get_skill_name(obj->affected[i].location / 1000).isEmpty())
               {
                  buffer = get_skill_name(obj->affected[i].location / 1000);
               }
               else
               {
                  buffer = "Invalid";
               }

               if (affects_found++ == 0)
               {
                  custom_columns += QStringLiteral(" [");
               }
               else
               {
                  custom_columns += QStringLiteral(",");
               }

               if (obj->affected[i].modifier > 0)
               {
                  custom_columns += QStringLiteral("$R%1%2+%3$R").arg(buffer).arg(getSettingAsColor("color.good")).arg(obj->affected[i].modifier);
               }
               else
               {
                  custom_columns += QStringLiteral("$R%1%2-%3$R").arg(buffer).arg(getSettingAsColor("color.bad")).arg(obj->affected[i].modifier);
               }
            }
         }
         if (affects_found)
         {
            custom_columns += QStringLiteral("]");
         }
      }

      send(QStringLiteral("[%1] [%2]%3\r\n").arg(GET_OBJ_VNUM(obj), 5).arg(obj->obj_flags.eq_level, 3).arg(custom_columns));
   }
   send("\r\nIdentify a virtual object with the command: identify v####\r\n");

   return eSUCCESS;
}

// search type = armor woodbey level = ?