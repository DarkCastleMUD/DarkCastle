#ifndef CLAN_H_
#define CLAN_H_

/************************************************************************
| $Id: clan.h,v 1.27 2013/12/30 22:46:43 jhhudso Exp $
| clan.h
| Description:  Header information for clans.
*/

constexpr size_t MAX_CLAN_LEN = 15;
#define CLAN_RIGHTS_ACCEPT 1
#define CLAN_RIGHTS_OUTCAST 1 << 1
#define CLAN_RIGHTS_B_READ 1 << 2
#define CLAN_RIGHTS_B_WRITE 1 << 3
#define CLAN_RIGHTS_B_REMOVE 1 << 4
#define CLAN_RIGHTS_MEMBER_LIST 1 << 5
#define CLAN_RIGHTS_RIGHTS 1 << 6
#define CLAN_RIGHTS_MESSAGES 1 << 7
#define CLAN_RIGHTS_INFO 1 << 8
#define CLAN_RIGHTS_TAX 1 << 9
#define CLAN_RIGHTS_WITHDRAW 1 << 10
#define CLAN_RIGHTS_CHANNEL 1 << 11
#define CLAN_RIGHTS_AREA 1 << 12
#define CLAN_RIGHTS_VAULT 1 << 13
#define CLAN_RIGHTS_VAULTLOG 1 << 14
#define CLAN_RIGHTS_LOG 1 << 15

struct clan_room_data
{
  int32_t room_number;
  struct clan_room_data *next;
};

struct clan_member_data
{
  char *member_name;
  uint32_t member_rights;
  int32_t member_rank;

  int32_t unused1;
  int32_t unused2;
  uint32_t unused3;
  char *unused4; // this is saved as a variable length string

  //  I'd like to put "time joined" here for CC purposes
  uint32_t time_joined;

  struct clan_member_data *next;
};

class clan_data
{
public:
  char *leader;
  char *founder;
  char *name;
  char *email;
  char *description;
  char *login_message;
  char *death_message;
  char *logout_message;
  char *clanmotd;
  uint16_t tax;
  uint16_t number;
  uint16_t amt;
  clan_room_data *rooms;
  clan_member_data *members;
  clan_data *next;
  struct vault_access_data *acc;
  clan_data(void);
  void cdeposit(const uint64_t &deposit);
  void cwithdraw(const uint64_t &withdraw);
  uint64_t getBalance(void);
  void setBalance(const uint64_t &value);
  std::queue<std::string> ctell_history;
  void log(QString log_entry);

private:
  uint64_t balance;
};
// if you add to the clan rights, update clan_rights[] in clan.C

void add_totem(obj_data *altar, obj_data *totem);
void remove_totem(obj_data *altar, obj_data *totem);
void add_totem_stats(char_data *ch, int stat = 0);
void remove_totem_stats(char_data *ch, int stat = 0);
bool others_clan_room(char_data *ch, room_data *room);
void clan_login(char_data *ch);
void clan_logout(char_data *ch);
int has_right(char_data *ch, uint32_t bit);
clan_data *get_clan(int nClan);
clan_data *get_clan(char_data *ch);
char *get_clan_name(int nClan);
char *get_clan_name(char_data *ch);
char *get_clan_name(clan_data *clan);
void save_clans();
int plr_rights(char_data *ch);
void add_clan(clan_data *new_new_clan);
void add_clan_member(clan_data *theClan, struct clan_member_data *new_new_member);
void add_clan_member(clan_data *theClan, char_data *ch);
void remove_clan_member(clan_data *theClan, char_data *ch);
void remove_clan_member(int clannumber, char_data *ch);
void free_member(struct clan_member_data *member);
struct clan_member_data *get_member(char *strName, int nClanId);
void show_clan_log(char_data *ch);
void clan_death(char_data *ch, char_data *killer);

#endif /* CLAN_H_ */
