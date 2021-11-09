#include <iostream>
#include <string.h>
#include <unistd.h>
#include <map>
#include <cmath>
#include <filesystem>

#include "spells.h"
#include "connect.h"
#include "utility.h"
#include "db.h"
#include "obj.h"
#include "const.h"
#include "utility.h"
#include "vault.h"

using namespace std;
void load_char_obj_error(FILE *fpsave, char strsave[MAX_INPUT_LENGTH]);
void store_to_char(struct char_file_u *st, CHAR_DATA *ch);
int store_to_char_variable_data(CHAR_DATA *ch, FILE *fpsave);
struct obj_data *my_obj_store_to_char(CHAR_DATA *ch, FILE *fpsave, struct obj_data *last_cont);
int read_pc_or_mob_data(CHAR_DATA *ch, FILE *fpsave);
void init_random();
void load_vaults();

extern struct index_data *obj_index;
extern struct vault_data *vault_table;

CVoteData *DCVote;
bool verbose_mode = FALSE;

struct obj_data *my_obj_store_to_char(CHAR_DATA *ch, FILE *fpsave, struct obj_data *last_cont)
{
  struct obj_data *obj;
  //  struct extra_descr_data *new_new_descr;
  //  struct extra_descr_data *ed, *next_ed;

  int j;
  int nr;
  uint16 length; // do not change this type
  int wear_pos;
  char mod_type[4];
  char buf[MAX_STRING_LENGTH];

  // read in the standard file data
  struct obj_file_elem object;
  fread(&object, sizeof(object), 1, fpsave);

  if (feof(fpsave))
    return NULL;

  // if it's a current object, clone it and continue
  // if it's not, then we need to remove it from the pfile so clone obj 1

  if ((nr = real_object(object.item_number)) > -1)
    obj = clone_object(nr);
  else
  {
    printf("Object %d not found. Deleting.\n\r", object.item_number);
    obj = clone_object(1);
  }

  obj->obj_flags.timer = object.timer;
  wear_pos = object.wear_pos;

  // begin sequence find any modifications to the item the person has
  // what happens, is the mods are written in a particular order to the pfile
  // so I only need to go through this once instead of looping through for each
  // one each time.  If we later decide to want to add something else, we just
  // put it at the end of the sequence and all is good.  We keep reading until
  // we hit a STP flag.  If we aren't on STP by the end of the sequence, then
  // something very bad has happened. -pir
  mod_type[3] = 0;
  fread(&mod_type, sizeof(char), 3, fpsave);

