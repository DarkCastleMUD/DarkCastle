#ifndef __IDENT_H__
#define __IDENT_H__
#include <sys/wait.h>
#include <netinet/in.h>
#include <waitflags.h>

/*******************************************************************/

struct message
{
  int                type;
  struct sockaddr_in addr;
  int                fd;
  char               host[256];
  char               user[256];
};
 
#define MSG_ERROR -1
#define MSG_NOP    0  /* don't do anything */
#define MSG_IDENT  1  /* lookup hostname and username for a user */
#define MSG_QUIT   2  /* kill the lookup process */
#define MSG_IDREP  3  /* reply from the lookup process */

/*******************************************************************/

struct descriptor_data;

void id_lookup(struct descriptor_data *d);
int  id_init(void);
void id_kill(void);

extern int id_serv_socket;

/*******************************************************************/

#endif /* __IDENT_H__ */
