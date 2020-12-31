/***************************************************************************
| comm.h: New comm stuff to make the rewrite of comm.C work properly
*/
#ifndef __COMM_H__
#define __COMM_H__

#include "character.h"

typedef int socket_t;

#define SEND_TO_Q(messg, desc)  write_to_output((messg), desc)
#ifndef WIN32
#define CLOSE_SOCKET(sock) close(sock)
#else
#define CLOSE_SOCKET(sock) closesocket(sock)
#endif

#define SMALL_BUFSIZE           1024
#define LARGE_BUFSIZE           (24 * 2048)
#define GARBAGE_SPACE           32
#define NUM_RESERVED_DESCS      8
//#define OPT_USEC                250000  
//#define PASSES_PER_SEC         (1000000 / OPT_USEC)
#define PASSES_PER_SEC          10
#define HOST_LENGTH             30

enum pulse_type { TIMER, MOBILE, OBJECT, VIOLENCE, BARD, TENSEC, WEATHER, TIME, REGEN, SHORT };

struct pulse_info {
	pulse_type pulse;
	uint64_t duration;
	char name[];
};

#define PULSE_TIMER	(1 * PASSES_PER_SEC)
#define PULSE_MOBILE    (4 * PASSES_PER_SEC)
#define PULSE_OBJECT    (4 * PASSES_PER_SEC)
#define PULSE_VIOLENCE  (2 * PASSES_PER_SEC)
#define PULSE_BARD      (1 * PASSES_PER_SEC)
#define PULSE_TENSEC	(10 * PASSES_PER_SEC)
#define PULSE_WEATHER   (45 * PASSES_PER_SEC)
#define PULSE_TIME      (60 * PASSES_PER_SEC)
#define PULSE_REGEN     (15 * PASSES_PER_SEC)
#define PULSE_SHORT     1 // Pulses all the time. 

#define BAN_NOT         0
#define BAN_NEW         1
#define BAN_SELECT      2
#define BAN_ALL         3



//void     write_to_output(const char *txt, struct descriptor_data *d);
void     write_to_output(const char *txt, struct descriptor_data *d);
void     scramble_text(char * txt);
void     warn_if_duplicate_ip(char_data * ch);
void     record_msg(char *messg, char_data *ch);
int      write_hotboot_file(char **argv);
void  send_info(string messg);
void  send_info(const char *messg);
#endif
