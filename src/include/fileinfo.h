#ifndef FILEINFO_H_
#define FILEINFO_H_
/*
| $Id: fileinfo.h,v 1.5 2004/05/01 11:05:47 urizen Exp $
| fileinfo.h
| Description:  Contains information for loading files, ie "../lib", etc..
*/
/*************************************************************************
 * Revision History                                                      *
 * 10/16/2003   Onager     Added FORBIDDEN_NAME_FILE                     *
 *************************************************************************/
#ifndef WIN32
#define  DFLT_DIR            "../lib"
#define  SAVE_DIR            "../save"
#define  NEWSAVE_DIR         "../newsave"
#define  ARCHIVE_DIR         "../archive"
#define  MOB_DIR             "../MOBProgs/"
#define  BAN_FILE            "banned.txt"
#define  SHOP_DIR            "../lib/shops"
#define  PLAYER_SHOP_DIR     "../lib/playershops"
#define  FORBIDDEN_NAME_FILE "../lib/forbidden_names.txt"
#define  SKILL_QUEST_FILE    "../lib/skill_quests.txt"
#else
#define DFLT_DIR             "lib"
#define  SAVE_DIR            "save"
#define  NEWSAVE_DIR         "newsave"
#define  ARCHIVE_DIR         "archive"
#define  MOB_DIR             "MOBProgs/"
#define  BAN_FILE            "banned.txt"
#define  SHOP_DIR            "shops"
#define  PLAYER_SHOP_DIR     "playershops"
#define  FORBIDDEN_NAME_FILE "forbidden_names.txt"
#define SKILL_QUEST_FILE "skill_quests.txt"
#endif

// TODO - Remove tinyworld.shp and divide the stops up into some meaningful
//        format in their own directory like the world/mob/obj files
#define  SHOP_FILE          "tinyworld.shp"

#define  WEBPAGE_FILE       "webresponse.txt"
#define  GREETINGS1_FILE    "greetings1.txt"
#define  GREETINGS2_FILE    "greetings3.txt"
#define  GREETINGS3_FILE    "greetings4.txt"
#define  GREETINGS4_FILE    "greetings5.txt"
#define  CREDITS_FILE       "credits.txt"
#define  NEWS_FILE          "news.txt"
#define  MOTD_FILE          "motd.txt"
#define  IMOTD_FILE         "motdimm.txt"
#define  STORY_FILE         "story.txt"
#define  TIME_FILE          "time.txt"
#define  IDEA_FILE          "ideas.txt"
#define  TYPO_FILE          "typos.txt"
#define  MESS_FILE          "messages.txt"
#define  SOCIAL_FILE        "social.txt"
#define  HELP_KWRD_FILE     "help_key.txt"
#define  HELP_PAGE_FILE     "help.txt"
#define  INFO_FILE          "info.txt"
#define  WIZLIST_FILE       "wizlist.txt"

#ifndef WIN32
#define BUG_FILE            "../log/bug.log"
#define GOD_FILE            "../log/god.log"
#define MORTAL_FILE         "../log/mortal.log"
#define SOCKET_FILE         "../log/socket.log"
#define PLAYER_FILE         "../log/player.log"
#define WORLD_LOG           "../log/world.log"
#define CHAOS_LOG           "../log/chaos.log"
#define CLAN_LOG            "../log/clan.log" 
#else
#define BUG_FILE            "bug.log"
#define GOD_FILE            "god.log"
#define MORTAL_FILE         "mortal.log"
#define SOCKET_FILE         "socket.log"
#define PLAYER_FILE         "player.log"
#define WORLD_LOG           "world.log"
#define CHAOS_LOG           "chaos.log"
#define CLAN_LOG            "clan.log" 
#endif
#define  WORLD_INDEX_FILE  "worldindex"
#define  OBJECT_INDEX_FILE "objectindex"
#define  MOB_INDEX_FILE    "mobindex"
#define  ZONE_INDEX_FILE   "zoneindex"
#define  PLAYER_SHOP_INDEX "playershopindex"

// We can't have a tiny_file for objects or morts that login would lose all their eq
//#define  OBJECT_INDEX_FILE_TINY "objectindex.tiny"
#define  WORLD_INDEX_FILE_TINY   "worldindex.tiny"
#define  MOB_INDEX_FILE_TINY "mobindex.tiny"
#define  ZONE_INDEX_FILE_TINY "zoneindex.tiny"

#endif // FILEINFO_H_
