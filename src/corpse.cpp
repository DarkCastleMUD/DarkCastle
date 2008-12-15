/***********************************************************************
*  File: corpse.c Version 1.1                                          *
*  Usage: Corpse Saving over reboots/crashes/copyovers                 *
*                                                                      *
*  By: Michael Cunningham (Romulus) IMP of Legends of The Phoenix MUD  *
*  Permission is granted to use and modify this software as long as    *
*  credits are given in the credits command in the game.               *
*  Built for circle30bpl15                                             *
*  Bunch of code reused from the XAP object patch by Patrick Dughi     *
*                                                  <dughi@IMAXX.NET>   *
*  Thankyou Patrick!                                                   *
*  Some functions have been renamed to protect the innocent            *
*  All Rights Reserved, Copyright (C) 1999                             *
***********************************************************************/

/* The standard includes */
extern "C"
{
  #include <ctype.h>
  #include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <obj.h>
#include <connect.h>
#include <utility.h>
#include <room.h>
#include <spells.h>
#include <player.h>
#include <handler.h>
#include <affect.h>
#include <levels.h>
#include <interp.h>
#include <character.h>
#include <act.h>
#include <structs.h>
#include <db.h>
#include <assert.h>
#include <mobile.h> // ACT_ISNPC
#include <race.h>
#include <returnvals.h>
#include <errno.h>

/* Set this define to wherever you want to save your corpses */
#define CORPSE_FILE "corpse.save" 
#define REAL 0
#define VIRTUAL 1

/* External Structures / Variables */
extern struct obj_data *object_list;
struct obj_data *obj_proto;
extern struct CWorld world;
extern struct index_data *obj_index;   /* index table for object file   */
int16 frozen_start_room = 1;

/* Local Function Declerations */
int count_hash_records(FILE * fl);
struct obj_data *create_obj_new(void);
void save_corpses(void);
void load_corpses(void);
int corpse_save(struct obj_data * obj, FILE * fp, int location, bool recurse_this_tree); 
int write_corpse_to_disk(FILE *fp, struct obj_data *obj, int locate);
void clean_string(char * buffer);
int get_line_new(FILE * fl, char *buf);
struct obj_data *read_object_new(int nr, int type);
char *fread_string_new(FILE * fl, char *error);

/* Tada! THE FUNCTIONS ! Yaaa! */

void clean_string(char * buffer) {
  register char *ptr, *str;

   ptr = buffer;
   str = ptr;
   
   while ((*str = *ptr)) {
     str++;
     ptr++;
     if (*ptr == '\r')
       ptr++;
    }
}

int corpse_save(struct obj_data * obj, FILE * fp, int location, bool recurse_this_tree) 
{
  /* This function basically is responsible for taking the    */
  /* supplied obj and figuring out if it has any contents. If */
  /* it does then we write those to disk.. Ad Nasum.          */

  //struct obj_data *tmp;
  int result;

  if (obj) { /* a little recursion (can be a dangerous thing:) */

    /* recurse_this_tree causes the recursion to branch only
       down the corpses content's tree and not the contents of the
       room. obj->next_content points to the rooms contents
       the first time this function is called from save_corpses
       hence we avoid going down there otherwise we will save
       the rooms contents as well as the corpses contents in the 
       corpse.save file. 
    */

    if (recurse_this_tree != FALSE)
    {
      corpse_save(obj->next_content, fp, location, recurse_this_tree);
    }

    recurse_this_tree = TRUE;
    corpse_save(obj->contains, fp, MIN(0, location) - 1, recurse_this_tree);
    result = write_corpse_to_disk(fp, obj, location); 

    /* readjust the wieght while we do this */
//    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
// No, let's not.      GET_OBJ_WEIGHT(tmp) += GET_OBJ_WEIGHT(obj);
  

    if (!result)
      return (0);
  }
  return (TRUE);
}


int write_corpse_to_disk(FILE *fp, struct obj_data *obj, int locate)
{
  /* This is basically Patrick's my_obj_save_to_disk function with    */ 
  /* a few minor tweaks to make it work for corpses. Basically it     */
  /* writes one object out to the corpse file every time it is called.*/
  /* It can handle regular obj's and XAP objects.                     */

  int counter;
  struct extra_descr_data *ex_desc;
  char buf1[MAX_STRING_LENGTH +1];
  //char buf2[256]; 

      if (obj->action_description) {
        strcpy(buf1, obj->action_description);
        clean_string(buf1);
      } else
        *buf1 = 0;
        fprintf(fp,
             "#%d\n"
             "%d %d %d %d %d %u %d %d\n",
              GET_OBJ_VNUM(obj),   
              locate,
              GET_OBJ_VAL(obj, 0),  
              GET_OBJ_VAL(obj, 1),
              GET_OBJ_VAL(obj, 2),
              GET_OBJ_VAL(obj, 3),
              GET_OBJ_EXTRA(obj),
              GET_OBJ_VROOM(obj),    /*vroom is the virtual room a corpse*/
              GET_OBJ_TIMER(obj));   /* was created in. See make_corpse */
                         
              
  if(!(IS_OBJ_STAT(obj,ITEM_UNIQUE_SAVE))) {
    return 1;
  }
       fprintf(fp,
             "XAP\n" 
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%d %d %d %d %d\n", 
              obj->name ? obj->name : "undefined",
              obj->short_description ? obj->short_description : "undefined",
              obj->description ? obj->description : "undefined",
              buf1,
              GET_OBJ_TYPE(obj),
              GET_OBJ_WEAR(obj),
              (GET_OBJ_WEIGHT(obj) < 0 ? 0 : GET_OBJ_WEIGHT(obj)),
	       GET_OBJ_COST(obj), obj->num_affects
        );
      /* Do we have affects? */   
      for (counter = 0; counter < obj->num_affects; counter++)
        if (obj->affected[counter].modifier)
          fprintf(fp, "A\n"
                  "%d %d\n",
                  obj->affected[counter].location,
                  obj->affected[counter].modifier
            );
              
      /* Do we have extra descriptions? */
      if (obj->ex_description) {        /*. Yep, save them too . */
        for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
          /*. Sanity check to prevent nasty protection faults . */
          if (!*ex_desc->keyword || !*ex_desc->description) {
            continue;
          }
          strcpy(buf1, ex_desc->description);
          clean_string(buf1);
          fprintf(fp, "E\n"
                  "%s~\n"
                  "%s~\n",
                  ex_desc->keyword,
                  buf1
            );
        }
      }
  return 1;
}

