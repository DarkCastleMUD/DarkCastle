/************************************************************************
| $Id: guild.cpp,v 1.107 2006/12/29 12:41:22 dcastle Exp $
| guild.C
| This contains all the guild commands - practice, gain, etc..
*/
#include <character.h>
#include <structs.h>
#include <spells.h>
#include <utility.h>
#include <levels.h>
#include <player.h>
#include <db.h> // exp_table
#include <interp.h>
#include <string.h>
#include <returnvals.h>
#include <ki.h>
#include <mobile.h>
#include <room.h>
#include <sing.h>
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif
extern CWorld world;
extern struct class_skill_defines g_skills[];
extern struct class_skill_defines w_skills[];
extern struct class_skill_defines t_skills[];
extern struct class_skill_defines d_skills[];
extern struct class_skill_defines b_skills[];
extern struct class_skill_defines a_skills[];
extern struct class_skill_defines p_skills[];
extern struct class_skill_defines r_skills[];
extern struct class_skill_defines k_skills[];
extern struct class_skill_defines u_skills[];
extern struct class_skill_defines c_skills[];
extern struct class_skill_defines m_skills[];
extern struct index_data *mob_index;

int get_max(CHAR_DATA *ch, int skill);

int guild(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *owner);

int do_practice(CHAR_DATA *ch, char *arg, int cmd)
{
  /* Call "guild" with a null string for an argument.
     This displays the character's skills. */

  if (arg[0] != '\0'){
    send_to_char("You can only practice in a guild.\n\r", ch);
  } else {
    guild(ch, 0, cmd, "", ch);
  }
  return eSUCCESS;
}

char *per_col(int percent)
{
  if (percent == 0)
    return ( "$B$0");
  if (percent <= 5)
    return ( "$4");
  if (percent <= 10)
    return ( "$4");
  if (percent <= 15)
    return ( "$B$4");
  if (percent <= 20)
    return ( "$B$4");
  if (percent <= 30)
    return ( "$5");
  if (percent <= 40)
    return ( "$5");
  if (percent <=50)
    return ( "$B$5");
  if (percent <= 60)
    return ( "$B$5");
  if (percent <= 70)
    return ( "$2");
  if (percent <= 80)
    return ( "$B$2");
  if(percent <= 85)
    return ( "$B$3");
  if (percent <= 90)
    return ("$B$3");

  return ("$B$7");
  
}

char *how_good(int percent)
{
  if (percent == 0)
    return ( " not learned$R");
  if (percent <= 5)
    return ( " horrible$R");
  if (percent <= 10)
    return ( " crappy$R");
  if (percent <= 15)
    return ( " meager$R");
  if (percent <= 20)
    return ( " bad$R");
  if (percent <= 30)
    return ( " poor$R");
  if (percent <= 40)
    return ( " decent$R");
  if (percent <= 50)
    return ( " average$R");
  if (percent <= 60)
    return ( " fair$R");
  if (percent <= 70)
    return ( " good$R");
  if (percent <= 80)
    return ( " very good$R");
  if(percent <= 85)
    return ( " excellent$R");
  if (percent <= 90)
    return (" superb$R");

  return (" Masterful$R");
}
	
// return a 1 if I just learned skill for first time
// else 0
int learn_skill(char_data * ch, int skill, int amount, int maximum)
{
  struct char_skill_data * curr = ch->skills;
  int old;
  while(curr)
    if(curr->skillnum == skill)
      break;
    else curr = curr->next;

  if(curr) 
  {
    old = curr->learned;
    curr->learned += amount;
    if (skill == SKILL_MAGIC_RESIST)
      barb_magic_resist(ch, old, curr->learned);
    if(curr->learned > maximum)
      curr->learned = maximum;
  }
  else 
  {
#ifdef LEAK_CHECK
    curr = (char_skill_data *)calloc(1, sizeof(char_skill_data));
#else
    curr = (char_skill_data *)dc_alloc(1, sizeof(char_skill_data));
#endif
    if (skill == SKILL_MAGIC_RESIST)
      barb_magic_resist(ch, 0, amount);
    curr->skillnum = skill;
    curr->learned = amount;
    curr->next = ch->skills;
    ch->skills = curr;

   // could save processing power by making it's own function since skillnum is already known
   // but *shrug*
    prepare_character_for_sixty(ch);
    return 1;
  }
  return 0;
}