  if (!strcmp("EQL", mod_type))
  {
    fread(&obj->obj_flags.eq_level, sizeof(obj->obj_flags.eq_level), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("VA0", mod_type))
  {
    fread(&obj->obj_flags.value[0], sizeof(obj->obj_flags.value[0]), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("VA1", mod_type))
  {
    fread(&obj->obj_flags.value[1], sizeof(obj->obj_flags.value[1]), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("VA2", mod_type))
  {
    fread(&obj->obj_flags.value[2], sizeof(obj->obj_flags.value[2]), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("VA3", mod_type))
  {
    fread(&obj->obj_flags.value[3], sizeof(obj->obj_flags.value[3]), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("EXF", mod_type))
  {
    fread(&obj->obj_flags.extra_flags, sizeof(obj->obj_flags.extra_flags), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("MOF", mod_type))
  {
    fread(&obj->obj_flags.more_flags, sizeof(obj->obj_flags.more_flags), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("TYF", mod_type))
  {
    fread(&obj->obj_flags.type_flag, sizeof(obj->obj_flags.type_flag), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("WEA", mod_type))
  {
    fread(&obj->obj_flags.wear_flags, sizeof(obj->obj_flags.wear_flags), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("SZE", mod_type))
  {
    fread(&obj->obj_flags.size, sizeof(obj->obj_flags.size), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("WEI", mod_type))
  {
    fread(&obj->obj_flags.weight, sizeof(obj->obj_flags.weight), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("AFF", mod_type))
  {
    fread(&obj->num_affects, sizeof(obj->num_affects), 1, fpsave);
    if (obj->affected)
      dc_free(obj->affected);

#ifdef LEAK_CHECK
    obj->affected = (obj_affected_type *)calloc(obj->num_affects, sizeof(obj_affected_type));
#else
    obj->affected = (obj_affected_type *)dc_alloc(obj->num_affects, sizeof(obj_affected_type));
#endif

    for (j = 0; j < obj->num_affects; j++)
    {
      fread(&obj->affected[j].location, sizeof(obj->affected[j].location), 1, fpsave);
      fread(&obj->affected[j].modifier, sizeof(obj->affected[j].modifier), 1, fpsave);
    }

    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("RPR", mod_type))
  {
    struct obj_affected_type *a;
#ifdef LEAK_CHECK
    a = (obj_affected_type *)calloc(obj->num_affects + 1, sizeof(obj_affected_type));
#else
    a = (obj_affected_type *)dc_alloc(obj->num_affects + 1, sizeof(obj_affected_type));
#endif
    int i;
    for (i = 0; i < obj->num_affects; i++)
    {
      a[i].location = obj->affected[i].location;
      a[i].modifier = obj->affected[i].modifier;
    }
    if (obj->affected)
      dc_free(obj->affected);
    a[i].location = APPLY_DAMAGED;
    fread(&a[i].modifier, sizeof(a[i].modifier), 1, fpsave);
    obj->affected = a;
    obj->num_affects++;
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("NAM", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->name = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("DES", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->description = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("SDE", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->short_description = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("ADE", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(char), length, fpsave);
    buf[length] = '\0';
    obj->action_description = str_hsh(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("COS", mod_type))
  {
    fread(&obj->obj_flags.cost, sizeof(obj->obj_flags.cost), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("SAV", mod_type))
  {
    fread(&obj->save_expiration, sizeof(time_t), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }

  // TODO - put extra desc support here
  // NEW READS GO HERE

  if (nr == -1)
  {
    extract_obj(obj);

    return last_cont;
  }

  // Handle worn EQ
  if ((wear_pos > -1) && (wear_pos < MAX_WEAR) && (!ch->equipment[wear_pos]) && CAN_WEAR(obj, wear_corr[wear_pos]))
  {
    equip_char(ch, obj, wear_pos, 1);
    return obj;
  }
  else if ((wear_pos > -1) && (wear_pos < MAX_WEAR) && (!ch->equipment[wear_pos + 1]) && CAN_WEAR(obj, wear_corr[wear_pos + 1]))
  {
    equip_char(ch, obj, wear_pos + 1, 1);
    return obj;
  }
  else if (object.container_depth == 1 && last_cont)
  {
    // put the eq in a container
    // this code does not currently support containers in containers
    if (GET_ITEM_TYPE(last_cont) == ITEM_CONTAINER)
    {
      obj_to_obj(obj, last_cont);
      // we don't add weight to the character for containers that are worn
      if (!last_cont->equipped_by && obj_index[last_cont->item_number].virt != 536)
        IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);
    }
    else
    {
      obj_to_char(obj, ch); // just in case
      return last_cont;
    }
  }
  // screw it, just put it in their inventory
  else
  {
    obj_to_char(obj, ch);
    return obj;
  }

  return last_cont;
}

bool my_load_char_obj(struct descriptor_data *d, const char *name)
{
  FILE *fpsave = NULL;
  char strsave[MAX_INPUT_LENGTH];
  struct char_file_u uchar;
  struct obj_data *last_cont = NULL;
  struct char_data *ch;

  if (!name || !strcmp(name, ""))
    return FALSE;

#ifdef LEAK_CHECK
  ch = (CHAR_DATA *)calloc(1, sizeof(CHAR_DATA));
#else
  ch = (CHAR_DATA *)dc_alloc(1, sizeof(CHAR_DATA));
#endif
  auto &free_list = DC::instance().free_list;
	free_list.erase(ch);

  if (d->character)
    free_char(d->character, Trace("my_load_char_obj"));

  d->character = ch;
  clear_char(ch);
  ch->desc = d;

  sprintf(strsave, "%s", name);

  if ((fpsave = dc_fopen(strsave, "rb")) == NULL)
    return FALSE;

  if (fread(&uchar, sizeof(uchar), 1, fpsave) == 0)
  {
    load_char_obj_error(fpsave, strsave);
    return FALSE;
  }

  reset_char(ch);

  store_to_char(&uchar, ch);
  store_to_char_variable_data(ch, fpsave);
  read_pc_or_mob_data(ch, fpsave);
  if (!IS_NPC(ch) && ch->pcdata->time.logon < 1117527906)
  {
    extern int do_clearaff(struct char_data * ch, char *argument, int cmd);
    do_clearaff(ch, "", 9);
    ch->affected_by[0] = ch->affected_by[1] = 0;
  }

  // stored names only matter for mobs
  if (!IS_MOB(ch))
  {
    dc_free(GET_NAME(ch));
    GET_NAME(ch) = str_dup(name);
    logf(ANGEL, LOG_BUG, "%s's height loaded as %d", GET_NAME(ch), GET_HEIGHT(ch));
  }

  while (!feof(fpsave))
  {
    last_cont = my_obj_store_to_char(ch, fpsave, last_cont);
    if (last_cont)
    {
      //printf("%d: %s\n", obj_index[last_cont->item_number].virt, last_cont->short_description);
    }
  }

  if (fpsave != NULL)
    dc_fclose(fpsave);
  return TRUE;
}

bool test_rolls(uint8_t total)
{
  int x, a, b;
  stat_shit stats;

  init_random();

  unsigned long long attempts = 0;
  while (1)
  {
    attempts++;
    for (x = 0; x <= 4; x++)
    {
      a = dice(3, 6);
      b = dice(6, 3);
      stats.str[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      stats.dex[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      stats.con[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      stats.tel[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      stats.wis[x] = MAX(12 + number(0, 1), MAX(a, b));
      unsigned total = stats.str[x] + stats.dex[x] + stats.con[x] + stats.tel[x] + stats.wis[x];
      if (total >= 88)
      {
        cout << "Total = " << total << endl;
        cout << "Took " << attempts << " attempts." << endl;
        cout << (float)attempts / 4 / 60.0 / 60.0 << " hours" << endl;
        return 0;
      }
    }
  }
}

void test_random_stats(void)
{
  init_random();

  map<int, int> results;
  for (int i = 0; i < 10000; ++i)
  {
    int result = random_percent_change(33, 6);
    results[result]++;
  }
  //printf("%d\n", result);
  for (auto &cur : results)
  {
    cout << cur.first << "=" << cur.second << endl;
  }
}

void showObjectAffects(obj_data* obj)
{
    for (int i=0; i < obj->num_affects; ++i)
  {
    if (i>0)
    {
      cout << ", ";
    }

    char buf2[255];
    if (obj->affected[i].location < 1000)
    {
      sprinttype(obj->affected[i].location, apply_types, buf2);
    }
    else if (get_skill_name(obj->affected[i].location / 1000))
    {
      strncpy(buf2, get_skill_name(obj->affected[i].location / 1000), sizeof(buf2));
    }
    else
    {
      strncpy(buf2, "Invalid", sizeof(buf2));
    }
    buf2[254] = 0;

    cout << buf2 << " by " << obj->affected[i].modifier;
  }
}

void showObjectVault(const char* owner, obj_data* obj)
{
  cout << obj_index[obj->item_number].virt << ":";
  char buf[255];

  sprintbit(obj->obj_flags.wear_flags, wear_bits, buf);
  cout << buf << ":";

  sprintbit(obj->obj_flags.size, size_bits, buf);                     
  cout << buf << ":";

  sprintbit(obj->obj_flags.extra_flags, extra_bits, buf);
  cout << buf << ":";

  sprintbit(obj->obj_flags.more_flags, more_obj_bits, buf);
  cout << buf << ":";

  showObjectAffects(obj);

  cout << " " << obj->short_description << " in " << owner << "'s vault." << endl;
}

void showObject(char_data* ch, obj_data* obj)
{
  cout << obj_index[obj->item_number].virt << ":";
  char buf[255];

  sprintbit(obj->obj_flags.wear_flags, wear_bits, buf);
  cout << buf << ":";

  sprintbit(obj->obj_flags.size, size_bits, buf);                     
  cout << buf << ":";

  sprintbit(obj->obj_flags.extra_flags, extra_bits, buf);
  cout << buf << ":";

  sprintbit(obj->obj_flags.more_flags, more_obj_bits, buf);
  cout << buf << ":";

  showObjectAffects(obj);

  cout << " " << obj->short_description << " in " << GET_NAME(ch) << endl;
}

int main(int argc, char **argv)
{
  string orig_cwd, dclib;
  if (argc < 2)
    return 1;

  if (getenv("DCLIB"))
  {
    dclib = string(getenv("DCLIB"));
    if (!dclib.empty())
    {
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

  load_vaults();

  chdir(orig_cwd.c_str());

  int vnum = 0;
  if (argc >= 3)
  {
    vnum = atoi(argv[2]);
  }

  if (argv[1] == string("all"))
  {
    string savepath = dclib + "../save/";
    for (const auto &entry : filesystem::directory_iterator(savepath))
    {
      if (entry.is_directory() && entry.path() != "../save/qdata" && entry.path() != "../save/deleted")
      {
        for (const auto &pfile : filesystem::directory_iterator(entry.path().c_str()))
        {
          memset(d, 0, sizeof(descriptor_data));
          
          //cout << pfile.path().c_str() << endl;

          try
          {
            obj_data *obj;
            my_load_char_obj(d, pfile.path().c_str());

            char_data *ch = d->character;
            for (int iWear = 0; iWear < MAX_WEAR; iWear++)
            {
              if (ch->equipment[iWear])
              {
                obj = ch->equipment[iWear];
                if (obj)
                {
                  if (vnum > 0 && obj_index[obj->item_number].virt == vnum)
                  {
                    showObject(ch, obj);
                  }
                }

              }
            }

            for (obj_data *obj = ch->carrying; obj; obj = obj->next_content)
            {
              if (vnum == 0 || (vnum > 0 && obj_index[obj->item_number].virt == vnum))
              {
                showObject(ch, obj);
              }

              if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
              {
                for (obj_data *container = obj->contains; container; container = container->next_content)
                {
                  if (vnum > 0 && obj_index[container->item_number].virt == vnum)
                  {
                    showObject(ch, container);
                  }
                }
              }
            }
          }
          catch (...)
          {
          }
        }
      }
    }

    struct vault_data *vault;

    for (vault = vault_table;vault;vault = vault->next)
    {
      for (vault_items_data* items = vault->items;items;items = items->next)
      {
        obj_data* obj = items->obj ? items->obj : get_obj(items->item_vnum);
        if (vnum > 0 && obj_index[obj->item_number].virt == vnum)
        {
          showObjectVault(vault->owner, obj);
        }
      }
    }
  }
  else
  {
    try
    {
      obj_data *obj;
      my_load_char_obj(d, argv[1]);

      char_data *ch = d->character;
      for (int iWear = 0; iWear < MAX_WEAR; iWear++)
      {
        if (ch->equipment[iWear])
        {
          obj = ch->equipment[iWear];
          if (obj)
          {
            showObject(ch, obj);
          }
        }
      }

      for (obj_data *obj = ch->carrying; obj; obj = obj->next_content)
      {
        showObject(ch, obj);

        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
        {
          for (obj_data *container = obj->contains; container; container = container->next_content)
          {
            showObject(ch, container);
          }
        }
      }
    }
    catch (...)
    {
    }

  }

  return 0;
}
//      cout << "Gold: " << d->character->gold << " Plat: " << d->character->plat << " XP: " << d->character->exp << " HP: " << d->character->raw_hit << " hpmeta: " << d->character->hpmetas << " Con: " << int(d->character->con) << "," << int(d->character->raw_con) << "," << int(d->character->con_bonus) << endl;
//      cout << "Mana: " << d->character->mana << " MetaMana: " << d->character->manametas << endl;