void save_corpses(void)
{
   /* This is basically the mother of all the save corpse functions */
   /* You can call it from anywhere in the game with no arguments */
   /* Basically any time a corpse is manipulated in any way..either */
   /* directly or indirectly you need to call this function */

   FILE *fp;
   struct obj_data *i, *next;
   int location = 0;
   char buf1[256] = { 0 };
   extern int do_not_save_corpses;

   if (do_not_save_corpses == 1) return;

   /* Open corpse file */
   if (!(fp = dc_fopen(CORPSE_FILE, "w"))){
     if (errno != ENOENT)   /* if it fails, NOT because of no file */   
     sprintf(buf1, "SYSERR: checking for corpse file %s : %s", CORPSE_FILE, strerror(errno));
     perror(buf1);
     return;
   }

   /* Scan the object list */
   for (i = object_list; i; i = next) {
       next = i->next;      

     /* Check if its a players corpse */
     if (IS_OBJ_STAT(i, ITEM_PC_CORPSE)) 
     {
         /* It is, so save it to a file */
         if (!corpse_save(i, fp, location, FALSE))
         {           
             perror("SYSERR: A corpse didnt save for some reason");
             dc_fclose(fp);
             return;
         }
      }
   }   
   /* Close the corpse file */
   dc_fclose(fp);
} 

