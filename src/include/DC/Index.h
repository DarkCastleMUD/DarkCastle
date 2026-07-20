#pragma once
#include <stdint.h>
#include <QSet>
#include <QList>
#include <QString>
#include <DC/Command.h>
#include "DC/dcstdio.h"

typedef quint64 vnum_t;
QString read_next_filename(FILEPtr stream, QString directoryName);

class world_file_list_item
{
public:
  QString filename{};
  vnum_t firstnum{};
  vnum_t lastnum{};
  QMap<vnum_t, bool> vnums_{};
  int32_t flags{};
};
using world_file_list_itemPtr = QSharedPointer<class world_file_list_item>;
using world_file_list_t = QList<world_file_list_itemPtr>;
world_file_list_itemPtr new_w_file_item(QString filename, world_file_list_t &list);
class index_data
{
public:
  void vnum(vnum_t v) { vnum_ = v; }
  [[nodiscard]] vnum_t vnum(void) const { return vnum_; }
  quint64 qty{};                                                                                 /* number of existing units of ths mob/obj */
  int (*non_combat_func)(class Character *, class Object *, cmd_t, const char *, Character *){}; // non Combat special proc
  int (*combat_func)(Character *, class Object *, cmd_t, const char *, Character *){};           // combat special proc
  class mob_prog_data *mobprogs{};
  class mob_prog_data *mobspec{};
  int progtypes{};
  world_file_list_itemPtr source;

private:
  vnum_t vnum_{};
};

class obj_index_data : public index_data
{
public:
  static QString read_next_file(FILEPtr stream);
  static world_file_list_itemPtr new_file_item(QString filename);
  static const QString indexFilename;
  Object *item{};
};

class mob_index_data : public index_data
{
public:
  static QString read_next_file(FILEPtr stream);
  static world_file_list_itemPtr new_file_item(QString filename);
  static const QString indexFilename;
  Character *mob{};
};