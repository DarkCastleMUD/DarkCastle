/***************************************************************************
 *  file: save.c, Database module.                        Part of DIKUMUD  *
 *  Usage: Saving and loading of characters                                *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Rewritten by MERC Industries, based on crash.c by prometheus           *
 *  (Taquin Ho) and abaddon (Jeff Stile).                                  *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec\@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: save.cpp,v 1.2 2002/06/13 04:41:08 dcastle Exp $ */

extern "C"
{
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
//#include <sys/stat.h>
//#include <unistd.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <obj.h>
#include <room.h>
#include <character.h>
#include <mobile.h>
#include <utility.h>
#include <fileinfo.h> // SAVE_DIR
#include <player.h>
#include <levels.h>
#include <db.h>
#include <connect.h>
#include <handler.h>
#include <race.h>

extern struct index_data *obj_index;
extern CWorld world;
 
struct obj_data * obj_store_to_char( CHAR_DATA *ch, FILE *fpsave, struct obj_data * last_cont );
bool put_obj_in_store( struct obj_data *obj, CHAR_DATA *ch, FILE *fpsave, int wear_pos);
void restore_weight(struct obj_data *obj);
void store_to_char(struct char_file_u *st, CHAR_DATA *ch);
void char_to_store(CHAR_DATA *ch, struct char_file_u *st);

// return 1 on success
// return 0 on failure
// donno where it would fail off hand though unless we ran out of HD space
// or had a failure.  I'm just not willing to code that much fault protection in
// -pir
int save_char_aliases(char_player_alias * alias, FILE * fpsave)
{
  sh_int tmp_size = 0;
  char_player_alias * curr;

  // count up the number of aliases
  for(curr = alias; curr; curr = curr->next)
    tmp_size++;

  // write out how many
  fwrite(&tmp_size, sizeof(sh_int), 1, fpsave);

  // write the aliases out
  for(curr = alias; curr; curr = curr->next)
  {
    // note that we save the number of characters in tmp_size
    tmp_size = strlen(curr->keyword);
    if(tmp_size < 1)  // minimal error checking:)
      continue;
    fwrite (&tmp_size, sizeof tmp_size, 1, fpsave);
    // but we actually write tmp_size +1 to get the trailing \0
    fwrite (curr->keyword, sizeof(char), (tmp_size+1), fpsave);

    tmp_size = strlen(curr->command);
    fwrite (&tmp_size, sizeof tmp_size, 1, fpsave);
    fwrite (curr->command, sizeof(char), (tmp_size+1), fpsave);
  }
  return 1; 
}

// return pointer to aliases or NULL
struct char_player_alias * read_char_aliases(FILE * fpsave)
{
  sh_int total, x;  
  sh_int tmp_size;
  struct char_player_alias * top = NULL;  
  struct char_player_alias * curr = NULL;  

  fread(&total, sizeof(sh_int), 1, fpsave);

  if(total < 1)
    return NULL;

  for(x = 0; x < total; x++) 
  {
#ifdef LEAK_CHECK
    curr = (char_player_alias *) calloc(1, sizeof(char_player_alias));    
#else
    curr = (char_player_alias *) dc_alloc(1, sizeof(char_player_alias));    
#endif

    fread (&tmp_size, sizeof tmp_size, 1, fpsave);
    if(tmp_size > 0) {
#ifdef LEAK_CHECK
       curr->keyword = (char *)calloc(tmp_size + 1, sizeof(char));
#else
       curr->keyword = (char *)dc_alloc(tmp_size + 1, sizeof(char));
#endif
       fread(curr->keyword, sizeof(char), (tmp_size+1), fpsave);

       fread(&tmp_size, sizeof tmp_size, 1, fpsave);
#ifdef LEAK_CHECK
       curr->command = (char *)calloc(tmp_size + 1, sizeof(char));
#else
       curr->command = (char *)dc_alloc(tmp_size + 1, sizeof(char));
#endif
       fread(curr->command, sizeof(char), (tmp_size+1), fpsave);

       curr->next = top;
       top = curr;
     }
  }
  return top;
}

void fwrite_var_string(char * string, FILE * fpsave)
{
   sh_int tmp_size;

   if(string) {
     tmp_size = strlen(string);
     tmp_size++; // count the null terminator
     fwrite(&tmp_size, sizeof(sh_int), 1, fpsave);
     fwrite(string, sizeof(char), (tmp_size), fpsave);
   }
   else {
     tmp_size = 0;
     fwrite(&tmp_size, sizeof(sh_int), 1, fpsave);
   }
}

char * fread_var_string(FILE * fpsave)
{
   sh_int tmp_size;
   char * tmp_str = NULL;

   fread(&tmp_size, sizeof (sh_int), 1, fpsave);
   if(tmp_size > 0) {
#ifdef LEAK_CHECK
     tmp_str = (char *) calloc(tmp_size, sizeof(char));
#else
     tmp_str = (char *) dc_alloc(tmp_size, sizeof(char));
#endif
     fread(tmp_str, sizeof(char), tmp_size, fpsave);
     return tmp_str;
   }
   else return NULL;
}

void save_mob_data(struct mob_data * i, FILE * fpsave)
{
  fwrite(&(i->nr), sizeof(long), 1, fpsave);
  fwrite(&(i->default_pos), sizeof(byte), 1, fpsave);
  fwrite(&(i->attack_type), sizeof(long), 1, fpsave); 
  fwrite(&(i->actflags),    sizeof(long), 1, fpsave);
  fwrite(&(i->damnodice),   sizeof(sh_int), 1, fpsave);
  fwrite(&(i->damsizedice), sizeof(sh_int), 1, fpsave);

  // Any future additions to this save file will need to be placed LAST here with a 3 letter code
  // and appropriate strcmp statement in the read_mob_data object

  fwrite("STP", sizeof(char), 3, fpsave);
}

void read_mob_data(struct mob_data * i, FILE * fpsave)
{
  char typeflag[4];

  fread(&(i->nr), sizeof(long), 1, fpsave);
  fread(&(i->default_pos), sizeof(byte), 1, fpsave);
  fread(&(i->attack_type), sizeof(long), 1, fpsave); 
  fread(&(i->actflags),    sizeof(long), 1, fpsave);
  fread(&(i->damnodice),   sizeof(sh_int), 1, fpsave);
  fread(&(i->damsizedice), sizeof(sh_int), 1, fpsave);

  typeflag[3] = '\0';
  fread(&typeflag, sizeof(char), 3, fpsave);

  // Add new items in this format
//  if(!strcmp(typeflag, "XXX"))
//    do_something

  // Any future additions to this read file will need to be placed LAST

  // at this point, typeflag should = "STP", and we're done reading mob data
}

// TODO - make sure I go back and update the time_data structs everywhere when 
// we lose link, or logout, etc so that the 'played' variable is correct

void save_pc_data(struct pc_data * i, FILE * fpsave)
{
  fwrite(i->pwd,         sizeof(char),       PASSWORD_LEN+1, fpsave);
  save_char_aliases(i->alias, fpsave);
  fwrite(&(i->rdeaths),     sizeof(long),        1, fpsave);
  fwrite(&(i->pdeaths),     sizeof(long),        1, fpsave);
  fwrite(&(i->pkills),      sizeof(long),        1, fpsave);
  fwrite(&(i->pklvl),       sizeof(long),        1, fpsave);
  fwrite(&(i->time),        sizeof(time_data),   1, fpsave);
  fwrite(&(i->bad_pw_tries),sizeof(long),        1, fpsave);
  fwrite(&(i->practices),   sizeof(long),        1, fpsave);
  fwrite(&(i->bank),        sizeof(long),        1, fpsave);
  fwrite(&(i->toggles),     sizeof(long),        1, fpsave);
  fwrite(&(i->punish),      sizeof(long),        1, fpsave);
  fwrite_var_string(i->last_site, fpsave);
  fwrite_var_string(i->poofin, fpsave);
  fwrite_var_string(i->poofout, fpsave);
  fwrite_var_string(i->prompt, fpsave);
  fwrite_var_string(i->rooms, fpsave);
  fwrite_var_string(i->mobiles, fpsave);
  fwrite_var_string(i->objects, fpsave);

  // Quest bitvector one
  if(i->quest_bv1) {
    fwrite("QS1", sizeof(char), 3, fpsave);
    fwrite(&(i->quest_bv1), sizeof(long), 1, fpsave);
  }

  // Saving throw mods
  fwrite("SVM", sizeof(char), 3, fpsave);
  fwrite(&(i->saves_mods), sizeof(int), SAVE_TYPE_MAX+1, fpsave);  // Write the whole array

  // Specializations
  fwrite("SPC", sizeof(char), 3, fpsave);
  fwrite(&(i->specializations), sizeof(long), 1, fpsave);

  // Any future additions to this save file will need to be placed LAST here with a 3 letter code
  // and appropriate strcmp statement in the read_mob_data object

  fwrite("STP", sizeof(char), 3, fpsave);
}

void read_pc_data(struct pc_data * i, FILE* fpsave)
{
  char typeflag[4];

  fread(i->pwd,         sizeof(char),       PASSWORD_LEN+1, fpsave);
  i->alias = read_char_aliases(fpsave);
  fread(&(i->rdeaths),     sizeof(long),        1, fpsave);
  fread(&(i->pdeaths),     sizeof(long),        1, fpsave);
  fread(&(i->pkills),      sizeof(long),        1, fpsave);
  fread(&(i->pklvl),       sizeof(long),        1, fpsave);
  fread(&(i->time),        sizeof(time_data),   1, fpsave);
  fread(&(i->bad_pw_tries),sizeof(long),        1, fpsave);
  fread(&(i->practices),   sizeof(long),        1, fpsave);
  fread(&(i->bank),        sizeof(long),        1, fpsave);
  fread(&(i->toggles),     sizeof(long),        1, fpsave);
  fread(&(i->punish),      sizeof(long),        1, fpsave);
  i->last_site = fread_var_string(fpsave);
  i->poofin    = fread_var_string(fpsave);
  i->poofout   = fread_var_string(fpsave);
  i->prompt    = fread_var_string(fpsave);
  i->rooms     = fread_var_string(fpsave);
  i->mobiles   = fread_var_string(fpsave);
  i->objects   = fread_var_string(fpsave);

  typeflag[3] = '\0';
  fread(&typeflag, sizeof(char), 3, fpsave);

  if(!strcmp("QS1", typeflag))
  {
    fread(&i->quest_bv1, sizeof(long), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  if(!strcmp("SVM", typeflag))
  {
    fread(&(i->saves_mods), sizeof(int),      SAVE_TYPE_MAX+1, fpsave); // read the whole array
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  if(!strcmp("SPC", typeflag))
  {
    fread(&(i->specializations), sizeof(long), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  // Add new items in this format
//  if(!strcmp(typeflag, "XXX"))
//    do_something

  // Any future additions to this read file will need to be placed LAST

  // at this point, typeflag should = "STP", and we're done reading mob data
}

int save_pc_or_mob_data(CHAR_DATA *ch, FILE * fpsave)
{
  if(IS_MOB(ch))
    save_mob_data(ch->mobdata, fpsave);
  else save_pc_data(ch->pcdata, fpsave);

  return 1;
}

int read_pc_or_mob_data(CHAR_DATA *ch, FILE *fpsave)
{
  if(IS_MOB(ch)) {
    ch->pcdata = NULL;
#ifdef LEAK_CHECK
    ch->mobdata = (mob_data *)calloc(1, sizeof(mob_data));
#else
    ch->mobdata = (mob_data *)dc_alloc(1, sizeof(mob_data));
#endif
    read_mob_data(ch->mobdata, fpsave);
  }
  else {
    ch->mobdata = NULL;
#ifdef LEAK_CHECK
    ch->pcdata = (pc_data *)calloc(1, sizeof(pc_data));
#else
    ch->pcdata = (pc_data *)dc_alloc(1, sizeof(pc_data));
#endif
    read_pc_data(ch->pcdata, fpsave);
  }
  return 1;
}

// return 1 on success
// return 0 on failure
int store_worn_eq(char_data * ch, FILE * fpsave)
{
  int wear_pos = -1;
  int iWear = 0;

  for (iWear = 0; iWear < MAX_WEAR; iWear++) 
  {
    wear_pos = iWear;
    if (ch->equipment[iWear]) 
    {
      if (!obj_to_store( ch->equipment[iWear], ch, fpsave, wear_pos))
        return 0;
    }
  }
  return 1;
}

int char_to_store_variable_data(CHAR_DATA * ch, FILE * fpsave)
{
  char_skill_data * skill = ch->skills;

  fwrite_var_string(ch->name, fpsave);
  fwrite_var_string(ch->short_desc, fpsave);
  fwrite_var_string(ch->long_desc, fpsave);
  fwrite_var_string(ch->description, fpsave);
  fwrite_var_string(ch->title, fpsave);

  while(skill) {
    fwrite("SKL", sizeof(char), 3, fpsave);
    fwrite(&(skill->skillnum), sizeof(sh_int), 1, fpsave);
    fwrite(&(skill->learned), sizeof(byte), 1, fpsave);
    fwrite(&(skill->unused), sizeof(long), 5, fpsave);
    skill = skill->next;
  }
  fwrite("END", sizeof(char), 3, fpsave);
  
  // Any future additions to this save file will need to be placed LAST here with a 3 letter code
  // and appropriate strcmp statement in the read_mob_data object

  fwrite("STP", sizeof(char), 3, fpsave);

  return 1;
}

void read_skill(CHAR_DATA * ch, FILE * fpsave)
{
  struct char_skill_data * curr;

#ifdef LEAK_CHECK
  curr = (char_skill_data *) calloc(1, sizeof(char_skill_data));
#else
  curr = (char_skill_data *) dc_alloc(1, sizeof(char_skill_data));
#endif

  fread(&(curr->skillnum), sizeof(sh_int), 1, fpsave);
  fread(&(curr->learned), sizeof(byte), 1, fpsave);
  fread(&(curr->unused), sizeof(long), 5, fpsave);

  curr->next = ch->skills;
  ch->skills = curr;
}

int store_to_char_variable_data(CHAR_DATA * ch, FILE * fpsave)
{
  char typeflag[4];

  ch->name = fread_var_string(fpsave);
  ch->short_desc = fread_var_string(fpsave);
  ch->long_desc = fread_var_string(fpsave);
  ch->description = fread_var_string(fpsave);
  ch->title = fread_var_string(fpsave);

  typeflag[3] = '\0';
  fread(&typeflag, sizeof(char), 3, fpsave);

  while(strcmp(typeflag, "END")) {
    read_skill(ch, fpsave);  
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  fread(&typeflag, sizeof(char), 3, fpsave);

  // Add new items in this format
//  if(!strcmp(typeflag, "XXX"))
//    do_something

  // Any future additions to this read file will need to be placed LAST

  // at this point, typeflag should = "STP", and we're done reading mob data

  return 1;
}

// save a character and inventory.
// maybe modify it to save mobs for quest purposes too
void save_char_obj (CHAR_DATA *ch)
{
  struct  char_file_u uchar;
  FILE *  fpsave  = NULL;
  char    strsave[MAX_INPUT_LENGTH];
  char    name[200];
  int     safe = START_ROOM;
   
  if(IS_NPC(ch) || GET_LEVEL(ch) < 2)
    return;

  // TODO - figure out a way for mob's to save...maybe <mastername>.pet ?

  sprintf (name, "%s/%c/%s", SAVE_DIR, ch->name[0], ch->name);
//  sprintf (strsave, "%s/%c/%s.back", SAVE_DIR, ch->name[0], ch->name);
// think this is slightly more efficient -pir
  sprintf (strsave, "%s.back", name);

  if (!(fpsave = dc_fopen(strsave, "wb"))) {
    send_to_char("Warning!  Did not save.  Could not open file.  Contact a god, do not logoff.\n\r", ch);
    sprintf(log_buf, "Could not open file in save_char_obj. '%s'", strsave);
    perror(log_buf);
    log(log_buf, ANGEL, LOG_BUG);
    return;
  }

  char_to_store (ch, &uchar);

  // if they're in a safe room, save them there.
  // otherwise save them in tavern
  if(IS_SET(world[ch->in_room].room_flags, SAFE))
    uchar.load_room = world[ch->in_room].number;
  else 
    uchar.load_room = real_room(safe);

  if((fwrite(&uchar, sizeof(uchar), 1, fpsave))               &&
     (char_to_store_variable_data(ch, fpsave))                &&
     (save_pc_or_mob_data(ch, fpsave))                        &&
     (obj_to_store (ch->carrying, ch, fpsave, -1))            &&
     (store_worn_eq(ch, fpsave))
    )
  {
    if(fpsave != NULL)
      dc_fclose(fpsave);
    sprintf(log_buf, "mv %s %s", strsave, name); 
    system(log_buf);
  }
  else
  {
    if(fpsave != NULL)
      dc_fclose(fpsave);
    sprintf(log_buf, "Save_char_obj: %s", strsave);
    send_to_char ("WARNING: file problem. You did not save!", ch);
    perror(log_buf);
    log(log_buf, ANGEL, LOG_BUG);
  }
}

// just error crap to avoid using "goto" like we were
void load_char_obj_error(FILE * fpsave, char strsave[MAX_INPUT_LENGTH])
{
  sprintf(log_buf, "Load_char_obj: %s", strsave);
  perror(log_buf);
  log(log_buf, ANGEL, LOG_BUG);
  if(fpsave != NULL)
    dc_fclose(fpsave);
}

// Load a char and inventory into a new_new ch structure.
bool load_char_obj( struct descriptor_data *d, char *name )
{
  FILE *  fpsave  = NULL;
  char    strsave[MAX_INPUT_LENGTH];
  struct  char_file_u   uchar;
  struct obj_data * last_cont = NULL;
  struct  char_data *ch;

  if(!name || !strcmp(name, ""))
    return FALSE;

#ifdef LEAK_CHECK
  ch = (CHAR_DATA *)calloc(1, sizeof(CHAR_DATA));
#else
  ch = (CHAR_DATA *)dc_alloc(1, sizeof(CHAR_DATA));
#endif

  if(d->character)
    free_char(d->character);

  d->character    = ch;
  clear_char(ch);
  ch->desc        = d;

  sprintf(strsave, "%s/%c/%s", SAVE_DIR, UPPER(name[0]), name);

//  struct stat mystats;
//  stat(strsave, &mystats);
//  TODO - Eventually, i'm going to just slurp in the whole file
//  then parse the memory instead of reading each item from file seperately
//  Should be much faster and save our HD from turning itself to mush -pir

  if ((fpsave = dc_fopen(strsave, "rb" )) == NULL)
    return FALSE;

  if(fread(&uchar, sizeof(uchar), 1, fpsave) == 0)
    { load_char_obj_error(fpsave, strsave);  return FALSE;  }

  reset_char(ch);

  store_to_char(&uchar, ch);
  store_to_char_variable_data(ch, fpsave);
  read_pc_or_mob_data(ch, fpsave);

  // stored names only matter for mobs
  if(!IS_MOB(ch)) {
    dc_free(GET_NAME(ch));
    GET_NAME(ch) = str_dup(name);
  }
  
  while(!feof(fpsave)) {
    last_cont = obj_store_to_char( ch, fpsave, last_cont );
  }

  if(fpsave != NULL)
    dc_fclose(fpsave);
  return TRUE;
}

// read data from file for an item.
struct obj_data *  obj_store_to_char(CHAR_DATA *ch, FILE *fpsave, struct obj_data * last_cont )
{
  struct obj_data *obj;
//  struct extra_descr_data *new_new_descr;
//  struct extra_descr_data *ed, *next_ed;

  int j;
  int nr;
  int length;
  int wear_pos;
  char mod_type[4];
  char buf[MAX_STRING_LENGTH];

  // read in the standard file data
  struct obj_file_elem object;
  fread(&object, sizeof(object), 1, fpsave);

  if(feof(fpsave))
    return NULL;

  // if it's a current object, clone it and continue
  // if it's not, then we need to remove it from the pfile so clone obj 1

  if ( ( nr = real_object(object.item_number) ) > -1 ) 
    obj = clone_object(nr);
  else obj = clone_object(1);

  obj->obj_flags.timer       = object.timer;
  wear_pos                   = object.wear_pos;

  // begin sequence find any modifications to the item the person has
  // what happens, is the mods are written in a particular order to the pfile
  // so I only need to go through this once instead of looping through for each
  // one each time.  If we later decide to want to add something else, we just 
  // put it at the end of the sequence and all is good.  We keep reading until
  // we hit a STP flag.  If we aren't on STP by the end of the sequence, then
  // something very bad has happened. -pir
  mod_type[3] = 0;
  fread(&mod_type, sizeof(char), 3, fpsave);

  if(!strcmp("EQL", mod_type))
  {
    fread(&obj->obj_flags.eq_level, sizeof(int), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("VA0", mod_type))
  {
    fread(&obj->obj_flags.value[0], sizeof(int), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("VA1", mod_type))
  {
    fread(&obj->obj_flags.value[1], sizeof(int), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("VA2", mod_type))
  {
    fread(&obj->obj_flags.value[2], sizeof(int), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("VA3", mod_type))
  {
    fread(&obj->obj_flags.value[3], sizeof(int), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("EXF", mod_type))
  {
    fread(&obj->obj_flags.extra_flags, sizeof(int), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("MOF", mod_type))
  {
    fread(&obj->obj_flags.more_flags, sizeof(int), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("TYF", mod_type))
  {
    fread(&obj->obj_flags.type_flag, sizeof(int), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("WEA", mod_type))
  {
    fread(&obj->obj_flags.wear_flags, sizeof(int), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("SZE", mod_type))
  {
    fread(&obj->obj_flags.size, sizeof(int), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("WEI", mod_type))
  {
    fread(&obj->obj_flags.weight, sizeof(int), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("AFF", mod_type))
  {
    fread(&obj->num_affects, sizeof(sh_int), 1, fpsave);
    if(obj->affected)
      dc_free(obj->affected);

#ifdef LEAK_CHECK
    obj->affected = (obj_affected_type *) calloc(obj->num_affects, sizeof(obj_affected_type));
#else
    obj->affected = (obj_affected_type *) dc_alloc(obj->num_affects, sizeof(obj_affected_type));
#endif

    for(j = 0; j < obj->num_affects; j++)
    {
      fread(&obj->affected[j].location, sizeof(sh_int), 1, fpsave);
      fread(&obj->affected[j].modifier, sizeof(sh_int), 1, fpsave);
    }

    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("NAM", mod_type))
  {
    fread(&length, sizeof(int), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->name = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("DES", mod_type))
  {
    fread(&length, sizeof(int), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->description = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("SDE", mod_type))
  {
    fread(&length, sizeof(int), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->short_description = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("ADE", mod_type))
  {
    fread(&length, sizeof(int), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->action_description = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }

  // TODO - put extra desc support here
  // NEW READS GO HERE

  if(nr == -1) {
    extract_obj(obj);
    return NULL;
  }

  // Handle worn EQ
  if ( (wear_pos > -1) && (wear_pos < MAX_WEAR) && (!ch->equipment[wear_pos]))
  {
    equip_char (ch, obj, wear_pos);
    return obj;
  }
  else if(object.container_depth == 1 && last_cont)
  {
    // put the eq in a container
    // this code does not currently support containers in containers
    if(GET_ITEM_TYPE(last_cont) == ITEM_CONTAINER)
    {
      obj_to_obj(obj, last_cont);
      // we don't add weight to the character for containers that are worn
      if(!last_cont->equipped_by)
          IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);
    }
    else {
      obj_to_char(obj, ch); // just in case
      return NULL;
    }
  }
  // screw it, just put it in their inventory
  else {
    obj_to_char(obj, ch);
    return obj;
  }
  return last_cont;
}



bool obj_to_store (struct obj_data *obj, CHAR_DATA *ch, FILE *fpsave, int wear_pos)
{
 // struct obj_data *tmp;

  if (obj == NULL)
    return TRUE;

  // recurse down next item in list
  if (!obj_to_store (obj->next_content, ch, fpsave, -1))
    return FALSE;

  // store myself
  if (!put_obj_in_store (obj, ch, fpsave, wear_pos ))
    return FALSE;

  // store anything IN myself.  That way they get put back in on read
  if (!obj_to_store (obj->contains, ch, fpsave, -1))
    return FALSE;

  return TRUE;
}

// return true on success
// return false on error
// write one object to file
bool put_obj_in_store (struct obj_data *obj, CHAR_DATA *ch, FILE *fpsave, int wear_pos)
{
  struct obj_data *standard_obj;
  struct obj_data *loop_obj;
  int    iAffect, iAff2;
  int    change;
  int    length;
  struct obj_file_elem object;
  sh_int tmp_weight = 0;

  if (GET_ITEM_TYPE(obj) == ITEM_KEY)
    return TRUE;

  if (GET_ITEM_TYPE(obj) == ITEM_NOTE)
    return TRUE;

  if(IS_SET(obj->obj_flags.extra_flags, ITEM_NOSAVE))
    return TRUE;

  if (obj->item_number == NOWHERE)
    return TRUE;

  // Set up items saved for all items
  object.version      = CURRENT_OBJ_VERSION;
  object.item_number  = obj_index[obj->item_number].virt;
  object.timer        = obj->obj_flags.timer;
  object.wear_pos     = wear_pos;
  if(obj->in_obj) // I'm in a container
     object.container_depth = 1;
  else object.container_depth = 0;

  // write basic item format to file
  if(!(fwrite(&object, sizeof(object), 1, fpsave )))
    return FALSE;

  // get a pointer to the standard version of this item
  standard_obj = ((struct obj_data *)obj_index[obj->item_number].item);

  // Begin checking if this item has been modified in any way from the standard
  // If it has, we need to save that particular modification to the file
  // THESE MUST REMAIN IN PROPER ORDER
  // IF YOU HAVE ANYMORE TO ADD, ADD THEM BEFORE THE "STP" FLAG AT END
  if(obj->obj_flags.eq_level    != standard_obj->obj_flags.eq_level) 
  {
    fwrite("EQL", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.eq_level, sizeof(int), 1, fpsave);
  }
  if(obj->obj_flags.value[0]    != standard_obj->obj_flags.value[0])
  {
    fwrite("VA0", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[0], sizeof(int), 1, fpsave);
  }
  if(obj->obj_flags.value[1]    != standard_obj->obj_flags.value[1])
  {
    fwrite("VA1", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[1], sizeof(int), 1, fpsave);
  }
  if(obj->obj_flags.value[2]    != standard_obj->obj_flags.value[2])
  {
    fwrite("VA2", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[2], sizeof(int), 1, fpsave);
  }
  if(obj->obj_flags.value[3]    != standard_obj->obj_flags.value[3])
  {
    fwrite("VA3", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[3], sizeof(int), 1, fpsave);
  }
  if(obj->obj_flags.extra_flags != standard_obj->obj_flags.extra_flags)
  {
    fwrite("EXF", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.extra_flags, sizeof(int), 1, fpsave);
  }
  if(obj->obj_flags.more_flags != standard_obj->obj_flags.more_flags)
  {
    fwrite("MOF", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.more_flags, sizeof(int), 1, fpsave);
  }
  if(obj->obj_flags.type_flag != standard_obj->obj_flags.type_flag)
  {
    fwrite("TYF", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.type_flag, sizeof(int), 1, fpsave);
  }
  if(obj->obj_flags.wear_flags != standard_obj->obj_flags.wear_flags)
  {
    fwrite("WEA", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.wear_flags, sizeof(int), 1, fpsave);
  }
  if(obj->obj_flags.size != standard_obj->obj_flags.size)
  {
    fwrite("SZE", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.size, sizeof(int), 1, fpsave);
  }

  tmp_weight = obj->obj_flags.weight;
  if(GET_ITEM_TYPE(obj) == ITEM_CONTAINER && (loop_obj = obj->contains))
    for (; loop_obj; loop_obj = loop_obj->next_content)
      tmp_weight -= GET_OBJ_WEIGHT(loop_obj);
  if(tmp_weight      != standard_obj->obj_flags.weight)
  {
    fwrite("WEI", sizeof(char), 3, fpsave);
    fwrite(&tmp_weight, sizeof(int), 1, fpsave);
  }

  change = (obj->num_affects != standard_obj->num_affects);
  // since they aren't always in the same order (builder might have swapped them in an
  // rsave or something) we have to search through for each one to see if they are there,
  // just in a different spot
  for (iAffect = 0; (iAffect < obj->num_affects) && !change; iAffect++) 
  {
    // set it to changed, and if we find it, set it back to unchanged, then continue prior loop
    change = 1;
    for(iAff2 = 0; (iAff2 < obj->num_affects) && change; iAff2++)
      if( (obj->affected[iAffect].location == standard_obj->affected[iAff2].location) ||
          (obj->affected[iAffect].modifier == standard_obj->affected[iAff2].modifier))
        change = 0;
  }
  if(change) {
    fwrite("AFF", sizeof(char), 3, fpsave);
    fwrite(&obj->num_affects, sizeof(sh_int), 1, fpsave);
    for (iAffect = 0; iAffect < obj->num_affects; iAffect++)
    {
      fwrite(&obj->affected[iAffect].location, sizeof(sh_int), 1, fpsave);
      fwrite(&obj->affected[iAffect].modifier, sizeof(sh_int), 1, fpsave);
    }
  }

  if(strcmp(obj->name, standard_obj->name))
  {
    fwrite("NAM", sizeof(char), 3, fpsave);
    length = strlen(obj->name);
    fwrite(&length, sizeof(int), 1, fpsave);
    fwrite(obj->name, sizeof(char), length, fpsave);
  }
  if(strcmp(obj->description, standard_obj->description))
  {
    fwrite("DES", sizeof(char), 3, fpsave);
    length = strlen(obj->description);
    fwrite(&length, sizeof(int), 1, fpsave);
    fwrite(obj->description, sizeof(char), length, fpsave);
  }
  if(strcmp(obj->short_description, standard_obj->short_description))
  {
    fwrite("SDE", sizeof(char), 3, fpsave);
    length = strlen(obj->short_description);
    fwrite(&length, sizeof(int), 1, fpsave);
    fwrite(obj->short_description, sizeof(char), length, fpsave);
  }
  if(strcmp(obj->action_description, standard_obj->action_description))
  {
    fwrite("ADE", sizeof(char), 3, fpsave);
    length = strlen(obj->action_description);
    fwrite(&length, sizeof(int), 1, fpsave);
    fwrite(obj->action_description, sizeof(char), length, fpsave);
  }
  // extra descs are a little strange...it's a pointer to a list of them
  // I don't really want to handle this right now, so I'm going to just ignore them now
  // TODO - figure out a way to save extra descs later.  I'll just make them impossible
  // to restring for now

  // THIS IS WHERE YOU SHOULD PUT ANY ADDITIONS TO THE OBJ PFILE THAT NEED TO BE SAVED
  // A CORRESPONDING ENTRY SHOULD BE MADE IN THE READ FUNCTION
  // MAKE SURE YOUR FLAG ISN'T ALREADY USED

  // Stop flag.  This means we are done with this object on the read
  fwrite("STP", sizeof(char), 3, fpsave);

  return TRUE;
}



/*
 * Restore container weights after a save.
 */
void restore_weight(struct obj_data *obj)
{
    struct obj_data *tmp;

    if ( obj == NULL )
  return;

    restore_weight( obj->contains );
    restore_weight( obj->next_content );
    for ( tmp = obj->in_obj; tmp; tmp = tmp->in_obj )
  GET_OBJ_WEIGHT( tmp ) += GET_OBJ_WEIGHT( obj );
}

// Read shared data from pfile
void store_to_char(struct char_file_u *st, CHAR_DATA *ch)
{
    int i;

    ch->clan = st->clan;

    GET_SEX(ch) = st->sex;
    GET_CLASS(ch) = st->c_class;
    GET_RACE(ch) = st->race;
    GET_LEVEL(ch) = st->level;

    ch->hometown = st->hometown;
    if(GET_LEVEL(ch) < 11)
      ch->hometown = START_ROOM;

    GET_STR(ch)       = GET_RAW_STR(ch)   = st->raw_str;
    GET_INT(ch)       = GET_RAW_INT(ch) = st->raw_intel;
    GET_WIS(ch)       = GET_RAW_WIS(ch)   = st->raw_wis;
    GET_DEX(ch)       = GET_RAW_DEX(ch)   = st->raw_dex;
    GET_CON(ch)       = GET_RAW_CON(ch)   = st->raw_con;

    ch->weight  = st->weight;
    ch->height  = st->height;
    ch->gold    = st->gold;
    ch->plat    = st->plat;
    ch->exp     = st->exp;
    ch->immune  = st->immune;
    ch->resist  = st->resist;
    ch->suscept = st->suscept;

    GET_HIT(ch)      = st->hit;
    GET_RAW_HIT(ch)  = st->raw_hit;
    GET_MANA(ch)     = st->mana;
    GET_RAW_MANA(ch) = st->raw_mana;

    // since move and ki don't get "redone" with stat bonuses we need to set the max here
    GET_MOVE(ch)     = st->move;
    ch->max_move     = GET_RAW_MOVE(ch) = st->raw_move;
    GET_KI(ch)       = st->ki;
    GET_MAX_KI(ch)   = GET_RAW_KI(ch)   = st->raw_ki;

    ch->alignment    = st->alignment;
    ch->misc         = st->misc;

    GET_HP_METAS(ch)   = st->hpmetas;
    GET_MANA_METAS(ch) = st->manametas;
    GET_MOVE_METAS(ch) = st->movemetas;

    ch->armor          = st->armor;
    ch->hitroll        = st->hitroll;
    ch->damroll        = st->damroll;
    ch->affected_by    = st->afected_by;
    ch->affected_by2   = st->afected_by2;

//  Just throw away the apply_saving_throw data.  We don't use it anymore
//  TODO - needs to be removed when we write a convertor
//    for(i = 0; i <= 4; i++)
//      ch->apply_saving_throw[i] = st->apply_saving_throw[i];

    for(i = 0; i <= 2; i++)
      GET_COND(ch, i) = st->conditions[i];

    // it's ok assigning the in_room directly since do_on_login_stuff() will
    // make the actual call to "char_to_room" using this data later
    ch->in_room = real_room(st->load_room);

    if(ch->in_room == (-1))
      if(GET_LEVEL(ch) >= IMMORTAL)
        ch->in_room = real_room(17);
      else
        ch->in_room = real_room(START_ROOM);

    affect_total(ch);
}



/* copy vital data from a players char-structure to the file structure */
void char_to_store(CHAR_DATA *ch, struct char_file_u *st)
{
  int i;
  int x;
  struct affected_type *af;
  struct obj_data *char_eq[MAX_WEAR];

  // Remove all the eq and store it in temp storage
  for(i=0; i<MAX_WEAR; i++) {
    if (ch->equipment[i])
      char_eq[i] = unequip_char(ch, i);
    else
      char_eq[i] = 0;
  }

  // Unaffect everything a character can be affected by spell-wise
  for(af = ch->affected; af; af = af->next) {
    affect_modify( ch, af->location, af->modifier, af->bitvector, FALSE);                         
  }

  st->sex      = GET_SEX(ch);
  st->c_class  = GET_CLASS(ch);
  st->race     = GET_RACE(ch);
  st->level    = GET_LEVEL(ch);

  st->raw_str   = GET_RAW_STR(ch);
  st->raw_intel = GET_RAW_INT(ch);
  st->raw_wis   = GET_RAW_WIS(ch);
  st->raw_dex   = GET_RAW_DEX(ch);
  st->raw_con   = GET_RAW_CON(ch);

  st->mana      = GET_MANA(ch);
  st->raw_mana  = GET_RAW_MANA(ch);
  st->hit       = GET_HIT(ch);
  st->raw_hit   = GET_RAW_HIT(ch);
  st->move      = GET_MOVE(ch);
  st->raw_move  = GET_RAW_MOVE(ch);
  st->ki        = GET_KI(ch);
  st->raw_ki    = GET_RAW_KI(ch);

  st->weight    = GET_WEIGHT(ch);
  st->height    = GET_HEIGHT(ch);
  for(i = 0; i <= 2; i++)
    st->conditions[i] = GET_COND(ch, i);

  st->hometown = ch->hometown;
  if(GET_LEVEL(ch) < 11)
    st->hometown = START_ROOM;

  st->load_room = world[ch->in_room].number;

  st->gold      = GET_GOLD(ch);
  st->plat      = GET_PLATINUM(ch);
  st->exp       = GET_EXP(ch);
  st->immune    = ch->immune;
  st->resist    = ch->resist;
  st->suscept   = ch->suscept;
  st->alignment = ch->alignment;
  st->misc      = ch->misc;

  st->hpmetas   = GET_HP_METAS(ch);
  st->manametas = GET_MANA_METAS(ch);
  st->movemetas = GET_MOVE_METAS(ch);
  st->clan      = ch->clan;

  // make sure rest of unused are set to 0
  for(x = 0; x < 5; x++)
    st->extra_ints[x] = 0;

  if(IS_MOB(ch)) {
    st->armor = ch->armor;
    st->hitroll =  ch->hitroll;
    st->damroll =  ch->damroll;
    st->afected_by = ch->affected_by;
    st->afected_by2 = ch->affected_by2;
  }
  else { 
    st->armor   = 100;
    st->hitroll =  0;
    st->damroll =  0;
    st->afected_by = 0;
    st->afected_by2 = 0;
  }

  // TODO - This needs to be remove when we write a pfile convertor to remove it
  for(i = 0; i <= 4; i++)
    st->apply_saving_throw[i] = 0;


  // re-affect the character with spells
  for(af = ch->affected; af; af = af->next) {
      affect_modify( ch, af->location, af->modifier, af->bitvector, TRUE);
  }

  // re-equip the character with his eq
  for(i=0; i<MAX_WEAR; i++) {
    if (char_eq[i])
      equip_char(ch, char_eq[i], i);
  }

  affect_total(ch);
}

