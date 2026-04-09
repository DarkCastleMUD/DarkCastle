#ifdef BANDWIDTH
#pragma once

void setup_bandwidth();
void add_bandwidth(qint32 amount);
bandwidth_type read_bandwidth(const char *file);
qint32 write_bandwidth();

qint32 get_bandwidth_start();
qint32 get_bandwidth_amount();

#endif /* #ifdef BANDWIDTH */