void load_corpses(void) 
{
  /* Ahh load corpses.. it was cake to write them out to a file      */
  /* it was a pain to load them back up through without a character. */
  /* Because I dont have a character I couldnt figure out how to     */
  /* put objects back into the corpse the exact way they came out..  */
  /* like objects back in their container, etc. So I just decided to */
  /* Dump it all in the corpse and let the character sort it out.    */
  /* If they dont like it, screwum. They are lucky I coded this:)    */
  /* Oh, and a bunch of this code is from Patricks XAP obj's code    */

  FILE *fp;
  char line[256] = { 0 };
  int t[15],zwei=0;
  int locate=0, j, k, nr, num_objs=0;
  struct obj_data *temp = NULL, *obj = NULL, *next_obj = NULL;
  struct extra_descr_data *new_descr;
  char buf1[256] = {0}, buf2[256] = {0}, buf3[256] = {0};
  bool end = FALSE;
  int number = -1;
  struct obj_data *money;
  int debug = 0;
  int alloc_num_affects = 0;

  if (!(fp = dc_fopen(CORPSE_FILE, "r"))) {
    sprintf(buf1, "SYSERR: READING CORPSE FILE %s in load_corpses", CORPSE_FILE);
    log (buf1, 0, LOG_MISC);
    return;
  }

  if(!feof(fp)) {
    get_line_new(fp, line);
  } else 
    log("No corpses in file to load", 0, LOG_MISC);

  while (!feof(fp) && !end) {
        temp=NULL; 
        /* first, we get the number. Not too hard. */
    if (*line == '|') break;
     else
    if(*line == '#') {
      if (sscanf(line, "#%d", &nr) != 1) {
        continue;
      }
      if (debug == 1) {
        sprintf(buf3, " -Loading Object: %d", nr);
        log (buf3, 0, LOG_MISC);   
      }
      /* we have the number, check it, load obj. */
      if (nr == -1) {   /* then it is unique */
        temp = create_obj_new();
        temp->item_number=nr;
      } else if (nr < 0) {
        continue;
      } else {
        if(nr >= 999999)
          continue;

        if((number = real_object(nr)) < 0)
           continue;
        temp = clone_object(number);
        //temp=read_object_new(nr,VIRTUAL);
        if (!temp) { 
          continue;
        }
      }
      
      get_line_new(fp,line);
      if (debug == 1) {
        sprintf(buf3, " -LINE: %s", line);
        log (buf3, 0, LOG_MISC);
      }
      sscanf(line,"%d %d %d %d %d %d %d %d",t, t + 1, t+2, t + 3, t + 4,t + 5,t + 6, t + 7);
      locate=t[0];
      GET_OBJ_VAL(temp,0) = t[1];
      GET_OBJ_VAL(temp,1) = t[2];
      GET_OBJ_VAL(temp,2) = t[3]; 
      GET_OBJ_VAL(temp,3) = t[4];
      GET_OBJ_EXTRA(temp) = t[5];
      GET_OBJ_VROOM(temp) = t[6];      
      GET_OBJ_TIMER(temp) = t[7];

      get_line_new(fp,line);
      if (debug == 1) {
        sprintf(buf3, " -LINE: %s", line);
        log (buf3, 0, LOG_MISC);
      }
       /* read line check for xap. */
      if(!strcmp("XAP\n", line)) {  /* then this is a Xap Obj, requires special care */
        if (debug == 1) log ("XAP Found", 0, LOG_MISC);
        if ((temp->name = fread_string_new(fp, buf2)) == NULL) {
          temp->name = "undefined";
        } else {
          if (debug == 1) {
            sprintf(buf3, "   -NAME: %s\n", temp->name);
            log (buf3, 0, LOG_MISC);
          }
        }
      
        if ((temp->short_description = fread_string_new(fp, buf2)) == NULL) {
          temp->short_description = "undefined";
        } else {
          if (debug == 1) {
            sprintf(buf3, "   -SHORT: %s\n", temp->short_description);
            log (buf3, 0, LOG_MISC);
          }
        }
      
        if ((temp->description = fread_string_new(fp, buf2)) == NULL) {
          temp->description = "undefined";
        } else {
          if (debug == 1) {
            sprintf(buf3, "   -DESC: %s\n", temp->description);
            log (buf3, 0, LOG_MISC);
          }
        }
      
        if ((temp->action_description = fread_string_new(fp, buf2)) == NULL) {
          temp->action_description = "undefined";
        } else {
          if (debug == 1) {
            sprintf(buf3, "   -ACT_DESC: %s\n", temp->action_description);
            log (buf3, 0, LOG_MISC);
          }
        }
        if (!get_line_new(fp, line) ||
	    (sscanf(line, "%d %d %d %d %d", t,t+1,t+2,t+3,t+4) != 5)) {
           log ("load_corpses: Format error in first numeric line (expecting 5 args)", 0, LOG_MISC);
        } else {
          if (debug == 1) {
            sprintf(buf3, "   -FLAGS: %s", line);
            log (buf3, 0, LOG_MISC);
          }
        }
        temp->obj_flags.type_flag = t[0];
        temp->obj_flags.wear_flags = t[1];
        temp->obj_flags.weight = (t[2] > 0 ? t[2] : 0);
        temp->obj_flags.cost = t[3];
        alloc_num_affects = t[4];

	temp->affected = (obj_affected_type *) calloc(alloc_num_affects, sizeof(obj_affected_type));

        /* buf2 is error codes pretty much */
        sprintf(buf2, ", after numeric constants (expecting E/#xxx)");
      
        /* we're clearing these for good luck */
         
	temp->num_affects = 0; // Cleared, No memory has previously
				// been assigned to 'em

        /* You have to null out the extradescs when you're parsing a xap_obj.
        This is done right before the extradescs are read. */

        if (temp->ex_description) {
           temp->ex_description = NULL;
           }
          
        get_line_new(fp,line);
        for (k=j=zwei=0;!zwei && !feof(fp);) {
          switch (*line) {
            case 'E':
              CREATE(new_descr, struct extra_descr_data, 1);
              new_descr->keyword = fread_string_new(fp, buf2);
              new_descr->description = fread_string_new(fp, buf2);
              new_descr->next = temp->ex_description;
              temp->ex_description = new_descr;
              get_line_new(fp,line);
              break;
            case 'A':
              get_line_new(fp, line);
              sscanf(line, "%d %d", t, t + 1);
      
              temp->affected[temp->num_affects].location = t[0];
              temp->affected[temp->num_affects].modifier = t[1];
	      temp->num_affects++;
              get_line_new(fp,line);
              break;
              
	    case '|':
		zwei = 1;
		end = TRUE; break;
            case '$':
            case '#':
              zwei=1;
              break; 
            default:
              zwei=1;
              break;
          }
        }      /* exit our for loop */
	if (alloc_num_affects != temp->num_affects) {
	    logf(0, LOG_BUG, "alloc_num_affects: %d != temp->num_affects: %d",
		 alloc_num_affects, temp->num_affects);
	}

      } else {  /* exit our xap loop */
         if (nr == -1) {
           if (debug == 1) sprintf(buf3, "GOLD FOUND: %d total", t[1]);
           money = create_money(t[1]);
           obj_to_room(money, real_room(frozen_start_room)); 
           continue;
         }
         if (debug == 1) log ("XAP NOT Found", 0, LOG_MISC);
      }
      if(temp != NULL) {
        num_objs++;
      /* Check if our object is a corpse */
      if (IS_OBJ_STAT(temp, ITEM_PC_CORPSE)) {
            /* scan our temp room for objects */
            for (obj = world[real_room(frozen_start_room)].contents; obj ; obj = next_obj)  {
              next_obj = obj->next_content;
              if (obj) {
                if (debug == 1) {
                  sprintf(buf3, "  -Moving [%s] to [%s]", obj->name, temp->name);
                  log(buf3, 0, LOG_MISC);
                }
                obj_from_room(obj);     /* get those objs from that room */       
                obj_to_obj(obj, temp);  /* and put them in the corpse */
              }
            } /* exit the room scanning loop */
           if (temp) {
               /* put the corpse in the right room */
                if (debug == 1) {
                  sprintf(buf3, "  -Moving corpse [%s] to [%d]", temp->name, GET_OBJ_VROOM(temp));
                  log(buf3, 0, LOG_MISC);
                }
             obj_to_room(temp, real_room(GET_OBJ_VROOM(temp)));
            }
       } else {       
        /* just a plain obj..send it to a temp room until we load a corpse */
          if (debug == 1) {
             sprintf(buf3, "  -Moving corpse [%s] to holding room.", temp->name);
             log(buf3, 0, LOG_MISC);
          }
          obj_to_room(temp, real_room(frozen_start_room)); 
        }
      }
    }
  }
  dc_fclose(fp); 
}

