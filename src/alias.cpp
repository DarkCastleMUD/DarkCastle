/************************************************************************
| $Id: alias.cpp,v 1.2 2002/06/13 04:41:07 dcastle Exp $
| alias.C
| Description:  Commands for the alias processor.
*/
extern "C"
{
  #include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <character.h>
#include <utility.h>
#include <levels.h>
#include <player.h>
#include <returnvals.h>

int do_alias(struct char_data *ch, char *arg, int cmd)
{
  int x,y;
  int z=0;
  char *buf, *buf1;
  char outbuf[MAX_STRING_LENGTH];
  char tmp[2];
  int found;
  int nokey;
  int count = 0;
  struct char_player_alias * curr = NULL;
  struct char_player_alias * next = NULL;

  if(IS_MOB(ch))
    return eFAILURE;

  if (!*arg) 
  {
    curr = ch->pcdata->alias;
    if(!curr) {
      send_to_char("No aliases defined.\r\n", ch);
      return eSUCCESS;
    }
    for (x = 1; curr; curr = curr->next, x++)
    {
      sprintf(outbuf, "Alias %2d:  %s == %s\n\r", x, 
              curr->keyword, curr->command);
      send_to_char(outbuf, ch);
    }
  } else {
    if (strlen(arg) > 180) {
      send_to_char("That's a little to long for an alias. Try something shorter. \n\r", ch);
      return eSUCCESS;
    }
    nokey = 0;
    found = 0;
    for (x=0; x <= (signed)strlen(arg); x++)
    {
      if (arg[x] == '=') {
        found = 1;
        z = x+1;
        break;
      }
      if (arg[x] != ' ')
        nokey = 1;
    }
	
    if (nokey == 0) {        /* No keyword to grab.. exit now...  */
      send_to_char ("You need more than just a space for a keyword.\n\r", ch);
      return eSUCCESS;
    }
	
    if (found == 1)  {    /*  = sign found.. assign an alias  */
      y = (strlen(arg) - z);

#ifdef LEAK_CHECK
      buf  = (char*) calloc(z+1, sizeof(char));
      buf1 = (char*) calloc(y+2, sizeof(char));
#else	    
      buf  = (char*) dc_alloc(z+1, sizeof(char));
      buf1 = (char*) dc_alloc(y+2, sizeof(char));
#endif	    

      tmp[1] = '\0';
	    
      for (x=1; x < z-1; x++) {
        tmp[0] = arg[x];
        strcat(buf, tmp);
      }
	    
      /*   Use only first word for keyword  */
      one_argument(buf,buf);
    
      for ( x = z; arg[x] == ' '; x++)
        z++;

      for (x = z; x <= (signed)strlen(arg); x++)
      {
        tmp[0] = arg[x];
        strcat(buf1, tmp);
      }
	    
      /*   Check for keyword match...
       *   If match found, replace command with command...
       */
      for (x = 1, curr = ch->pcdata->alias; curr; curr = curr->next, x++)  
      {
          if (!str_cmp(curr->keyword, buf)) {
            sprintf(outbuf, "Alias %d: %s == %s    REPLACED with '%s'.\n\r", x,
               buf, curr->command, buf1);
            send_to_char(outbuf, ch);
            dc_free (curr->command);
            curr->command = str_dup(buf1);
            dc_free(buf);
            dc_free(buf1);
            return eSUCCESS;
          }
      }

      /*  If no match found, add a new alias! :) */
      curr = ch->pcdata->alias;
      while(curr) {
        curr = curr->next;
        count++;
      }

      if(count > 24) {
        send_to_char("You can only have 25 aliases, sorry.  Go get tintin or something.\r\n", ch);
        return eSUCCESS;
      }

#ifdef LEAK_CHECK
      curr = (char_player_alias *)calloc(1, sizeof(struct char_player_alias));
#else
      curr = (char_player_alias *)dc_alloc(1, sizeof(struct char_player_alias));
#endif
      curr->keyword = str_dup(buf);
      curr->command = str_dup(buf1);
      send_to_char ("New Alias Defined.\n\r", ch);
      curr->next = ch->pcdata->alias;
      ch->pcdata->alias = curr;
      dc_free(buf);
      dc_free(buf1);
      return eSUCCESS;

    }  else {       /*  only 1 arg passed... delete the shit... */
	    
      x = strlen(arg);

      // need the +1 here.  Otherwise, we end up overrunning our buffer with
      // the \0 character at the end from one_argument
#ifdef LEAK_CHECK
      buf = (char*) calloc(x+1, sizeof(char));
#else
      buf = (char*) dc_alloc(x+1, sizeof(char));
#endif	    
      one_argument(arg, buf);
	    
      if(!str_cmp(buf, "deleteall"))
      {
         for (curr = ch->pcdata->alias; curr; curr = next)
         {
            next = curr->next;
            dc_free (curr->keyword);
            dc_free (curr->command);
            dc_free (curr);
         }
         ch->pcdata->alias = NULL;
         send_to_char("All aliases deleted.\r\n", ch);
         dc_free(buf);
         return eSUCCESS;
      }
            
      for (curr = ch->pcdata->alias; curr; curr = curr->next)
      {
          if (!str_cmp(buf, curr->keyword)) {
            sprintf(outbuf,"Alias %2d: %s == %s DELETED.\n\r",x+1, 
                   curr->keyword, curr->command);
            send_to_char(outbuf, ch);
            // if we're first, reassign the chain
            if(curr == ch->pcdata->alias)
              ch->pcdata->alias = curr->next;
            else {
              // loop through the chain
              next = ch->pcdata->alias;
              while(next->next != curr)
                next = next->next;
              // we should now be pointing at curr, or the world will blow up
              // remove curr from the chain
              next->next = curr->next;
            }
            dc_free (curr->keyword);
            dc_free (curr->command);
            dc_free (curr);
            dc_free(buf);
            return eSUCCESS;
          }
      }
      send_to_char("Alias not found to delete.\r\n", ch);
      dc_free(buf);
    }
  } 
  return eSUCCESS;
}
