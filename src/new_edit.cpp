extern "C"
{
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <connect.h> // descriptor_data
#include <character.h>
#include <utility.h>
#include <mobile.h>
#include <interp.h>
#include <levels.h>
#include <player.h>
#include <obj.h>
#include <handler.h>
#include <db.h>
#include <newedit.h>

// send_to_char("Write your note.  (/s saves /h for help)
void new_edit_board_unlock_board(CHAR_DATA *ch, int abort);
void format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen);
int replace_str(char **string, char *pattern, char *replacement, int rep_all, int max_size);

/*  handle some editor commands */
void parse_action(int command, char *string, struct descriptor_data *d) {
   int indent = 0, rep_all = 0, flags = 0, total_len, replaced;
   register int j = 0;
   int i, line_low, line_high;
   char *s, *t, temp, buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
   
   switch (command) { 
    case PARSE_HELP: 
      sprintf(buf, 
            "Editor command formats: /<letter>\r\n\r\n"
            "/a         -  aborts editor\r\n"
            "/c         -  clears buffer\r\n"
            "/d#        -  deletes a line #\r\n"
            "/e# <text> -  changes the line at # with <text>\r\n"
            "/f         -  formats text\r\n"
            "/fi        -  indented formatting of text\r\n"
            "/h         -  list text editor commands\r\n"
            "/i# <text> -  inserts <text> before line #\r\n"
            "/l         -  lists buffer\r\n"
            "/n         -  lists buffer with line numbers\r\n"
            "/r 'a' 'b' -  replace 1st occurance of text <a> in buffer with text <b>\r\n"
            "/ra 'a' 'b'-  replace all occurances of text <a> within buffer with text <b>\r\n"
            "              usage: /r[a] 'pattern' 'replacement'\r\n"
            "/s         -  saves text\r\n");
      SEND_TO_Q(buf, d);
      break;
    case PARSE_FORMAT: 
      while (isalpha(string[j]) && j < 2) {
       switch (string[j]) {
        case 'i':
          if (!indent) {
             indent = 1;
             flags += FORMAT_INDENT;
          }             
          break;
        default:
          break;
       }     
       j++;
      }
      format_text(d->strnew, flags, d, d->max_str);
      sprintf(buf, "Text formarted with%s indent.\r\n", (indent ? "" : "out")); 
      SEND_TO_Q(buf, d);
      break;    
    case PARSE_REPLACE: 
      while (isalpha(string[j]) && j < 2) {
       switch (string[j]) {
        case 'a':
          if (!indent) {
             rep_all = 1;
          }             
          break;
        default:
          break;
       }     
       j++;
      }
      s = strtok(string, "'");
      if (s == NULL) {
       SEND_TO_Q("Invalid format.\r\n", d);
       return;
      }
      s = strtok(NULL, "'");
      if (s == NULL) {
       SEND_TO_Q("Target string must be enclosed in single quotes.\r\n", d);
       return;
      }
      t = strtok(NULL, "'");
      if (t == NULL) {
       SEND_TO_Q("No replacement string.\r\n", d);
       return;
      }
      t = strtok(NULL, "'");
      if (t == NULL) {
       SEND_TO_Q("Replacement string must be enclosed in single quotes.\r\n", d);
       return;
      }
      total_len = ((strlen(t) - strlen(s)) + strlen(*d->strnew));
    //  sprintf(buf, "ORG: '%s'(%d), NEW: '%s'(%d), OTOT: %d, NTOT: %d\r\n", t, strlen(t), s, strlen(s), strlen(*d->strnew), total_lenth);
  //    SEND_TO_Q(buf, d);
      if (total_len < d->max_str) {
        if ((replaced = replace_str(d->strnew, s, t, rep_all, d->max_str)) > 0) {
          sprintf(buf, "Replaced %d occurance%sof '%s' with '%s'.\r\n", replaced, ((replaced != 1)?"s ":" "), s, t); 
          SEND_TO_Q(buf, d);
        } else if (replaced == 0) {
          sprintf(buf, "String '%s' not found.\r\n", s); 
          SEND_TO_Q(buf, d);
        } else {
          SEND_TO_Q("ERROR: Replacement string causes buffer overflow, aborted replace.\r\n", d);
        }
      } else {
        SEND_TO_Q("Not enough space left in buffer.\r\n", d);
      }
      break;
    case PARSE_DELETE:
      switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
       case 0:
       SEND_TO_Q("You must specify a line number or range to delete.\r\n", d);
       return;
       case 1:
       line_high = line_low;
       break;
       case 2:
       if (line_high < line_low) {
          SEND_TO_Q("That range is invalid.\r\n", d);
          return;
       }
       break;
      }
      
      i = 1;
      total_len = 1;
      if ((s = *d->strnew) == NULL) {
       SEND_TO_Q("Buffer is empty.\r\n", d);
       return;
      }
      if (line_low > 0) {
               while (s && (i < line_low))
         if ((s = strchr(s, '\n')) != NULL) {
            i++;
            s++;
         }
       if ((i < line_low) || (s == NULL)) {
          SEND_TO_Q("Line(s) out of range; not deleting.\r\n", d);
          return;
       }
       
       t = s;
       while (s && (i < line_high))
         if ((s = strchr(s, '\n')) != NULL) {
            i++;
            total_len++;
            s++;
         }
       if ((s) && ((s = strchr(s, '\n')) != NULL)) {
          s++;
          while (*s != '\0') *(t++) = *(s++);
       }
       else total_len--;
       *t = '\0';
       RECREATE(*d->strnew, char, strlen(*d->strnew) + 3);
       sprintf(buf, "%d line%sdeleted.\r\n", total_len,
               ((total_len != 1)?"s ":" "));
       SEND_TO_Q(buf, d);
      }
      else {
       SEND_TO_Q("Invalid line numbers to delete must be higher than 0.\r\n", d);
       return;
      }
      break;
    case PARSE_LIST_NORM:
      /* note: my buf,buf1,buf2 vars are defined at 32k sizes so they
       * are prolly ok fer what i want to do here. */
      *buf = '\0';
      if (*string != '\0')
      switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
       case 0:
         line_low = 1;
         line_high = 999999;
         break;
       case 1:
         line_high = line_low;
         break;
      }
      else {
       line_low = 1;
       line_high = 999999;
      }
      
      if (line_low < 1) {
       SEND_TO_Q("Line numbers must be greater than 0.\r\n", d);
       return;
      }
      if (line_high < line_low) {
       SEND_TO_Q("That range is invalid.\r\n", d);
       return;
      }
      *buf = '\0';
      if ((line_high < 999999) || (line_low > 1)) {
       sprintf(buf, "Current buffer range [%d - %d]:\r\n", line_low, line_high);
      }
      i = 1;
      total_len = 0;
      s = *d->strnew;
      while (s && (i < line_low))
      if ((s = strchr(s, '\n')) != NULL) {
         i++;
         s++;
      }
      if ((i < line_low) || (s == NULL)) {
       SEND_TO_Q("Line(s) out of range; no buffer listing.\r\n", d);
       return;
      }
      
      t = s;
      while (s && (i <= line_high))
      if ((s = strchr(s, '\n')) != NULL) {
         i++;
         total_len++;
         s++;
      }
      if (s)  {
       temp = *s;
       *s = '\0';
       strcat(buf, t);
       *s = temp;
      }
      else strcat(buf, t);
      /* this is kind of annoying.. will have to take a poll and see..
      sprintf(buf, "%s\r\n%d line%sshown.\r\n", buf, total_len,
            ((total_len != 1)?"s ":" "));
       */
      //page_string(d, buf, TRUE);
      SEND_TO_Q(buf, d);
      break;
    case PARSE_LIST_NUM:
      /* note: my buf,buf1,buf2 vars are defined at 32k sizes so they
       * are prolly ok fer what i want to do here. */
      *buf = '\0';
      if (*string != '\0')
      switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
       case 0:
         line_low = 1;
         line_high = 999999;
         break;
       case 1:
         line_high = line_low;
         break;
      }
      else {
       line_low = 1;
       line_high = 999999;
      }
      
      if (line_low < 1) {
       SEND_TO_Q("Line numbers must be greater than 0.\r\n", d);
       return;
      }
      if (line_high < line_low) {
       SEND_TO_Q("That range is invalid.\r\n", d);
       return;
      }
      *buf = '\0';
      i = 1;
      total_len = 0;
      s = *d->strnew;
      while (s && (i < line_low))
      if ((s = strchr(s, '\n')) != NULL) {
         i++;
         s++;
      }
      if ((i < line_low) || (s == NULL)) {
       SEND_TO_Q("Line(s) out of range; no buffer listing.\r\n", d);
       return;
      }
      
      t = s;
      while (s && (i <= line_high))
      if ((s = strchr(s, '\n')) != NULL) {
         i++;
         total_len++;
         s++;
         temp = *s;
         *s = '\0';
         sprintf(buf, "%s%4d: ", buf, (i-1));
         strcat(buf, t);
         *s = temp;
         t = s;
      }
      if (s && t) {
       temp = *s;
       *s = '\0';
       strcat(buf, t);
       *s = temp;
      }
      else if (t) strcat(buf, t);
      /* this is kind of annoying .. seeing as the lines are #ed
      sprintf(buf, "%s\r\n%d numbered line%slisted.\r\n", buf, total_len,
            ((total_len != 1)?"s ":" "));
       */
      //page_string(d, buf, TRUE);
      SEND_TO_Q(buf, d);
      break;

    case PARSE_INSERT:
      half_chop(string, buf, buf2);
      if (*buf == '\0') {
       SEND_TO_Q("You must specify a line number before which to insert text.\r\n", d);
       return;
      }
      line_low = atoi(buf);
      strcat(buf2, "\r\n");
      
      i = 1;
      *buf = '\0';
      if ((s = *d->strnew) == NULL) {
       SEND_TO_Q("Buffer is empty, nowhere to insert.\r\n", d);
       return;
      }
      if (line_low > 0) {
               while (s && (i < line_low))
         if ((s = strchr(s, '\n')) != NULL) {
            i++;
            s++;
         }
       if ((i < line_low) || (s == NULL)) {
          SEND_TO_Q("Line number out of range; insert aborted.\r\n", d);
          return;
       }
       temp = *s;
       *s = '\0';
       if ((int)((strlen(*d->strnew) + strlen(buf2) + strlen(s+1) + 3)) > d->max_str) {
          *s = temp;
          SEND_TO_Q("Insert text pushes buffer over maximum size, insert aborted.\r\n", d);
          return;
       }
       if (*d->strnew && (**d->strnew != '\0')) strcat(buf, *d->strnew);
       *s = temp;
       strcat(buf, buf2);
       if (s && (*s != '\0')) strcat(buf, s);
       RECREATE(*d->strnew, char, strlen(buf) + 3);
       strcpy(*d->strnew, buf);
       SEND_TO_Q("Line inserted.\r\n", d);
      }
      else {
       SEND_TO_Q("Line number must be higher than 0.\r\n", d);
       return;
      }
      break;

    case PARSE_EDIT:
      half_chop(string, buf, buf2);
      if (*buf == '\0') {
       SEND_TO_Q("You must specify a line number at which to change text.\r\n", d);
       return;
      }
      line_low = atoi(buf);
      strcat(buf2, "\r\n");
      
      i = 1;
      *buf = '\0';
      if ((s = *d->strnew) == NULL) {
       SEND_TO_Q("Buffer is empty, nothing to change.\r\n", d);
       return;
      }
      if (line_low > 0) {
       /* loop through the text counting /n chars till we get to the line */
               while (s && (i < line_low))
         if ((s = strchr(s, '\n')) != NULL) {
            i++;
            s++;
         }
       /* make sure that there was a THAT line in the text */
       if ((i < line_low) || (s == NULL)) {
          SEND_TO_Q("Line number out of range; change aborted.\r\n", d);
          return;
       }
       /* if s is the same as *d->strnew that means im at the beginning of the
        * message text and i dont need to put that into the changed buffer */
       if (s != *d->strnew) {
          /* first things first .. we get this part into buf. */
          temp = *s;
          *s = '\0';
          /* put the first 'good' half of the text into storage */
          strcat(buf, *d->strnew);
          *s = temp;
       }
       /* put the new 'good' line into place. */
       strcat(buf, buf2);
       if ((s = strchr(s, '\n')) != NULL) {
          /* this means that we are at the END of the line we want outta there. */
          /* BUT we want s to point to the beginning of the line AFTER
           * the line we want edited */
          s++;
          /* now put the last 'good' half of buffer into storage */
          strcat(buf, s);
       }
       /* check for buffer overflow */
       if ((int)strlen(buf) > d->max_str) {
          SEND_TO_Q("Change causes new length to exceed buffer maximum size, aborted.\r\n", d);
          return;
       }
       /* change the size of the REAL buffer to fit the new text */
       RECREATE(*d->strnew, char, strlen(buf) + 3);
       strcpy(*d->strnew, buf);
       SEND_TO_Q("Line changed.\r\n", d);
      }
      else {
       SEND_TO_Q("Line number must be higher than 0.\r\n", d);
       return;
      }
      break;
    default:
      SEND_TO_Q("Invalid option.\r\n", d);
      log("SYSERR: invalid command passed to parse_action", OVERSEER, LOG_MISC);
      return;
   }
}


