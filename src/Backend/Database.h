#ifdef USE_SQL
#ifndef DATABASE_H_
#define DATABASE_H_

#include <iostream>
#include <map>
#include <vector>
#include <queue>
#include <string>
#include <libpq-fe.h>
#include "character.h"
#include "Backend.h"

using namespace std;

const char CONN_OPTS[]="host=127.0.0.1 dbname=dcastle user=dcastle password=wm42LyP1";

typedef vector<pair<string, string> > PrepareVector;

class Prepare {
 private:
  string generateInsertQuery(void);
  string generateUpdateQuery(void);
  static map<string, PGresult *> existing_prepares;
  PGconn *conn;
    
  string tableName;
  string prepareID;
  string whereString;
  string whereData;
  PrepareVector current_prepare;

 public:
  Prepare(PGconn *conn, string prepare_id);
  ~Prepare();
  void setTableName(string tableName);
  void setConn(PGconn *conn);
  void setPrepareID(string prepareID);
  void addCol(string colName, int value);
  void addCol(string colName, char * value);
  void where(string ws, int wd);
  void where(string ws, char * wd);
  void exec(void);

  // Exceptions
  class emptyName {};
  class emptyPrepareID {};
  
  PGresult *lastResult;
};

class Database : public Backend {
 private:
  static PGconn *conn;
  queue<Prepare> execqueue;
 protected:

 public:
  Database();
  virtual void save(CHAR_DATA *ch, char_file_u *st);
  virtual CHAR_DATA *load(void);
  Prepare createPrepare(string prepare_id);
  int lookupPlayerID(const char *name);
  int createPlayerID(char *name);  
  void processqueue(void);
};


#endif
#endif