int get_line_new(FILE * fl, char *buf)
{
  char temp[256] = {0};
  int lines = 0,a=0;

  while (!feof(fl))
  {
    switch ((temp[a++] = fgetc(fl)))
    {
	case EOF:
	    return 0;
	case '|':
	case '\n':
	case '\0':
	  if (a<1) return 0;
	  strcpy(buf,temp);
	  buf[a] = '\0';
		return 1;
	  break;
    }
  }
  if (a<1) return 0;
  strcpy(buf,temp);
  buf[a] = '\0';
  return 1;
  do {
    lines++;
    fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && (*temp == '*' || !*temp));

  if (feof(fl))
    return 0;
  else {
    strcpy(buf, temp);
    return lines;
  }
}

struct obj_data *create_obj_new(void)
{ 
  struct obj_data *obj;
  
  CREATE(obj, struct obj_data, 1);
  clear_object(obj); 
  obj->next = object_list;
  object_list = obj;
  /* Corpse saving stuff */
  GET_OBJ_VROOM(obj) = NOWHERE;
  GET_OBJ_TIMER(obj) = 0;
  return obj;
}

/* create a new object from a prototype */
struct obj_data *read_object_new(int nr, int type)
{
  struct obj_data *obj;
  int i;
  char buf[256] = { 0 };

