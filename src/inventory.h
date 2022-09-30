void get(struct char_data *ch, struct obj_data *obj_object, struct obj_data *sub_object, bool has_consent, int cmd);
void log_sacrifice(struct char_data *ch, obj_data *obj, bool decay);
int search_char_for_item_count(struct char_data * ch, int16_t item_number, bool wearonly);
struct obj_data * search_char_for_item(struct char_data * ch, int16_t item_number, bool wearonly);
int find_door(struct char_data *ch, char *type, char *dir);
int palm(struct char_data *ch, struct obj_data *obj_object, struct obj_data *sub_object, bool has_consent);
bool search_container_for_item(obj_data *obj, int item_number);