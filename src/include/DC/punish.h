#pragma once
/************************************************************************
| $Id: punish.h,v 1.4 2008/11/13 19:10:49 kkoons Exp $
| punish.h
| Description: Contains punishment vectors
*/

constexpr auto PUNISH_SILENCED = 1;
constexpr auto PUNISH_NOEMOTE = 1 << 1;
constexpr auto PUNISH_LOG = 1 << 2;
constexpr auto PUNISH_FREEZE = 1 << 3;
constexpr auto PUNISH_DENY = 1 << 4;
constexpr auto PUNISH_UNUSED = 1 << 5;
constexpr auto PUNISH_NONAME = 1 << 6;
constexpr auto PUNISH_SPAMMER = 1 << 7;
constexpr auto PUNISH_STUPID = 1 << 8;
constexpr auto PUNISH_NOARENA = 1 << 9;
constexpr auto PUNISH_NOTITLE = 1 << 10;
constexpr auto PUNISH_UNLUCKY = 1 << 11;
constexpr auto PUNISH_NOTELL = 1 << 12;
constexpr auto PUNISH_NOPRAY = 1 << 13;
