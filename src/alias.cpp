/************************************************************************
| $Id: alias.cpp,v 1.8 2011/11/26 03:35:36 jhhudso Exp $
| alias.C
| Description:  Commands for the alias processor.
*/
#include "DC/DC.h"

QString pet_info(CharacterPtr ch, QString type, quint32 victim_count)
{
  quint32 attacks = 1;

  if (ISSET(ch->mobdata->actflags, ACT_2ND_ATTACK))
    attacks++;
  if (ISSET(ch->mobdata->actflags, ACT_3RD_ATTACK))
    attacks++;
  if (ISSET(ch->mobdata->actflags, ACT_4TH_ATTACK))
    attacks++;

  auto bare_damage_str = u"$7$B%1$Rd$7$B%2$R"_s.arg(ch->mobdata->damnodice).arg(ch->mobdata->damsizedice);

  QString buffer = {};
  sprintbit(ch->affected_by, affected_bits, buffer);
  QString affected_by_str = QString(buffer).trimmed();

  return u"%1,%2,%3,%4,%5,%6,%7,%8, $B%9$R [$B$0%10$R] %11"_s
      .arg(ch->getLevel(), 3)
      .arg(attacks, 3)
      .arg(GET_REAL_HITROLL(ch), 3)
      .arg(GET_REAL_DAMROLL(ch), 3)
      .arg(ch->hit, 5)
      .arg(GET_ARMOR(ch), 4)
      .arg(bare_damage_str, 17)
      .arg(type, 7)
      .arg(ch->short_description())
      .arg(affected_by_str)
      .arg((victim_count ? "$B$5*$R" : ""));
}