/* string manipulation fucntion originally by Darren Wilson */
/* (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) */
/* completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), */
/* substitute appearances of 'pattern' with 'replacement' in string */
/* and return the # of replacements */
int replace_str(char **string, char *pattern, char *replacement, int rep_all,
              int max_size) {
   char *replace_buffer = NULL;
   char *flow, *jetsam, temp;
   int len, i;
   
   if ((int)(strlen(*string) - strlen(pattern)) + (int)strlen(replacement) > max_size)
     return -1;
   
   CREATE(replace_buffer, char, max_size);
   i = 0;
   jetsam = *string;
   flow = *string;
   *replace_buffer = '\0';
   if (rep_all) {
      while ((flow = (char *)strstr(flow, pattern)) != NULL) {
       i++;
       temp = *flow;
       *flow = '\0';
       if ((int)(strlen(replace_buffer) + (int)strlen(jetsam) + (int)strlen(replacement)) > max_size) {
          i = -1;
          break;
       }
       strcat(replace_buffer, jetsam);
       strcat(replace_buffer, replacement);
       *flow = temp;
       flow += strlen(pattern);
       jetsam = flow;
      }
      strcat(replace_buffer, jetsam);
   }
   else {
      if ((flow = (char *)strstr(*string, pattern)) != NULL) {
       i++;
       flow += strlen(pattern);  
       len = ((char *)flow - (char *)*string) - strlen(pattern);
   
       strncpy(replace_buffer, *string, len);
       strcat(replace_buffer, replacement);
       strcat(replace_buffer, flow);
      }
   }
   if (i == 0) return 0;
   if (i > 0) {
      RECREATE(*string, char, strlen(replace_buffer) + 3);
      strcpy(*string, replace_buffer);
   }
   free(replace_buffer);
   return i;
}

