void get(struct char_data *ch, struct obj_data *obj_object, struct obj_data *sub_object, bool has_consent, int cmd);
void log_sacrifice(CHAR_DATA *ch, OBJ_DATA *obj, bool decay);
int search_char_for_item_count(char_data * ch, int16 item_number, bool wearonly);
struct obj_data * search_char_for_item(char_data * ch, int16 item_number, bool wearonly);
int find_door(CHAR_DATA *ch, char *type, char *dir);
int palm(CHAR_DATA *ch, struct obj_data *obj_object, struct obj_data *sub_object, bool has_consent);
bool search_container_for_item(obj_data *obj, int item_number);