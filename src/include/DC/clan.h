#pragma once

/************************************************************************
| $Id: clan.h,v 1.27 2013/12/30 22:46:43 jhhudso Exp $
| clan.h
| Description:  Header information for clans.
*/

#include <cstddef>
#include <QString>
#include <qqueue.h>

constexpr size_t MAX_CLAN_LEN = 15;
constexpr auto CLAN_RIGHTS_ACCEPT = 1;
constexpr auto CLAN_RIGHTS_OUTCAST = 1 << 1;
constexpr auto CLAN_RIGHTS_B_READ = 1 << 2;
constexpr auto CLAN_RIGHTS_B_WRITE = 1 << 3;
constexpr auto CLAN_RIGHTS_B_REMOVE = 1 << 4;
constexpr auto CLAN_RIGHTS_MEMBER_LIST = 1 << 5;
constexpr auto CLAN_RIGHTS_RIGHTS = 1 << 6;
constexpr auto CLAN_RIGHTS_MESSAGES = 1 << 7;
constexpr auto CLAN_RIGHTS_INFO = 1 << 8;
constexpr auto CLAN_RIGHTS_TAX = 1 << 9;
constexpr auto CLAN_RIGHTS_WITHDRAW = 1 << 10;
constexpr auto CLAN_RIGHTS_CHANNEL = 1 << 11;
constexpr auto CLAN_RIGHTS_AREA = 1 << 12;
constexpr auto CLAN_RIGHTS_VAULT = 1 << 13;
constexpr auto CLAN_RIGHTS_VAULTLOG = 1 << 14;
constexpr auto CLAN_RIGHTS_LOG = 1 << 15;

// if you add to the clan rights, update clan_rights[] in clan.C

void save_clans(void);