/* re-formats message type formatted char * */
/* (for strings edited with d->strnew) (mostly olc and mail)     */
void format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen) {
   int total_chars, cap_next = TRUE, cap_next_next = FALSE;
   char *flow, *start = NULL, temp;
   /* warning: do not edit messages with max_str's of over this value */
   char formated[MAX_STRING_LENGTH];
   
   flow   = *ptr_string;
   if (!flow) return;

   if (IS_SET(mode, FORMAT_INDENT)) {
      strcpy(formated, "   ");
      total_chars = 3;
   }
   else {
      *formated = '\0';
      total_chars = 0;
   } 

   while (*flow != '\0') {
      while ((*flow == '\n') ||
           (*flow == '\r') ||
           (*flow == '\f') ||
           (*flow == '\t') ||
           (*flow == '\v') ||
           (*flow == ' ')) flow++;

      if (*flow != '\0') {

       start = flow++;
       while ((*flow != '\0') &&
              (*flow != '\n') &&
              (*flow != '\r') &&
              (*flow != '\f') &&
              (*flow != '\t') &&
              (*flow != '\v') &&
              (*flow != ' ') &&
              (*flow != '.') &&
              (*flow != '?') &&
              (*flow != '!')) flow++;

       if (cap_next_next) {
          cap_next_next = FALSE;
          cap_next = TRUE;
       }

       /* this is so that if we stopped on a sentance .. we move off the sentance delim. */
       while ((*flow == '.') || (*flow == '!') || (*flow == '?')) {
          cap_next_next = TRUE;
          flow++;
       }
       
       temp = *flow;
       *flow = '\0';

       if ((total_chars + strlen(start) + 1) > 79) {
          strcat(formated, "\r\n");
          total_chars = 0;
       }

       if (!cap_next) {
          if (total_chars > 0) {
             strcat(formated, " ");
             total_chars++;
          }
       }
       else {
          cap_next = FALSE;
          *start = UPPER(*start);
       }

       total_chars += strlen(start);
       strcat(formated, start);

       *flow = temp;
      }

      if (cap_next_next) {
       if ((total_chars + 3) > 79) {
          strcat(formated, "\r\n");
          total_chars = 0;
       }
       else {
          strcat(formated, "  ");
          total_chars += 2;
       }
      }
   }
   strcat(formated, "\r\n");

   if ((int)strlen(formated) > maxlen) formated[maxlen] = '\0';
   RECREATE(*ptr_string, char, MIN(maxlen, (int)strlen(formated)+3));
   strcpy(*ptr_string, formated);
}

