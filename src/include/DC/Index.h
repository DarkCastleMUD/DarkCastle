#pragma once
#include <stdint.h>
#include <qnumeric.h>
#include <DC/Command.h>

typedef uint64_t vnum_t;

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

private:
  vnum_t vnum_{};
};

class obj_index_data : public index_data
{
public:
  Object *item{};
};

class mob_index_data : public index_data
{
public:
  Character *mob{};
};