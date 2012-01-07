#include <iostream>
#include <string.h>
#include "spells.h"
#include "connect.h"
#include "utility.h"
#include "db.h"
#include "obj.h"
#include "const.h"


using namespace std;
void load_char_obj_error(FILE * fpsave, char strsave[MAX_INPUT_LENGTH]);
void store_to_char(struct char_file_u *st, CHAR_DATA *ch);
int store_to_char_variable_data(CHAR_DATA * ch, FILE * fpsave);
struct obj_data *  my_obj_store_to_char(CHAR_DATA *ch, FILE *fpsave, struct obj_data * last_cont );
int read_pc_or_mob_data(CHAR_DATA *ch, FILE *fpsave);

extern struct index_data *obj_index;

CVoteData *DCVote;
bool verbose_mode = FALSE;

struct obj_data *  my_obj_store_to_char(CHAR_DATA *ch, FILE *fpsave, struct obj_data * last_cont )
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
  else {
	  printf("Object %d not found. Deleting.\n\r", object.item_number);
	  obj = clone_object(1);
  }

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



bool my_load_char_obj( struct descriptor_data *d, char *name )
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

  sprintf(strsave, "%s", name);

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
    last_cont = my_obj_store_to_char( ch, fpsave, last_cont );
    if (last_cont) {
    	printf("%d: %s\n", obj_index[last_cont->item_number].virt, last_cont->short_description);
    }
  }

  if(fpsave != NULL)
    dc_fclose(fpsave);
  return TRUE;
}

int main(int argc, char **argv)
{
	string orig_cwd;
  if (argc < 2)
    return 1;

  if (getenv("DCLIB")) {
	  string dclib(getenv("DCLIB"));
	  if (!dclib.empty()) {
		  orig_cwd = getcwd(NULL, 0);
		  chdir(dclib.c_str());
	  }
  }
  //log("Generating object indices/loading all objects", 0, LOG_MISC);
  generate_obj_indices(&top_of_objt, obj_index);

  struct descriptor_data *d = new descriptor_data;
  memset(d, 0, sizeof(descriptor_data));

  /* Create 1 blank obj to be used when playerfile loads */
  create_blank_item(1);

  chdir(orig_cwd.c_str());
  my_load_char_obj(d, argv[1]);
  cout << "Gold: " << d->character->gold << " Plat: " << d->character->plat << " XP: " << d->character->exp << " HP: " << d->character->raw_hit << " hpmeta: " << d->character->hpmetas << " Con: " << int(d->character->con) << "," << int(d->character->raw_con) << "," << int(d->character->con_bonus) << endl;

  int equip_count = 0;
  char_data *ch = d->character;
  for (int iWear = 0; iWear < MAX_WEAR; iWear++)
    {
      if (ch->equipment[iWear])
      {
        //obj_data *obj = ch->equipment[iWear];
        equip_count++;
      }
    }
  
  cout << "Equipped count: " << equip_count << endl;
  return 0;
}
