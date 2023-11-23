#include <unistd.h>

#include <iostream>
#include <map>
#include <cmath>
#include <filesystem>
#include <queue>
#include <cassert>

#include <QtSql/QSqlRelationalTableModel>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>
#include <QtSql/QSqlError>

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
void store_to_char(struct char_file_u4 *st, Character *ch);
int store_to_char_variable_data(Character *ch, FILE *fpsave);
class Object *my_obj_store_to_char(Character *ch, FILE *fpsave, class Object *last_cont);
int read_pc_or_mob_data(Character *ch, FILE *fpsave);
void load_vaults();

extern struct index_data *obj_index;
extern struct vault_data *vault_table;
extern Leaderboard leaderboard;

CVoteData *DCVote;
bool verbose_mode = false;

void test_handle_ansi(QString test)
{
  // cerr <<  "Testing '" << test << "'" << endl;
  Character *ch = new Character;
  ch->player = new Player;
  SET_BIT(ch->player->toggles, Player::PLR_ANSI);
  GET_LEVEL(ch) = 1;

  // QString str1 = "$b$B$1test$R $ $$ $$$ $$$";
  QString str1 = test;
  char *str2 = new char[1024];
  memset(str2, 1024, 0);
  strncpy(str2, str1.toStdString().c_str(), 1024);
  QString result1 = handle_ansi(str1, ch);
  QString result2 = QString(handle_ansi_(str2, ch));
  // cerr <<  "Result1: [" << result1 << "]" << endl;
  // cerr <<  "Result2: [" << result2 << "]" << endl;
  assert(handle_ansi(str1, ch) == QString(handle_ansi_(str2, ch)));
  delete[] str2;
}

bool test_rolls(uint8_t total)
{
  int x, a, b;
  stat_data stats;

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
        // cerr <<  "Total = " << total << endl;
        // cerr <<  "Took " << attempts << " attempts." << endl;
        // cerr <<  (float)attempts / 4 / 60.0 / 60.0 << " hours" << endl;
        return 0;
      }
    }
  }
}

void test_random_stats(void)
{
  map<int, int> results;
  for (int i = 0; i < 10000; ++i)
  {
    int result = random_percent_change(33, 6);
    results[result]++;
  }
  // printf("%d\n", result);
  for (const auto &cur : results)
  {
    // cerr <<  cur.first << "=" << cur.second << endl;
  }
}

QString showObjectAffects(Object *obj)
{
  QString buffer;
  for (int i = 0; i < obj->num_affects; ++i)
  {
    if (i > 0)
    {
      buffer += ", ";
    }

    if (obj->affected[i].location < 1000)
    {
      buffer += sprinttype(obj->affected[i].location, apply_types);
    }
    else if (get_skill_name(obj->affected[i].location / 1000))
    {
      buffer += get_skill_name(obj->affected[i].location / 1000);
    }
    else
    {
      buffer += "Invalid";
    }

    buffer += " by " + QString::number(obj->affected[i].modifier);
  }
  return buffer;
}

QString showObjectVault(Object *obj)
{
  // cerr <<  obj_index[obj->item_number].virt << ":";
  QString buffer = sprintbit(obj->obj_flags.wear_flags, Object::wear_bits);
  // cerr <<  buf << ":";

  buffer += sprintbit(obj->obj_flags.size, Object::size_bits);
  // cerr <<  buf << ":";

  buffer += sprintbit(obj->obj_flags.extra_flags, Object::extra_bits);
  // cerr <<  buf << ":";

  buffer += sprintbit(obj->obj_flags.more_flags, Object::more_obj_bits);
  // cerr <<  buf << ":";

  buffer += showObjectAffects(obj);

  // cerr <<  " " << obj->short_description << " in " << owner << "'s vault." << endl;
  return buffer;
}

void showObject(Character *ch, Object *obj)
{
  // cerr <<  obj_index[obj->item_number].virt << ":";
  char buf[255];

  sprintbit(obj->obj_flags.wear_flags, Object::wear_bits, buf);
  // cerr <<  buf << ":";

  sprintbit(obj->obj_flags.size, Object::size_bits, buf);
  // cerr <<  buf << ":";

  sprintbit(obj->obj_flags.extra_flags, Object::extra_bits, buf);
  // cerr <<  buf << ":";

  sprintbit(obj->obj_flags.more_flags, Object::more_obj_bits, buf);
  // cerr <<  buf << ":";

  showObjectAffects(obj);

  // cerr <<  " " << obj->short_description << " in " << GET_NAME(ch) << endl;
}

