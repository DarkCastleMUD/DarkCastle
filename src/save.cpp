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
/* $Id: save.cpp,v 1.76 2015/06/15 01:06:10 pirahna Exp $ */

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
#include <spells.h>
#include <fileinfo.h> // SAVE_DIR
#include <player.h>
#include <levels.h>
#include <db.h>
#include <connect.h>
#include <handler.h>
#include <race.h>
#include <vault.h>
#include "const.h"

#ifdef USE_SQL
#include <iostream>
#include <libpq-fe.h>
#include "Backend/Database.h"

extern Database db;
#endif

using namespace std;

extern struct index_data *obj_index;
extern CWorld world;
extern short bport;

struct obj_data * obj_store_to_char( CHAR_DATA *ch, FILE *fpsave, struct obj_data * last_cont );
bool put_obj_in_store( struct obj_data *obj, CHAR_DATA *ch, FILE *fpsave, int wear_pos);
void restore_weight(struct obj_data *obj);
void store_to_char(struct char_file_u *st, CHAR_DATA *ch);
char *fread_alias_string(FILE *fpsave);

// return 1 on success
// return 0 on failure
// donno where it would fail off hand though unless we ran out of HD space
// or had a failure.  I'm just not willing to code that much fault protection in
// -pir
int save_char_aliases(char_player_alias * alias, FILE * fpsave)
{
  uint32 tmp_size = 0;
  char_player_alias * curr;

  // count up the number of aliases
  for(curr = alias; curr; curr = curr->next)
    tmp_size++;

  // write out how many
  fwrite(&tmp_size, sizeof(tmp_size), 1, fpsave);

  // write the aliases out
  for(curr = alias; curr; curr = curr->next)
  {
    // note that we save the number of characters in tmp_size
    tmp_size = strlen(curr->keyword);
    if(tmp_size < 1)  // minimal error checking:)
      continue;
    fwrite (&tmp_size, sizeof(tmp_size), 1, fpsave);
    // but we actually write tmp_size +1 to get the trailing \0
    fwrite (curr->keyword, sizeof(char), (tmp_size+1), fpsave);

    tmp_size = strlen(curr->command);
    fwrite (&tmp_size, sizeof(tmp_size), 1, fpsave);
    fwrite (curr->command, sizeof(char), (tmp_size+1), fpsave);
  }
  return 1; 
}

// return pointer to aliases or NULL
struct char_player_alias * read_char_aliases(FILE * fpsave)
{
  uint32 total, x;  
  struct char_player_alias * top = NULL;  
  struct char_player_alias * curr = NULL;  

  fread(&total, sizeof(total), 1, fpsave);

  if(total < 1)
    return NULL;

  for(x = 0; x < total; x++) 
  {
#ifdef LEAK_CHECK
    curr = (char_player_alias *) calloc(1, sizeof(char_player_alias));    
#else
    curr = (char_player_alias *) dc_alloc(1, sizeof(char_player_alias));    
#endif

    curr->keyword = fread_alias_string(fpsave);
    curr->command = fread_alias_string(fpsave);
    if (curr->keyword == NULL || curr->command == NULL) {
       if (curr->keyword == NULL && curr->command)
          dc_free(curr->command);
       if (curr->command == NULL && curr->keyword)
          dc_free(curr->keyword);
       dc_free(curr);
    } else {
       curr->next = top;
       top = curr;
    }
  }
  return top;
}

char *fread_alias_string(FILE *fpsave)
{
    uint32 tmp_size;
    char *buf = NULL;

    fread (&tmp_size, sizeof tmp_size, 1, fpsave);
    if(tmp_size > 0) {
       if (tmp_size > MAX_INPUT_LENGTH) {
          /* flush this string and continue on with the next */
          while (fgetc(fpsave))
             ;
       } else {
#ifdef LEAK_CHECK
          buf = (char *)calloc(tmp_size + 1, sizeof(char));
#else
          buf = (char *)dc_alloc(tmp_size + 1, sizeof(char));
#endif
          fread(buf, sizeof(char), (tmp_size+1), fpsave);
       }
    }
    return(buf);
}

void fwrite_var_string(char * string, FILE * fpsave)
{
   uint16 tmp_size;

   if(string) {
     tmp_size = strlen(string);
     tmp_size++; // count the null terminator
     fwrite(&tmp_size, sizeof(tmp_size), 1, fpsave);
     fwrite(string, sizeof(char), (tmp_size), fpsave);
   }
   else {
     tmp_size = 0;
     fwrite(&tmp_size, sizeof(tmp_size), 1, fpsave);
   }
}

char * fread_var_string(FILE * fpsave)
{
   uint16 tmp_size;
   char * tmp_str = NULL;

   fread(&tmp_size, sizeof (tmp_size), 1, fpsave);
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
  fwrite(&(i->nr),          sizeof(i->nr),          1, fpsave);
  fwrite(&(i->default_pos), sizeof(i->default_pos), 1, fpsave);
  fwrite(&(i->attack_type), sizeof(i->attack_type), 1, fpsave); 
  fwrite(&(i->actflags),    sizeof(i->actflags),    1, fpsave);
  fwrite(&(i->damnodice),   sizeof(i->damnodice),   1, fpsave);
  fwrite(&(i->damsizedice), sizeof(i->damsizedice), 1, fpsave);

  // Any future additions to this save file will need to be placed LAST here with a 3 letter code
  // and appropriate strcmp statement in the read_mob_data object

  fwrite("STP", sizeof(char), 3, fpsave);
}

