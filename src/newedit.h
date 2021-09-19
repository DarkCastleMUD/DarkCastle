
/* action modes for parse_action */
#define PARSE_FORMAT          0 
#define PARSE_REPLACE         1 
#define PARSE_HELP            2 
#define PARSE_DELETE          3
#define PARSE_INSERT          4
#define PARSE_LIST_NORM       5
#define PARSE_LIST_NUM        6
#define PARSE_EDIT            7


/* misc editor defines **************************************************/

/* format modes for format_text */
#define FORMAT_INDENT         (1 << 0)

void parse_action(int command, char *string, struct descriptor_data *d);