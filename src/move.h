#ifndef MOVE_H_
#define MOVE_H_

int attempt_move(struct char_data *ch, int cmd, int is_retreat = 0);
int ambush(struct char_data *ch);

#endif