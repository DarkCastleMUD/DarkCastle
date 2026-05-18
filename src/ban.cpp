#include "DC/DC.h"

void Bans::load(void)
{
  QFile file(BANNED_FILE);

  if (!file.open(QIODeviceBase::Text))
  {
    qWarning() << "Unable to open" << BANNED_FILE;
    return;
  }

  QTextStream out(&file);
  QString name, site_name;
  Ban::type_t ban_type;
  qint32 i = {};
  qint64 date = {};
  list_.clear();
  while (out >> ban_type >> site_name >> date >> name)
  {
    Ban ban;
    ban.name(name);
    ban.site(site_name);
    auto datetime = QDateTime::fromSecsSinceEpoch(date);
    ban.date(datetime);
    switch (ban_type)
    {
    case Ban::type_t::NOT:
    case Ban::type_t::NEW:
    case Ban::type_t::SELECT:
    case Ban::type_t::ALL:
      ban.type(ban_type);
      break;
    }

    add(ban);
  }
}

void Bans::clear(void)
{
  list_.clear();
}

void Bans::add(Ban ban)
{
  if (!ban.site().isNull())
    list_[ban.site()] = ban;
}

void DC::free_ban_list_from_memory(void)
{
  bans_.clear();
}

Ban::type_t Bans::is_banned(QString site) const
{
  return list_.value(site).type();
}

void Ban::save(QTextStream &out) const
{
  out << ban_types.value(qsizetype(type())) << site() << date().toString() << name();
}

void Bans::save(void) const
{
  QFile file(BANNED_FILE);

  if (!file.open(QIODeviceBase::Text | QIODeviceBase::WriteOnly))
  {
    qWarning() << "Unable to open" << BANNED_FILE;
    return;
  }

  QTextStream out(&file);
  for (auto &entry : list_)
  {
    entry.save(out);
  }
}
