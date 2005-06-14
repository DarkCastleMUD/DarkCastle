#ifndef CONNECT_H_
#define CONNECT_H_
/************************************************************************
| $Id: connect.h,v 1.9 2005/06/14 22:13:45 shane Exp $
| connect.h
| Description: State of connectedness information.
*/
#include <structs.h>  // MAX_INPUT_LENGTH
#include <comm.h>
 
#define STATE(d)  ((d)->connected) 
#define CON_PLAYING                        0 
#define CON_GET_NAME                       1 
#define CON_GET_OLD_PASSWORD               2
#define CON_CONFIRM_NEW_NAME               3 
#define CON_GET_NEW_PASSWORD               4
#define CON_CONFIRM_NEW_PASSWORD           5 
#define CON_GET_NEW_SEX                    6 
#define CON_GET_NEW_CLASS                  7 
#define CON_READ_MOTD                      8 
#define CON_SELECT_MENU                    9 
#define CON_RESET_PASSWORD                 10 
#define CON_CONFIRM_RESET_PASSWORD         11 
#define CON_EXDSCR                         12 
#define CON_GET_RACE                       13 
#define CON_WRITE_BOARD                    14 
#define CON_EDITING                        15 
#define CON_SEND_MAIL                      16 
#define CON_DELETE_CHAR                    17
#define CON_CHOOSE_STATS                   18 
#define CON_PFILE_WIPE                     19 
#define CON_ARCHIVE_CHAR                   20 
#define CON_CLOSE                          21 
#define CON_CONFIRM_PASSWORD_CHANGE        22
#define CON_EDIT_MPROG                     23 
#define CON_DISPLAY_ENTRANCE               24 
#define CON_PRE_DISPLAY_ENTRANCE           25

// if you change, make sure you update char *connected_states[] in const.C
// also update connected_types[]

#define MAX_RAW_INPUT_LENGTH  512

struct stat_shit
{   
  int str[5], tel[5], wis[5], dex[5], con[5];
};

struct descriptor_data
{
    unsigned	descriptor;		/* file descriptor for socket	*/
    int         desc_num;    
    char *	name;			/* Copy of the player name	*/
    char	host[80];              	/* hostname			*/
    int		connected;		/* mode of 'connectedness'	*/
    int		wait;			/* wait for how many loops	*/
    char *	showstr_head;		/* for paging through texts	*/
    char **     showstr_vector;
    int         showstr_count;
    int         showstr_page;
    bool	new_newline;		/* prepend newline in output	*/
    char	**str;			/* for the modify-str system	*/
    char        **hashstr;
    char        *astr;
    int		max_str;		/*	-			*/
    char	buf[10 * MAX_INPUT_LENGTH];	/* buffer for raw input	*/
    char	last_input[MAX_INPUT_LENGTH];	/* the last input	*/
    char *	output;			/* queue of strings to send	*/
    char inbuf[MAX_RAW_INPUT_LENGTH];
    TXT_Q	input;			/* queue of unprocessed input	*/
    CHAR_DATA *	character;		/* linked to char		*/
    CHAR_DATA *	original;		/* for switch / return		*/
    struct descriptor_data *snooping; /* Who is this char snooping       */
    struct descriptor_data *snoop_by; /* And who is snooping this char   */
    struct descriptor_data *	next;	/* link to next descriptor	*/
    int         tick_wait;      	/* # ticks desired to wait	*/
    int         reallythere;            /* Goddamm #&@$*% sig 13 (hack) */
    int         prompt_mode;
    txt_block   *large_outbuf;
    int         bufptr;
    int         bufspace;
    char        small_outbuf[SMALL_BUFSIZE];
    byte         idle_tics;
    time_t      login_time;
    struct stat_shit * stats;            // for rolling up a char 

    char	**strnew;			/* for the modify-str system	*/
    char        *backstr;
    int         idle_time; // How long the descriptor has been idle, overall.
};

#endif
