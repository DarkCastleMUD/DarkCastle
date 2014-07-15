#ifndef FILEINFO_H_
#define FILEINFO_H_
/*
| $Id: fileinfo.h,v 1.33 2014/07/15 21:34:46 jhhudso Exp $
| fileinfo.h
| Description:  Contains information for loading files, ie "../lib", etc..
*/
/*************************************************************************
 * Revision History                                                      *
 * 10/16/2003   Onager     Added FORBIDDEN_NAME_FILE                     *
 *************************************************************************/
#define  DFLT_DIR            "../lib"
#define  SAVE_DIR            "../save"
#define  BSAVE_DIR            "../bsave"
#define  QSAVE_DIR            "../save/qdata"
#define  NEWSAVE_DIR         "../newsave"
#define  ARCHIVE_DIR         "../archive"
#define  MOB_DIR             "../MOBProgs/"
#define  BAN_FILE            "banned.txt"
#define  SHOP_DIR            "../lib/shops"
#define  PLAYER_SHOP_DIR     "../lib/playershops"
#define  FORBIDDEN_NAME_FILE "../lib/forbidden_names.txt"
#define  SKILL_QUEST_FILE    "../lib/skill_quests.txt"
#define  FAMILIAR_DIR        "../familiar"
#define  VAULT_DIR           "../vaults"

// TODO - Remove tinyworld.shp and divide the stops up into some meaningful
//        format in their own directory like the world/mob/obj files
#define  SHOP_FILE          "tinyworld.shp"

#define  WEBPAGE_FILE       "webresponse.txt"
#define  GREETINGS1_FILE    "greetings1.txt"
#define  GREETINGS2_FILE    "greetings3.txt"
#define  GREETINGS3_FILE    "greetings4.txt"
#define  GREETINGS4_FILE    "greetings5.txt"
#define  CREDITS_FILE       "credits.txt"
#define  MOTD_FILE          "../lib/motd.txt"
#define  IMOTD_FILE         "motdimm.txt"
#define  STORY_FILE         "story.txt"
#define  TIME_FILE          "time.txt"
#define  IDEA_LOG           "ideas.log"
#define  TYPO_LOG           "typos.log"
#define  MESS_FILE          "messages.txt"
#define  MESS2_FILE         "messages2.txt"
#define  SOCIAL_FILE        "social.txt"
#define  HELP_KWRD_FILE     "help_key.txt"
#define  HELP_PAGE_FILE     "help.txt"
#define  INFO_FILE          "info.txt"
#define  LOCAL_WHO_FILE     "onlinewho.txt"

#define  WEB_WHO_FILE        "/srv/www/www.dcastle.org/htdocs/onlinewho.txt"
#define  WEB_AUCTION_FILE    "/srv/www/www.dcastle.org/htdocs/auctions.txt"
#define  NEW_HELP_FILE	     "new_help.txt"
#define  WEB_HELP_FILE       "/srv/www/www.dcastle.org/htdocs/webhelp.txt"
#define  NEW_HELP_PAGE_FILE  "new_help_screen.txt"
#define  NEW_IHELP_PAGE_FILE "new_ihelp_screen.txt"
#define  LEADERBOARD_FILE    "leaderboard.txt"
#define  QUEST_FILE         "quests.txt"
#define  WEBCLANSLIST_FILE  "webclanslist.txt"
#define  HTDOCS_DIR         "/srv/www/www.dcastle.org/htdocs/"

#define PLAYER_DIR         "player/"
#define BUG_LOG            "bug.log"
#define GOD_LOG            "god.log"
#define MORTAL_LOG         "mortal.log"
#define SOCKET_LOG         "socket.log"
#define PLAYER_LOG         "player.log"
#define WORLD_LOG          "world.log"
#define ARENA_LOG          "arena.log"
#define CLAN_LOG           "clan.log"
#define OBJECTS_LOG        "objects.log"
#define QUEST_LOG          "quest.log"

#define  WORLD_INDEX_FILE  "worldindex"
#define  OBJECT_INDEX_FILE "objectindex"
#define  MOB_INDEX_FILE    "mobindex"
#define  ZONE_INDEX_FILE   "zoneindex"
#define  PLAYER_SHOP_INDEX "playershopindex"

#define  OBJECT_INDEX_FILE_TINY "objectindex.tiny"
#define  WORLD_INDEX_FILE_TINY   "worldindex.tiny"
#define  MOB_INDEX_FILE_TINY "mobindex.tiny"
#define  ZONE_INDEX_FILE_TINY "zoneindex.tiny"

#define VAULT_INDEX_FILE     "../vaults/vaultindex"
#define VAULT_INDEX_FILE_TMP "../vaults/vaultindex.tmp"

#define HINTS_FILE           "playerhints.txt"
#endif // FILEINFO_H_