int search_skills2(int arg, class_skill_defines * list_skills)
{
  for(int i = 0; *list_skills[i].skillname != '\n'; i++)
    if(arg == list_skills[i].skillnum)
      return i;

  return -1;
}

int search_skills(char * arg, class_skill_defines * list_skills)
{
  for(int i = 0; *list_skills[i].skillname != '\n'; i++)
    if(is_abbrev(arg, list_skills[i].skillname))
      return i;

  return -1;
}

int default_master[] = {
  -1,        // no class 0
   1937,     // CLASS_MAGIC_USER   1
   1930,     // CLASS_CLERIC       2   
   1928,     // CLASS_THIEF        3   
   1926,     // CLASS_WARRIOR      4   
   1920,     // CLASS_ANTI_PAL     5   
   1935,     // CLASS_PALADIN      6   
   1922,     // CLASS_BARBARIAN    7   
   1932,     // CLASS_MONK         8   
   1924,     // CLASS_RANGER       9   
   1939,     // CLASS_BARD        10   
   1941,     // CLASS_DRUID       11
   0,        // CLASS_PSIONIC     12
   0         // CLASS_NECROMANCER 13
};

class_skill_defines * get_skill_list(char_data * ch)
{
  class_skill_defines * skilllist = NULL;

  switch(GET_CLASS(ch)) {
    case CLASS_THIEF:      skilllist = t_skills;  break;  
    case CLASS_WARRIOR:    skilllist = w_skills;  break;  
    case CLASS_BARBARIAN:  skilllist = b_skills;  break;  
    case CLASS_PALADIN:    skilllist = p_skills;  break;  
    case CLASS_ANTI_PAL:   skilllist = a_skills;  break;  
    case CLASS_RANGER:     skilllist = r_skills;  break;  
    case CLASS_BARD:       skilllist = d_skills;  break;  
    case CLASS_MONK:       skilllist = k_skills;  break;  
    case CLASS_DRUID:      skilllist = u_skills;  break;
    case CLASS_CLERIC:     skilllist = c_skills;  break;
    case CLASS_MAGIC_USER: skilllist = m_skills;  break;
    default:
      break;
  }
  return skilllist;
}
void debug_here()
{}

char *attrname(int attr)
{
  switch (attr)
  {
    case STR: return "Str";
    case INT: return "Int";
    case WIS: return "Wis";
    case DEX: return "Dex";
    case CON: return "Con";
    default: return "Err";
  }
}
int skillmax(struct char_data *ch, int skill, int eh)
{
  class_skill_defines * skilllist = get_skill_list(ch);
  if (IS_NPC(ch)) return eh;
  if(!skilllist)
    skilllist = g_skills;
  int i = search_skills2(skill, skilllist);
  if (i==-1 && skilllist != g_skills){ skilllist = g_skills; i = search_skills2(skill, g_skills);}
  if (i==-1) return eh;
  if (skilllist[i].maximum < i) return skilllist[i].maximum;
  return eh;
}

char charthing(struct char_data *ch, int known, int skill, int maximum)
{
  if (get_max(ch, skill) <= known) return '*';
  if (known >= maximum/2) return '=';
//  if (known >= GET_LEVEL(ch)*2) return '=';
  return '+';
}

