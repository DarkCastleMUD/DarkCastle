#ifdef BANDWIDTH
#ifndef BANDWIDTH_H_
#define BANDWIDTH_H_

void setup_bandwidth();
void add_bandwidth(int amount);
struct bandwidth_type read_bandwidth(const char *file);
int write_bandwidth();

long int get_bandwidth_start();
long int get_bandwidth_amount();

#endif /* #ifndef BANDWIDTH_H_ */
#endif /* #ifdef BANDWIDTH */
