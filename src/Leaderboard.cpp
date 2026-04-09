/*
 * Leaderboard.cpp
 *
 *  Created on: Jul 13, 2014
 *      Author: jhhudso
 */

#include "DC/DC.h"
#include "DC/structs.h"
#include "DC/interp.h"
#include <cstring>

void Leaderboard::check(void)
{
  // check online players to the file and make sure the file is up to date
  qint32 i, j, k;

  DC::getInstance()->currentType("leaderboard");
  DC::getInstance()->currentName("NA");
  DC::getInstance()->currentVNUM(0);

  read_file();

  for (auto &d : DC::getInstance()->connections_)
  {
    if (!conn->character || conn->character->getLevel() >= IMMORTAL || conn->character->isNonPlayer())
      continue;
    if (!conn->connected == Connection::states::PLAYING)
      continue;
    if (!conn->character->player)
      continue;

    k = MIN(CLASS_DRUID - 1, GET_CLASS(conn->character) - 1);

    for (i = {}; i < 5; i++)
    {
      if (hpactivename[i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          hpactive[j] = hpactive[j + 1];
          hpactivename[j] = hpactivename[j + 1];
        }
        hpactive[4] = {};
        hpactivename[4] = " ";
      }
      if (mnactivename[i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          mnactive[j] = mnactive[j + 1];
          mnactivename[j] = mnactivename[j + 1];
        }
        mnactive[4] = {};
        mnactivename[4] = " ";
      }
      if (kiactivename[i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          kiactive[j] = kiactive[j + 1];
          kiactivename[j] = kiactivename[j + 1];
        }
        kiactive[4] = {};
        kiactivename[4] = " ";
      }
      if (pkactivename[i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          pkactive[j] = pkactive[j + 1];
          pkactivename[j] = pkactivename[j + 1];
        }
        pkactive[4] = {};
        pkactivename[4] = QStringLiteral(" ");
      }
      if (pdactivename[i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          pdactive[j] = pdactive[j + 1];
          pdactivename[j] = pdactivename[j + 1];
        }
        pdactive[4] = {};
        pdactivename[4] = QStringLiteral(" ");
      }
      if (rdactivename[i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          rdactive[j] = rdactive[j + 1];
          rdactivename[j] = rdactivename[j + 1];
        }
        rdactive[4] = {};
        rdactivename[4] = QStringLiteral(" ");
      }
      if (mvactivename[i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          mvactive[j] = mvactive[j + 1];
          mvactivename[j] = mvactivename[j + 1];
        }
        mvactive[4] = {};
        mvactivename[4] = QStringLiteral(" ");
      }
    }

    for (i = {}; i < 5; i++)
    {
      if (hpactiveclassname[k][i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          hpactiveclass[k][j] = hpactiveclass[k][j + 1];
          hpactiveclassname[k][j] = hpactiveclassname[k][j + 1];
        }
        hpactiveclass[k][4] = {};
        hpactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (mnactiveclassname[k][i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          mnactiveclass[k][j] = mnactiveclass[k][j + 1];
          mnactiveclassname[k][j] = mnactiveclassname[k][j + 1];
        }
        mnactiveclass[k][4] = {};
        mnactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (kiactiveclassname[k][i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          kiactiveclass[k][j] = kiactiveclass[k][j + 1];
          kiactiveclassname[k][j] = kiactiveclassname[k][j + 1];
        }
        kiactiveclass[k][4] = {};
        kiactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (pkactiveclassname[k][i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          pkactiveclass[k][j] = pkactiveclass[k][j + 1];
          pkactiveclassname[k][j] = pkactiveclassname[k][j + 1];
        }
        pkactiveclass[k][4] = {};
        pkactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (pdactiveclassname[k][i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          pdactiveclass[k][j] = pdactiveclass[k][j + 1];
          pdactiveclassname[k][j] = pdactiveclassname[k][j + 1];
        }
        pdactiveclass[k][4] = {};
        pdactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (rdactiveclassname[k][i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          rdactiveclass[k][j] = rdactiveclass[k][j + 1];
          rdactiveclassname[k][j] = rdactiveclassname[k][j + 1];
        }
        rdactiveclass[k][4] = {};
        rdactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (mvactiveclassname[k][i] == conn->character->name())
      {
        for (j = i; j < 4; j++)
        {
          mvactiveclass[k][j] = mvactiveclass[k][j + 1];
          mvactiveclassname[k][j] = mvactiveclassname[k][j + 1];
        }
        mvactiveclass[k][4] = {};
        mvactiveclassname[k][4] = QStringLiteral(" ");
      }
    }

    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_HIT(conn->character) > hpactive[i])
      {
        for (j = 4; j > i; j--)
        {
          hpactive[j] = hpactive[j - 1];
          hpactivename[j] = hpactivename[j - 1];
        }
        hpactive[i] = GET_MAX_HIT(conn->character);
        hpactivename[i] = conn->character->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_MANA(conn->character) > mnactive[i])
      {
        for (j = 4; j > i; j--)
        {
          mnactive[j] = mnactive[j - 1];
          mnactivename[j] = mnactivename[j - 1];
        }
        mnactive[i] = GET_MAX_MANA(conn->character);
        mnactivename[i] = conn->character->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_KI(conn->character) > kiactive[i])
      {
        for (j = 4; j > i; j--)
        {
          kiactive[j] = kiactive[j - 1];
          kiactivename[j] = kiactivename[j - 1];
        }
        kiactive[i] = GET_MAX_KI(conn->character);
        kiactivename[i] = conn->character->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if ((qint32)GET_PKILLS(conn->character) > pkactive[i])
      {
        for (j = 4; j > i; j--)
        {
          pkactive[j] = pkactive[j - 1];
          pkactivename[j] = pkactivename[j - 1];
        }
        pkactive[i] = (qint32)GET_PKILLS(conn->character);
        pkactivename[i] = conn->character->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (pdscore(conn->character) > pdactive[i])
      {
        for (j = 4; j > i; j--)
        {
          pdactive[j] = pdactive[j - 1];
          pdactivename[j] = pdactivename[j - 1];
        }
        pdactive[i] = pdscore(conn->character);
        pdactivename[i] = conn->character->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (conn->character->getLevel() < DC::MAX_MORTAL_LEVEL)
        break;
      if ((qint32)GET_RDEATHS(conn->character) > rdactive[i])
      {
        for (j = 4; j > i; j--)
        {
          rdactive[j] = rdactive[j - 1];
          rdactivename[j] = rdactivename[j - 1];
        }
        rdactive[i] = (qint32)GET_RDEATHS(conn->character);
        rdactivename[i] = conn->character->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_MOVE(conn->character) > mvactive[i])
      {
        for (j = 4; j > i; j--)
        {
          mvactive[j] = mvactive[j - 1];
          mvactivename[j] = mvactivename[j - 1];
        }
        mvactive[i] = GET_MAX_MOVE(conn->character);
        mvactivename[i] = conn->character->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_HIT(conn->character) > hpactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          hpactiveclass[k][j] = hpactiveclass[k][j - 1];
          hpactiveclassname[k][j] = hpactiveclassname[k][j - 1];
        }
        hpactiveclass[k][i] = GET_MAX_HIT(conn->character);
        hpactiveclassname[k][i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_MANA(conn->character) > mnactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          mnactiveclass[k][j] = mnactiveclass[k][j - 1];
          mnactiveclassname[k][j] = mnactiveclassname[k][j - 1];
        }
        mnactiveclass[k][i] = GET_MAX_MANA(conn->character);
        mnactiveclassname[k][i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_KI(conn->character) > kiactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          kiactiveclass[k][j] = kiactiveclass[k][j - 1];
          kiactiveclassname[k][j] = kiactiveclassname[k][j - 1];
        }
        kiactiveclass[k][i] = GET_MAX_KI(conn->character);
        kiactiveclassname[k][i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if ((qint32)GET_PKILLS(conn->character) > pkactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          pkactiveclass[k][j] = pkactiveclass[k][j - 1];
          pkactiveclassname[k][j] = pkactiveclassname[k][j - 1];
        }
        pkactiveclass[k][i] = (qint32)GET_PKILLS(conn->character);
        pkactiveclassname[k][i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (pdscore(conn->character) > pdactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          pdactiveclass[k][j] = pdactiveclass[k][j - 1];
          pdactiveclassname[k][j] = {};
          pdactiveclassname[k][j] = pdactiveclassname[k][j - 1];
        }
        pdactiveclass[k][i] = pdscore(conn->character);
        pdactiveclassname[k][i] = {};
        pdactiveclassname[k][i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (conn->character->getLevel() < DC::MAX_MORTAL_LEVEL)
        break;
      if ((qint32)GET_RDEATHS(conn->character) > rdactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          rdactiveclass[k][j] = rdactiveclass[k][j - 1];
          rdactiveclassname[k][j] = {};
          rdactiveclassname[k][j] = rdactiveclassname[k][j - 1];
        }
        rdactiveclass[k][i] = (qint32)GET_RDEATHS(conn->character);
        rdactiveclassname[k][i] = {};
        rdactiveclassname[k][i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_MOVE(conn->character) > mvactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          mvactiveclass[k][j] = mvactiveclass[k][j - 1];
          mvactiveclassname[k][j] = {};
          mvactiveclassname[k][j] = mvactiveclassname[k][j - 1];
        }
        mvactiveclass[k][i] = GET_MAX_MOVE(conn->character);
        mvactiveclassname[k][i] = {};
        mvactiveclassname[k][i] = (qPrintable(conn->character->name()));
        break;
      }
    }
  }

  write_file(LEADERBOARD_FILE);

  in_port_t port1 = {};
  if (DC::getInstance()->cf.ports.size() > 0)
  {
    port1 = DC::getInstance()->cf.ports[0];
  }

  write_file(QStringLiteral("%1%2/%3").arg(HTDOCS_DIR).arg(port1).arg(LEADERBOARD_FILE));

  for (i = {}; i < 5; i++)
  {
    hpactivename[i] = {};
    hpactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    mnactivename[i] = {};
    mnactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    kiactivename[i] = {};
    kiactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    pkactivename[i] = {};
    pkactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    pdactivename[i] = {};
    pdactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    rdactivename[i] = {};
    rdactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    mvactivename[i] = {};
    mvactivename[i] = {};
  }
  for (j = {}; j < CLASS_MAX - 2; j++)
  {
    for (i = {}; i < 5; i++)
    {
      hpactiveclassname[j][i] = {};
      hpactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      mnactiveclassname[j][i] = {};
      mnactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      kiactiveclassname[j][i] = {};
      kiactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      pkactiveclassname[j][i] = {};
      pkactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      pdactiveclassname[j][i] = {};
      pdactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      rdactiveclassname[j][i] = {};
      rdactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      mvactiveclassname[j][i] = {};
      mvactiveclassname[j][i] = {};
    }
  }
}

void Leaderboard::check_offline(void)
{
  CharacterPtr ch;
  qint32 i, j, k;

  DC::getInstance()->currentType("leaderboard");
  DC::getInstance()->currentName("NA");
  DC::getInstance()->currentVNUM(0);

  for (const auto &ch : DC::getInstance()->character_list)
  {
    if (!ch->isMortalPlayer())
      continue;

    if (ch->player == nullptr)
      continue;

    k = MIN(CLASS_DRUID - 1, GET_CLASS(ch) - 1);

    for (i = {}; i < 5; i++)
    {
      if (hpactivename[i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          hpactive[j] = hpactive[j + 1];
          hpactivename[j] = {};
          hpactivename[j] = hpactivename[j + 1];
        }
        hpactive[4] = {};
        hpactivename[4] = {};
        hpactivename[4] = QStringLiteral(" ");
      }
      if (mnactivename[i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          mnactive[j] = mnactive[j + 1];
          mnactivename[j] = {};
          mnactivename[j] = mnactivename[j + 1];
        }
        mnactive[4] = {};
        mnactivename[4] = {};
        mnactivename[4] = QStringLiteral(" ");
      }
      if (kiactivename[i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          kiactive[j] = kiactive[j + 1];
          kiactivename[j] = {};
          kiactivename[j] = kiactivename[j + 1];
        }
        kiactive[4] = {};
        kiactivename[4] = {};
        kiactivename[4] = QStringLiteral(" ");
      }
      if (pkactivename[i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          pkactive[j] = pkactive[j + 1];
          pkactivename[j] = {};
          pkactivename[j] = pkactivename[j + 1];
        }
        pkactive[4] = {};
        pkactivename[4] = {};
        pkactivename[4] = QStringLiteral(" ");
      }
      if (pdactivename[i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          pdactive[j] = pdactive[j + 1];
          pdactivename[j] = {};
          pdactivename[j] = pdactivename[j + 1];
        }
        pdactive[4] = {};
        pdactivename[4] = {};
        pdactivename[4] = QStringLiteral(" ");
      }
      if (rdactivename[i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          rdactive[j] = rdactive[j + 1];
          rdactivename[j] = {};
          rdactivename[j] = rdactivename[j + 1];
        }
        rdactive[4] = {};
        rdactivename[4] = {};
        rdactivename[4] = QStringLiteral(" ");
      }
      if (mvactivename[i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          mvactive[j] = mvactive[j + 1];
          mvactivename[j] = {};
          mvactivename[j] = mvactivename[j + 1];
        }
        mvactive[4] = {};
        mvactivename[4] = {};
        mvactivename[4] = QStringLiteral(" ");
      }
    }

    for (i = {}; i < 5; i++)
    {
      if (hpactiveclassname[k][i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          hpactiveclass[k][j] = hpactiveclass[k][j + 1];
          hpactiveclassname[k][j] = {};
          hpactiveclassname[k][j] = hpactiveclassname[k][j + 1];
        }
        hpactiveclass[k][4] = {};
        hpactiveclassname[k][4] = {};
        hpactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (mnactiveclassname[k][i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          mnactiveclass[k][j] = mnactiveclass[k][j + 1];
          mnactiveclassname[k][j] = {};
          mnactiveclassname[k][j] = mnactiveclassname[k][j + 1];
        }
        mnactiveclass[k][4] = {};
        mnactiveclassname[k][4] = {};
        mnactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (kiactiveclassname[k][i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          kiactiveclass[k][j] = kiactiveclass[k][j + 1];
          kiactiveclassname[k][j] = {};
          kiactiveclassname[k][j] = kiactiveclassname[k][j + 1];
        }
        kiactiveclass[k][4] = {};
        kiactiveclassname[k][4] = {};
        kiactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (pkactiveclassname[k][i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          pkactiveclass[k][j] = pkactiveclass[k][j + 1];
          pkactiveclassname[k][j] = {};
          pkactiveclassname[k][j] = pkactiveclassname[k][j + 1];
        }
        pkactiveclass[k][4] = {};
        pkactiveclassname[k][4] = {};
        pkactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (pdactiveclassname[k][i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          pdactiveclass[k][j] = pdactiveclass[k][j + 1];
          pdactiveclassname[k][j] = {};
          pdactiveclassname[k][j] = pdactiveclassname[k][j + 1];
        }
        pdactiveclass[k][4] = {};
        pdactiveclassname[k][4] = {};
        pdactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (rdactiveclassname[k][i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          rdactiveclass[k][j] = rdactiveclass[k][j + 1];
          rdactiveclassname[k][j] = {};
          rdactiveclassname[k][j] = rdactiveclassname[k][j + 1];
        }
        rdactiveclass[k][4] = {};
        rdactiveclassname[k][4] = {};
        rdactiveclassname[k][4] = QStringLiteral(" ");
      }
      if (mvactiveclassname[k][i] == ch->name())
      {
        for (j = i; j < 4; j++)
        {
          mvactiveclass[k][j] = mvactiveclass[k][j + 1];
          mvactiveclassname[k][j] = {};
          mvactiveclassname[k][j] = mvactiveclassname[k][j + 1];
        }
        mvactiveclass[k][4] = {};
        mvactiveclassname[k][4] = {};
        mvactiveclassname[k][4] = QStringLiteral(" ");
      }
    }

    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_HIT(ch) > hpactive[i])
      {
        for (j = 4; j > i; j--)
        {
          hpactive[j] = hpactive[j - 1];
          hpactivename[j] = {};
          hpactivename[j] = hpactivename[j - 1];
        }
        hpactive[i] = GET_MAX_HIT(ch);
        hpactivename[i] = {};
        hpactivename[i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_MANA(ch) > mnactive[i])
      {
        for (j = 4; j > i; j--)
        {
          mnactive[j] = mnactive[j - 1];
          mnactivename[j] = {};
          mnactivename[j] = mnactivename[j - 1];
        }
        mnactive[i] = GET_MAX_MANA(ch);
        mnactivename[i] = {};
        mnactivename[i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_KI(ch) > kiactive[i])
      {
        for (j = 4; j > i; j--)
        {
          kiactive[j] = kiactive[j - 1];
          kiactivename[j] = {};
          kiactivename[j] = kiactivename[j - 1];
        }
        kiactive[i] = GET_MAX_KI(ch);
        kiactivename[i] = {};
        kiactivename[i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if ((qint32)GET_PKILLS(ch) > pkactive[i])
      {
        for (j = 4; j > i; j--)
        {
          pkactive[j] = pkactive[j - 1];
          pkactivename[j] = {};
          pkactivename[j] = pkactivename[j - 1];
        }
        pkactive[i] = (qint32)GET_PKILLS(ch);
        pkactivename[i] = {};
        pkactivename[i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (pdscore(ch) > pdactive[i])
      {
        for (j = 4; j > i; j--)
        {
          pdactive[j] = pdactive[j - 1];
          pdactivename[j] = {};
          pdactivename[j] = pdactivename[j - 1];
        }
        pdactive[i] = pdscore(ch);
        pdactivename[i] = {};
        pdactivename[i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (ch->getLevel() < DC::MAX_MORTAL_LEVEL)
        break;
      if ((qint32)GET_RDEATHS(ch) > rdactive[i])
      {
        for (j = 4; j > i; j--)
        {
          rdactive[j] = rdactive[j - 1];
          rdactivename[j] = {};
          rdactivename[j] = rdactivename[j - 1];
        }
        rdactive[i] = (qint32)GET_RDEATHS(ch);
        rdactivename[i] = {};
        rdactivename[i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_MOVE(ch) > mvactive[i])
      {
        for (j = 4; j > i; j--)
        {
          mvactive[j] = mvactive[j - 1];
          mvactivename[j] = {};
          mvactivename[j] = mvactivename[j - 1];
        }
        mvactive[i] = GET_MAX_MOVE(ch);
        mvactivename[i] = {};
        mvactivename[i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_HIT(ch) > hpactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          hpactiveclass[k][j] = hpactiveclass[k][j - 1];
          hpactiveclassname[k][j] = {};
          hpactiveclassname[k][j] = hpactiveclassname[k][j - 1];
        }
        hpactiveclass[k][i] = GET_MAX_HIT(ch);
        hpactiveclassname[k][i] = {};
        hpactiveclassname[k][i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_MANA(ch) > mnactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          mnactiveclass[k][j] = mnactiveclass[k][j - 1];
          mnactiveclassname[k][j] = {};
          mnactiveclassname[k][j] = mnactiveclassname[k][j - 1];
        }
        mnactiveclass[k][i] = GET_MAX_MANA(ch);
        mnactiveclassname[k][i] = {};
        mnactiveclassname[k][i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_KI(ch) > kiactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          kiactiveclass[k][j] = kiactiveclass[k][j - 1];
          kiactiveclassname[k][j] = {};
          kiactiveclassname[k][j] = kiactiveclassname[k][j - 1];
        }
        kiactiveclass[k][i] = GET_MAX_KI(ch);
        kiactiveclassname[k][i] = {};
        kiactiveclassname[k][i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if ((qint32)GET_PKILLS(ch) > pkactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          pkactiveclass[k][j] = pkactiveclass[k][j - 1];
          pkactiveclassname[k][j] = {};
          pkactiveclassname[k][j] = pkactiveclassname[k][j - 1];
        }
        pkactiveclass[k][i] = (qint32)GET_PKILLS(ch);
        pkactiveclassname[k][i] = {};
        pkactiveclassname[k][i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (pdscore(ch) > pdactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          pdactiveclass[k][j] = pdactiveclass[k][j - 1];
          pdactiveclassname[k][j] = {};
          pdactiveclassname[k][j] = pdactiveclassname[k][j - 1];
        }
        pdactiveclass[k][i] = pdscore(ch);
        pdactiveclassname[k][i] = {};
        pdactiveclassname[k][i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (ch->getLevel() < DC::MAX_MORTAL_LEVEL)
        break;
      if ((qint32)GET_RDEATHS(ch) > rdactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          rdactiveclass[k][j] = rdactiveclass[k][j - 1];
          rdactiveclassname[k][j] = {};
          rdactiveclassname[k][j] = rdactiveclassname[k][j - 1];
        }
        rdactiveclass[k][i] = (qint32)GET_RDEATHS(ch);
        rdactiveclassname[k][i] = {};
        rdactiveclassname[k][i] = ch->name();
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_MOVE(ch) > mvactiveclass[k][i])
      {
        for (j = 4; j > i; j--)
        {
          mvactiveclass[k][j] = mvactiveclass[k][j - 1];
          mvactiveclassname[k][j] = {};
          mvactiveclassname[k][j] = mvactiveclassname[k][j - 1];
        }
        mvactiveclass[k][i] = GET_MAX_MOVE(ch);
        mvactiveclassname[k][i] = {};
        mvactiveclassname[k][i] = ch->name();
        break;
      }
    }
  }

  write_file(LEADERBOARD_FILE);

  in_port_t port1 = {};
  if (DC::getInstance()->cf.ports.size() > 0)
  {
    port1 = DC::getInstance()->cf.ports[0];
  }

  write_file(QStringLiteral("%1%2/%3").arg(HTDOCS_DIR).arg(port1).arg(LEADERBOARD_FILE));

  for (i = {}; i < 5; i++)
  {
    hpactivename[i] = {};
    hpactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    mnactivename[i] = {};
    mnactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    kiactivename[i] = {};
    kiactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    pkactivename[i] = {};
    pkactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    pdactivename[i] = {};
    pdactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    rdactivename[i] = {};
    rdactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    mvactivename[i] = {};
    mvactivename[i] = {};
  }
  for (j = {}; j < CLASS_MAX - 2; j++)
  {
    for (i = {}; i < 5; i++)
    {
      hpactiveclassname[j][i] = {};
      hpactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      mnactiveclassname[j][i] = {};
      mnactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      kiactiveclassname[j][i] = {};
      kiactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      pkactiveclassname[j][i] = {};
      pkactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      pdactiveclassname[j][i] = {};
      pdactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      rdactiveclassname[j][i] = {};
      rdactiveclassname[j][i] = {};
    }
    for (i = {}; i < 5; i++)
    {
      mvactiveclassname[j][i] = {};
      mvactiveclassname[j][i] = {};
    }
  }
}

void Leaderboard::read_file(void)
{
  FILE *fl;
  qint32 i, j;

  if (!(fl = fopen(LEADERBOARD_FILE, "r")))
  {
    logf(0, DC::LogChannel::LOG_BUG, "Cannot open leaderboard file '%s'", LEADERBOARD_FILE);
  }
  else
  {
    try
    {
      for (i = {}; i < 5; i++)
      {
        hpactivename[i] = fread_string(fl, 0);
        hpactive[i] = fread_int(fl, 0, 2147483467);
        if (char_file_exists(hpactivename[i]) == false)
        {
          hpactivename[i] = QStringLiteral("UNKNOWN");
          hpactive[i] = {};
        }
      }
      for (i = {}; i < 5; i++)
      {
        mnactivename[i] = fread_string(fl, 0);
        mnactive[i] = fread_int(fl, 0, 2147483467);
        if (char_file_exists(mnactivename[i]) == false)
        {
          mnactivename[i] = QStringLiteral("UNKNOWN");
        }
      }
      for (i = {}; i < 5; i++)
      {
        kiactivename[i] = fread_string(fl, 0);
        kiactive[i] = fread_int(fl, 0, 2147483467);
        if (char_file_exists(kiactivename[i]) == false)
        {
          kiactivename[i] = QStringLiteral("UNKNOWN");
        }
      }
      for (i = {}; i < 5; i++)
      {
        pkactivename[i] = fread_string(fl, 0);
        pkactive[i] = fread_int(fl, 0, 2147483467);
        if (char_file_exists(pkactivename[i]) == false)
        {
          pkactivename[i] = QStringLiteral("UNKNOWN");
        }
      }
      for (i = {}; i < 5; i++)
      {
        pdactivename[i] = fread_string(fl, 0);
        pdactive[i] = fread_int(fl, 0, 2147483467);
        if (char_file_exists(pdactivename[i]) == false)
        {
          pdactivename[i] = QStringLiteral("UNKNOWN");
        }
      }
      for (i = {}; i < 5; i++)
      {
        rdactivename[i] = fread_string(fl, 0);
        rdactive[i] = fread_int(fl, 0, 2147483467);
        if (char_file_exists(rdactivename[i]) == false)
        {
          rdactivename[i] = QStringLiteral("UNKNOWN");
        }
      }
      for (i = {}; i < 5; i++)
      {
        mvactivename[i] = fread_string(fl, 0);
        mvactive[i] = fread_int(fl, 0, 2147483467);
        if (char_file_exists(mvactivename[i]) == false)
        {
          mvactivename[i] = QStringLiteral("UNKNOWN");
        }
      }
      for (j = {}; j < CLASS_MAX - 2; j++)
      {
        for (i = {}; i < 5; i++)
        {
          hpactiveclassname[j][i] = fread_string(fl, 0);
          hpactiveclass[j][i] = fread_int(fl, 0, 2147483467);
          if (char_file_exists(hpactiveclassname[j][i]) == false)
          {
            hpactiveclassname[j][i] = QStringLiteral("UNKNOWN");
          }
        }
        for (i = {}; i < 5; i++)
        {
          mnactiveclassname[j][i] = fread_string(fl, 0);
          mnactiveclass[j][i] = fread_int(fl, 0, 2147483467);
          if (char_file_exists(mnactiveclassname[j][i]) == false)
          {
            mnactiveclassname[j][i] = QStringLiteral("UNKNOWN");
          }
        }
        for (i = {}; i < 5; i++)
        {
          kiactiveclassname[j][i] = fread_string(fl, 0);
          kiactiveclass[j][i] = fread_int(fl, 0, 2147483467);
          if (char_file_exists(kiactiveclassname[j][i]) == false)
          {
            kiactiveclassname[j][i] = QStringLiteral("UNKNOWN");
          }
        }
        for (i = {}; i < 5; i++)
        {
          pkactiveclassname[j][i] = fread_string(fl, 0);
          pkactiveclass[j][i] = fread_int(fl, 0, 2147483467);
          if (char_file_exists(pkactiveclassname[j][i]) == false)
          {
            pkactiveclassname[j][i] = QStringLiteral("UNKNOWN");
          }
        }
        for (i = {}; i < 5; i++)
        {
          pdactiveclassname[j][i] = fread_string(fl, 0);
          pdactiveclass[j][i] = fread_int(fl, 0, 2147483467);
          if (char_file_exists(pdactiveclassname[j][i]) == false)
          {
            pdactiveclassname[j][i] = QStringLiteral("UNKNOWN");
          }
        }
        for (i = {}; i < 5; i++)
        {
          rdactiveclassname[j][i] = fread_string(fl, 0);
          rdactiveclass[j][i] = fread_int(fl, 0, 2147483467);
          if (char_file_exists(rdactiveclassname[j][i]) == false)
          {
            rdactiveclassname[j][i] = QStringLiteral("UNKNOWN");
          }
        }
        for (i = {}; i < 5; i++)
        {
          mvactiveclassname[j][i] = fread_string(fl, 0);
          mvactiveclass[j][i] = fread_int(fl, 0, 2147483467);
          if (char_file_exists(mvactiveclassname[j][i]) == false)
          {
            mvactiveclassname[j][i] = QStringLiteral("UNKNOWN");
          }
        }
      }
    }
    catch (error_eof &)
    {
      logf(0, DC::LogChannel::LOG_BUG, "Corrupt leaderboard file '%s': eof reached prematurely", LEADERBOARD_FILE);
    }
    catch (error_negative_int &)
    {
      logf(0, DC::LogChannel::LOG_BUG, "Corrupt leaderboard file '%s': negative qint32 found where positive expected", LEADERBOARD_FILE);
    }

    fclose(fl);
  }
}

void Leaderboard::write_file(QString filename)
{
  if (DC::getInstance()->cf.leaderboard_check == "suspend" || DC::getInstance()->cf.bport == true)
  {
    return;
  }

  FILE *fl;
  qint32 i, j;

  if (!(fl = fopen(qPrintable(filename), "w")))
  {
    logf(0, DC::LogChannel::LOG_BUG, "Cannot open leaderboard file '%s'", filename);
    return;
  }
  for (i = {}; i < 5; i++)
    qfprintf(fl, "%s~ %d\n", hpactivename[i], hpactive[i]);
  for (i = {}; i < 5; i++)
    qfprintf(fl, "%s~ %d\n", mnactivename[i], mnactive[i]);
  for (i = {}; i < 5; i++)
    qfprintf(fl, "%s~ %d\n", kiactivename[i], kiactive[i]);
  for (i = {}; i < 5; i++)
    qfprintf(fl, "%s~ %d\n", pkactivename[i], pkactive[i]);
  for (i = {}; i < 5; i++)
    qfprintf(fl, "%s~ %d\n", pdactivename[i], pdactive[i]);
  for (i = {}; i < 5; i++)
    qfprintf(fl, "%s~ %d\n", rdactivename[i], rdactive[i]);
  for (i = {}; i < 5; i++)
    qfprintf(fl, "%s~ %d\n", mvactivename[i], mvactive[i]);
  for (j = {}; j < CLASS_MAX - 2; j++)
  {
    for (i = {}; i < 5; i++)
      qfprintf(fl, "%s~ %d\n", hpactiveclassname[j][i], hpactiveclass[j][i]);
    for (i = {}; i < 5; i++)
      qfprintf(fl, "%s~ %d\n", mnactiveclassname[j][i], mnactiveclass[j][i]);
    for (i = {}; i < 5; i++)
      qfprintf(fl, "%s~ %d\n", kiactiveclassname[j][i], kiactiveclass[j][i]);
    for (i = {}; i < 5; i++)
      qfprintf(fl, "%s~ %d\n", pkactiveclassname[j][i], pkactiveclass[j][i]);
    for (i = {}; i < 5; i++)
      qfprintf(fl, "%s~ %d\n", pdactiveclassname[j][i], pdactiveclass[j][i]);
    for (i = {}; i < 5; i++)
      qfprintf(fl, "%s~ %d\n", rdactiveclassname[j][i], rdactiveclass[j][i]);
    for (i = {}; i < 5; i++)
      qfprintf(fl, "%s~ %d\n", mvactiveclassname[j][i], mvactiveclass[j][i]);
  }
  fclose(fl);
}

qint32 Leaderboard::pdscore(CharacterPtr ch)
{
  return ch->player->pdeaths;
}

/*
 If you add anything to this function (more displays) make sure you change
 the lines = ## * (CLASS_MAX-1) to equal the number of added leaderboard
 things, otherwise renames will crash the server hard.
 */

qint32 do_leaderboard(CharacterPtr ch, QString argument, cmd_t cmd)
{
  class Connection *d;
  FILE *fl;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  qint32 i, j, k, validclass = {};
  char *hponlinename[5], *mnonlinename[5], *kionlinename[5], *pkonlinename[5],
      *pdonlinename[5], *rdonlinename[5], *mvonlinename[5];
  qint32 hponline[] = {0, 0, 0, 0, 0}, mnonline[] = {0, 0, 0, 0, 0},
         kionline[] = {0, 0, 0, 0, 0}, pkonline[] = {0, 0, 0, 0, 0},
         pdonline[] = {0, 0, 0, 0, 0}, rdonline[] = {0, 0, 0, 0, 0},
         mvonline[] = {0, 0, 0, 0, 0};
  char *hpactivename[5], *mnactivename[5], *kiactivename[5], *pkactivename[5],
      *pdactivename[5], *rdactivename[5], *mvactivename[5];
  qint32 hpactive[5], mnactive[5], kiactive[5], pkactive[5], pdactive[5],
      rdactive[5], mvactive[5];
  qint32 placea = 1, placeb = 1, placec = 1, placed = 1;
  qint32 skippeda = 0, skippedb = 0, skippedc = 0, skippedd = {};
  char *clss_types[] = {"mage", "cleric", "thief", "warrior", "antipaladin",
                        "paladin", "barbarian", "monk", "ranger", "bard", "druid", "\n"};

  if (ch->isPlayer() && ch->getLevel() >= IMPLEMENTER)
  {
    QString arg1, remainder;
    std::tie(arg1, remainder) = half_chop(argument);
    if (arg1 == "suspend")
    {
      if (DC::getInstance()->cf.leaderboard_check == "suspend")
      {
        ch->sendln("Leaderboard writes already suspended.");
      }
      else
      {
        DC::getInstance()->cf.leaderboard_check = "suspend";
        ch->sendln("Leaderboard writes suspended.");
        logf(IMPLEMENTER, DC::LogChannel::LOG_GOD, "Leaderboard writes suspended by %s.", qPrintable(ch->name()));
      }

      return ReturnValue::eSUCCESS;
    }
    else if (arg1 == "resume")
    {
      if (DC::getInstance()->cf.leaderboard_check == "")
      {
        ch->sendln("Leaderboard writes already resumed.");
      }
      else
      {
        DC::getInstance()->cf.leaderboard_check = "";
        ch->sendln("Leaderboard writes resumed.");
        logf(IMPLEMENTER, DC::LogChannel::LOG_GOD, "Leaderboard writes resumed by %s.", qPrintable(ch->name()));
      }

      return ReturnValue::eSUCCESS;
    }
    else if (arg1 == "scan")
    {
      leaderboard.scan(ch);
    }
  }
  else
  {
    leaderboard.check();
  }

  for (i = {}; i < 5; i++)
    hponlinename[i] = QStringLiteral(" ");
  for (i = {}; i < 5; i++)
    mnonlinename[i] = QStringLiteral(" ");
  for (i = {}; i < 5; i++)
    kionlinename[i] = QStringLiteral(" ");
  for (i = {}; i < 5; i++)
    pkonlinename[i] = QStringLiteral(" ");
  for (i = {}; i < 5; i++)
    pdonlinename[i] = QStringLiteral(" ");
  for (i = {}; i < 5; i++)
    rdonlinename[i] = QStringLiteral(" ");
  for (i = {}; i < 5; i++)
    mvonlinename[i] = QStringLiteral(" ");

  one_argument(argument, buf);
  for (k = {}; k < 11; k++)
  {
    if (is_abbrev(buf, clss_types[k]))
    {
      validclass = 1;
      break;
    }
  }

  if (!(fl = fopen(LEADERBOARD_FILE, "r")))
  {
    logf(0, DC::LogChannel::LOG_BUG, "Cannot open leaderboard file '%s'", LEADERBOARD_FILE);
    return ReturnValue::eFAILURE;
  }
  for (i = {}; i < 5; i++)
  {
    hpactivename[i] = fread_string(fl, 0);
    hpactive[i] = fread_int(fl, 0, 2147483467);
  }
  for (i = {}; i < 5; i++)
  {
    mnactivename[i] = fread_string(fl, 0);
    mnactive[i] = fread_int(fl, 0, 2147483467);
  }
  for (i = {}; i < 5; i++)
  {
    kiactivename[i] = fread_string(fl, 0);
    kiactive[i] = fread_int(fl, 0, 2147483467);
  }
  for (i = {}; i < 5; i++)
  {
    pkactivename[i] = fread_string(fl, 0);
    pkactive[i] = fread_int(fl, 0, 2147483467);
  }
  for (i = {}; i < 5; i++)
  {
    pdactivename[i] = fread_string(fl, 0);
    pdactive[i] = fread_int(fl, 0, 2147483467);
  }
  for (i = {}; i < 5; i++)
  {
    rdactivename[i] = fread_string(fl, 0);
    rdactive[i] = fread_int(fl, 0, 2147483467);
  }
  for (i = {}; i < 5; i++)
  {
    mvactivename[i] = fread_string(fl, 0);
    mvactive[i] = fread_int(fl, 0, 2147483467);
  }
  if (validclass)
  {
    for (j = {}; j < k + 1; j++)
    {
      for (i = {}; i < 5; i++)
      {
        hpactivename[i] = {};
        hpactivename[i] = {};
      }
      for (i = {}; i < 5; i++)
      {
        mnactivename[i] = {};
        mnactivename[i] = {};
      }
      for (i = {}; i < 5; i++)
      {
        kiactivename[i] = {};
        kiactivename[i] = {};
      }
      for (i = {}; i < 5; i++)
      {
        pkactivename[i] = {};
        pkactivename[i] = {};
      }
      for (i = {}; i < 5; i++)
      {
        pdactivename[i] = {};
        pdactivename[i] = {};
      }
      for (i = {}; i < 5; i++)
      {
        rdactivename[i] = {};
        rdactivename[i] = {};
      }
      for (i = {}; i < 5; i++)
      {
        mvactivename[i] = {};
        mvactivename[i] = {};
      }
      for (i = {}; i < 5; i++)
      {
        hpactivename[i] = fread_string(fl, 0);
        hpactive[i] = fread_int(fl, 0, 2147483467);
      }
      for (i = {}; i < 5; i++)
      {
        mnactivename[i] = fread_string(fl, 0);
        mnactive[i] = fread_int(fl, 0, 2147483467);
      }
      for (i = {}; i < 5; i++)
      {
        kiactivename[i] = fread_string(fl, 0);
        kiactive[i] = fread_int(fl, 0, 2147483467);
      }
      for (i = {}; i < 5; i++)
      {
        pkactivename[i] = fread_string(fl, 0);
        pkactive[i] = fread_int(fl, 0, 2147483467);
      }
      for (i = {}; i < 5; i++)
      {
        pdactivename[i] = fread_string(fl, 0);
        pdactive[i] = fread_int(fl, 0, 2147483467);
      }
      for (i = {}; i < 5; i++)
      {
        rdactivename[i] = fread_string(fl, 0);
        rdactive[i] = fread_int(fl, 0, 2147483467);
      }
      for (i = {}; i < 5; i++)
      {
        mvactivename[i] = fread_string(fl, 0);
        mvactive[i] = fread_int(fl, 0, 2147483467);
      }
    }
  }
  fclose(fl);

  // top 5 online
  for (d = DC::getInstance()->connections_; d; d = conn->next)
  {

    if (!conn->character || conn->character->getLevel() >= IMMORTAL)
      continue;
    if (!conn->connected == Connection::states::PLAYING)
      continue;
    if (!conn->character->player)
      continue;
    if (!CAN_SEE(ch, conn->character))
      continue;

    if (validclass && GET_CLASS(conn->character) != k + 1)
      continue;

    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_HIT(conn->character) > hponline[i])
      {
        for (j = 4; j > i; j--)
        {
          hponline[j] = hponline[j - 1];
          hponlinename[j] = {};
          hponlinename[j] = (hponlinename[j - 1]);
        }
        hponline[i] = GET_MAX_HIT(conn->character);
        hponlinename[i] = {};
        hponlinename[i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_MANA(conn->character) > mnonline[i])
      {
        for (j = 4; j > i; j--)
        {
          mnonline[j] = mnonline[j - 1];
          mnonlinename[j] = {};
          mnonlinename[j] = (mnonlinename[j - 1]);
        }
        mnonline[i] = GET_MAX_MANA(conn->character);
        mnonlinename[i] = {};
        mnonlinename[i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_KI(conn->character) > kionline[i])
      {
        for (j = 4; j > i; j--)
        {
          kionline[j] = kionline[j - 1];
          kionlinename[j] = {};
          kionlinename[j] = (kionlinename[j - 1]);
        }
        kionline[i] = GET_MAX_KI(conn->character);
        kionlinename[i] = {};
        kionlinename[i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if ((qint32)GET_PKILLS(conn->character) > pkonline[i])
      {
        for (j = 4; j > i; j--)
        {
          pkonline[j] = pkonline[j - 1];
          pkonlinename[j] = {};
          pkonlinename[j] = (pkonlinename[j - 1]);
        }
        pkonline[i] = (qint32)GET_PKILLS(conn->character);
        pkonlinename[i] = {};
        pkonlinename[i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (leaderboard.pdscore(conn->character) > pdonline[i])
      {
        for (j = 4; j > i; j--)
        {
          pdonline[j] = pdonline[j - 1];
          pdonlinename[j] = {};
          pdonlinename[j] = (pdonlinename[j - 1]);
        }
        pdonline[i] = leaderboard.pdscore(conn->character);
        pdonlinename[i] = {};
        pdonlinename[i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (conn->character->getLevel() < DC::MAX_MORTAL_LEVEL)
        break;
      if ((qint32)GET_RDEATHS(conn->character) > rdonline[i])
      {
        for (j = 4; j > i; j--)
        {
          rdonline[j] = rdonline[j - 1];
          rdonlinename[j] = {};
          rdonlinename[j] = (rdonlinename[j - 1]);
        }
        rdonline[i] = (qint32)GET_RDEATHS(conn->character);
        rdonlinename[i] = {};
        rdonlinename[i] = (qPrintable(conn->character->name()));
        break;
      }
    }
    for (i = {}; i < 5; i++)
    {
      if (GET_MAX_MOVE(conn->character) > mvonline[i])
      {
        for (j = 4; j > i; j--)
        {
          mvonline[j] = mvonline[j - 1];
          mvonlinename[j] = {};
          mvonlinename[j] = (mvonlinename[j - 1]);
        }
        mvonline[i] = GET_MAX_MOVE(conn->character);
        mvonlinename[i] = {};
        mvonlinename[i] = (qPrintable(conn->character->name()));
        break;
      }
    }
  }
  sprintf(buf, "(*)**************************************************************************(*)\r\n");
  strcat(buf,
         "(*)                          $BDark Castle Leaderboard$R                         (*)\r\n");
  if (validclass)
  {
    k != 2 ? sprintf(buf2,
                     "(*)                             $Bfor %11ss$R                             (*)\r\n",
                     clss_types[k])
           : sprintf(buf2,
                     "(*)                             $Bfor      thieves$R                             (*)\r\n");
    strcat(buf, buf2);
  }
  strcat(buf,
         "(*)--------------------------------------------------------------------------(*)\r\n");
  strcat(buf,
         "(*)                                                                          (*)\r\n");
  strcat(buf,
         "(*)    Online         All Time                Online        All Time         (*)\r\n");
  strcat(buf,
         "(*)                                                                          (*)\r\n");
  strcat(buf,
         "(*)            $2$BHit Points                               Mana$R                 (*)\r\n");
  sprintf(buf2,
          "(*) 1) $5$B%-12s$R1) $5$B%-12s$R        1) $5$B%-12s$R1) $5$B%-12s$R     (*)\r\n",
          hponlinename[0], hpactivename[0], mnonlinename[0], mnactivename[0]);
  strcat(buf, buf2);
  for (i = 1; i < 5; i++)
  {
    if (hponline[i] != hponline[i - 1])
    {
      placea += ++skippeda;
      skippeda = {};
    }
    else
      skippeda++;
    if (hpactive[i] != hpactive[i - 1])
    {
      placeb += ++skippedb;
      skippedb = {};
    }
    else
      skippedb++;
    if (mnonline[i] != mnonline[i - 1])
    {
      placec += ++skippedc;
      skippedc = {};
    }
    else
      skippedc++;
    if (mnactive[i] != mnactive[i - 1])
    {
      placed += ++skippedd;
      skippedd = {};
    }
    else
      skippedd++;
    sprintf(buf2,
            "(*) %d) $B%-12s$R%d) $B%-12s$R-%-4d   %d) $B%-12s$R%d) $B%-12s$R-%-4d(*)\r\n",
            placea, hponlinename[i], placeb, hpactivename[i],
            hpactive[0] - hpactive[i], placec, mnonlinename[i], placed,
            mnactivename[i], mnactive[0] - mnactive[i]);
    strcat(buf, buf2);
  }
  placea = 1;
  placeb = 1;
  placec = 1;
  placed = 1;
  skippeda = {};
  skippedb = {};
  skippedc = {};
  skippedd = {};
  strcat(buf,
         "(*)                                                                          (*)\r\n");
  strcat(buf,
         "(*)                $2$BKi                                Movement        $R        (*)\r\n");
  sprintf(buf2,
          "(*) 1) $5$B%-12s$R1) $5$B%-12s$R        1) $5$B%-12s$R1) $5$B%-12s$R     (*)\r\n",
          kionlinename[0], kiactivename[0], mvonlinename[0], mvactivename[0]);
  strcat(buf, buf2);
  for (i = 1; i < 5; i++)
  {
    if (kionline[i] != kionline[i - 1])
    {
      placea += ++skippeda;
      skippeda = {};
    }
    else
      skippeda++;
    if (kiactive[i] != kiactive[i - 1])
    {
      placeb += ++skippedb;
      skippedb = {};
    }
    else
      skippedb++;
    if (mvonline[i] != mvonline[i - 1])
    {
      placec += ++skippedc;
      skippedc = {};
    }
    else
      skippedc++;
    if (mvactive[i] != mvactive[i - 1])
    {
      placed += ++skippedd;
      skippedd = {};
    }
    else
      skippedd++;
    sprintf(buf2,
            "(*) %d) $B%-12s$R%d) $B%-12s$R-%-4d   %d) $B%-12s$R%d) $B%-12s$R-%-4d(*)\r\n",
            placea, kionlinename[i], placeb, kiactivename[i],
            kiactive[0] - kiactive[i], placec, mvonlinename[i], placed,
            mvactivename[i], mvactive[0] - mvactive[i]);
    strcat(buf, buf2);
  }
  placea = 1;
  placeb = 1;
  placec = 1;
  placed = 1;
  skippeda = {};
  skippedb = {};
  skippedc = {};
  skippedd = {};
  strcat(buf,
         "(*)                                                                          (*)\r\n");
  strcat(buf,
         "(*)         $2$BPlayer Kill Score                   Player Death Score       $R    (*)\r\n");
  sprintf(buf2,
          "(*) 1) $5$B%-12s$R1) $5$B%-12s$R        1) $5$B%-12s$R1) $5$B%-12s$R     (*)\r\n",
          pkonlinename[0], pkactivename[0], pdonlinename[0], pdactivename[0]);
  strcat(buf, buf2);
  for (i = 1; i < 5; i++)
  {
    if (pkonline[i] != pkonline[i - 1])
    {
      placea += ++skippeda;
      skippeda = {};
    }
    else
      skippeda++;
    if (pkactive[i] != pkactive[i - 1])
    {
      placeb += ++skippedb;
      skippedb = {};
    }
    else
      skippedb++;
    if (pdonline[i] != pdonline[i - 1])
    {
      placec += ++skippedc;
      skippedc = {};
    }
    else
      skippedc++;
    if (pdactive[i] != pdactive[i - 1])
    {
      placed += ++skippedd;
      skippedd = {};
    }
    else
      skippedd++;
    sprintf(buf2,
            "(*) %d) $B%-12s$R%d) $B%-12s$R-%-4d   %d) $B%-12s$R%d) $B%-12s$R-%-4d(*)\r\n",
            placea, pkonlinename[i], placeb, pkactivename[i],
            pkactive[0] - pkactive[i], placec, pdonlinename[i], placed,
            pdactivename[i], pdactive[0] - pdactive[i]);
    strcat(buf, buf2);
  }
  // FROM HERE
  placea = 1;
  placeb = 1;
  placec = 1;
  placed = 1;
  skippeda = {};
  skippedb = {};
  skippedc = {};
  skippedd = {};
  strcat(buf,
         "(*)                                                                          (*)\r\n");
  strcat(buf,
         "(*)        $2$BReal Deaths (Level 60)                                      $R      (*)\r\n");
  sprintf(buf2,
          "(*) 1) $5$B%-12s$R1) $5$B%-12s$R                                           (*)\r\n",
          rdonlinename[0], rdactivename[0]);
  strcat(buf, buf2);
  for (i = 1; i < 5; i++)
  {
    if (rdonline[i] != rdonline[i - 1])
    {
      placea += ++skippeda;
      skippeda = {};
    }
    else
      skippeda++;
    if (rdactive[i] != rdactive[i - 1])
    {
      placeb += ++skippedb;
      skippedb = {};
    }
    else
      skippedb++;
    //   if(rdonline[i] != rdonline[i-1]) {
    //    placec += ++skippedc;
    //    skippedc = {};
    // }
    // else skippedc++;
    //  if(rdactive[i] != rdactive[i-1]) {
    //      placed += ++skippedd;
    //       skippedd = {};
    //     }
    //      else skippedd++;
    sprintf(buf2,
            "(*) %d) $B%-12s$R%d) $B%-12s$R-%-4d                                      (*)\r\n",
            placea, rdonlinename[i], placeb, rdactivename[i],
            rdactive[0] - rdactive[i]);
    strcat(buf, buf2);
  }

  // TO HERE
  strcat(buf,
         "(*)                                                                          (*)\r\n");
  strcat(buf,
         "(*)--------------------------------------------------------------------------(*)\r\n");
  strcat(buf,
         "(*)**************************************************************************(*)\r\n");
  page_string(ch->desc, buf, 1);
  for (i = {}; i < 5; i++)
  {
    hponlinename[i] = {};
    hponlinename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    mnonlinename[i] = {};
    mnonlinename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    kionlinename[i] = {};
    kionlinename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    pkonlinename[i] = {};
    pkonlinename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    pdonlinename[i] = {};
    pdonlinename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    rdonlinename[i] = {};
    rdonlinename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    mvonlinename[i] = {};
    mvonlinename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    hpactivename[i] = {};
    hpactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    mnactivename[i] = {};
    mnactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    kiactivename[i] = {};
    kiactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    pkactivename[i] = {};
    pkactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    pdactivename[i] = {};
    pdactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    rdactivename[i] = {};
    rdactivename[i] = {};
  }
  for (i = {}; i < 5; i++)
  {
    mvactivename[i] = {};
    mvactivename[i] = {};
  }

  return ReturnValue::eSUCCESS;
}

void Leaderboard::rename(QString oldname, QString newname)
{
  FILE *fl = {};
  // lines is the number of lines rewritten back to leaderboard file
  // after a rename.. must sync up with # of outputs

  if (DC::getInstance()->cf.bport)
  {
    return;
  }

  if (!(fl = fopen(LEADERBOARD_FILE, "r")))
  {
    logf(0, DC::LogChannel::LOG_BUG, "Cannot open leaderboard file: %s", LEADERBOARD_FILE);
    abort();
  }

  QList<qint32> value;
  QList<QString> name;
  auto lines = 35 * (CLASS_MAX - 1);
  for (auto i = {}; i < lines; i++)
  {
    name.insert(i, fread_string(fl, 0));
    value.insert(i, fread_int(fl, 0, 2147483467));
  }
  fclose(fl);

  for (auto i = {}; i < lines; i++)
  {
    if (name[i] == oldname)
    {
      name[i] = newname;
    }
  }

  if (DC::getInstance()->cf.leaderboard_check == "suspend")
  {
    logf(IMMORTAL, DC::LogChannel::LOG_GOD, "Leaderboard rename of %s to %s failed because writes are suspended.", qPrintable(oldname), qPrintable(newname));
  }
  else
  {
    if (!(fl = fopen(LEADERBOARD_FILE, "w")))
    {
      logf(0, DC::LogChannel::LOG_BUG, "Cannot open leaderboard file: %s", LEADERBOARD_FILE);
      abort();
    }

    for (auto i = {}; i < lines; i++)
    {
      qfprintf(fl, "%s~ %d\n", qPrintable(name[i]), value[i]);
    }

    fclose(fl);
  }
}

void Leaderboard::setHP(quint32 placement, QString name, qint32 value)
{
  hpactive[placement] = value;
  hpactivename[placement] = name.data();
}

qint32 Leaderboard::scan(CharacterPtr ch)
{
  check_offline();

  return ReturnValue::eSUCCESS;
}

Leaderboard leaderboard;