void output_praclist(struct char_data *ch, class_skill_defines *skilllist)
{
  int known;
  char buf[MAX_STRING_LENGTH];
     for(int i=0; *skilllist[i].skillname != '\n';i++) 
    {
      known = has_skill(ch, skilllist[i].skillnum);
      if(!known && GET_LEVEL(ch) < skilllist[i].levelavailable)
          continue;
      known = known % 100;
      sprintf(buf, " %c%-24s%s%14s   $B$0($R%c %s%2d$B$0) $R    $B%2d$R   ", UPPER(*skilllist[i].skillname), (skilllist[i].skillname+1),
                   per_col(known), how_good(known),charthing(ch, known,skilllist[i].skillnum, skilllist[i].maximum), per_col(known), known,
		skilllist[i].levelavailable);
      send_to_char(buf, ch);
      if(skilllist[i].skillnum >= 1 && skilllist[i].skillnum <= MAX_SPL_LIST) 
      {
	if (skilllist[i].skillnum == SPELL_PORTAL && GET_CLASS(ch) == CLASS_CLERIC)
	sprintf(buf," Mana: $B%4d$R ", 150);
	else
        sprintf(buf," Mana: $B%4d$R ",use_mana(ch,skilllist[i].skillnum));
        send_to_char(buf, ch);
      }
      else if (skilllist[i].skillnum >= SKILL_SONG_BASE && skilllist[i].skillnum <= SKILL_SONG_MAX)
      {
	extern struct song_info_type song_info[];
	sprintf(buf, " Ki: $B%4d$R   ",song_info[skilllist[i].skillnum-SKILL_SONG_BASE].min_useski);
	send_to_char(buf,ch);
      }
      else if (skilllist[i].skillnum >= KI_OFFSET && skilllist[i].skillnum <= KI_OFFSET+MAX_KI_LIST)
      {
	extern struct ki_info_type ki_info[];
	sprintf(buf, " Ki: $B%4d$R   ",ki_info[skilllist[i].skillnum-KI_OFFSET].min_useski);
	send_to_char(buf,ch);
      }
      else if (skilllist[i].skillnum == 318) // scan
      {
	sprintf(buf, " Move: $B%4d$R ",2);
	send_to_char(buf,ch);
      }
      else if (skilllist[i].skillnum == 320) // switch
      {
	sprintf(buf, " Move: $B%4d$R ",4);
	send_to_char(buf,ch);
      }
      else if (skilllist[i].skillnum == 319) // consider
      {
	sprintf(buf, " Move: $B%4d$R ",5);
	send_to_char(buf,ch);
      }
      else if (skilllist[i].skillnum == 368) // release
      {
	sprintf(buf, " Move: $B%4d$R ",25);
	send_to_char(buf,ch);
      }

	else send_to_char("            ", ch);
      if (skilllist[i].attrs[0])
      {
	sprintf(buf, " %s ",attrname(skilllist[i].attrs[0]));
	send_to_char(buf, ch);
      }
      if (skilllist[i].attrs[1])
      {
        sprintf(buf, " (%s) ",attrname(skilllist[i].attrs[1]));
        send_to_char(buf, ch);
      }
/*      if(specialization) 
      {
        for(loop = 0; loop < specialization; loop++)
          send_to_char("*", ch);
        send_to_char("\r\n", ch);
      }*/
      if(skilllist[i].skillnum == SKILL_SONG_DISARMING_LIMERICK) { 
        send_to_char("#\n\r", ch);
      }
      else if(skilllist[i].skillnum == SKILL_SONG_FANATICAL_FANFARE) { 
        send_to_char("#\n\r", ch);
      }
      else if(skilllist[i].skillnum == SKILL_SONG_SEARCHING_SONG) { 
        send_to_char("#\n\r", ch);
      }
      else if(skilllist[i].skillnum == SKILL_SONG_VIGILANT_SIREN) { 
        send_to_char("#\n\r", ch);
      }
      else if(skilllist[i].skillnum == SKILL_SONG_MKING_CHARGE) { 
        send_to_char("#\n\r", ch);
      }
      else if(skilllist[i].skillnum == SKILL_SONG_HYPNOTIC_HARMONY) { 
        send_to_char("#\n\r", ch);
      }
      else if(skilllist[i].skillnum == SKILL_SONG_SHATTERING_RESO) { 
        send_to_char("#\n\r", ch);
      }
      else send_to_char("\n\r", ch);
    }

}
int skills_guild(struct char_data *ch, char *arg, struct char_data *owner)
{
  char buf[160];
  int known, x;
  int skillnumber;
  int percent;

  if (IS_NPC(ch)) return eFAILURE;
    class_skill_defines * skilllist = get_skill_list(ch);
    if(!skilllist)
      return eFAILURE;  // no skills to train

  if (!*arg) // display skills that can be learned
  {
    sprintf(buf,"You have %d practice sessions left.\n\r", ch->pcdata->practices);
    send_to_char(buf, ch);
    send_to_char("You can practice any of these skills:\n\r\r\n", ch);

    send_to_char("$BUniversal skills:$R\r\n",ch);
    send_to_char(" Ability:                  Expertise:              Level:   Cost:     Requisites:\r\n",ch);
    send_to_char("---------------------------------------------------------------------------------\r\n",ch);
    output_praclist(ch, g_skills);
     extern char *pc_clss_types[];
    sprintf(buf, "\r\n$B%c%s skills:$R\r\n", UPPER(*pc_clss_types[GET_CLASS(ch)]), (1+pc_clss_types[GET_CLASS(ch)]));
    send_to_char(buf, ch);
    send_to_char(" Ability:                  Expertise:              Level:   Cost:     Requisites:\r\n",ch);
    send_to_char("---------------------------------------------------------------------------------\r\n",ch);
    output_praclist(ch, skilllist);
     send_to_char("\r\n",ch);
    if(GET_CLASS(ch) == CLASS_BARD)
       send_to_char("# denotes a song which requires an instrument.\n\r", ch);
    send_to_char("$B*$R indicates this skill is at its maximum for your race and class.\r\n"
		"$B+$R indicates you can still use practice sessions to improve this skill.\r\n"
		"$B=$R indicates you must use this skill to continue improving.\r\n",ch);
    
    return eSUCCESS;
 
  }
  if (GET_POS(ch) == POSITION_SLEEPING) {
   send_to_char("You can't practice in your sleep.\r\n",ch);
   return eSUCCESS;
  }
  skillnumber = search_skills(arg, skilllist);
    
  if (skillnumber == -1) {
    return eFAILURE;
  }

  if(GET_LEVEL(ch) < skilllist[skillnumber].levelavailable) {
    send_to_char("You aren't advanced enough for that yet.\r\n", ch);
    return eSUCCESS;
  }

  x = skilllist[skillnumber].skillnum;
  known = has_skill(ch, x);

  // we can only train them if they already know it, or if we're the trainer for that skill
  if (!known)
  {
   if (default_master[GET_CLASS(ch)] != mob_index[owner->mobdata->nr].virt) {
	do_say(owner, "I'm sorry, I can't teach you that.  You'll have to find another trainer.",9);
       return eSUCCESS;
   }
   else {
      struct skill_quest *sq;
     if ((sq=find_sq(skilllist[skillnumber].skillname)) != NULL && sq->message && IS_SET(sq->clas, 1<<(GET_CLASS(ch)-1)))
     {
	mprog_driver(sq->message, owner, ch, NULL, NULL);
	return eSUCCESS;
     }
   }

//    if(skilllist[skillnumber].clue && mob_index[owner->mobdata->nr].virt 
//== default_master[GET_CLASS(ch)])
 //     do_say(owner, skilllist[skillnumber].clue, 9);
   // else do_say(owner, "I'm sorry, I can't teach you that.  You'll have 
//to find another trainer.", 9);
//    return eSUCCESS;
  }
  if (ch->pcdata->practices <= 0) {
    send_to_char("You do not seem to be able to practice now.\n\r", ch);
    return eSUCCESS;
  }

  if(known >= get_max(ch, x)) {
     do_emote(owner, "eyes you up and down.", 9);
     do_say(owner, "Taking into account your current attributes, your",9);
     do_say(owner, "maximum proficiency in this ability has been reached.", 9);
     return eSUCCESS;
  }

  if (known >= skilllist[skillnumber].maximum) {
    send_to_char("You are already learned in this area.\n\r", ch);
    return eSUCCESS;
  }
  float maxlearn = (float)skilllist[skillnumber].maximum;
  maxlearn *= 0.5;

  if (known >= ( GET_LEVEL(ch) * 2 )) {
    send_to_char("You aren't experienced enough to practice that any further right now.\n\r", ch);
    return eSUCCESS;
  }
  if (known >= maxlearn)
  {
    send_to_char("You cannot learn more here.. you need to go out into the world and use it.\r\n",ch);
    return eSUCCESS;
  }
  switch(skillnumber)
  {
      case SPELL_VAMPIRIC_AURA:
      case SPELL_DIVINE_INTER:
      case SKILL_BULLRUSH:
      case SPELL_HOLY_AURA:
      case SKILL_SPELLCRAFT:
      case SKILL_COMBAT_MASTERY:
      case KI_MEDITATION+KI_OFFSET:
      case SKILL_CRIPPLE:
      case SKILL_NAT_SELECT:
	do_say(owner, "I cannot teach you that. You need to learn it by yourself.\r\n",9);
	return eFAILURE;
      default: break;
  }
   if (!known)
  switch (GET_CLASS(ch))
  {
    case CLASS_WARRIOR:
      do_say(owner, "Yar! I can be teachin' ye that skill myself! It should only take but a moment.",9);
      break;
    case CLASS_BARBARIAN:
      do_say(owner,"Hah! That easy to learn! I teach you meself.",9);
      break;
    case CLASS_THIEF:
      do_say(owner,"So young rogue, you wish to advance your skills.  I can teach you of this particular one myself.",9);
	break;
     case CLASS_MONK:
      do_say(owner,"Ahh, well met grasshopper!  I can teach you of this from my own knowledge.",9);
	break;
     case CLASS_RANGER:
     do_say(owner,"My woodland lore is more than sufficient to teach you this myself young apprentice!",9);
	break;
     case CLASS_ANTI_PAL:
     do_say(owner,"Ahh, young dark lord, it is but a simple matter to learn this ability.  Follow my instruction.",9);
	break;
     case CLASS_PALADIN:
     do_say(owner,"This ability is one that I am capable of teaching you myself young novice.  Observe closely.",9);
	break;
     case CLASS_BARD:
      do_say(owner, "Ahh young prodigy, that is a tune with which I myself am familiar! Allow me to show you...",9);
	break;
     case CLASS_MAGIC_USER:
     do_say(owner,"Ahh young apprentice, this is a simple matter for me to teach you if you are capable of comprehending.",9);
	break;
     case CLASS_CLERIC:
     do_say(owner, "Well met Acolyte!  I shall pray for you to receive knowledge of the blessing you request.",9);
	break;
     case CLASS_DRUID:
     do_say(owner, "Nature be with you, young druid.  I can teach you the ability you seek myself if you are willing.",9);
	break;
  }
  send_to_char("You practice for a while...\n\r", ch);
  ch->pcdata->practices--;

  percent = 1 + (int)int_app[GET_INT(ch)].learn_bonus;

  learn_skill(ch, x, percent, skilllist[skillnumber].maximum);
    
  if (known >= skilllist[skillnumber].maximum) {
    send_to_char("You are now learned in this area.\n\r", ch);
    return eSUCCESS;
  }

  return eSUCCESS;
}

