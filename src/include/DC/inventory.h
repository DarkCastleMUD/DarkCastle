#pragma once
#include "DC/Index.h"
#include "DC/common.h"

class Character;
class Object;
enum class cmd_t;

void get(CharacterPtr ch, ObjectPtr obj_object, ObjectPtr sub_object, bool has_consent, cmd_t cmd);
void log_sacrifice(CharacterPtr ch, ObjectPtr obj, bool decay);
int search_char_for_item_count(CharacterPtr ch, vnum_t vnum, bool wearonly);
ObjectPtr search_char_for_item(CharacterPtr ch, vnum_t vnum, bool wearonly);
int find_door(CharacterPtr ch, char *type, char *dir);
int palm(CharacterPtr ch, ObjectPtr obj_object, ObjectPtr sub_object, bool has_consent);
bool search_container_for_item(ObjectPtr obj, vnum_t vnum);
ObjectPtr bring_type_to_front(CharacterPtr ch, int item_type);