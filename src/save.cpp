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

#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cstring>

#include <fmt/format.h>
#include <memory>

#include "DC/obj.h"
#include "DC/room.h"
#include "DC/character.h"
#include "DC/mobile.h"
#include "DC/utility.h"
#include "DC/spells.h"
#include "DC/fileinfo.h" // SAVE_DIR
#include "DC/player.h"
#include "DC/levels.h"
#include "DC/db.h"
#include "DC/connect.h"
#include "DC/handler.h"
#include "DC/race.h"
#include "DC/vault.h"
#include "DC/const.h"
#include "DC/guild.h"

#ifdef USE_SQL
#include <iostream>
#include <libpq-fe.h>
#include "Backend/Database.h"

extern Database db;
#endif

class Object *obj_store_to_char(Character *ch, FILE *fpsave, class Object *last_cont);
bool put_obj_in_store(class Object *obj, Character *ch, FILE *fpsave, int wear_pos);
void restore_weight(class Object *obj);
void store_to_char(struct char_file_u4 *st, Character *ch);
QString fread_alias_string(FILE *fpsave);

// return 1 on success
// return 0 on failure
// donno where it would fail off hand though unless we ran out of HD space
// or had a failure.  I'm just not willing to code that much fault protection in
// -pir
void Player::save_char_aliases(FILE *fpsave)
{
  uint32_t tmp_size = aliases_.size();
  fwrite(&tmp_size, sizeof(tmp_size), 1, fpsave);

  // write the aliases out
  for (const auto [alias, command] : aliases_.asKeyValueRange())
  {
    // note that we save the number of characters in tmp_size
    tmp_size = alias.length();
    fwrite(&tmp_size, sizeof(tmp_size), 1, fpsave);
    // but we actually write tmp_size +1 to get the trailing \0
    fwrite(alias.toStdString().c_str(), sizeof(char), (tmp_size + 1), fpsave);

    tmp_size = command.length();
    fwrite(&tmp_size, sizeof(tmp_size), 1, fpsave);
    fwrite(command.toStdString().c_str(), sizeof(char), (tmp_size + 1), fpsave);
  }
}

// return pointer to aliases or nullptr
aliases_t read_char_aliases(FILE *fpsave)
{
  uint32_t total{};
  fread(&total, sizeof(total), 1, fpsave);

  aliases_t aliases;
  for (auto x = 0; x < total; x++)
  {
    QString keyword = fread_alias_string(fpsave);
    QString command = fread_alias_string(fpsave);
    if (keyword.isEmpty() || command.isEmpty())
    {
      logentry(QStringLiteral("Removing command alias [%1] because it's missing a keyword.").arg(command));
      continue;
    }

    aliases[keyword] = command;
  }

  return aliases;
}

QString fread_alias_string(FILE *fpsave)
{
  uint32_t tmp_size{};
  size_t read_count = fread(&tmp_size, sizeof(tmp_size), 1, fpsave);
  if (read_count != 1)
  {
    logentry(QStringLiteral("fread_alias_string: fread() read %1 bytes instead of %2 at position %3").arg(read_count).arg(sizeof(tmp_size)).arg(ftell(fpsave)));
    return QString();
  }

  if (!tmp_size)
  {
    fseek(fpsave, 1, SEEK_CUR);
    qDebug() << "fread_alias_string: tmp_size=" << tmp_size << " forwarding FILE* 1 byte.";
    return QString();
  }
  else
  {
    std::unique_ptr<char[]> buffer(new char[tmp_size + 1]);
    assert(buffer);
    fread(buffer.get(), sizeof(char), (tmp_size + 1), fpsave);
    return QString(buffer.get());
  }
  return QString();
}

void fwrite_var_string(const char *str, FILE *fpsave)
{
  uint16_t tmp_size{};

  if (str)
  {
    tmp_size = strlen(str) + 1; // count the null terminator
  }

  fwrite(&tmp_size, sizeof(tmp_size), 1, fpsave);

  if (str)
  {
    fwrite(str, sizeof(char), (tmp_size), fpsave);
  }
}

void fwrite_var_string(QString str, FILE *fpsave)
{
  fwrite_var_string(str.toStdString().c_str(), fpsave);
}

char *fread_var_string(FILE *fpsave)
{
  uint16_t tmp_size = 0;
  char *tmp_str = nullptr;

  size_t records_read = fread(&tmp_size, sizeof(tmp_size), 1, fpsave);
  if (tmp_size > 0 && records_read > 0)
  {
    tmp_str = (char *)dc_alloc(tmp_size, sizeof(char));
    assert(tmp_str);
    if (tmp_str == nullptr)
    {
      return nullptr;
    }

    records_read = fread(tmp_str, sizeof(char), tmp_size, fpsave);
    if (records_read > 0)
    {
      return tmp_str;
    }
    dc_free(tmp_str);
  }

  return nullptr;
}

void Mobile::save(FILE *fpsave)
{
  fwrite(&(nr), sizeof(nr), 1, fpsave);
  fwrite(&(default_pos), sizeof(default_pos), 1, fpsave);
  fwrite(&(attack_type), sizeof(attack_type), 1, fpsave);
  fwrite(&(actflags), sizeof(actflags), 1, fpsave);
  fwrite(&(damnodice), sizeof(damnodice), 1, fpsave);
  fwrite(&(damsizedice), sizeof(damsizedice), 1, fpsave);

  // Any future additions to this save file will need to be placed LAST here with a 3 letter code
  // and appropriate strcmp statement in the read_mob_data object

  fwrite("STP", sizeof(char), 3, fpsave);
}

