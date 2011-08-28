#include <iostream>
#include "connect.h"
#include "utility.h"
#include "db.h"

using namespace std;
void load_char_obj_error(FILE * fpsave, char strsave[MAX_INPUT_LENGTH]);
void store_to_char(struct char_file_u *st, CHAR_DATA *ch);
int store_to_char_variable_data(CHAR_DATA * ch, FILE * fpsave);
int read_pc_or_mob_data(CHAR_DATA *ch, FILE *fpsave);
struct obj_data *  obj_store_to_char(CHAR_DATA *ch, FILE *fpsave, struct obj_data * last_cont );

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
    last_cont = obj_store_to_char( ch, fpsave, last_cont );
  }

  if(fpsave != NULL)
    dc_fclose(fpsave);
  return TRUE;
}

int main(int argc, char **argv)
{
  if (argc < 2)
    return 1;

  struct descriptor_data *d = new descriptor_data;
  memset(d, 0, sizeof(descriptor_data));

  /* Create 1 blank obj to be used when playerfile loads */
  create_blank_item(1);

  my_load_char_obj(d, argv[1]);
  cout << "Gold: " << d->character->gold << " Plat: " << d->character->plat << " XP: " << d->character->exp << " HP: " << d->character->raw_hit << " hpmeta: " << d->character->hpmetas << " Con: " << d->character->con << d->character->raw_con << ", " << d->character->con_bonus << endl;
  
  return 0;
}