int guild(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  int64 exp_needed;
  int x = 0;

  if(cmd == 171 && !IS_MOB(ch)) {         /*   gain crap...  */

    if((GET_LEVEL(ch) >= IMMORTAL) || (GET_LEVEL(ch) == MAX_MORTAL)) {
      send_to_char("Only a god can level you now.\n\r", ch);
      return eSUCCESS;
    }

   // TODO - make it so you have to be at YOUR guildmaster to gain

    if (GET_LEVEL(ch) == 50)
    { // To get past 50 you need to know your q skill.
       int skl = -1;
       switch (GET_CLASS(ch))
       {
	 case CLASS_MAGE: skl = SKILL_SPELLCRAFT; break;
	 case CLASS_BARBARIAN: skl = SKILL_BULLRUSH; break;
	 case CLASS_PALADIN: skl = SPELL_HOLY_AURA; break;
	 case CLASS_MONK: skl = KI_OFFSET+KI_MEDITATION; break;
	 case CLASS_WARRIOR: skl = SKILL_COMBAT_MASTERY; break;
	 case CLASS_THIEF: skl = SKILL_CRIPPLE; break;
	 case CLASS_RANGER: skl = SKILL_NAT_SELECT; break;
         case CLASS_CLERIC: skl = SPELL_DIVINE_INTER; break;
 	 case CLASS_ANTI_PAL: skl = SPELL_VAMPIRIC_AURA; break;
         case CLASS_DRUID: skl = SPELL_CONJURE_ELEMENTAL; break;   
	 case CLASS_BARD: skl = SKILL_SONG_HYPNOTIC_HARMONY; break;
       }
       if (!has_skill(ch, skl))
       {
	 send_to_char("You need to learn your questkill before you can progress further.\r\n",ch);
         return eSUCCESS;
       }
    }
    exp_needed = exp_table[(int)GET_LEVEL(ch) + 1];

    if(exp_needed > GET_EXP(ch)) {
      x = (int)(exp_needed - GET_EXP(ch));

      csendf(ch, "You need %d experience to level.\n\r", x);
      return eSUCCESS;
    }

    send_to_char ("You raise a level!\n\r", ch);
    logf(IMMORTAL, LOG_MORTAL, "%s (%s) just gained level %d.", GET_NAME(ch), GET_CLASS(ch), GET_LEVEL(ch)+1);

    GET_LEVEL(ch) +=1;
    advance_level(ch, 0);
    GET_EXP(ch) -= (int)exp_needed;
    int bonus = (GET_LEVEL(ch)-50)*500;
    if (bonus > 0 && MAX_MORTAL == 60)
    {
        char buf[MAX_STRING_LENGTH];
        if (GET_LEVEL(ch) == 60)
	{
	     GET_COND(ch, THIRST) = -1;
	     GET_COND(ch, FULL)   = -1;
	     sprintf(buf, "You have truly reached the highest level of <class> mastery.  As such, the guild will imbue into you some of our most powerful magic and grant you freedom from hunger and thirst!\r\n");
	     send_to_char(buf, ch);
	} else {
	 extern char *pc_clss_types3[];
	 sprintf(buf, "Well done master %s, the guild has collected a tithe to reward your continued support of our profession.\r\nYour guildmaster gives you %d platinum coins.\r\n", pc_clss_types3[GET_CLASS(ch)],bonus);
	 send_to_char(buf,ch);
	 GET_PLATINUM(ch) += bonus;
	}
    }
    return eSUCCESS;
  }
  
  if((cmd != 164)) 
   return eFAILURE;
  
  if(IS_MOB(ch)) {
    send_to_char("Why practice?  You're just going to die anyway...\r\n", ch);
    return eFAILURE;
  }

  for(; *arg==' '; arg++);  // skip whitespace
    
  if(!*arg)
  {
    skills_guild(ch, arg, owner);
  }
  else
  {
    if(IS_SET(skills_guild(ch, arg, owner), eSUCCESS))
      return eSUCCESS;
    else if (search_skills(arg,g_skills) != -1)
       do_say(owner,"Seek out the SKILLS MASTER in the forests west of Sorpigal to learn this ability.", 9);
    else send_to_char("You do not know of this ability...\n\r", ch);
  }

  return eSUCCESS;
}