void Mobile::read(FILE *fpsave)
{
  fread(&(nr), sizeof(nr), 1, fpsave);
  fread(&(default_pos), sizeof(default_pos), 1, fpsave);
  fread(&(attack_type), sizeof(attack_type), 1, fpsave);
  fread(&(actflags), sizeof(actflags), 1, fpsave);
  fread(&(damnodice), sizeof(damnodice), 1, fpsave);
  fread(&(damsizedice), sizeof(damsizedice), 1, fpsave);

  char typeflag[4] = {};
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
  strcpy(buf, "Bugfixbugfixbugfixbugfixbugfixbugfix~");
  fwrite(&buf, 37, 1, fpsave);
}
void Player::save(FILE *fpsave, struct time_data tmpage)
{
  fwrite(pwd, sizeof(char), PASSWORD_LEN + 1, fpsave);
  save_char_aliases(fpsave);

  fwrite_string_tilde(fpsave);
  fwrite(&(rdeaths), sizeof(rdeaths), 1, fpsave);
  fwrite(&(pdeaths), sizeof(pdeaths), 1, fpsave);
  fwrite(&(pkills), sizeof(pkills), 1, fpsave);
  fwrite(&(pklvl), sizeof(pklvl), 1, fpsave);
  // we save tmpage cause it was calculated when all eq was off
  fwrite(&(tmpage), sizeof(time_data), 1, fpsave);
  fwrite(&(bad_pw_tries), sizeof(bad_pw_tries), 1, fpsave);
  fwrite(&(practices), sizeof(practices), 1, fpsave);
  fwrite(&(bank), sizeof(bank), 1, fpsave);
  fwrite(&(toggles), sizeof(toggles), 1, fpsave);
  fwrite(&(punish), sizeof(punish), 1, fpsave);
  fwrite_var_string(last_site, fpsave);
  fwrite_var_string(poofin, fpsave);
  fwrite_var_string(poofout, fpsave);
  fwrite_var_string(prompt, fpsave);
  fwrite_var_string("NewSaveType", fpsave);

  // Quest bitvector one
  if (quest_bv1)
  {
    fwrite("QS1", sizeof(char), 3, fpsave);
    fwrite(&(quest_bv1), sizeof(quest_bv1), 1, fpsave);
  }

  // Saving throw mods
  fwrite("SVM", sizeof(char), 3, fpsave);
  fwrite(&(saves_mods), sizeof(saves_mods[0]), SAVE_TYPE_MAX + 1, fpsave); // Write the whole array

  // Specializations
  fwrite("SPC", sizeof(char), 3, fpsave);
  fwrite(&(specializations), sizeof(specializations), 1, fpsave);

  // Stat metas
  if (statmetas)
  {
    fwrite("STM", sizeof(char), 3, fpsave);
    fwrite(&(statmetas), sizeof(statmetas), 1, fpsave);
  }

  // Ki metas
  if (kimetas)
  {
    fwrite("KIM", sizeof(char), 3, fpsave);
    fwrite(&(kimetas), sizeof(kimetas), 1, fpsave);
  }
  // autojoinin'
  if (!joining.empty())
  {
    fwrite("JIN", sizeof(char), 3, fpsave);
    fwrite_var_string(getJoining(), fpsave);
  }

  fwrite("QST", sizeof(char), 3, fpsave);
  fwrite(&(quest_points), sizeof(quest_points), 1, fpsave);
  for (int j = 0; j < QUEST_CANCEL; j++)
    fwrite(&(quest_cancel[j]), sizeof(quest_cancel[j]), 1, fpsave);
  for (int j = 0; j <= QUEST_TOTAL / ASIZE; j++)
    fwrite(&(quest_complete[j]), sizeof(quest_complete[j]), 1, fpsave);
  if (buildLowVnum)
  {
    fwrite("BLO", sizeof(char), 3, fpsave);
    fwrite(&(buildLowVnum), sizeof(buildLowVnum), 1, fpsave);
  }
  if (buildHighVnum)
  {
    fwrite("BHI", sizeof(char), 3, fpsave);
    fwrite(&(buildHighVnum), sizeof(buildHighVnum), 1, fpsave);
  }
  if (buildMLowVnum)
  {
    fwrite("BMO", sizeof(char), 3, fpsave);
    fwrite(&(buildMLowVnum), sizeof(buildMLowVnum), 1, fpsave);
  }
  if (buildMHighVnum)
  {
    fwrite("BMI", sizeof(char), 3, fpsave);
    fwrite(&(buildMHighVnum), sizeof(buildMHighVnum), 1, fpsave);
  }
  if (buildOLowVnum)
  {
    fwrite("BOO", sizeof(char), 3, fpsave);
    fwrite(&(buildOLowVnum), sizeof(buildOLowVnum), 1, fpsave);
  }
  if (buildOHighVnum)
  {
    fwrite("BOI", sizeof(char), 3, fpsave);
    fwrite(&(buildOHighVnum), sizeof(buildOHighVnum), 1, fpsave);
  }
  if (profession)
  {
    fwrite("PRO", sizeof(char), 3, fpsave);
    fwrite(&(profession), sizeof(profession), 1, fpsave);
  }
  if (wizinvis)
  {
    fwrite("WIZ", sizeof(char), 3, fpsave);
    fwrite(&(wizinvis), sizeof(wizinvis), 1, fpsave);
  }
  if (config != nullptr)
  {
    for (auto setting = config->constBegin(); setting != config->constEnd(); ++setting)
    {
      if (setting.key() == "color.good" ||
          setting.key() == "color.bad" ||
          setting.key() == "tell.history.timestamp" ||
          setting.key() == "locale" ||
          setting.key() == "mode")
      {
        fwrite("OPT", sizeof(char), 3, fpsave);
        fwrite_var_string(setting.key(), fpsave);
        fwrite_var_string(setting.value(), fpsave);
      }
    }
  }
  if (ignoring.empty() == false)
  {
    for (const auto &name : ignoring)
    {
      if (name.second.ignore)
      {
        fwrite("IGN", sizeof(char), 3, fpsave);
        fwrite_var_string(name.first.c_str(), fpsave);
        fwrite(&name.second.ignored_count, sizeof(name.second.ignored_count), 1, fpsave);
      }
    }
  }

  // Any future additions to this save file will need to be placed LAST here with a 3 letter code
  // and appropriate strcmp statement in the read_mob_data object

  fwrite("STP", sizeof(char), 3, fpsave);
}

qsizetype fread_to_tilde(FILE *fpsave, QString filename)
{
  qsizetype characters_read{};
  QString buffer;
  char a{};

  while (characters_read++ < 160)
  {
    if (feof(fpsave))
    {
      qDebug(QStringLiteral("fread_to_tilde: unexpected EOF in %1").arg(filename).toStdString().c_str());
      return characters_read;
    }

    if (ferror(fpsave))
    {
      qDebug(QStringLiteral("fread_to_tilde: unexpected error in %1").arg(filename).toStdString().c_str());
      return characters_read;
    }

    long before_fread_offset = ftell(fpsave);
    size_t read_count = fread(&a, 1, 1, fpsave);
    long after_fread_offset = ftell(fpsave);
    if (read_count != 1)
    {
      qDebug(QStringLiteral("fread_to_tilde: fread returned %1 at position %2 now at position %3 in %4").arg(read_count).arg(before_fread_offset).arg(after_fread_offset).arg(filename).toStdString().c_str());
    }

    buffer += a;
    if (a == '~')
    {
      break;
    }
  }

  if (characters_read >= 160)
  {
    qDebug(QStringLiteral("fread_to_tilde: >= 160 buffer: [%1] in %2").arg(buffer).arg(filename).toStdString().c_str());
  }

  return characters_read;
}