void read_mob_data(struct mob_data * i, FILE * fpsave)
{
  char typeflag[4];

  fread(&(i->nr),          sizeof(i->nr),          1, fpsave);
  fread(&(i->default_pos), sizeof(i->default_pos), 1, fpsave);
  fread(&(i->attack_type), sizeof(i->attack_type), 1, fpsave); 
  fread(&(i->actflags),    sizeof(i->actflags),    1, fpsave);
  fread(&(i->damnodice),   sizeof(i->damnodice),   1, fpsave);
  fread(&(i->damsizedice), sizeof(i->damsizedice), 1, fpsave);

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

void fwrite_string_tilde(FILE *fpsave)
{
  char buf[40];
  strcpy(buf,"Bugfixbugfixbugfixbugfixbugfixbugfix~");
  fwrite(&buf, 37, 1, fpsave); 
}
void save_pc_data(struct pc_data * i, FILE * fpsave, struct time_data tmpage)
{
  fwrite(i->pwd,            sizeof(char),        PASSWORD_LEN+1, fpsave);
  save_char_aliases(i->alias, fpsave);

  fwrite_string_tilde(fpsave);
  fwrite(&(i->rdeaths),     sizeof(i->rdeaths),  1, fpsave);
  fwrite(&(i->pdeaths),     sizeof(i->pdeaths),  1, fpsave);
  fwrite(&(i->pkills),      sizeof(i->pkills),   1, fpsave);
  fwrite(&(i->pklvl),       sizeof(i->pklvl),    1, fpsave);
  // we save tmpage cause it was calculated when all eq was off
  fwrite(&(tmpage),         sizeof(time_data),   1, fpsave);
  fwrite(&(i->bad_pw_tries),sizeof(i->bad_pw_tries), 1, fpsave);
  fwrite(&(i->practices),   sizeof(i->practices), 1, fpsave);
  fwrite(&(i->bank),        sizeof(i->bank),     1, fpsave);
  fwrite(&(i->toggles),     sizeof(i->toggles),  1, fpsave);
  fwrite(&(i->punish),      sizeof(i->punish),   1, fpsave);
  fwrite_var_string(i->last_site, fpsave);
  fwrite_var_string(i->poofin, fpsave);
  fwrite_var_string(i->poofout, fpsave);
  fwrite_var_string(i->prompt, fpsave);
  fwrite_var_string("NewSaveType",fpsave);

  // Quest bitvector one
  if(i->quest_bv1) {
    fwrite("QS1", sizeof(char), 3, fpsave);
    fwrite(&(i->quest_bv1), sizeof(i->quest_bv1), 1, fpsave);
  }

  // Saving throw mods
  fwrite("SVM", sizeof(char), 3, fpsave);
  fwrite(&(i->saves_mods), sizeof(i->saves_mods[0]), SAVE_TYPE_MAX+1, fpsave);  // Write the whole array

  // Specializations
  fwrite("SPC", sizeof(char), 3, fpsave);
  fwrite(&(i->specializations), sizeof(i->specializations), 1, fpsave);

  // Stat metas
  if(i->statmetas) {
    fwrite("STM", sizeof(char), 3, fpsave);
    fwrite(&(i->statmetas), sizeof(i->statmetas), 1, fpsave);
  }

  // Ki metas
  if(i->kimetas) {
    fwrite("KIM", sizeof(char), 3, fpsave);
    fwrite(&(i->kimetas), sizeof(i->kimetas), 1, fpsave);
  }
  // autojoinin'
  if(i->joining) {
    fwrite("JIN", sizeof(char), 3, fpsave);
    fwrite_var_string(i->joining, fpsave);
  }

  fwrite("QST", sizeof(char), 3, fpsave);
  fwrite(&(i->quest_points), sizeof(i->quest_points), 1, fpsave);
  for(int j=0;j<QUEST_CANCEL;j++)
    fwrite(&(i->quest_cancel[j]), sizeof(i->quest_cancel[j]), 1, fpsave);
  for(int j=0;j<=QUEST_TOTAL/ASIZE;j++)
    fwrite(&(i->quest_complete[j]), sizeof(i->quest_complete[j]), 1, fpsave);
  if (i->buildLowVnum) {
    fwrite("BLO", sizeof(char), 3, fpsave);
    fwrite(&(i->buildLowVnum), sizeof(i->buildLowVnum), 1, fpsave);
  }
  if (i->buildHighVnum) {
    fwrite("BHI", sizeof(char), 3, fpsave);
    fwrite(&(i->buildHighVnum), sizeof(i->buildHighVnum), 1, fpsave);
  }
  if (i->buildMLowVnum) {
    fwrite("BMO", sizeof(char), 3, fpsave);
    fwrite(&(i->buildMLowVnum), sizeof(i->buildMLowVnum), 1, fpsave);
  }
  if (i->buildMHighVnum) {
    fwrite("BMI", sizeof(char), 3, fpsave);
    fwrite(&(i->buildMHighVnum), sizeof(i->buildMHighVnum), 1, fpsave);
  }
  if (i->buildOLowVnum) {
    fwrite("BOO", sizeof(char), 3, fpsave);
    fwrite(&(i->buildOLowVnum), sizeof(i->buildOLowVnum), 1, fpsave);
  }
  if (i->buildOHighVnum) {
    fwrite("BOI", sizeof(char), 3, fpsave);
    fwrite(&(i->buildOHighVnum), sizeof(i->buildOHighVnum), 1, fpsave);
  }
  if (i->profession) {
    fwrite("PRO", sizeof(char), 3, fpsave);
    fwrite(&(i->profession), sizeof(i->profession), 1, fpsave);
  }
  if (i->wizinvis) {
	fwrite("WIZ", sizeof(char), 3, fpsave);
	fwrite(&(i->wizinvis), sizeof(i->wizinvis), 1, fpsave);
  }
  // Any future additions to this save file will need to be placed LAST here with a 3 letter code
  // and appropriate strcmp statement in the read_mob_data object

  fwrite("STP", sizeof(char), 3, fpsave);
}

void fread_to_tilde(FILE *fpsave)
{
  char a;
  while (TRUE)
  {
    fread(&a, 1, 1, fpsave);
    if (a == '~') break; 
  }
}

void read_pc_data(struct char_data *ch, FILE* fpsave)
{
  char typeflag[4];
  struct pc_data * i = ch->pcdata;

  i->golem = 0;
  i->quest_points = 0;
  for(int j=0;j<QUEST_CANCEL;j++)
	i->quest_cancel[j] = 0;
  for(int j=0;j<=QUEST_TOTAL/ASIZE;j++)
	i->quest_complete[j] = 0;

  fread(i->pwd,            sizeof(char),       PASSWORD_LEN+1, fpsave);
  i->alias = read_char_aliases(fpsave);
  if (has_skill(ch, NEW_SAVE))
     fread_to_tilde(fpsave);
  fread(&(i->rdeaths),     sizeof(i->rdeaths),  1, fpsave);
  fread(&(i->pdeaths),     sizeof(i->pdeaths),  1, fpsave);
  fread(&(i->pkills),      sizeof(i->pkills),   1, fpsave);
  fread(&(i->pklvl),       sizeof(i->pklvl),    1, fpsave);
  fread(&(i->time),        sizeof(time_data),   1, fpsave);
  fread(&(i->bad_pw_tries),sizeof(i->bad_pw_tries), 1, fpsave);
  fread(&(i->practices),   sizeof(i->practices), 1, fpsave);
  fread(&(i->bank),        sizeof(i->bank),     1, fpsave);
  fread(&(i->toggles),     sizeof(i->toggles),  1, fpsave);
  fread(&(i->punish),      sizeof(i->punish),   1, fpsave);
  i->last_site = fread_var_string(fpsave);
  i->poofin    = fread_var_string(fpsave);
  i->poofout   = fread_var_string(fpsave);
  i->prompt    = fread_var_string(fpsave);

  char *tmp = fread_var_string(fpsave);
  if (!tmp || str_cmp(tmp,"NewSaveType"))
  {
    tmp = fread_var_string(fpsave);
    tmp = fread_var_string(fpsave);
  }
  i->skillchange = 0;
  typeflag[3] = '\0';
  fread(&typeflag, sizeof(char), 3, fpsave);

  if(!strcmp("QS1", typeflag))
  {
    fread(&i->quest_bv1, sizeof(i->quest_bv1), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  if(!strcmp("SVM", typeflag))
  {
    fread(&(i->saves_mods), sizeof(i->saves_mods[0]), SAVE_TYPE_MAX+1, fpsave); // read the whole array
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  if(!strcmp("SPC", typeflag))
  {
    fread(&(i->specializations), sizeof(i->specializations), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  if(!strcmp("STM", typeflag))
  {
    fread(&i->statmetas, sizeof(i->statmetas), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  if(!strcmp("KIM", typeflag))
  {
    fread(&i->kimetas, sizeof(i->kimetas), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  i->joining = 0;
  if (!strcmp("JIN", typeflag))
  {
    i->joining = fread_var_string(fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("QST", typeflag))
  {
    fread(&(i->quest_points), sizeof(i->quest_points), 1, fpsave);
    for(int j = 0;j<QUEST_CANCEL;j++)
      fread(&(i->quest_cancel[j]), sizeof(i->quest_cancel[j]), 1, fpsave);
    for(int j=0;j<=QUEST_TOTAL/ASIZE;j++)
      fread(&(i->quest_complete[j]), sizeof(i->quest_complete[j]), 1, fpsave);
   fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BLO", typeflag))
  {
    fread(&i->buildLowVnum, sizeof(i->buildLowVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BHI", typeflag))
  {
    fread(&i->buildHighVnum, sizeof(i->buildHighVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BMO", typeflag))
  {
    fread(&i->buildMLowVnum, sizeof(i->buildMLowVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BMI", typeflag))
  {
    fread(&i->buildMHighVnum, sizeof(i->buildMHighVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BOO", typeflag))
  {
    fread(&i->buildOLowVnum, sizeof(i->buildOLowVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BOI", typeflag))
  {
    fread(&i->buildOHighVnum, sizeof(i->buildOHighVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("PRO", typeflag))
  {
    fread(&i->profession, sizeof(i->profession), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("WIZ", typeflag))
  {
	  fread(&i->wizinvis, sizeof(i->wizinvis), 1, fpsave);
	  fread(&typeflag, sizeof(char), 3, fpsave);
  }
  i->skillchange = 0;
  // Add new items in this format
//  if(!strcmp(typeflag, "XXX"))
//    do_something

  // Any future additions to this read file will need to be placed LAST

  // at this point, typeflag should = "STP", and we're done reading mob data
}

int save_pc_or_mob_data(CHAR_DATA *ch, FILE * fpsave, struct time_data tmpage)
{
  if(IS_MOB(ch))
    save_mob_data(ch->mobdata, fpsave);
  else
    save_pc_data(ch->pcdata, fpsave, tmpage);

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
    read_pc_data(ch, fpsave);
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
extern int learn_skill(char_data * ch, int skill, int amount, int maximum);

int char_to_store_variable_data(CHAR_DATA * ch, FILE * fpsave)
{

  fwrite_var_string(ch->name, fpsave);
  fwrite_var_string(ch->short_desc, fpsave);
  fwrite_var_string(ch->long_desc, fpsave);
  fwrite_var_string(ch->description, fpsave);
  fwrite_var_string(ch->title, fpsave);

  if (!has_skill(ch, NEW_SAVE)) // New save.
     learn_skill(ch, NEW_SAVE, 1, 100);    

  char_skill_data * skill = ch->skills;

  while(skill) {
    fwrite("SKL", sizeof(char), 3, fpsave);
    fwrite(&(skill->skillnum), sizeof(skill->skillnum), 1, fpsave);
    fwrite(&(skill->learned), sizeof(skill->learned), 1, fpsave);
    // this writes all 5 of them
    fwrite(&(skill->unused), sizeof(skill->unused[0]), 5, fpsave);
    skill = skill->next;
  }
  fwrite("END", sizeof(char), 3, fpsave);

  struct affected_type *af;
  int16 aff_count = 0; // do not change from int16

  for(af = ch->affected; af; af = af->next)
    aff_count++;

  if(aff_count)
  {
    fwrite("AFS", sizeof(char), 3, fpsave);
    fwrite(&aff_count, sizeof(aff_count), 1, fpsave);
    for(af = ch->affected; af; af = af->next)
    {
       fwrite(&(af->type),      sizeof(af->type),      1, fpsave);
       fwrite(&(af->duration),  sizeof(af->duration),  1, fpsave);
       fwrite(&(af->modifier),  sizeof(af->modifier),  1, fpsave);
       fwrite(&(af->location),  sizeof(af->location),  1, fpsave);
       fwrite(&(af->bitvector), sizeof(af->bitvector), 1, fpsave);
    }
  }

  struct tempvariable *mpv;
  for (mpv = ch->tempVariable;mpv;mpv = mpv->next)
  {
    if (!mpv->save) continue;
    fwrite("MPV", sizeof(char),3, fpsave);
    fwrite_var_string(mpv->name,fpsave);
    fwrite_var_string(mpv->data,fpsave);
  }
  
  fwrite("GLD",sizeof(char),3,fpsave);
  fwrite(&ch->gold, sizeof(ch->gold), 1, fpsave);

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

  fread(&(curr->skillnum), sizeof(curr->skillnum), 1, fpsave);
  fread(&(curr->learned), sizeof(curr->learned), 1, fpsave);
  fread(&(curr->unused), sizeof(curr->unused[0]), 5, fpsave);

//  The above line takes care of these four.  They are here for future use
//  fread(&(curr->unused[1]), sizeof(curr->unused[1]), 1, fpsave);
//  fread(&(curr->unused[2]), sizeof(curr->unused[2]), 1, fpsave);
//  fread(&(curr->unused[3]), sizeof(curr->unused[3]), 1, fpsave);
//  fread(&(curr->unused[4]), sizeof(curr->unused[4]), 1, fpsave);

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

  if(!strncmp(typeflag, "AFS", 3)) // affects
  {
    int16 aff_count; // do not change form int16
    fread(&aff_count, sizeof(aff_count), 1, fpsave);
    ch->affected = NULL;
    for(int16 i = 0; i < aff_count; i++)
    {
       affected_type * af = new (nothrow) affected_type;
       af->duration_type = 0;
       af->next = ch->affected;
       ch->affected = af;

       fread(&(af->type),      sizeof(af->type),      1, fpsave);
       fread(&(af->duration),  sizeof(af->duration),  1, fpsave);
       fread(&(af->modifier),  sizeof(af->modifier),  1, fpsave);
       fread(&(af->location),  sizeof(af->location),  1, fpsave);
       fread(&(af->bitvector), sizeof(af->bitvector), 1, fpsave);

       affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE); // re-affect the char
    }
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  while (!strcmp(typeflag, "MPV"))
  {  // MobProgVars
     struct tempvariable *mpv;
#ifdef LEAK_CHECK
     mpv = (struct tempvariable *)calloc(1, sizeof(struct tempvariable));
#else
     mpv = (struct tempvariable *)dc_alloc(1, sizeof(struct tempvariable));
#endif
     mpv->name = fread_var_string(fpsave);
     mpv->data = fread_var_string(fpsave);
     mpv->save = 1;
     mpv->next = ch->tempVariable;
     ch->tempVariable = mpv;
     fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp(typeflag, "GLD"))
  {
      fread(&(ch->gold), sizeof(ch->gold), 1, fpsave);
      fread(&typeflag, sizeof(char), 3, fpsave);
  }
  // Add new items in this format
//  if(!strcmp(typeflag, "XXX"))
//    do_something

  // Any future additions to this read file will need to be placed LAST

  // at this point, typeflag should = "STP", and we're done reading mob data

  return 1;
}


#ifdef USE_SQL
void save_char_obj_db(CHAR_DATA *ch)
{
  if (ch == 0)
    return;

  if(IS_NPC(ch) || GET_LEVEL(ch) < 2)
    return;

  // so weapons stop falling off
  SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); 
  
  char_file_u uchar;
  time_data tmpage;
  memset(&uchar, 0, sizeof(uchar));
  memset(&tmpage, 0, sizeof(tmpage));

  char_to_store(ch, &uchar, tmpage);

  // if they're in a safe room, save them there.
  // if they're a god, send 'em home
  // otherwise save them in tavern
  if(IS_SET(world[ch->in_room].room_flags, SAFE))
    uchar.load_room = world[ch->in_room].number;
  else
    uchar.load_room = real_room(GET_HOME(ch));

  timeval start, finish;

  gettimeofday(&start, NULL);
  db.save(ch, &uchar);
  gettimeofday(&finish, NULL);

  int msec = finish.tv_sec*1000 + finish.tv_usec/1000;
  msec -= start.tv_sec*1000 + start.tv_usec/1000;
  csendf(ch, "Save took %dms\n\r", msec);


  /*
  if((fwrite(&uchar, sizeof(uchar), 1, fpsave))               &&
     (char_to_store_variable_data(ch, fpsave))                &&
     (save_pc_or_mob_data(ch, fpsave, tmpage))                &&
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

  REMBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT);
  struct vault_data *vault;
  if ((vault = has_vault(GET_NAME(ch))))
    save_vault(vault->owner);
  */

}
#endif

// save a character and inventory.
// maybe modify it to save mobs for quest purposes too
void save_char_obj (CHAR_DATA *ch)
{
  char_file_u uchar;
  time_data tmpage;
  FILE * fpsave  = 0;
  char strsave[MAX_INPUT_LENGTH] = {0};
  char name[200] = {0};

  memset(&uchar, 0, sizeof(uchar));
  memset(&tmpage, 0, sizeof(tmpage));

  if(IS_NPC(ch) || GET_LEVEL(ch) < 2)
    return;

  // TODO - figure out a way for mob's to save...maybe <mastername>.pet ?
  if (bport)
  sprintf (name, "%s/%c/%s", BSAVE_DIR, ch->name[0], ch->name);
  else
  sprintf (name, "%s/%c/%s", SAVE_DIR, ch->name[0], ch->name);

  sprintf (strsave, "%s.back", name);

  if (!(fpsave = dc_fopen(strsave, "wb"))) {
    send_to_char("Warning!  Did not save.  Could not open file.  Contact a god, do not logoff.\n\r", ch);
    sprintf(log_buf, "Could not open file in save_char_obj. '%s'", strsave);
    perror(log_buf);
    log(log_buf, ANGEL, LOG_BUG);
    return;
  }
  
  SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop falling off

  char_to_store (ch, &uchar, tmpage);

  // if they're in a safe room, save them there.
  // if they're a god, send 'em home
  // otherwise save them in tavern
  if(IS_SET(world[ch->in_room].room_flags, SAFE))
    uchar.load_room = world[ch->in_room].number;
  else
    uchar.load_room = real_room(GET_HOME(ch));

  if((fwrite(&uchar, sizeof(uchar), 1, fpsave))               &&
     (char_to_store_variable_data(ch, fpsave))                &&
     (save_pc_or_mob_data(ch, fpsave, tmpage))                &&
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

  REMBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT);
  struct vault_data *vault;
  if ((vault = has_vault(GET_NAME(ch))))
    save_vault(vault->owner);
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

  if (bport)
  sprintf(strsave, "%s/%c/%s", BSAVE_DIR, UPPER(name[0]), name);
  else
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
 if (!IS_NPC(ch) && ch->pcdata->time.logon < 1117527906)
  {
    extern int do_clearaff(struct char_data *ch, char *argument, int cmd);
    do_clearaff(ch,"",9);
    ch->affected_by[0] = ch->affected_by[1] = 0;
  }
  
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
  uint16 length;  // do not change this type
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
    fread(&obj->obj_flags.eq_level, sizeof(obj->obj_flags.eq_level), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("VA0", mod_type))
  {
    fread(&obj->obj_flags.value[0], sizeof(obj->obj_flags.value[0]), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("VA1", mod_type))
  {
    fread(&obj->obj_flags.value[1], sizeof(obj->obj_flags.value[1]), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("VA2", mod_type))
  {
    fread(&obj->obj_flags.value[2], sizeof(obj->obj_flags.value[2]), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("VA3", mod_type))
  {
    fread(&obj->obj_flags.value[3], sizeof(obj->obj_flags.value[3]), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("EXF", mod_type))
  {
    fread(&obj->obj_flags.extra_flags, sizeof(obj->obj_flags.extra_flags), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("MOF", mod_type))
  {
    fread(&obj->obj_flags.more_flags, sizeof(obj->obj_flags.more_flags), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("TYF", mod_type))
  {
    fread(&obj->obj_flags.type_flag, sizeof(obj->obj_flags.type_flag), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("WEA", mod_type))
  {
    fread(&obj->obj_flags.wear_flags, sizeof(obj->obj_flags.wear_flags), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("SZE", mod_type))
  {
    fread(&obj->obj_flags.size, sizeof(obj->obj_flags.size), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("WEI", mod_type))
  {
    fread(&obj->obj_flags.weight, sizeof(obj->obj_flags.weight), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("AFF", mod_type))
  {
    fread(&obj->num_affects, sizeof(obj->num_affects), 1, fpsave);
    if(obj->affected)
      dc_free(obj->affected);

#ifdef LEAK_CHECK
    obj->affected = (obj_affected_type *) calloc(obj->num_affects, sizeof(obj_affected_type));
#else
    obj->affected = (obj_affected_type *) dc_alloc(obj->num_affects, sizeof(obj_affected_type));
#endif

    for(j = 0; j < obj->num_affects; j++)
    {
      fread(&obj->affected[j].location, sizeof(obj->affected[j].location), 1, fpsave);
      fread(&obj->affected[j].modifier, sizeof(obj->affected[j].modifier), 1, fpsave);
    }

    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("RPR",mod_type))
  {
    struct obj_affected_type *a;
#ifdef LEAK_CHECK
    a = (obj_affected_type *) calloc(obj->num_affects+1, sizeof(obj_affected_type));
#else
    a = (obj_affected_type *) dc_alloc(obj->num_affects+1, sizeof(obj_affected_type));
#endif
    int i;
    for (i = 0; i < obj->num_affects;i++)
    {
        a[i].location = obj->affected[i].location;
        a[i].modifier = obj->affected[i].modifier;
    }
    if(obj->affected)
      dc_free(obj->affected);
    a[i].location = APPLY_DAMAGED;
    fread(&a[i].modifier, sizeof(a[i].modifier), 1, fpsave);
    obj->affected = a;
    obj->num_affects++;
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("NAM", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->name = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("DES", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->description = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("SDE", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->short_description = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("ADE", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->action_description = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if(!strcmp("COS", mod_type))
  {
    fread(&obj->obj_flags.cost, sizeof(obj->obj_flags.cost), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }

  // TODO - put extra desc support here
  // NEW READS GO HERE

  if(nr == -1) {
    extract_obj(obj);
    return last_cont;
  }
  // Handle worn EQ
  if ( (wear_pos > -1) && (wear_pos < MAX_WEAR) && (!ch->equipment[wear_pos])
	&& CAN_WEAR(obj, wear_corr[wear_pos]))
  {
    equip_char (ch, obj, wear_pos, 1);
    return obj;
  }
  else if ( (wear_pos > -1) && (wear_pos < MAX_WEAR) && (!ch->equipment[wear_pos+1])
      && CAN_WEAR(obj, wear_corr[wear_pos+1]))
  {
     equip_char(ch, obj, wear_pos+1, 1);
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
      if(!last_cont->equipped_by && obj_index[last_cont->item_number].virt != 536)
          IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);
    }
    else {
      obj_to_char(obj, ch); // just in case
      return last_cont;
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
  obj_file_elem object;
  obj_data *standard_obj = 0;
  uint16 length = 0;  // do not change this type

  memset(&object, 0, sizeof(object));

  if (GET_ITEM_TYPE(obj) == ITEM_KEY)
    return TRUE;

  if (GET_ITEM_TYPE(obj) == ITEM_NOTE)
    return TRUE;

  if(IS_SET(obj->obj_flags.extra_flags, ITEM_NOSAVE))
    return TRUE;

  if (obj->item_number < 0)
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
/*  if(obj->obj_flags.eq_level    != standard_obj->obj_flags.eq_level) 
  {
    fwrite("EQL", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.eq_level, sizeof(obj->obj_flags.eq_level), 1, fpsave);
  }
  if(obj->obj_flags.value[0]    != standard_obj->obj_flags.value[0])
  {
    fwrite("VA0", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[0], sizeof(obj->obj_flags.value[0]), 1, fpsave);
  }*/

  if (IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM)
		  && obj->obj_flags.value[0] != standard_obj->obj_flags.value[0])
  {
    fwrite("VA0", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[0], sizeof(obj->obj_flags.value[0]), 1, fpsave);
  }

  if ((obj->obj_flags.type_flag == ITEM_CONTAINER || obj->obj_flags.type_flag == ITEM_DRINKCON || IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM))
		  && obj->obj_flags.value[1] != standard_obj->obj_flags.value[1])
  {
    fwrite("VA1", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[1], sizeof(obj->obj_flags.value[1]), 1, fpsave);
  }

  if ((obj->obj_flags.type_flag == ITEM_DRINKCON || obj->obj_flags.type_flag == ITEM_STAFF || obj->obj_flags.type_flag == ITEM_WAND || IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM))
		  && obj->obj_flags.value[2] != standard_obj->obj_flags.value[2])
  {
    fwrite("VA2", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[2], sizeof(obj->obj_flags.value[2]), 1, fpsave);
  }

  if (IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM)
		  && obj->obj_flags.value[3] != standard_obj->obj_flags.value[3])
  {
	  fwrite("VA3", sizeof(char), 3, fpsave);
	  fwrite(&obj->obj_flags.value[3], sizeof(obj->obj_flags.value[3]), 1, fpsave);
  }

  if(obj->obj_flags.extra_flags != standard_obj->obj_flags.extra_flags)
  {
    fwrite("EXF", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.extra_flags, sizeof(obj->obj_flags.extra_flags), 1, fpsave);
  }

  if (IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM)
		  && obj->obj_flags.more_flags != standard_obj->obj_flags.more_flags)
  {
    fwrite("MOF", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.more_flags, sizeof(obj->obj_flags.more_flags), 1, fpsave);   
  }


/*
  if(obj->obj_flags.more_flags != standard_obj->obj_flags.more_flags)
  {
    fwrite("MOF", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.more_flags, sizeof(obj->obj_flags.more_flags), 1, fpsave);
  }
  if(obj->obj_flags.type_flag != standard_obj->obj_flags.type_flag)
  {
    fwrite("TYF", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.type_flag, sizeof(obj->obj_flags.type_flag), 1, fpsave);
  }
  if(obj->obj_flags.wear_flags != standard_obj->obj_flags.wear_flags)
  {
    fwrite("WEA", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.wear_flags, sizeof(obj->obj_flags.wear_flags), 1, fpsave);
  }
  if(obj->obj_flags.size != standard_obj->obj_flags.size)
  {
    fwrite("SZE", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.size, sizeof(obj->obj_flags.size), 1, fpsave);
  }

  if(obj->obj_flags.weight != standard_obj->obj_flags.weight)
    {
      fwrite("WEI", sizeof(char), 3, fpsave);
      fwrite(&obj->obj_flags.weight, sizeof(obj->obj_flags.weight), 1, fpsave);
    }


  tmp_weight = obj->obj_flags.weight;
  if(GET_ITEM_TYPE(obj) == ITEM_CONTAINER && (loop_obj = obj->contains)
	&& obj_index[obj->item->number].virt != 536)
    for (; loop_obj; loop_obj = loop_obj->next_content)
      tmp_weight -= GET_OBJ_WEIGHT(loop_obj);
  if(tmp_weight      != standard_obj->obj_flags.weight)
  {
    fwrite("WEI", sizeof(char), 3, fpsave);
    fwrite(&tmp_weight, sizeof(tmp_weight), 1, fpsave);
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
  */
  // Custom objects get all of their affects copied
  if (IS_SET(obj->obj_flags.more_flags, ITEM_CUSTOM)) {
	  fwrite("AFF", sizeof(char), 3, fpsave);
	  fwrite(&obj->num_affects, sizeof(obj->num_affects), 1, fpsave);
	  for (int iAffect = 0; iAffect < obj->num_affects; iAffect++)
	  {
		fwrite(&obj->affected[iAffect].location, sizeof(obj->affected[iAffect].location), 1, fpsave);
		fwrite(&obj->affected[iAffect].modifier, sizeof(obj->affected[iAffect].modifier), 1, fpsave);
	  }
  } else { // non-custom objects only get the damaged affect copied by way of RPR
	  int i;
	  for (i = 0; i < obj->num_affects; i++)
	  {
		if (obj->affected[i].location == APPLY_DAMAGED)
		{
		  fwrite("RPR", sizeof(char), 3, fpsave);
		  fwrite(&obj->affected[i].modifier,sizeof(obj->affected[i].modifier),1,fpsave);
		  break; // Fixed!
		}
	  }
  }

  if(strcmp(obj->name, standard_obj->name))
  {
    fwrite("NAM", sizeof(char), 3, fpsave);
    length = strlen(obj->name);
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(obj->name, sizeof(char), length, fpsave);
  }
  if(strcmp(obj->description, standard_obj->description))
  {
    fwrite("DES", sizeof(char), 3, fpsave);
    length = strlen(obj->description);
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(obj->description, sizeof(char), length, fpsave);
  }
  if(strcmp(obj->short_description, standard_obj->short_description))
  {
    fwrite("SDE", sizeof(char), 3, fpsave);
    length = strlen(obj->short_description);
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(obj->short_description, sizeof(char), length, fpsave);
  }
  if(strcmp(obj->action_description, standard_obj->action_description))
  {
    fwrite("ADE", sizeof(char), 3, fpsave);
    length = strlen(obj->action_description);
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(obj->action_description, sizeof(char), length, fpsave);
  }

  if(obj->obj_flags.cost != standard_obj->obj_flags.cost)
  {
    fwrite("COS", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.cost, sizeof(obj->obj_flags.cost), 1, fpsave);
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
    if (obj_index[obj->item_number].virt == 536) return;
    restore_weight( obj->contains );
    restore_weight( obj->next_content );
    for ( tmp = obj->in_obj; tmp; tmp = tmp->in_obj )
       GET_OBJ_WEIGHT( tmp ) += GET_OBJ_WEIGHT( obj );
}

void donothin() {}
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
    GET_RAW_KI(ch)   = st->raw_ki;

    ch->alignment    = st->alignment;
    ch->misc         = st->misc;

    GET_HP_METAS(ch)   = st->hpmetas;
    GET_MANA_METAS(ch) = st->manametas;
    GET_MOVE_METAS(ch) = st->movemetas;
    GET_AC_METAS(ch)   = st->acmetas;
    GET_AGE_METAS(ch)  = st->agemetas;

    ch->armor          = st->armor;
    ch->hitroll        = st->hitroll;
    ch->damroll        = st->damroll;
    donothin();

    ch->affected_by[0] = st->afected_by;
    ch->affected_by[1] = st->afected_by2;
   
/*    i = 0;
    while(st->afected_by[i] != -1) {
       ch->affected_by[i] = st->afected_by[i];
       i++;
    }
    st->afected_by[i] = -1;
*/
    for(i = 0; i <= 2; i++)
      GET_COND(ch, i) = st->conditions[i];

    // it's ok assigning the in_room directly since do_on_login_stuff() will
    // make the actual call to "char_to_room" using this data later
    ch->in_room = real_room(st->load_room);

    if(ch->in_room == (-1)) {
      if(GET_LEVEL(ch) >= IMMORTAL)
        ch->in_room = real_room(17);
      else
        ch->in_room = real_room(START_ROOM);
    }
}



// copy vital data from a players char-structure to the file structure 
// return 'age' of character unmodified
void char_to_store(CHAR_DATA *ch, struct char_file_u *st, struct time_data & tmpage)
{
  int i;
  int x;
  struct affected_type *af;
  struct obj_data *char_eq[MAX_WEAR];

  // Remove all the eq and store it in temp storage
  for(i=0; i<MAX_WEAR; i++) {
    if (ch->equipment[i])
      char_eq[i] = unequip_char(ch, i,1);
    else
      char_eq[i] = 0;
  }

  // Unaffect everything a character can be affected by spell-wise
  for(af = ch->affected; af; af = af->next) 
  {
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
  for(i = 0; i < 3; i++)
    st->conditions[i] = GET_COND(ch, i);

  st->hometown = ch->hometown;

//  gets set outside
//  st->load_room = world[ch->in_room].number;

//  st->gold      = GET_GOLD(ch);
  st->gold      = 0; // Moved
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
  for(x = 0; x < 3; x++)
    st->extra_ints[x] = 0;

  if(IS_MOB(ch)) {
    st->armor = ch->armor;
    st->hitroll =  ch->hitroll;
    st->damroll =  ch->damroll;
    st->afected_by = ch->affected_by[0];
    st->afected_by2 = ch->affected_by[1];
//  x=0;
//  while(ch->afected_by[x] != -1) {
//     st->afected_by[x] = ch->affected_by[x];
//     x++;
//  }
//  st->afected_by[x] = -1;
  } else { 
    switch(GET_CLASS(ch)) {
      case CLASS_MAGE: st->armor = 150; break;
      case CLASS_DRUID: st->armor = 140; break;
      case CLASS_CLERIC: st->armor = 130; break;
      case CLASS_ANTI_PAL: st->armor = 120; break;
      case CLASS_THIEF: st->armor = 110; break;
      case CLASS_BARD: st->armor = 100; break;
      case CLASS_BARBARIAN: st->armor = 80; break;
      case CLASS_RANGER: st->armor = 60; break;
      case CLASS_PALADIN: st->armor = 40; break;
      case CLASS_WARRIOR: st->armor = 20; break;
      case CLASS_MONK: st->armor = 0; break;
      default: st->armor   = 100; break;
    }
    st->hitroll =  0;
    st->damroll =  0;
    st->afected_by = 0;
    st->afected_by2 = 0;
    st->acmetas = GET_AC_METAS(ch);
    st->agemetas = GET_AGE_METAS(ch);
    tmpage = ch->pcdata->time;
  }

  // re-affect the character with spells
  for(af = ch->affected; af; af = af->next) {
      affect_modify( ch, af->location, af->modifier, af->bitvector, TRUE);
  }

  // re-equip the character with his eq
  for(i=0; i<MAX_WEAR; i++) {
    if (char_eq[i])
      equip_char(ch, char_eq[i], i,1);
  }
}