int skill_master(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, 
                 struct char_data * invoker)
{

  char buf[MAX_STRING_LENGTH];
  int number, i, percent;
  int learned = 0;
  class_skill_defines * skilllist = get_skill_list(ch);

  if(IS_MOB(ch)) {
    send_to_char("Why practice?  You're just going to die anyway...\r\n", ch);
    return eFAILURE;
  }

  if ((cmd != 164) && (cmd != 170)) 
    return eFAILURE;
  
  for(; *arg==' '; arg++);

  if (!*arg) {
      sprintf(buf,"You have %d practice sessions left.\n\r", ch->pcdata->practices);
      send_to_char(buf, ch);
      send_to_char("You can practice any of these skills:\n\r", ch);
      for(i=0; *g_skills[i].skillname != '\n';i++) 
      {
        int known = has_skill(ch, g_skills[i].skillnum);
        if(GET_LEVEL(ch) < g_skills[i].levelavailable) 
          continue;
        sprintf(buf, " %-20s%14s   (Level %2d)\r\n", g_skills[i].skillname,
                   how_good(known), g_skills[i].levelavailable);
        send_to_char(buf, ch);
      }
      return eSUCCESS;
    }
    number = search_skills(arg,g_skills);
    
    if (number == -1) {
      if(!skilllist) return eFAILURE;
      if(search_skills(arg, skilllist) != -1)
         do_say(invoker,"You must speak with your guildmaster to learn such a complicated ability.", 9);
      else send_to_char("You do not know of this skill...\n\r", ch);
      return eSUCCESS;
    }

