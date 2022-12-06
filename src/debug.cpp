#include <string.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <cmath>
#include <filesystem>
#include <queue>
#include <cassert>

#include "spells.h"
#include "connect.h"
#include "utility.h"
#include "db.h"
#include "obj.h"
#include "const.h"
#include "utility.h"
#include "vault.h"
#include "Leaderboard.h"
#include "interp.h"
#include "DC.h"

using namespace std;

void load_char_obj_error(FILE *fpsave, char strsave[MAX_INPUT_LENGTH]);
void store_to_char(struct char_file_u4 *st, char_data *ch);
int store_to_char_variable_data(char_data *ch, FILE *fpsave);
struct obj_data *my_obj_store_to_char(char_data *ch, FILE *fpsave, struct obj_data *last_cont);
int read_pc_or_mob_data(char_data *ch, FILE *fpsave);
void init_random();
void load_vaults();

extern struct index_data *obj_index;
extern struct vault_data *vault_table;
extern Leaderboard leaderboard;

CVoteData *DCVote;
bool verbose_mode = FALSE;

void test_handle_ansi(string test)
{
  cout << "Testing '" << test << "'" << endl;
  char_data *ch = new char_data;
  ch->pcdata = new pc_data;
  SET_BIT(ch->pcdata->toggles, PLR_ANSI);
  GET_LEVEL(ch) = 1;

  // string str1 = "$b$B$1test$R $ $$ $$$ $$$";
  string str1 = test;
  char *str2 = new char[1024];
  memset(str2, 1024, 0);
  strncpy(str2, str1.c_str(), 1024);
  string result1 = handle_ansi(str1, ch);
  string result2 = string(handle_ansi_(str2, ch));
  cout << "Result1: [" << result1 << "]" << endl;
  cout << "Result2: [" << result2 << "]" << endl;
  assert(handle_ansi(str1, ch) == string(handle_ansi_(str2, ch)));
  delete str2;
}

bool test_rolls(uint8_t total)
{
  int x, a, b;
  stat_data stats;

  init_random();

  uint64_t attempts = 0;
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
  // printf("%d\n", result);
  for (auto &cur : results)
  {
    cout << cur.first << "=" << cur.second << endl;
  }
}