bool Player::read(FILE *fpsave, Character *ch, QString filename)
{
  if (!ch)
  {
    return false;
  }

  char typeflag[4] = {};
  golem = 0;
  quest_points = 0;
  for (int j = 0; j < QUEST_CANCEL; j++)
    quest_cancel[j] = 0;
  for (int j = 0; j <= QUEST_TOTAL / ASIZE; j++)
    quest_complete[j] = 0;

  fread(pwd, sizeof(char), PASSWORD_LEN + 1, fpsave);
  aliases_ = read_char_aliases(fpsave);
  if (ch->has_skill(NEW_SAVE))
  {
    if (fread_to_tilde(fpsave, filename) >= 160)
    {
      buglog(QStringLiteral("read_Player: Error reading %1. fread_to_tilde >= 160. Aborting.").arg(ch->getName()));
      return false;
    }
  }
  fread(&(rdeaths), sizeof(rdeaths), 1, fpsave);
  fread(&(pdeaths), sizeof(pdeaths), 1, fpsave);
  fread(&(pkills), sizeof(pkills), 1, fpsave);
  fread(&(pklvl), sizeof(pklvl), 1, fpsave);
  fread(&(time), sizeof(time_data), 1, fpsave);
  fread(&(bad_pw_tries), sizeof(bad_pw_tries), 1, fpsave);
  fread(&(practices), sizeof(practices), 1, fpsave);
  fread(&(bank), sizeof(bank), 1, fpsave);
  fread(&(toggles), sizeof(toggles), 1, fpsave);
  fread(&(punish), sizeof(punish), 1, fpsave);
  last_site = fread_var_string(fpsave);
  poofin = fread_var_string(fpsave);
  poofout = fread_var_string(fpsave);
  prompt = fread_var_string(fpsave);

  char *tmp = fread_var_string(fpsave);
  if (!tmp || str_cmp(tmp, "NewSaveType"))
  {
    tmp = fread_var_string(fpsave);
    tmp = fread_var_string(fpsave);
  }
  skillchange = 0;

  fread(&typeflag, sizeof(char), 3, fpsave);

  if (!strcmp("QS1", typeflag))
  {
    fread(&quest_bv1, sizeof(quest_bv1), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  if (!strcmp("SVM", typeflag))
  {
    fread(&(saves_mods), sizeof(saves_mods[0]), SAVE_TYPE_MAX + 1, fpsave); // read the whole array
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  if (!strcmp("SPC", typeflag))
  {
    fread(&(specializations), sizeof(specializations), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  if (!strcmp("STM", typeflag))
  {
    fread(&statmetas, sizeof(statmetas), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  if (!strcmp("KIM", typeflag))
  {
    fread(&kimetas, sizeof(kimetas), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  joining = {};

  if (!strcmp("JIN", typeflag))
  {
    QString buffer = fread_var_string(fpsave);
    setJoining(buffer);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("QST", typeflag))
  {
    fread(&(quest_points), sizeof(quest_points), 1, fpsave);
    for (int j = 0; j < QUEST_CANCEL; j++)
      fread(&(quest_cancel[j]), sizeof(quest_cancel[j]), 1, fpsave);
    for (int j = 0; j <= QUEST_TOTAL / ASIZE; j++)
      fread(&(quest_complete[j]), sizeof(quest_complete[j]), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BLO", typeflag))
  {
    fread(&buildLowVnum, sizeof(buildLowVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BHI", typeflag))
  {
    fread(&buildHighVnum, sizeof(buildHighVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BMO", typeflag))
  {
    fread(&buildMLowVnum, sizeof(buildMLowVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BMI", typeflag))
  {
    fread(&buildMHighVnum, sizeof(buildMHighVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BOO", typeflag))
  {
    fread(&buildOLowVnum, sizeof(buildOLowVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("BOI", typeflag))
  {
    fread(&buildOHighVnum, sizeof(buildOHighVnum), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("PRO", typeflag))
  {
    fread(&profession, sizeof(profession), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp("WIZ", typeflag))
  {
    fread(&wizinvis, sizeof(wizinvis), 1, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  while (!strcmp("OPT", typeflag))
  {
    if (config == nullptr)
    {
      config = new PlayerConfig();
    }

    QString key = fread_var_string(fpsave);
    QString value = fread_var_string(fpsave);
    if (key == "color.good" || key == "color.bad" || key == "tell.history.timestamp" || key == "locale" || key == "mode")
    {
      config->insert(key, value);
    }

    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  while (!strcmp("IGN", typeflag))
  {
    char *var_string = fread_var_string(fpsave);
    if (var_string != nullptr)
    {
      std::string name = var_string;
      if (name.empty() == false)
      {
        ignore_entry ie = {true, 0};
        fread(&ie.ignored_count, sizeof(ie.ignored_count), 1, fpsave);
        ignoring[name] = ie;
      }
    }

    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  skillchange = 0;
  // Add new items in this format
  //  if(!strcmp(typeflag, "XXX"))
  //    do_something

  // Any future additions to this read file will need to be placed LAST

  // at this point, typeflag should = "STP", and we're done reading mob data
  return true;
}

bool Character::save_pc_or_mob_data(FILE *fpsave, struct time_data tmpage)
{
  if (isNPC())
  {
    mobdata->save(fpsave);
    return true;
  }
  else if (isPlayer())
  {
    player->save(fpsave, tmpage);
    return true;
  }

  buglog(QStringLiteral("save_pc_or_mob_data: %1 not an NPC and not a player.").arg(getName()));
  return false;
}

bool read_pc_or_mob_data(Character *ch, FILE *fpsave, QString filename)
{
  if (IS_MOB(ch))
  {
    ch->player = nullptr;
#ifdef LEAK_CHECK
    ch->mobdata = (Mobile *)calloc(1, sizeof(Mobile));
#else
    ch->mobdata = (Mobile *)dc_alloc(1, sizeof(Mobile));
#endif
    ch->mobdata->read(fpsave);
  }
  else
  {
    ch->mobdata = nullptr;
    ch->player = new Player;
    if (!ch->player->read(fpsave, ch, filename))
    {
      return false;
    }
  }
  return true;
}

// return 1 on success
// return 0 on failure
int store_worn_eq(Character *ch, FILE *fpsave)
{
  int wear_pos = -1;
  int iWear = 0;

  for (iWear = 0; iWear < MAX_WEAR; iWear++)
  {
    wear_pos = iWear;
    if (ch->equipment[iWear])
    {
      if (!obj_to_store(ch->equipment[iWear], ch, fpsave, wear_pos))
        return 0;
    }
  }
  return 1;
}

int Character::char_to_store_variable_data(FILE *fpsave)
{
  fwrite_var_string(this->getName(), fpsave);
  fwrite_var_string(this->short_desc, fpsave);
  fwrite_var_string(this->long_desc, fpsave);
  fwrite_var_string(this->description, fpsave);
  fwrite_var_string(this->title, fpsave);

  if (!has_skill(NEW_SAVE)) // New save.
    learn_skill(NEW_SAVE, 1, 100);

  for (const auto &skill : this->skills)
  {
    fwrite("SKL", sizeof(char), 3, fpsave);
    fwrite(&(skill.first), sizeof(skill.first), 1, fpsave);
    fwrite(&(skill.second.learned), sizeof(skill.second.learned), 1, fpsave);
    fwrite(&(skill.second.unused), sizeof(skill.second.unused[0]), 5, fpsave);
  }
  fwrite("END", sizeof(char), 3, fpsave);

  struct affected_type *af;
  int16_t aff_count = 0; // do not change from int16_t

  for (af = this->affected; af; af = af->next)
    aff_count++;

  if (aff_count)
  {
    fwrite("AFS", sizeof(char), 3, fpsave);
    fwrite(&aff_count, sizeof(aff_count), 1, fpsave);
    for (af = this->affected; af; af = af->next)
    {
      fwrite(&(af->type), sizeof(af->type), 1, fpsave);
      fwrite(&(af->duration), sizeof(af->duration), 1, fpsave);
      fwrite(&(af->modifier), sizeof(af->modifier), 1, fpsave);
      fwrite(&(af->location), sizeof(af->location), 1, fpsave);
      fwrite(&(af->bitvector), sizeof(af->bitvector), 1, fpsave);
    }
  }

  struct tempvariable *mpv;
  for (mpv = this->tempVariable; mpv; mpv = mpv->next)
  {
    if (!mpv->save)
      continue;
    fwrite("MPV", sizeof(char), 3, fpsave);
    fwrite_var_string(mpv->name, fpsave);
    fwrite_var_string(mpv->data, fpsave);
  }

  fwrite("GLD", sizeof(char), 3, fpsave);
  fwrite(&this->gold_, sizeof(this->gold_), 1, fpsave);

  // Any future additions to this save file will need to be placed LAST here with a 3 letter code
  // and appropriate strcmp statement in the read_mob_data object

  fwrite("STP", sizeof(char), 3, fpsave);

  return 1;
}

void read_skill(Character *ch, FILE *fpsave)
{
  char_skill_data curr = {};

  if (fread(&(curr.skillnum), sizeof(curr.skillnum), 1, fpsave) != 1)
  {
    logentry(QStringLiteral("Unable to read a skill from player file for %1.").arg(GET_NAME(ch)), IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  if (fread(&(curr.learned), sizeof(curr.learned), 1, fpsave) != 1)
  {
    logentry(QStringLiteral("Unable to read a skill from player file for %1.").arg(GET_NAME(ch)), IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  if (fread(&(curr.unused), sizeof(curr.unused[0]), 5, fpsave) != 5)
  {
    logentry(QStringLiteral("Unable to read a skill from player file for %1.").arg(GET_NAME(ch)), IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  //  The above line takes care of these four.  They are here for future use
  //  fread(&(curr.unused[1]), sizeof(curr.unused[1]), 1, fpsave);
  //  fread(&(curr.unused[2]), sizeof(curr.unused[2]), 1, fpsave);
  //  fread(&(curr.unused[3]), sizeof(curr.unused[3]), 1, fpsave);
  //  fread(&(curr.unused[4]), sizeof(curr.unused[4]), 1, fpsave);
  ch->skills[curr.skillnum] = curr;
}

int Character::store_to_char_variable_data(FILE *fpsave)
{
  char typeflag[4];

  this->setName(fread_var_string(fpsave));
  this->short_desc = fread_var_string(fpsave);
  this->long_desc = fread_var_string(fpsave);
  this->description = fread_var_string(fpsave);
  this->title = fread_var_string(fpsave);

  typeflag[3] = '\0';
  fread(&typeflag, sizeof(char), 3, fpsave);

  while (strcmp(typeflag, "END"))
  {
    read_skill(this, fpsave);
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  fread(&typeflag, sizeof(char), 3, fpsave);

  if (!strncmp(typeflag, "AFS", 3)) // affects
  {
    int16_t aff_count; // do not change form int16_t
    fread(&aff_count, sizeof(aff_count), 1, fpsave);
    this->affected = nullptr;
    for (int16_t i = 0; i < aff_count; i++)
    {
      affected_type *af = new (std::nothrow) affected_type;
      af->duration_type = 0;
      af->next = this->affected;
      this->affected = af;

      fread(&(af->type), sizeof(af->type), 1, fpsave);
      fread(&(af->duration), sizeof(af->duration), 1, fpsave);
      fread(&(af->modifier), sizeof(af->modifier), 1, fpsave);
      fread(&(af->location), sizeof(af->location), 1, fpsave);
      fread(&(af->bitvector), sizeof(af->bitvector), 1, fpsave);

      affect_modify(this, af->location, af->modifier, af->bitvector, true); // re-affect the char
    }
    fread(&typeflag, sizeof(char), 3, fpsave);
  }

  while (!strcmp(typeflag, "MPV"))
  { // MobProgVars6
    struct tempvariable *mpv;
#ifdef LEAK_CHECK
    mpv = (struct tempvariable *)calloc(1, sizeof(struct tempvariable));
#else
    mpv = (struct tempvariable *)dc_alloc(1, sizeof(struct tempvariable));
#endif
    mpv->name = fread_var_string(fpsave);
    mpv->data = fread_var_string(fpsave);
    mpv->save = 1;
    mpv->next = this->tempVariable;
    this->tempVariable = mpv;
    fread(&typeflag, sizeof(char), 3, fpsave);
  }
  if (!strcmp(typeflag, "GLD"))
  {
    fread(&(this->gold_), sizeof(this->gold_), 1, fpsave);
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
void save_char_obj_db(Character *ch)
{
  if (ch == 0)
    return;

  if (IS_NPC(ch) || ch->getLevel() < 2)
    return;

  // so weapons stop falling off
  SETBIT(this->affected_by, AFF_IGNORE_WEAPON_WEIGHT);

  char_file_u4 uchar;
  time_data tmpage;
  memset(&uchar, 0, sizeof(uchar));
  memset(&tmpage, 0, sizeof(tmpage));

  char_to_store(ch, &uchar, tmpage);

  // if they're in a safe room, save them there.
  // if they're a god, send 'em home
  // otherwise save them in tavern
  if (isSet(DC::getInstance()->world[this->in_room].room_flags, SAFE))
    uchar.load_room = DC::getInstance()->world[this->in_room].number;
  else
    uchar.load_room = real_room(GET_HOME(ch));

  timeval start, finish;

  gettimeofday(&start, nullptr);
  db.save(ch, &uchar);
  gettimeofday(&finish, nullptr);

  int msec = finish.tv_sec * 1000 + finish.tv_usec / 1000;
  msec -= start.tv_sec * 1000 + start.tv_usec / 1000;
  ch->send(QStringLiteral("Save took %1ms\n\r").arg(msec));

  /*
  if((fwrite(&uchar, sizeof(uchar), 1, fpsave))               &&
     (char_to_store_variable_data(ch, fpsave))                &&
     (ch->save_pc_or_mob_data(fpsave, tmpage))                &&
     (obj_to_store (this->carrying, ch, fpsave, -1))            &&
     (store_worn_eq(ch, fpsave))
    )
  {
    if(fpsave != nullptr)
      fclose(fpsave);
    sprintf(log_buf, "mv -f %s %s", strsave, name);
    system(log_buf);
  }
  else
  {
    if(fpsave != nullptr)
      fclose(fpsave);
    sprintf(log_buf, "Save_char_obj: %s", strsave);
    ch->send("WARNING: file problem. You did not save!");
    perror(log_buf);
    logentry(log_buf, ANGEL, LogChannels::LOG_BUG);
  }

  REMBIT(this->affected_by, AFF_IGNORE_WEAPON_WEIGHT);
  struct vault_data *vault;
  if ((vault = has_vault(GET_NAME(ch))))
    save_vault(vault->owner);
  */
}
#endif

// save a character and inventory.
// maybe modify it to save mobs for quest purposes too
void save_char_obj(Character *ch)
{
  char_file_u4 uchar = {};
  time_data tmpage;
  FILE *fpsave = 0;
  char strsave[MAX_INPUT_LENGTH] = {0};
  char name[200] = {0};

  memset(&tmpage, 0, sizeof(tmpage));

  if (IS_NPC(ch) || ch->getLevel() < 1)
  {
    return;
  }

  if (ch->getName().isEmpty())
  {
    ch->setName("Unknown");
  }

  // TODO - figure out a way for mob's to save...maybe <mastername>.pet ?
  if (DC::getInstance()->cf.bport)
  {
    sprintf(name, "%s/%c/%s", BSAVE_DIR, ch->getNameC()[0], ch->getNameC());
  }
  else
  {
    sprintf(name, "%s/%c/%s", SAVE_DIR, ch->getNameC()[0], ch->getNameC());
  }

  sprintf(strsave, "%s.back", name);

  if (!(fpsave = fopen(strsave, "wb")))
  {
    ch->sendln("Warning!  Did not save.  Could not open file.  Contact a god, do not logoff.");
    char log_buf[MAX_STRING_LENGTH] = {};
    sprintf(log_buf, "Could not open file in save_char_obj. '%s'", strsave);
    perror(log_buf);
    logentry(log_buf, ANGEL, LogChannels::LOG_BUG);
    return;
  }

  SETBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop falling off

  char_to_store(ch, &uchar, tmpage);

  // if they're in a safe room, save them there.
  // if they're a god, send 'em home
  // otherwise save them in tavern

  if (ch->in_room < 1)
  {
    uchar.load_room = START_ROOM;
  }
  else
  {
    if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
      uchar.load_room = DC::getInstance()->world[ch->in_room].number;
    else
      uchar.load_room = real_room(GET_HOME(ch));
  }

  if ((fwrite(&uchar, sizeof(uchar), 1, fpsave)) &&
      (ch->char_to_store_variable_data(fpsave)) &&
      (ch->save_pc_or_mob_data(fpsave, tmpage)) &&
      (obj_to_store(ch->carrying, ch, fpsave, -1)) &&
      (store_worn_eq(ch, fpsave)))
  {
    if (fpsave != nullptr)
      fclose(fpsave);

    char log_buf[MAX_STRING_LENGTH] = {};
    sprintf(log_buf, "mv -f %s %s", strsave, name);
    system(log_buf);
  }
  else
  {
    if (fpsave != nullptr)
      fclose(fpsave);
    char log_buf[MAX_STRING_LENGTH] = {};
    sprintf(log_buf, "Save_char_obj: %s", strsave);
    ch->send("WARNING: file problem. You did not save!");
    perror(log_buf);
    logentry(log_buf, ANGEL, LogChannels::LOG_BUG);
  }

  REMBIT(ch->affected_by, AFF_IGNORE_WEAPON_WEIGHT);
  struct vault_data *vault;
  if ((vault = has_vault(GET_NAME(ch))))
    save_vault(vault->owner);
}

// just error crap to avoid using "goto" like we were
void load_char_obj_error(FILE *fpsave, QString strsave)
{
  QString log_buf = QStringLiteral("Load_char_obj: %1").arg(strsave);
  perror(log_buf.toStdString().c_str());
  logentry(log_buf, ANGEL, LogChannels::LOG_BUG);
  if (fpsave != nullptr)
    fclose(fpsave);
}

// Load a char and inventory into a new_new ch structure.
load_status_t load_char_obj(class Connection *d, QString name)
{
  FILE *fpsave = nullptr;
  QString strsave;
  struct char_file_u4 uchar;
  class Object *last_cont = nullptr;
  Character *ch;

  if (name.isEmpty())
    return load_status_t::bad_input;
  name[0] = name[0].toUpper();

  ch = new Character;
  auto &free_list = DC::getInstance()->free_list;
  free_list.erase(ch);

  if (d->character)
  {
    free_char(d->character, Trace("load_char_obj"));
  }

  d->character = ch;
  clear_char(ch);
  ch->desc = d;

  if (DC::getInstance()->cf.bport)
  {
    strsave = QStringLiteral("%1/%2/%3").arg(BSAVE_DIR).arg(name[0]).arg(name);
  }
  else
  {
    strsave = QStringLiteral("%1/%2/%3").arg(SAVE_DIR).arg(name[0]).arg(name);
  }

  //  struct stat mystats;
  //  stat(strsave, &mystats);
  //  TODO - Eventually, i'm going to just slurp in the whole file
  //  then parse the memory instead of reading each item from file seperately
  //  Should be much faster and save our HD from turning itself to mush -pir

  if ((fpsave = fopen(strsave.toStdString().c_str(), "rb")) == nullptr)
    return load_status_t::missing;

  if (fread(&uchar, sizeof(uchar), 1, fpsave) == 0)
  {
    load_char_obj_error(fpsave, strsave);
    return load_status_t::error;
  }

  reset_char(ch);

  store_to_char(&uchar, ch);
  ch->store_to_char_variable_data(fpsave);
  if (!read_pc_or_mob_data(ch, fpsave, strsave))
  {
    return load_status_t::error;
  }

  if (IS_PC(ch) && ch->player->time.logon < 1117527906)
  {
    extern int do_clearaff(Character * ch, char *argument, int cmd);
    do_clearaff(ch, "", 9);
    ch->affected_by[0] = ch->affected_by[1] = 0;
  }

  // stored names only matter for mobs
  if (!IS_MOB(ch))
  {
    ch->setName(name);
  }

  while (!feof(fpsave))
  {
    last_cont = obj_store_to_char(ch, fpsave, last_cont);
  }

  if (fpsave != nullptr)
    fclose(fpsave);
  return load_status_t::success;
}

// read data from file for an item.
class Object *obj_store_to_char(Character *ch, FILE *fpsave, class Object *last_cont)
{
  class Object *obj;
  //  struct extra_descr_data *new_new_descr;
  //  struct extra_descr_data *ed, *next_ed;

  int j;
  int nr;
  uint16_t length; // do not change this type
  int wear_pos;
  char mod_type[4];
  char buf[MAX_STRING_LENGTH];

  // read in the standard file data
  struct obj_file_elem object;
  fread(&object, sizeof(object), 1, fpsave);

  if (feof(fpsave))
    return nullptr;

  // if it's a current object, clone it and continue
  // if it's not, then we need to remove it from the pfile so clone obj 1

  if ((nr = real_object(object.item_number)) > -1)
    obj = clone_object(nr);
  else
    obj = clone_object(1);

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
    obj->ActionDescription(buf);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("COS", mod_type))
  {
    fread(&obj->obj_flags.cost, sizeof(obj->obj_flags.cost), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("SAV", mod_type))
  {
    fread(&obj->save_expiration, sizeof(uint32_t), 1, fpsave);
    fread(&mod_type, sizeof(char), 3, fpsave);
  }
  if (!strcmp("SEL", mod_type))
  {
    fread(&obj->no_sell_expiration, sizeof(uint32_t), 1, fpsave);
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
  if ((wear_pos > -1) && (wear_pos < MAX_WEAR) && (!ch->equipment[wear_pos]) && CAN_WEAR(obj, Character::wear_to_item_wear[wear_pos]))
  {
    equip_char(ch, obj, wear_pos, 1);
    return obj;
  }
  else if ((wear_pos > -1) && (wear_pos < MAX_WEAR) && (!ch->equipment[wear_pos + 1]) && CAN_WEAR(obj, Character::wear_to_item_wear[wear_pos + 1]))
  {
    equip_char(ch, obj, wear_pos + 1, 1);
    return obj;
  }
  else if (object.container_depth == 1 && last_cont)
  {
    // put the eq in a container
    // this code does not currently support containers in containers
    if (ARE_CONTAINERS(last_cont))
    {
      obj_to_obj(obj, last_cont);
      // we don't add weight to the character for containers that are worn
      if (!last_cont->equipped_by && DC::getInstance()->obj_index[last_cont->item_number].virt != 536)
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
    if (wear_pos > -1 && wear_pos < MAX_WEAR)
    {
      SETBIT(ch->affected_by, AFF_ITEM_REMOVE);
    }
    return obj;
  }

  return last_cont;
}

bool obj_to_store(class Object *obj, Character *ch, FILE *fpsave, int wear_pos)
{
  // class Object *tmp;

  if (obj == nullptr)
    return true;

  // recurse down next item in list
  if (!obj_to_store(obj->next_content, ch, fpsave, -1))
    return false;

  // store myself
  if (!put_obj_in_store(obj, ch, fpsave, wear_pos))
    return false;

  // store anything IN myself.  That way they get put back in on read
  if (!obj_to_store(obj->contains, ch, fpsave, -1))
    return false;

  return true;
}

// return true on success
// return false on error
// write one object to file
bool put_obj_in_store(class Object *obj, Character *ch, FILE *fpsave, int wear_pos)
{
  obj_file_elem object;
  Object *standard_obj = 0;
  uint16_t length = 0; // do not change this type

  memset(&object, 0, sizeof(object));

  if (GET_ITEM_TYPE(obj) == ITEM_NOTE)
    return true;

  if (isSet(obj->obj_flags.extra_flags, ITEM_NOSAVE))
    return true;

  if (isSet(obj->obj_flags.more_flags, ITEM_24H_SAVE))
  {
    // First time we try to save this object we set the
    // expiration to 24 hours from this point
    if (obj->save_expiration == 0)
    {
      obj->save_expiration = time(nullptr) + (60 * 60 * 24);
    }
    else if (time(nullptr) > obj->save_expiration)
    {
      // If the object's window for saving has expired then
      // we don't save it as-if it had ITEM_NOSAVE
      return true;
    }
  }

  if (obj->item_number < 0)
    return true;

  // Set up items saved for all items
  object.version = CURRENT_OBJ_VERSION;
  object.item_number = DC::getInstance()->obj_index[obj->item_number].virt;
  object.timer = obj->obj_flags.timer;
  object.wear_pos = wear_pos;
  if (obj->in_obj) // I'm in a container
    object.container_depth = 1;
  else
    object.container_depth = 0;

  // write basic item format to file
  if (!(fwrite(&object, sizeof(object), 1, fpsave)))
    return false;

  // get a pointer to the standard version of this item
  standard_obj = ((class Object *)DC::getInstance()->obj_index[obj->item_number].item);

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

  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) && obj->obj_flags.value[0] != standard_obj->obj_flags.value[0])
  {
    fwrite("VA0", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[0], sizeof(obj->obj_flags.value[0]), 1, fpsave);
  }

  if ((obj->obj_flags.type_flag == ITEM_CONTAINER || obj->obj_flags.type_flag == ITEM_DRINKCON || isSet(obj->obj_flags.more_flags, ITEM_CUSTOM)) && obj->obj_flags.value[1] != standard_obj->obj_flags.value[1])
  {
    fwrite("VA1", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[1], sizeof(obj->obj_flags.value[1]), 1, fpsave);
  }

  if ((obj->obj_flags.type_flag == ITEM_DRINKCON || obj->obj_flags.type_flag == ITEM_STAFF || obj->obj_flags.type_flag == ITEM_WAND || isSet(obj->obj_flags.more_flags, ITEM_CUSTOM)) && obj->obj_flags.value[2] != standard_obj->obj_flags.value[2])
  {
    fwrite("VA2", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[2], sizeof(obj->obj_flags.value[2]), 1, fpsave);
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) && obj->obj_flags.value[3] != standard_obj->obj_flags.value[3])
  {
    fwrite("VA3", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.value[3], sizeof(obj->obj_flags.value[3]), 1, fpsave);
  }

  if (obj->obj_flags.extra_flags != standard_obj->obj_flags.extra_flags)
  {
    fwrite("EXF", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.extra_flags, sizeof(obj->obj_flags.extra_flags), 1, fpsave);
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) && obj->obj_flags.more_flags != standard_obj->obj_flags.more_flags)
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
    && DC::getInstance()->obj_index[obj->item->number].virt != 536)
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
  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM))
  {
    fwrite("AFF", sizeof(char), 3, fpsave);
    fwrite(&obj->num_affects, sizeof(obj->num_affects), 1, fpsave);
    for (int iAffect = 0; iAffect < obj->num_affects; iAffect++)
    {
      fwrite(&obj->affected[iAffect].location, sizeof(obj->affected[iAffect].location), 1, fpsave);
      fwrite(&obj->affected[iAffect].modifier, sizeof(obj->affected[iAffect].modifier), 1, fpsave);
    }
  }
  else
  { // non-custom objects only get the damaged affect copied by way of RPR
    int i;
    for (i = 0; i < obj->num_affects; i++)
    {
      if (obj->affected[i].location == APPLY_DAMAGED)
      {
        fwrite("RPR", sizeof(char), 3, fpsave);
        fwrite(&obj->affected[i].modifier, sizeof(obj->affected[i].modifier), 1, fpsave);
        break; // Fixed!
      }
    }
  }

  if (obj->name && strcmp(obj->name, standard_obj->name))
  {
    fwrite("NAM", sizeof(char), 3, fpsave);
    length = strlen(obj->name);
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(obj->name, sizeof(char), length, fpsave);
  }
  if (obj->description && strcmp(obj->description, standard_obj->description))
  {
    fwrite("DES", sizeof(char), 3, fpsave);
    length = strlen(obj->description);
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(obj->description, sizeof(char), length, fpsave);
  }
  if (obj->short_description && strcmp(obj->short_description, standard_obj->short_description))
  {
    fwrite("SDE", sizeof(char), 3, fpsave);
    length = strlen(obj->short_description);
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(obj->short_description, sizeof(char), length, fpsave);
  }
  if (!obj->ActionDescription().isEmpty() && obj->ActionDescription() != standard_obj->ActionDescription())
  {
    fwrite("ADE", sizeof(char), 3, fpsave);
    length = obj->ActionDescription().length();
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(obj->ActionDescription().toStdString().c_str(), sizeof(char), length, fpsave);
  }

  if (obj->obj_flags.cost != standard_obj->obj_flags.cost)
  {
    fwrite("COS", sizeof(char), 3, fpsave);
    fwrite(&obj->obj_flags.cost, sizeof(obj->obj_flags.cost), 1, fpsave);
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_24H_SAVE))
  {
    fwrite("SAV", sizeof(char), 3, fpsave);
    fwrite(&obj->save_expiration, sizeof(uint32_t), 1, fpsave);
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_24H_NO_SELL))
  {
    fwrite("SEL", sizeof(char), 3, fpsave);
    fwrite(&obj->no_sell_expiration, sizeof(uint32_t), 1, fpsave);
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

  return true;
}

/*
 * Restore container weights after a save.
 */
void restore_weight(class Object *obj)
{
  class Object *tmp;

  if (obj == nullptr)
    return;
  if (DC::getInstance()->obj_index[obj->item_number].virt == 536)
    return;
  restore_weight(obj->contains);
  restore_weight(obj->next_content);
  for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
    GET_OBJ_WEIGHT(tmp) += GET_OBJ_WEIGHT(obj);
}

void donothin() {}
// Read shared data from pfile
void store_to_char(struct char_file_u4 *st, Character *ch)
{
  int i;

  ch->clan = st->clan;

  GET_SEX(ch) = st->sex;
  GET_CLASS(ch) = st->c_class;
  GET_RACE(ch) = st->race;
  ch->setLevel(st->level);

  ch->hometown = st->hometown;
  if (ch->getLevel() < 11)
    ch->hometown = START_ROOM;

  GET_STR(ch) = GET_RAW_STR(ch) = st->raw_str;
  GET_INT(ch) = GET_RAW_INT(ch) = st->raw_intel;
  GET_WIS(ch) = GET_RAW_WIS(ch) = st->raw_wis;
  GET_DEX(ch) = GET_RAW_DEX(ch) = st->raw_dex;
  GET_CON(ch) = GET_RAW_CON(ch) = st->raw_con;

  ch->weight = st->weight;
  ch->height = st->height;
  ch->setGold(st->gold);
  ch->plat = st->plat;
  ch->exp = st->exp;
  ch->immune = st->immune;
  ch->resist = st->resist;
  ch->suscept = st->suscept;

  ch->setHP(st->hit);
  GET_RAW_HIT(ch) = st->raw_hit;
  GET_MANA(ch) = st->mana;
  GET_RAW_MANA(ch) = st->raw_mana;

  // since move and ki don't get "redone" with stat bonuses we need to set the max here
  ch->setMove(st->move);
  ch->max_move = GET_RAW_MOVE(ch) = st->raw_move;
  GET_KI(ch) = st->ki;
  GET_RAW_KI(ch) = st->raw_ki;

  ch->alignment = st->alignment;
  ch->misc = st->misc;

  GET_HP_METAS(ch) = st->hpmetas;
  GET_MANA_METAS(ch) = st->manametas;
  GET_MOVE_METAS(ch) = st->movemetas;
  GET_AC_METAS(ch) = st->acmetas;
  GET_AGE_METAS(ch) = st->agemetas;

  ch->armor = st->armor;
  ch->hitroll = st->hitroll;
  ch->damroll = st->damroll;
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
  for (i = 0; i <= 2; i++)
    GET_COND(ch, i) = st->conditions[i];

  // it's ok assigning the in_room directly since do_on_login_stuff() will
  // make the actual call to "char_to_room" using this data later
  ch->in_room = real_room(st->load_room);

  if (ch->in_room == DC::NOWHERE)
  {
    if (ch->getLevel() >= IMMORTAL)
      ch->in_room = real_room(17);
    else
      ch->in_room = real_room(START_ROOM);
  }
}

// copy vital data from a players char-structure to the file structure
// return 'age' of character unmodified
void char_to_store(Character *ch, struct char_file_u4 *st, struct time_data &tmpage)
{
  int i;
  int x;
  struct affected_type *af;
  class Object *char_eq[MAX_WEAR];

  // Remove all the eq and store it in temp storage
  for (i = 0; i < MAX_WEAR; i++)
  {
    if (ch->equipment[i])
      char_eq[i] = unequip_char(ch, i, 1);
    else
      char_eq[i] = 0;
  }

  // Unaffect everything a character can be affected by spell-wise
  for (af = ch->affected; af; af = af->next)
  {
    affect_modify(ch, af->location, af->modifier, af->bitvector, false);
  }

  st->sex = GET_SEX(ch);
  st->c_class = GET_CLASS(ch);
  st->race = GET_RACE(ch);
  st->level = ch->getLevel();

  st->raw_str = GET_RAW_STR(ch);
  st->raw_intel = GET_RAW_INT(ch);
  st->raw_wis = GET_RAW_WIS(ch);
  st->raw_dex = GET_RAW_DEX(ch);
  st->raw_con = GET_RAW_CON(ch);

  st->mana = GET_MANA(ch);
  st->raw_mana = GET_RAW_MANA(ch);
  st->hit = ch->getHP();
  st->raw_hit = GET_RAW_HIT(ch);
  st->move = GET_MOVE(ch);
  st->raw_move = GET_RAW_MOVE(ch);
  st->ki = GET_KI(ch);
  st->raw_ki = GET_RAW_KI(ch);

  st->weight = GET_WEIGHT(ch);
  st->height = GET_HEIGHT(ch);
  for (i = 0; i < 3; i++)
    st->conditions[i] = GET_COND(ch, i);

  st->hometown = ch->hometown;

  //  gets set outside
  //  st->load_room = DC::getInstance()->world[ch->in_room].number;

  //  st->gold      = ch->getGold();
  st->gold = 0; // Moved
  st->plat = GET_PLATINUM(ch);
  st->exp = GET_EXP(ch);
  st->immune = ch->immune;
  st->resist = ch->resist;
  st->suscept = ch->suscept;
  st->alignment = ch->alignment;
  st->misc = ch->misc;

  st->hpmetas = GET_HP_METAS(ch);
  st->manametas = GET_MANA_METAS(ch);
  st->movemetas = GET_MOVE_METAS(ch);
  st->clan = ch->clan;

  // make sure rest of unused are set to 0
  for (x = 0; x < 3; x++)
    st->extra_ints[x] = 0;

  if (IS_MOB(ch))
  {
    st->armor = ch->armor;
    st->hitroll = ch->hitroll;
    st->damroll = ch->damroll;
    st->afected_by = ch->affected_by[0];
    st->afected_by2 = ch->affected_by[1];
    //  x=0;
    //  while(ch->afected_by[x] != -1) {
    //     st->afected_by[x] = ch->affected_by[x];
    //     x++;
    //  }
    //  st->afected_by[x] = -1;
  }
  else
  {
    switch (GET_CLASS(ch))
    {
    case CLASS_MAGE:
      st->armor = 150;
      break;
    case CLASS_DRUID:
      st->armor = 140;
      break;
    case CLASS_CLERIC:
      st->armor = 130;
      break;
    case CLASS_ANTI_PAL:
      st->armor = 120;
      break;
    case CLASS_THIEF:
      st->armor = 110;
      break;
    case CLASS_BARD:
      st->armor = 100;
      break;
    case CLASS_BARBARIAN:
      st->armor = 80;
      break;
    case CLASS_RANGER:
      st->armor = 60;
      break;
    case CLASS_PALADIN:
      st->armor = 40;
      break;
    case CLASS_WARRIOR:
      st->armor = 20;
      break;
    case CLASS_MONK:
      st->armor = 0;
      break;
    default:
      st->armor = 100;
      break;
    }
    st->hitroll = 0;
    st->damroll = 0;
    st->afected_by = 0;
    st->afected_by2 = 0;
    st->acmetas = GET_AC_METAS(ch);
    st->agemetas = GET_AGE_METAS(ch);
    tmpage = ch->player->time;
  }

  // re-affect the character with spells
  for (af = ch->affected; af; af = af->next)
  {
    affect_modify(ch, af->location, af->modifier, af->bitvector, true);
  }

  // re-equip the character with his eq
  for (i = 0; i < MAX_WEAR; i++)
  {
    if (char_eq[i])
      equip_char(ch, char_eq[i], i, 1);
  }
}
