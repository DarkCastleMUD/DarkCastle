void get(Character *ch, class Object *obj_object, class Object *sub_object, bool has_consent, cmd_t cmd);
void log_sacrifice(Character *ch, Object *obj, bool decay);
int search_char_for_item_count(Character *ch, int16_t item_number, bool wearonly);
class Object *search_char_for_item(Character *ch, int16_t item_number, bool wearonly);
int find_door(Character *ch, char *type, char *dir);
int palm(Character *ch, class Object *obj_object, class Object *sub_object, bool has_consent);
bool search_container_for_item(Object *obj, int item_number);
class Object *bring_type_to_front(Character *ch, int item_type);