#ifdef USE_SQL
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "levels.h"
#include "Database.h"

PGconn *Database::conn = 0;

using namespace std;


map<string, PGresult *> Prepare::existing_prepares;


Database::Database()
  :Backend()
{
  if (conn == 0 || PQstatus(conn) != CONNECTION_OK) {
    conn = PQconnectdb("dbname=dcastle user=dcastle password=W2dFF83");
    if (PQstatus(conn) != CONNECTION_OK) {
      char logBuffer[256];
      snprintf(logBuffer, 256,
	       "Unable to connect to database in save_char_obj_db: %s",
	       PQerrorMessage(conn));
      cerr << logBuffer << endl;
      log(logBuffer, ANGEL, LOG_BUG);
      return;
    }
  }
}

  /* PQprepare(conn, "char_file_u_INSERT", "UPDATE players 

  const char *players_cols[] = {"sex", "c_class", "race", "level",
			      "raw_str", "raw_intel", "raw_wiz", "raw_dex",
			      "raw_con", "conditions0", "conditions1",
			      "conditions2", "conditions3", "weight", "height",
			      "hometown", "gold", "plat", "exp", "immune",
			      "resist", "suscept", "mana", "raw_mana",
			      "hit", "raw_hit", "move", "raw_move", "ki",
			      "raw_ki", "alignment", "hpmetas",
			      "manametas", "movemetas", "armor", "hitroll",
			      "damroll", "afected_by", "afected_by2",
			      "misc", "clan", "load_room", NULL};
  generic_prepares("players", "player_id", players_cols);
  */

void Database::save(CHAR_DATA *ch)
{

}

void Database::save(char_file_u *st)
{
  if (st == 0)
    return;

  Prepare p = createPrepare("char_file_u");
  p.setTableName("players");
  p.addCol("sex", st->sex);
  p.addCol("c_class", st->c_class);
  p.addCol("race", st->race);
  p.addCol("level", st->level);
  p.addCol("raw_str", st->raw_str);
  p.addCol("raw_intel", st->raw_intel);
  p.addCol("raw_wis", st->raw_wis);
  p.addCol("raw_dex", st->raw_dex);
  p.addCol("raw_con", st->raw_con);
  p.addCol("conditions_0", st->conditions[0]);
  p.addCol("conditions_1", st->conditions[1]);
  p.addCol("conditions_2", st->conditions[2]);
  p.exec();
}

CHAR_DATA *Database::load(void)
{

  return 0;
}

Prepare Database::createPrepare(string prepareID)
{
  return Prepare(Database::conn, prepareID);
}


Prepare::Prepare(PGconn *conn, string prepareID)
  :prepareID(prepareID), conn(conn)
{
  
}

void Prepare::setTableName(string tableName)
{
  this->tableName = tableName;
}

void Prepare::setConn(PGconn *conn)
{
  this->conn = conn;
}

void Prepare::setPrepareID(string prepareID)
{
  this->prepareID = prepareID;
}

void Prepare::addCol(string colName, int value)
{
  if (tableName.empty())
    throw emptyName();

  if (prepareID.empty())
    throw emptyPrepareID();

  paramData pD;
  pD.int_data = value;

  param p;
  p.type = INTEGER;
  p.data = pD;

  this->current_prepare.push_back(make_pair(colName, p));
}

void Prepare::addCol(string colName, char value)
{
  if (tableName.empty())
    throw emptyName();

  if (prepareID.empty())
    throw emptyPrepareID();

  paramData pD;
  pD.char_data = value;

  param p;
  p.type = CHARACTER;
  p.data = pD;

  this->current_prepare.push_back(make_pair(colName, p));
}

void Prepare::addCol(string colName, char *value)
{
  if (tableName.empty())
    throw emptyName();

  if (prepareID.empty())
    throw emptyPrepareID();

  paramData pD;
  pD.char_ptr_data = value;

  param p;
  p.type = CHARACTER_PTR;
  p.data = pD;

  this->current_prepare.push_back(make_pair(colName, p));
}

void Prepare::exec(void)
{
  // Check if prepare exists already
  PGresult *r = Prepare::existing_prepares[prepareID];
  
  // If so execute prepared statement
  if ((r != 0) && (PQresultStatus(r) == PGRES_COMMAND_OK)) {
    //    r = PQexecPrepared(this->conn, prepareID.c_str(), current_prepare.size());
  }

  //  int nParams = current_prepare.size();
  //  Oid *paramTypes = new Oid[nParams];
  //  string query = generateInsertQuery();

}

string Prepare::generateInsertQuery(void)
{
  /*
  stringstream sql;
  sql << "INSERT INTO " << tableName << " (";

  PrepareVector::iterator index = current_prepare.begin();
  while (index != current_prepare.end()) {
    //    sql << current_prepare[index];
    if (tableCols[++index] != current_prepare.end())
      sql << ", ";
  }
  int numCols = index;

  sql << ") VALUES (";

  for (PrepareVector::iterator index = current_prepare.begin();
       index != current_prepare.end();
       index++) {
    sql << "$" << index+1;
    if (index+1 != numCols)
      sql << ", ";
  }

  sql << ")";

  cerr << sql.str() << endl;
  prepare::declaration pd = Database::conn.prepare(tableName+"_INSERT",
						   sql.str());
  for(index = 0; index < numCols; index++) {
    pd("varchar", prepare::treat_string);
  }


  sql.str("");
  sql << "UPDATE " << tableName << " SET ";

  for(index = 0; index < numCols; index++) {
    sql << tableCols[index] << "=$" << index+1;
    if (index+1 != numCols)
      sql << ", ";
  }
  sql << " WHERE " << tablePrimaryKey << "=$" << numCols+1;

  cerr << sql.str() << endl;
  prepare::declaration pd2 = Database::conn.prepare(tableName+"_UPDATE",
						    sql.str());

  for(index = 0; index < numCols; index++) {
    pd2("varchar", prepare::treat_string);
  }

}
  */  

  return string();
}

#endif