    if (ch->pcdata->practices <= 0) {
      send_to_char("You do not seem to be able to practice now.\n\r", ch);
      return eSUCCESS;
    }
    learned = has_skill(ch, g_skills[number].skillnum);

    if (learned >= g_skills[number].maximum) {
      send_to_char("You are already learned in this area.\n\r", ch);
      return eSUCCESS;
    }
    if (learned >= ( GET_LEVEL(ch) * 3 ) ) {
      send_to_char("You aren't experienced enough to practice that any further right now.\n\r", ch);
      return eSUCCESS;
    }

    send_to_char("You practice for a while...\n\r", ch);
    ch->pcdata->practices--;
    
    percent = 1+(int)int_app[GET_INT(ch)].learn_bonus;

    learn_skill(ch, g_skills[number].skillnum, percent, g_skills[number].maximum);
    learned = has_skill(ch, g_skills[number].skillnum);
    
    if (learned >= g_skills[number].maximum) {
      send_to_char("You are now learned in this area.\n\r", ch);
      return eSUCCESS;
    }
    return eSUCCESS;
}

int get_stat(CHAR_DATA *ch, int stat)
{
      switch (stat)
      {
        case STRENGTH:
          return GET_RAW_STR(ch);
          break;
        case INTELLIGENCE:
          return GET_RAW_INT(ch);
          break;
        case WISDOM:
          return GET_RAW_WIS(ch);
          break;
        case DEXTERITY:
          return GET_RAW_DEX(ch);
          break;
        case CONSTITUTION:
          return GET_RAW_CON(ch);
          break;
      };
 return 0;
}


