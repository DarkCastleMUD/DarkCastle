#ifndef CLAN_H_
#define CLAN_H_

/************************************************************************
| $Id: clan.h,v 1.13 2004/11/16 00:52:09 Zaphod Exp $
| clan.h
| Description:  Header information for clans.
*/

void clan_login(char_data * ch);
void clan_logout(char_data * ch);
int has_right(char_data * ch, uint32 bit);
struct clan_data * get_clan(int nClan);
struct clan_data * get_clan(CHAR_DATA *ch);
void save_clans();
#define CLAN_RIGHTS_ACCEPT       1
#define CLAN_RIGHTS_OUTCAST      1<<1
#define CLAN_RIGHTS_B_READ       1<<2
#define CLAN_RIGHTS_B_WRITE      1<<3
#define CLAN_RIGHTS_B_REMOVE     1<<4
#define CLAN_RIGHTS_MEMBER_LIST  1<<5
#define CLAN_RIGHTS_RIGHTS       1<<6
#define CLAN_RIGHTS_MESSAGES     1<<7
#define CLAN_RIGHTS_INFO         1<<8
#define CLAN_RIGHTS_TAX		 1<<9
#define CLAN_RIGHTS_WITHDRAW	 1<<10
#define CLAN_RIGHTS_CHANNEL	 1<<11
//#define CLAN_RIGHTS_DEPOSIT      1<<11 Anyone can deposit
// if you add to the clan rights, update clan_rights[] in clan.C


struct clan_room_data
{
   int32 room_number;
  struct clan_room_data * next;
};

struct clan_member_data
{
  char * member_name;
  uint32 member_rights;
   int32 member_rank;
  
   int32 unused1;
   int32 unused2;
  uint32 unused3;
  char * unused4; // this is saved as a variable length string
  
  //  I'd like to put "time joined" here for CC purposes
  uint32  time_joined;

  struct clan_member_data * next;
};

struct clan_data
{
  char *leader;
  char *founder;
  char *name;
  char * email;
  char * description;
  char * login_message;
  char * death_message;
  char * logout_message;
  char * clanmotd;
  long balance;
  uint16 tax;
  uint16 number;
  clan_room_data * rooms;
  clan_member_data * members;
  struct clan_data * next;
};

void add_totem(OBJ_DATA *altar, OBJ_DATA *totem);
void remove_totem(OBJ_DATA *altar, OBJ_DATA *totem);
void add_totem_stats(CHAR_DATA *ch);
void remove_totem_stats(CHAR_DATA *ch);

#endif /* CLAN_H_ */
