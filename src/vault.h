#include "utility.h"

#define VAULT_UPGRADE_COST 100 // plats
#define VAULT_BASE_SIZE 10  // weight
#define VAULT_MAX_SIZE  10000 // weight

#define VAULT_MAX_DEPWITH 2000000000 // 2 bil max to add/remove from bank at a time

extern struct vault_data *vault_table;
struct vault_data *has_vault(const char *name);

void vault_stats(Character *ch, char *name);
void vault_withdraw(Character *ch, unsigned int amount, char *owner);
void vault_deposit(Character *ch, unsigned int amount, char *owner);
void save_vault(char *name);
void vault_access(Character *ch, char *who);
void my_vault_access(Character *ch);
void load_vaults(void);
void add_vault_access(Character *ch, char *name, struct vault_data *vault);
void remove_vault_access(Character *ch, char *name, struct vault_data *vault);
void vault_get(Character *ch, char *object, char *owner);
void vault_put(Character *ch, char *object, char *owner);
void vault_list(Character *ch, char *owner);
void add_new_vault(char *name, int indexonly);
void reload_vaults(void);
void vault_cost(Character *ch, char *object, char *arg);
void remove_vault(char *name, BACKUP_TYPE backup = NONE);
void rename_vault_owner(char *oldname, char *newname);
void access_remove(char *name, struct vault_data *vault);
void remove_vault_accesses(char *name);
void vault_sell(Character *ch, char *object, char *arg);
int vault_log_to_string(const char *name, char *buf);
void vlog(const char *message, const char *name);

struct obj_data *get_obj_in_vault(struct vault_data *vault, char *object, int num);
struct vault_items_data *get_item_in_vault(struct vault_data *vault, char *object, int num);

struct obj_data *get_unique_obj_in_vault(struct vault_data *vault, char *object, int num);
struct vault_items_data *get_unique_item_in_vault(struct vault_data *vault, char *object, int num);

struct obj_data *get_obj_in_all_vaults(char *object, int num);
struct vault_items_data *get_items_in_all_vaults(char *object, int num);

int has_vault_access(char *owner, struct vault_data *vault);
int vault_search(Character *ch, const char *keyword);

struct vault_data {
   char *owner;
   unsigned int size;
   unsigned int weight;
   uint64_t gold;

   struct vault_access_data *access;
   struct vault_items_data *items;

   struct vault_data *next;
};

struct vault_access_data {
  char *name;
  struct vault_access_data *next;
};

struct vault_items_data {
  int item_vnum;
  int count;
  obj_data *obj; // for full-save items
  struct vault_items_data *next;
};
