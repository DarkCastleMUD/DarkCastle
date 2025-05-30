/***************************************************************************
| comm.h: New comm stuff to make the rewrite of comm.C work properly
*/
#ifndef __COMM_H__
#define __COMM_H__

#include <string>

typedef int socket_t;

#define SEND_TO_Q(messg, desc) write_to_output((messg), desc)
#ifndef WIN32
#define CLOSE_SOCKET(sock) close(sock)
#else
#define CLOSE_SOCKET(sock) closesocket(sock)
#endif

#define SMALL_BUFSIZE 1024
#define LARGE_BUFSIZE (24 * 2048)
#define GARBAGE_SPACE 32
#define NUM_RESERVED_DESCS 8
#define HOST_LENGTH 30

enum pulse_type
{
	TIMER,
	MOBILE,
	OBJECT,
	VIOLENCE,
	BARD,
	TENSEC,
	WEATHER,
	TIME,
	REGEN,
	SHORT
};

struct pulse_info
{
	pulse_type pulse;
	uint64_t duration;
	char name[];
};

#define BAN_NOT 0
#define BAN_NEW 1
#define BAN_SELECT 2
#define BAN_ALL 3

#include "DC/character.h"
void write_to_output(const char *txt, class Connection *t);
void write_to_output(QByteArray txt, class Connection *d);
void write_to_output(std::string txt, class Connection *d);
void write_to_output(QString txt, class Connection *t);

void scramble_text(char *txt);
QString scramble_text(QString txt);
void warn_if_duplicate_ip(Character *ch);
void record_msg(QString messg, Character *ch);
void send_info(QString messg);
void send_info(std::string messg);
void send_info(const char *messg);
bool is_multi(Character *ch);
void new_string_add(class Connection *d, char *str);
void telnet_ga(Connection *d);
void telnet_sga(Connection *d);
void telnet_echo_off(class Connection *d);
void telnet_echo_on(class Connection *d);
void update_bard_singing(void);
void affect_update(int32_t duration_type); /* In spells.c */
char *calc_color(int hit, int max_hit);
char *calc_color_align(int align);
char *calc_condition(Character *ch, bool colour = false);
Character *get_charmie(Character *ch);
#endif