void new_string_add(struct descriptor_data *d, char *str)
{
   // char *scan;
    int terminator = 0, action = 0;
    CHAR_DATA *ch = d->character;
    register int i = 2, j = 0;
    char actions[MAX_INPUT_LENGTH];

   int a=0;
      while (str[a] != '\0') {
        if (str[a++] == '~')
        {
          SEND_TO_Q("Cannot add tildes.\r\n",d);
          return;
        }
      }

    if ((action = (*str == '/'))) {
      while (str[i] != '\0') {
         actions[j] = str[i];
         i++;
         j++;
      }
      actions[j] = '\0';
      *str = '\0';
      switch (str[1]) {
       case 'a':
         terminator = 2; /* working on an abort message */
         break;
       case 'c':
         if (*(d->strnew)) {
            *(d->strnew) = NULL;
            SEND_TO_Q("Current buffer cleared.\r\n", d);
         } else
           SEND_TO_Q("Current buffer empty.\r\n", d);
         break;
       case 'd':
         parse_action(PARSE_DELETE, actions, d);
         break;
       case 'e':
         parse_action(PARSE_EDIT, actions, d);
         break;
       case 'f':
         if (*(d->strnew))
           parse_action(PARSE_FORMAT, actions, d);
         else
           SEND_TO_Q("Current buffer empty.\r\n", d);
         break;
       case 'i':
         if (*(d->strnew))
           parse_action(PARSE_INSERT, actions, d);
         else
           SEND_TO_Q("Current buffer empty.\r\n", d);
         break;
       case 'h':
         parse_action(PARSE_HELP, actions, d);
         break;
       case 'l':
         if (*d->strnew)
           parse_action(PARSE_LIST_NORM, actions, d);
         else SEND_TO_Q("Current buffer empty.\r\n", d);
         break;
       case 'n':
         if (*d->strnew)
           parse_action(PARSE_LIST_NUM, actions, d);
         else SEND_TO_Q("Current buffer empty.\r\n", d);
         break;
       case 'r':
         parse_action(PARSE_REPLACE, actions, d);
         break;
       case 's':
         terminator = 1;
         *str = '\0';
         break;
       default:
         SEND_TO_Q("Invalid option.\r\n", d);
         break;
      }
    }


  if (!(*d->strnew)) {
    if ((int)strlen(str) > d->max_str) {
      SEND_TO_Q("String too long - Truncated.\r\n", d);
      *(str + d->max_str) = '\0';
    }
    CREATE(*d->strnew, char, strlen(str) + 5);
    strcpy(*d->strnew, str);
  } else {
    if ((int)(strlen(str) + strlen(*d->strnew)) > d->max_str) {
      if (!action)
        SEND_TO_Q("String too long, limit reached on message.  Last line ignored.\r\n", d);
    } else {
      if (!(*d->strnew = (char *) dc_realloc(*d->strnew, strlen(*d->strnew) + strlen(str) + 5))) {
        perror("string_add");
        abort();
      }
      strcat(*d->strnew, str);
    }
  }
bool ishashed(char *arg);
  if (terminator) {
    if (terminator == 2 || *(d->strnew) == NULL) {
      if ((d->strnew) && (*d->strnew) && (**d->strnew == '\0')
	&& !ishashed(*d->strnew)) 
         dc_free(*d->strnew);
       if (d->backstr) {
         *d->strnew = d->backstr;
       } else {
//         *d->strnew = NULL;
	  *d->strnew = strdup("");
       }
       d->backstr = NULL;
       d->strnew = NULL;
       if(d->connected == CON_WRITE_BOARD) {
         if(d->character)
           d->connected = CON_PLAYING;
           new_edit_board_unlock_board(d->character, 1);
       } else {
         d->connected = CON_PLAYING;
       }
       send_to_char("Aborted.\r\n", ch);
    } else {
      if (strlen(*d->strnew) == 0) {
        SEND_TO_Q("You can't save blank messages, try /a for abort.\r\n", d);
      } else {
        if ((d->strnew) && (*d->strnew) && (**d->strnew == '\0') &&
	!ishashed(*d->strnew))
          dc_free(*d->strnew);
        d->backstr = NULL;
        d->strnew = NULL;
        if(d->connected == CON_WRITE_BOARD) {
          if(d->character)
            d->connected = CON_PLAYING;
            new_edit_board_unlock_board(d->character, 0);
        } else {
          d->connected = CON_PLAYING;
          send_to_char("Ok.\n\r", ch);
        }
      }
    }
  } else {
    if (!action && !((int)(strlen(str) + strlen(*d->strnew) + 2) > d->max_str)) {
      strcat(*d->strnew, "\r\n");
    }
  }
}