void showObjectAffects(obj_data *obj)
{
  for (int i = 0; i < obj->num_affects; ++i)
  {
    if (i > 0)
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

void showObjectVault(const char *owner, obj_data *obj)
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

void showObject(char_data *ch, obj_data *obj)
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
  // Tests only
  if (argc < 2)
  {
    test_handle_ansi("");
    test_handle_ansi("$");
    test_handle_ansi("$$");
    test_handle_ansi("$$$");
    test_handle_ansi("$ $$ $$$");
    test_handle_ansi("$$$$$$$$");
    test_handle_ansi("$ ");
    test_handle_ansi("$x");
    test_handle_ansi("$1$2$5$B$b$rttessd$Rddd");

    string arg1 = {}, remainder = "charm sleep ";
    do
    {
      tie(arg1, remainder) = half_chop(remainder);
      cerr << "[" << arg1 << "]"
           << "[" << remainder << "]" << endl;
    } while (arg1.empty() == false);

    char c_arg1[2048] = {}, c_arg2[2048] = {}, c_input[] = "charm sleep ";
    do
    {
      half_chop(c_input, c_arg1, c_arg2);
      cerr << "[" << c_arg1 << "]"
           << "[" << c_arg2 << "]" << endl;
      strncpy(c_input, c_arg2, sizeof(c_input) - 1);
    } while (c_arg1[0] != '\0');

    cerr << sizeof(char_file_u) << " " << sizeof(char_file_u4) << endl;

    exit(0);
  }

  string orig_cwd, dclib;
  if (getenv("DCLIB"))
  {
    dclib = string(getenv("DCLIB"));
    if (!dclib.empty())
    {
      orig_cwd = getcwd(NULL, 0);
      chdir(dclib.c_str());
    }
  }

  log("Loading the zones", 0, LogChannels::LOG_MISC);
  boot_zones();

  log("Loading the world.", 0, LogChannels::LOG_MISC);
  world_array = (room_data **)realloc(world_array, 2000 * sizeof(room_data *));
  extern int top_of_world_alloc;
  top_of_world_alloc = 2000;
  // clear it (realloc = malloc, not calloc)
  for (int counter = 0; counter < top_of_world_alloc; counter++)
    world_array[counter] = 0;

  boot_world();

  log("Renumbering the world.", 0, LogChannels::LOG_MISC);
  renum_world();

  log("Generating object indices/loading all objects", 0, LogChannels::LOG_MISC);
  generate_obj_indices(&top_of_objt, obj_index);

  log("Generating mob indices/loading all mobiles", 0, LogChannels::LOG_MISC);
  generate_mob_indices(&top_of_mobt, mob_index);

  log("renumbering zone table", 0, LogChannels::LOG_MISC);
  renum_zone_table();

  struct descriptor_data *d = new descriptor_data;

  /* Create 1 blank obj to be used when playerfile loads */
  create_blank_item(1);

  load_vaults();

  chdir(orig_cwd.c_str());

  cerr << real_mobile(0) << " " << real_mobile(1) << endl;

  int vnum = 0;
  if (argc >= 3)
  {
    vnum = atoi(argv[2]);
  }

  d = new descriptor_data;
  char_data *ch = new char_data;
  ch->name = strdup("DebugIMP");
  ch->pcdata = new pc_data;

  ch->desc = d;
  ch->level = 110;
  d->descriptor = 1;
  d->character = ch;
  d->output = {};
  do_stand(ch, "", CMD_DEFAULT);
  char_to_room(ch, 3001);
  do_toggle(ch, "pager", CMD_DEFAULT);
  do_toggle(ch, "ansi", CMD_DEFAULT);
  // do_toggle(ch, "", CMD_DEFAULT);
  //  do_goto(ch, "23", CMD_DEFAULT);
  do_look(ch, "", CMD_LOOK);
  process_output(d);

  if (argv[1] == string("all") || argv[1] == string("leaderboard"))
  {
    obj_data *obj = nullptr;
    string savepath = dclib + "../save/";
    for (const auto &entry : filesystem::directory_iterator(savepath))
    {
      if (entry.is_directory() && entry.path() != "../save/qdata" && entry.path() != "../save/deleted")
      {
        for (const auto &pfile : filesystem::directory_iterator(entry.path().c_str()))
        {
          try
          {
            string path = pfile.path().string();
            path.erase(0, path.find_last_of('/') + 1);
            if (path.empty() == false && path[0] != '.')
            {
              cerr << pfile.path().c_str() << endl;
              do_linkload(ch, path.data(), CMD_DEFAULT);
              process_output(d);
              do_fsave(ch, path, CMD_DEFAULT);
              process_output(d);
            }
            else
            {
              continue;
            }

            if (argv[1] == string("all"))
            {
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
          }
          catch (...)
          {
          }
        }
      }
    }

    if (argv[1] == string("leaderboard"))
    {
      do_leaderboard(ch, "scan", CMD_DEFAULT);
      process_output(d);
      do_leaderboard(ch, "", CMD_DEFAULT);
      process_output(d);
    }
    /*
        multimap<int32_t, string> hp_leaders;
        for (auto& ch : DC::getInstance()->character_list)
        {
          if (IS_PC(ch))
          {
            hp_leaders.insert(pair<int32_t,string>(ch->max_hit, ch->name));
          }
        }

        queue<pair<int32_t,string>> top_hp_leaders;
        for (auto& l : hp_leaders)
        {
          //cout << l.first << " " << l.second << endl;
          top_hp_leaders.push(l);
          if (top_hp_leaders.size() > 5)
          {
            top_hp_leaders.pop();
          }
        }

        unsigned int placement = 0;
        while (top_hp_leaders.size() > 0)
        {
          cout << top_hp_leaders.front().first << " " << top_hp_leaders.front().second << endl;
          leaderboard.setHP(placement++, top_hp_leaders.front().second, top_hp_leaders.front().first);
          top_hp_leaders.pop();
        }
    */

    // leaderboard.check_offline();
    // cout << DC::getInstance()->character_list.size() << endl;
    // do_leaderboard(ch, "", CMD_DEFAULT);
    // process_output(d);

    struct vault_data *vault;

    for (vault = vault_table; vault; vault = vault->next)
    {
      for (vault_items_data *items = vault->items; items; items = items->next)
      {
        obj_data *obj = items->obj ? items->obj : get_obj(items->item_vnum);
        if (vnum > 0 && obj_index[obj->item_number].virt == vnum)
        {
          showObjectVault(vault->owner, obj);
        }
      }
    }
    do_look(ch, "", CMD_LOOK);
    process_output(d);
    do_force(ch, "all save", CMD_FORCE);
    process_output(d);
  }
  else
  {
    try
    {
      obj_data *obj;
      if (load_char_obj(d, argv[1]) == false)
      {
        cerr << "Unable to load " << argv[1] << endl;
        exit(1);
      }
      else
      {
        do_save(d->character, "", CMD_DEFAULT);
      }

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
