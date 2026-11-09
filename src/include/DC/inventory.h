#pragma once
#include "DC/Index.h"
#include "DC/common.h"

class Character;
class Object;
enum class cmd_t;

void get(CharacterPtr ch, class Object *obj_object, class Object *sub_object, bool has_consent, cmd_t cmd);
void log_sacrifice(CharacterPtr ch, Object *obj, bool decay);
int search_char_for_item_count(CharacterPtr ch, vnum_t vnum, bool wearonly);
class Object *search_char_for_item(CharacterPtr ch, vnum_t vnum, bool wearonly);
int find_door(CharacterPtr ch, char *type, char *dir);
int palm(CharacterPtr ch, class Object *obj_object, class Object *sub_object, bool has_consent);
bool search_container_for_item(Object *obj, vnum_t vnum);
class Object *bring_type_to_front(CharacterPtr ch, int item_type);