  if (nr < 0) {
    perror("SYSERR: trying to create obj with negative num!");
    return NULL;
  }
  if (type == VIRTUAL) {
    if ((i = real_object(nr)) < 0) {
      sprintf(buf, "Object (V) %d does not exist in database.", nr);
      return NULL;
    }
  } else
    i = nr;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
//  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;

  obj_index[i].number++;

  return obj;
}

char *fread_string_new(FILE * fl, char *error)
{ 
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
  register char *point;
  int done = 0, length = 0, templength = 0;
    
  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      fprintf(stderr, "SYSERR: fread_string_new: format error at or near %s\n",
              error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    if ((point = strchr(tmp, '~')) != NULL) {
      *point = '\0';
      done = 1;
    } else {
      point = tmp + strlen(tmp) - 1;
      *(point++) = '\r';
      *(point++) = '\n'; 
      *point = '\0';
    }   

    templength = strlen(tmp);
        
    if (length + templength >= MAX_STRING_LENGTH) {
      perror("SYSERR: fread_string_new: string too large (db.c)");
      perror(error);
      exit(1);
    } else {
      strcat(buf + length, tmp);
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  if (strlen(buf) > 0) {
    CREATE(rslt, char, length + 1);
    strcpy(rslt, buf);
  } else
    rslt = NULL;

  return rslt;
}

int count_hash_records(FILE * fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return count;
}