void testStrings(void)
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

  char c_arg1[2048] = {}, c_arg2[2048] = {}, c_input[] = "charm sleep ";
  QString arg1 = {}, remainder = "charm sleep ";
  do
  {
    tie(arg1, remainder) = half_chop(remainder);

    half_chop(c_input, c_arg1, c_arg2);
    strncpy(c_input, c_arg2, sizeof(c_input) - 1);

    cerr << "[" << arg1.toStdString() << "]"
         << "[" << remainder.toStdString() << "]" << endl;
    cerr << "[" << c_arg1 << "]"
         << "[" << c_arg2 << "]" << endl;
    assert(arg1 == c_arg1);
  } while (!arg1.isEmpty() && c_arg1[0] != '\0');

  cerr << sizeof(char_file_u) << " " << sizeof(char_file_u4) << endl;
}

int main(int argc, char **argv)
{
  DC debug(argc, argv);

  testStrings();

  QString orig_cwd, dclib;
  if (getenv("DCLIB"))
  {
    dclib = QString(getenv("DCLIB"));
    if (!dclib.isEmpty())
    {
      orig_cwd = getcwd(nullptr, 0);
      chdir(dclib.toStdString().c_str());
    }
  }

  logentry("Loading the zones", 0, LogChannels::LOG_MISC);
  DC::getInstance()->boot_zones();

  logentry("Loading the world.", 0, LogChannels::LOG_MISC);
  extern room_t top_of_world_alloc;
  top_of_world_alloc = 2000;

  DC::getInstance()->boot_world();

  logentry("Renumbering the world.", 0, LogChannels::LOG_MISC);
  renum_world();

  logentry("Generating object indices/loading all objects", 0, LogChannels::LOG_MISC);
  generate_obj_indices(&top_of_objt, obj_index);

  logentry("Generating mob indices/loading all mobiles", 0, LogChannels::LOG_MISC);
  generate_mob_indices(&top_of_mobt, mob_index);

  logentry("renumbering zone table", 0, LogChannels::LOG_MISC);
  renum_zone_table();

  class Connection *d = new Connection;

  /* Create 1 blank obj to be used when playerfile loads */
  create_blank_item(1);

  load_vaults();

  chdir(orig_cwd.toStdString().c_str());

  // cerr << real_mobile(0) << " " << real_mobile(1) << endl;

  int vnum = 0;
  if (argc >= 3)
  {
    vnum = atoi(argv[2]);
  }

  d = new Connection;
  Character *ch = new Character;
  ch->name = strdup("DebugIMP");
  ch->player = new Player;

  ch->desc = d;
  ch->level = 110;
  d->descriptor = 1;
  d->character = ch;
  d->output = {};
  do_stand(ch, "", CMD_DEFAULT);
  char_to_room(ch, 3001);
  ch->do_toggle({"pager"}, CMD_DEFAULT);
  ch->do_toggle({"ansi"}, CMD_DEFAULT);
  ch->do_toggle({}, CMD_DEFAULT);
  // do_goto(ch, "23", CMD_DEFAULT);
  do_look(ch, "", CMD_LOOK);
  process_output(d);

  qDebug("\n");

  qsizetype size_bits = 8 * sizeof(ch->player->toggles);
  const char *data = reinterpret_cast<const char *>(&ch->player->toggles);
  QBitArray ba;
  if (data)
  {
    ba = QBitArray::fromBits(data, size_bits);
  }

  qDebug() << ch->player->toggles;
  const uint32_t *nr = reinterpret_cast<const uint32_t *>(ba.bits());
  qDebug() << *nr;
  qDebug() << ba;

  QSqlDatabase db = QSqlDatabase::database();
  if (!db.isValid())
  {
    db.close();
    db = QSqlDatabase::addDatabase("QPSQL");
    // db.setHostName("localhost");
    // db.setDatabaseName("dcastle");
    // db.setUserName("dcastle");
    db.open();
  }

  if (!db.isValid())
  {
    qDebug("Database error");
    exit(1);
  }

  QSqlTableModel model{&debug, db};
  model.setEditStrategy(QSqlTableModel::EditStrategy::OnFieldChange);
  model.setTable("player_configurations");
  model.select();
  int row = model.rowCount();
  qDebug() << "Rows: " << row;
  model.insertRows(row, 1);
  qDebug() << QVariant(ba);
  int field_index = model.fieldIndex("testbit");
  bool success = model.setData(model.index(row, field_index), ba.toUInt32(QSysInfo::ByteOrder));
  if (!success)
  {
    qDebug("Failed to setData\n");
    qDebug() << model.lastError();
    exit(1);
  }
  if (!model.submitAll())
  {
    qDebug("Failed to submitAll");
    qDebug() << model.lastError();
    exit(1);
  }

  if (model.rowCount() > 0)
  {
    for (int row = 0; row < model.rowCount(); row++)
    {
      QSqlRecord rec = model.record(row);
      qDebug() << rec.field("testbit");
    }
  }

  if (argc > 1 && (QString(argv[1]) == "all" || QString(argv[1]) == "leaderboard"))
  {
    Object *obj = nullptr;
    QString savepath = dclib + "../save/";
    for (const auto &entry : filesystem::directory_iterator(savepath.toStdString()))
    {
      if (entry.is_directory() && entry.path() != "../save/qdata" && entry.path() != "../save/deleted")
      {
        for (const auto &pfile : filesystem::directory_iterator(entry.path().c_str()))
        {
          try
          {
            QString path = pfile.path().string().c_str();
            path.remove(0, path.lastIndexOf('/') + 1);
            if (path.isEmpty() == false && path[0] != '.')
            {
              // cerr << pfile.path().c_str() << endl;
              ch->do_linkload(path.split(' '), CMD_DEFAULT);
              process_output(d);
              do_fsave(ch, path.toStdString().c_str(), CMD_DEFAULT);
              process_output(d);
            }
            else
            {
              continue;
            }

            if (argv[1] == QString("all"))
            {
              Character *ch = d->character;
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

              for (Object *obj = ch->carrying; obj; obj = obj->next_content)
              {
                if (vnum == 0 || (vnum > 0 && obj_index[obj->item_number].virt == vnum))
                {
                  showObject(ch, obj);
                }

                if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
                {
                  for (Object *container = obj->contains; container; container = container->next_content)
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

    if (argv[1] == QString("leaderboard"))
    {
      do_leaderboard(ch, "scan", CMD_DEFAULT);
      process_output(d);
      do_leaderboard(ch, "", CMD_DEFAULT);
      process_output(d);
    }
    /*
        multimap<int32_t, QString> hp_leaders;
        for (auto& ch : DC::getInstance()->character_list)
        {
          if (IS_PC(ch))
          {
            hp_leaders.insert(pair<int32_t,QString>(ch->max_hit, ch->name));
          }
        }

        queue<pair<int32_t,QString>> top_hp_leaders;
        for (auto& l : hp_leaders)
        {
          //// cerr <<  l.first << " " << l.second << endl;
          top_hp_leaders.push(l);
          if (top_hp_leaders.size() > 5)
          {
            top_hp_leaders.pop();
          }
        }

        unsigned int placement = 0;
        while (top_hp_leaders.size() > 0)
        {
          // cerr <<  top_hp_leaders.front().first << " " << top_hp_leaders.front().second << endl;
          leaderboard.setHP(placement++, top_hp_leaders.front().second, top_hp_leaders.front().first);
          top_hp_leaders.pop();
        }
    */

    // leaderboard.check_offline();
    // // cerr <<  DC::getInstance()->character_list.size() << endl;
    // do_leaderboard(ch, "", CMD_DEFAULT);
    // process_output(d);

    struct vault_data *vault;

    for (vault = vault_table; vault; vault = vault->next)
    {
      for (vault_items_data *items = vault->items; items; items = items->next)
      {
        Object *obj = items->obj ? items->obj : get_obj(items->item_vnum);
        if (vnum > 0 && obj_index[obj->item_number].virt == vnum)
        {
          ch->send(showObjectVault(obj));
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
      Object *obj;
      if (load_char_obj(d, argv[1]) == false)
      {
        // cerr << "Unable to load " << argv[1] << endl;
        exit(1);
      }
      else
      {
        d->character->save();
      }

      Character *ch = d->character;
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

      for (Object *obj = ch->carrying; obj; obj = obj->next_content)
      {
        showObject(ch, obj);

        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
        {
          for (Object *container = obj->contains; container; container = container->next_content)
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
//      // cerr <<  "Gold: " << d->character->gold << " Plat: " << d->character->plat << " XP: " << d->character->exp << " HP: " << d->character->raw_hit << " hpmeta: " << d->character->hpmetas << " Con: " << int(d->character->con) << "," << int(d->character->raw_con) << "," << int(d->character->con_bonus) << endl;
//      // cerr <<  "Mana: " << d->character->mana << " MetaMana: " << d->character->manametas << endl;