// TODO - go ahead and remove 'learned' from everywhere to use it.
// We can't always pass it in, since in 'group' type spells or
// object affects someone that doesn't have the skill is getting a
// valid amount passed in.

void skill_increase_check(char_data * ch, int skill, int learned, int difficulty)
{
   int chance, maximum;
if (ch->in_room && IS_SET(world[ch->in_room].room_flags, NOLEARN))
	return;

   if (!difficulty) 
   {
     logf(IMMORTAL, LOG_BUG, "%s had an invalid skill level of %d in skill %d.", GET_NAME(ch), difficulty, skill);
     return; // Skill w/out difficulty.
   }
   if( ! ( learned = has_skill(ch, skill) ) )
      return; // get out if i don't have the skill

   if(learned >= ( GET_LEVEL(ch) * 2 ))
      return;

   if(IS_MOB(ch))
      return;
   if (IS_SET(world[ch->in_room].room_flags, SAFE)) return;
   class_skill_defines * skilllist = get_skill_list(ch);
   if(!skilllist)
     return;  // class has no skills by default

   maximum = 0;
   int i;
   for(i = 0; *skilllist[i].skillname != '\n'; i++)
     if(skilllist[i].skillnum == skill)
     {
       maximum = skilllist[i].maximum;
       break;
     }
   if (!maximum) {
    skilllist = g_skills;
   for(i = 0; *skilllist[i].skillname != '\n'; i++)
     if(skilllist[i].skillnum == skill)
     {
       maximum = skilllist[i].maximum;
       break;
     }
   }
   if (!maximum) return;
   int to = maximum-3,mod = 0;
   if (skilllist[i].attrs[0])
   {
	int thing = get_stat(ch,skilllist[i].attrs[0])-20;
	if (thing >= 8)
 	{ to += 2; mod += thing/2;}
	
   }
   if (skilllist[i].attrs[1])
   {
	int thing = get_stat(ch,skilllist[i].attrs[1])-20;
	if (thing > 6)
         {to += 1;mod+= thing/4;}
   }

   if(learned >= to)
     return;

   chance = number(1, 101);
   if(learned < 15)
      chance += 5;

   int oi = 101;
   if (difficulty > 500) { oi = 100; difficulty -= 500; }
   switch(difficulty) {
     case SKILL_INCREASE_EASY:
	 if (oi==101) oi = 94;
	 oi -= int_app[GET_INT(ch)].easy_bonus;
        break;
     case SKILL_INCREASE_MEDIUM:
	if (oi==101)oi = 96;
	oi -= int_app[GET_INT(ch)].medium_bonus;
        break;
     case SKILL_INCREASE_HARD:
	if (oi==101)oi = 98;
	oi -= int_app[GET_INT(ch)].hard_bonus;
        break;
     default:
       log("Illegal difficulty value sent to skill_increase_check", IMMORTAL, LOG_BUG);
       break;
   }
   oi -= mod;
   if (oi > chance) return;
   // figure out the name of the affect (if any)
   char * skillname = get_skill_name(skill);

   if(!skillname) {
      csendf(ch, "Increase in unknown skill %d.  Tell a god. (bug)\r\n", skill);
      return;
   }

   // increase the skill by one
   learn_skill(ch, skill, 1, maximum);
   csendf(ch, "You feel more competent in your %s ability.\r\n", skillname);
}

