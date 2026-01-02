#include "DC/utility.h"

#define VAULT_UPGRADE_COST 100 // plats
#define VAULT_BASE_SIZE 10     // weight
#define VAULT_MAX_SIZE 10000   // weight

#define VAULT_MAX_DEPWITH 2000000000 // 2 bil max to add/remove from bank at a time

extern struct vault_data *vault_table;
struct vault_data *has_vault(QString name);

void vault_withdraw(Character *ch, unsigned int amount, char *owner);
void vault_deposit(Character *ch, unsigned int amount, char *owner);
void save_vault(QString name);
void my_vault_access(Character *ch);
void vault_get(Character *ch, QString object, QString owner);
void vault_put(Character *ch, QString object, QString owner);
void add_new_vault(const char *name, int indexonly);
void vault_cost(Character *ch, char *object, char *arg);
void remove_vault(QString name, BACKUP_TYPE backup = NONE);
void rename_vault_owner(QString oldname, QString newname);
void access_remove(QString name, struct vault_data *vault);
void remove_vault_accesses(QString name);
void vault_sell(Character *ch, char *object, char *arg);
int vault_log_to_string(const char *name, char *buf);
void logvault(QString message, QString name);
void item_remove(Object *obj, struct vault_data *vault);
void item_add(int vnum, struct vault_data *vault);

class Object *get_obj_in_vault(struct vault_data *vault, QString object, int num);
struct vault_items_data *get_item_in_vault(struct vault_data *vault, char *object, int num);

class Object *get_unique_obj_in_vault(struct vault_data *vault, char *object, int num);
struct vault_items_data *get_unique_item_in_vault(struct vault_data *vault, char *object, int num);

class Object *get_obj_in_all_vaults(char *object, int num);
struct vault_items_data *get_items_in_all_vaults(char *object, int num);

int vault_search(Character *ch, const char *keyword);
void sort_vault(const vault_data &vault, struct sorted_vault &sv);

struct vault_data
{
  QString owner;
  unsigned int size{};
  unsigned int weight{};
  uint64_t gold{};

  struct vault_access_data *access{};
  struct vault_items_data *items{};

  struct vault_data *next{};
};

struct vault_access_data
{
  QString name;
  struct vault_access_data *next{};
};

struct vault_items_data
{
  int item_vnum;
  int count;
  Object *obj{}; // for full-save items
  struct vault_items_data *next{};
};

struct sorted_vault
{
  // This stores the quantity of each item found in a vault
  std::map<std::string, std::pair<Object *, uint32_t>> vault_content_qty{};

  // This stores the order in which vault items are found
  std::vector<std::string> vault_contents{};

  unsigned int weight{};
};