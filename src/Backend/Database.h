#ifdef USE_SQL
#ifndef DATABASE_H_
#define DATABASE_H_

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <libpq-fe.h>
#include "character.h"
#include "Backend.h"

#define CONN_OPTS "dbname=dcastle user=dcastle password=W2dFF83"

using namespace std;

union paramData {
  int int_data;
  char char_data;
  char *char_ptr_data;
};

enum paramType { INTEGER, CHARACTER, CHARACTER_PTR };

struct param {
  union paramData data;
  enum paramType type;
};

typedef vector<pair<string, param> > PrepareVector;

class Prepare {
 private:
  string generateInsertQuery(void);
  string generateUpdateQuery(void);
  static map<string, PGresult *> existing_prepares;
  string tableName;
  string prepareID;
  PGconn *conn;
  PrepareVector current_prepare;

 public:
  Prepare(PGconn *conn, string prepare_id);
  void setTableName(string tableName);
  void setConn(PGconn *conn);
  void setPrepareID(string prepareID);
  void addCol(string colName, int value);
  void addCol(string colName, char value);
  void addCol(string colName, char *value);
  void exec(void);

  // Exceptions
  class emptyName {};
  class emptyPrepareID {};
};

class Database : public Backend {
 private:
  static PGconn *conn;
 protected:

 public:
  Database();
  virtual void save(CHAR_DATA *ch);
  virtual void save(char_file_u *st);
  virtual CHAR_DATA *load(void);
  Prepare createPrepare(string prepare_id);
};


#endif
#endif