int get_max(CHAR_DATA *ch, int skill)
{
   int maximum;
   class_skill_defines * skilllist = get_skill_list(ch);
   if(!skilllist)
	skilllist = g_skills;
//     return -1;  // class has no skills by default
   maximum = 0;
   int i;
   for(i = 0; *skilllist[i].skillname != '\n'; i++)
     if(skilllist[i].skillnum == skill)
     {
       maximum = skilllist[i].maximum;
       break;
     }
   if (!maximum && skilllist != g_skills) { skilllist = g_skills; 
   for(i = 0; *skilllist[i].skillname != '\n'; i++)
     if(skilllist[i].skillnum == skill)
     {
       maximum = skilllist[i].maximum;
       break;
     }
   }
   int percent = maximum-3;
   if (maximum && skilllist[i].attrs[0])
   {
        int thing = get_stat(ch,skilllist[i].attrs[0])-20;
	if (thing >= 8) percent+=2;
   }
   if (maximum && skilllist[i].attrs[1])
   {
        int thing = get_stat(ch,skilllist[i].attrs[1])-20;
        if (thing > 6)
        percent += 1;
  }

   return percent;
}

void check_maxes(CHAR_DATA *ch)
{
   int maximum;
   class_skill_defines * skilllist = get_skill_list(ch);
   if(!skilllist)
     return;  // class has no skills by default
   maximum = 0;
   int i;
   for(i = 0; *skilllist[i].skillname != '\n'; i++)
     {
       maximum = skilllist[i].maximum;
	int to = maximum-3;
       if (skilllist[i].attrs[0])
       {
            int thing = get_stat(ch,skilllist[i].attrs[0])-20;
            if (thing >= 8)
              to += 2;
       }
       if (skilllist[i].attrs[1])
       {
            int thing = get_stat(ch,skilllist[i].attrs[1])-20;
            if (thing > 6)
              to += 1;
       }
       if (has_skill(ch,skilllist[i].skillnum) > to)
       {
	  struct char_skill_data * curr = ch->skills;

  	  while(curr)
    	  if(curr->skillnum == skilllist[i].skillnum)
      	  break;
    	  else curr = curr->next;

	  if (curr) curr->learned = to;
       }
   }
}
