#define VAULT_UPGRADE_COST 100 // plats
#define VAULT_BASE_SIZE 500  // weight
#define VAULT_MAX_SIZE  10000 // weight

#define VAULT_MAX_DEPWITH 2000000000 // 2 bil max to add/remove from bank at a time

extern struct vault_data *vault_table;
struct vault_data *has_vault(char *name);

void vault_stats(CHAR_DATA *ch, char *name);
void vault_withdraw(CHAR_DATA *ch, unsigned int amount, char *owner);
void vault_deposit(CHAR_DATA *ch, unsigned int amount, char *owner);
void save_vault(char *name);
void vault_access(CHAR_DATA *ch, char *who);
void my_vault_access(CHAR_DATA *ch);
void load_vaults(void);
void add_vault_access(CHAR_DATA *ch, char *name, struct vault_data *vault);
void remove_vault_access(CHAR_DATA *ch, char *name, struct vault_data *vault);
void get_from_vault(CHAR_DATA *ch, char *object, char *owner);
void put_in_vault(CHAR_DATA *ch, char *object, char *owner);
void show_vault(CHAR_DATA *ch, char *owner);
void add_new_vault(char *name, int indexonly);
void reload_vaults(void);
void vault_cost(CHAR_DATA *ch, char *object, char *arg);
void remove_vault(char *name);
void rename_vault_owner(char *oldname, char *newname);
void access_remove(char *name, struct vault_data *vault);
void remove_vault_accesses(char *name);
void vault_sell(CHAR_DATA *ch, char *object, char *arg);
int vault_log_to_string(const char *name, char *buf);
struct vault_data *get_vault_in_all_vaults(char *object, int num);

struct obj_data *get_obj_in_vault(struct vault_data *vault, char *object, int num);
struct vault_items_data *get_item_in_vault(struct vault_data *vault, char *object, int num);

struct obj_data *get_unique_obj_in_vault(struct vault_data *vault, char *object, int num);
struct vault_items_data *get_unique_item_in_vault(struct vault_data *vault, char *object, int num);

struct obj_data *get_obj_in_all_vaults(char *object, int num);
struct vault_items_data *get_items_in_all_vaults(char *object, int num);

int has_vault_access(char *owner, struct vault_data *vault);


struct vault_data {
   char *owner;
   unsigned int size;
   unsigned int weight;
   long long unsigned int gold;

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
