#ifndef TOKEN_H_
#define TOKEN_H_
/*-----------------------------------------------------------------------
| Token section.  This section begins the Token class and methods.
*/

const int ANSI = 1 << 0;  /* If it's an ansi code */
const int VT100 = 1 << 1; /* If it's a vt100 code */
const int CODE = 1 << 2;  /* If it should be interped */
const int TEXT = 1 << 3;  /* If it's just text */
const int _MAX_STR = 2048;

class Token
{
public:
  //--
  // Public functions
  //--

  Token(char *rhs);
  ~Token();
  int IsAnsi() { return (type & ANSI); }
  int IsVt100() { return (type & VT100); }
  int IsCode() { return (type & CODE); }
  int IsText() { return (type & TEXT); }
  const char *GetBuf() { return (buf); }
  void SetBuf(char *);
  Token *Next() { return next; }
  void Next(Token *n) { next = n; }

private:
  //--
  // Private variables
  //--
  char *buf;   /* Holds the buffer */
  int type;    /* Holds type of buffer */
  Token *next; /* Next token in the list */
};             // end of Token class

class TokenList
{
public:
  //--
  // Public functions
  //--

  TokenList(const char *);
  ~TokenList();
  string Interpret(CHAR_DATA *from, OBJ_DATA *obj, void *vict_obj, CHAR_DATA *send_to, int flags);

private:
  //--
  // Private functions
  //--
  void AddToken(const Token *);
  void Reset();
  void Next();

  //--
  // Private constants
  //--

  //--
  // Private variables
  //--
  Token *head;    // Head of the list
  Token *current; // Current token
  string interp;  // Interpreted tokens

};     // end of TokenList class
#endif /* TOKEN_H_ */
