#ifdef USE_SQL
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "levels.h"
#include "Database.h"

using namespace std;

PGconn *Database::conn = 0;
map<string, PGresult *> Prepare::existing_prepares;

Database::Database()
  :Backend()
{
  if (conn == 0 || PQstatus(conn) != CONNECTION_OK) {
//    cerr << "Connecting to database..." << endl;
    conn = PQconnectdb(CONN_OPTS);
    if (PQstatus(conn) != CONNECTION_OK) {
      stringstream errormsg;
      errormsg << "Unable to connect to database in save_char_obj_db: " << PQerrorMessage(conn);

      cerr << errormsg << endl;
      log(errormsg.str().c_str(), ANGEL, LOG_BUG);
      return;
    }
//  } else {
//    cerr << "Reconnecting..." << endl;
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

int Database::lookupPlayerID(const char *name)
{
  if (name == 0)
    return 0;
  
  const char * paramValues[1];
  paramValues[0] = name;
  
  PGresult *res = PQexecParams(conn, "SELECT player_id FROM player_lookup WHERE name=$1", 1, NULL, paramValues, NULL, NULL, 0);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    cerr << "Error occured: " << PQresultErrorMessage(res) << endl;
    PQclear(res);
    return 0;      
  }

  if (PQntuples(res) < 1) {
    PQclear(res);
    return 0;
  }
    
  char *buffer = PQgetvalue(res, 0, 0);    
  if (buffer == 0 || PQgetlength(res, 0, 0) < 1) {
    cerr << "Error occured getting player_id from player_lookup table." << endl;
    PQclear(res);
    return 0;
  }
  
  int player_id = atoi(buffer);    
  PQclear(res);
  return player_id;
}

int Database::createPlayerID(char *name)
{
  if (name == 0)
    return 0;
  
  // New player in database
  Prepare pl = createPrepare("player_lookup_add_player");
  pl.setTableName("player_lookup");
  pl.addCol("name", name);
  pl.exec();
  
  PGresult *res = PQexec(conn, "SELECT lastval()");
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    cerr << "Error occured: " << PQresultErrorMessage(res) << endl;
    PQclear(res);
    return 0;      
  }

  if (PQntuples(res) < 1) {
    PQclear(res);
    return 0;
  }
          
  char *buffer = PQgetvalue(res, 0, 0);
  if (buffer == 0 || PQgetlength(res, 0, 0) < 1) {
    cerr << "Error occured getting player_id from player_lookup table." << endl;
    PQclear(res);
    return 0;
  }
  
  int player_id = atoi(buffer);
  PQclear(res);
  return player_id;         
}

void Database::save(CHAR_DATA *ch, char_file_u *st)
{
  if (ch == 0 || st == 0 || IS_NPC(ch))
    return;

  if (ch->player_id == 0) {
    ch->player_id = lookupPlayerID(ch->name);
  
    if (ch->player_id == 0) {
      ch->player_id = createPlayerID(ch->name);
      
      if (ch->player_id == 0) {
        cerr << "Error creating player_id. Aborting DB save." << endl;
        return;
      }
    }    
  }

  Prepare p = createPrepare("char_file_u_update");
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
  p.addCol("weight", st->weight);
/*
  p.addCol(ubyte height;

    int16 hometown;
    uint32 gold;
    uint32 plat;
    int64 exp;
    uint32 immune;
    uint32 resist;
    uint32 suscept;

    int32 mana;        // current
    int32 raw_mana;    // max without eq/stat bonuses
    int32 hit;
    int32 raw_hit;
    int32 move;
    int32 raw_move;
    int32 ki;
    int32 raw_ki;

    int16 alignment;
   uint32 hpmetas; // Used by familiars too... why not.
   uint32 manametas;
   uint32 movemetas;

    int16 armor;       // have to save these since mobs have different bases
    int16 hitroll;
    int16 damroll;
    int32 afected_by;
    int32 afected_by2;
    uint32 misc;          // channel flags

    int16 clan; 
    int32 load_room;                  // Which room to place char in

    int32 extra_ints[5];             // available just in case
*/


  p.where("player_id", ch->player_id);
  p.exec();
  
  // If unable to UPDATE then INSERT a new entry
  if (p.lastResult) {
    char *buffer = PQcmdTuples(p.lastResult);
    if (buffer && !strcmp("0", buffer)) {
      Prepare p2 = createPrepare("char_file_u_insert");
      p2.setTableName("players");
      p2.addCol("sex", st->sex);
      p2.addCol("c_class", st->c_class);
      p2.addCol("race", st->race);
      p2.addCol("level", st->level);
      p2.addCol("raw_str", st->raw_str);
      p2.addCol("raw_intel", st->raw_intel);
      p2.addCol("raw_wis", st->raw_wis);
      p2.addCol("raw_dex", st->raw_dex);
      p2.addCol("raw_con", st->raw_con);
      p2.addCol("conditions_0", st->conditions[0]);
      p2.addCol("conditions_1", st->conditions[1]);
      p2.addCol("conditions_2", st->conditions[2]); 
      p2.addCol("player_id", ch->player_id);
      p2.exec();
    }
  }  
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
  :conn(conn), prepareID(prepareID), lastResult(0)
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

  stringstream ss;
  ss << value;  

  current_prepare.push_back(make_pair(colName, ss.str().c_str()));
}

void Prepare::addCol(string colName, char *value)
{
  if (tableName.empty())
    throw emptyName();

  if (prepareID.empty())
    throw emptyPrepareID();

  stringstream ss;
  ss << value;  

  current_prepare.push_back(make_pair(colName, ss.str().c_str()));
}

void Prepare::where(string ws, int wd)
{
  whereString = ws;
  
  stringstream ss;
  ss << wd;
  whereData = ss.str();
}

void Prepare::where(string ws, char *wd)
{
  whereString = ws;
  
  stringstream ss;
  ss << wd;
  whereData = ss.str();
}

void Prepare::exec(void)
{
  // Number of parameters in the current prepare
	int nParams = current_prepare.size();
  if (whereString.size() > 0) {
    nParams++;
  }

  // Check if prepare exists already
  PGresult *res = Prepare::existing_prepares[prepareID];

	// If not or existing one is bad, create a new one
	if ((res == 0) || (PQresultStatus(res) != PGRES_COMMAND_OK)) {
    // Perform update
    string query;
    if (whereString.size() > 0) {
      query = generateUpdateQuery();
    } else {
      query = generateInsertQuery();
    }
    
		res = PQprepare(conn, prepareID.c_str(), query.c_str(), nParams, NULL);    
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			cerr << "Error occured trying to prepare SQL: "	<< PQresultErrorMessage(res) << endl;
      PQclear(res);
			return;
		}
    
    Prepare::existing_prepares[prepareID] = res;
  }

  const char * *paramValues = new const char *[nParams];
  int i = 0;
  for(PrepareVector::iterator j=current_prepare.begin(); j != current_prepare.end(); j++) {   	
		paramValues[i++] = j->second.c_str();
  }
  
  // Perform update
  if (whereString.size() > 0) {
    paramValues[i] = whereData.c_str();    
  }
  
  if (lastResult)
    PQclear(lastResult);
  
  lastResult = res = PQexecPrepared(this->conn, prepareID.c_str(), nParams, paramValues, NULL, NULL, 0);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    cerr << "Error occured trying to prepare SQL: " << PQresultErrorMessage(res) << endl;
    delete paramValues;
	  return;
  }
  
  delete paramValues;
}

string Prepare::generateInsertQuery(void)
{
  stringstream sql;
  sql << "INSERT INTO " << tableName << " (";

  PrepareVector::iterator index;
  for (index = current_prepare.begin(); index != current_prepare.end(); index++) {
    sql << index->first;
    if (index+1 != current_prepare.end())
      sql << ", ";
  }

  sql << ") VALUES (";
  int numCols = current_prepare.size();
  for (int j=0; j < numCols; j++) {
    sql << "$" << j+1;
    if (j+1 != numCols)
      sql << ", ";
  }

  sql << ")";

  return sql.str().c_str();
}

string Prepare::generateUpdateQuery(void)
{
  stringstream sql;
  sql << "UPDATE " << tableName << " SET ";

  int j=1;
  for (PrepareVector::iterator index = current_prepare.begin(); index != current_prepare.end(); index++) {
    sql << index->first << " = $" << j++;
    if (index+1 != current_prepare.end())
      sql << ", ";
  }

  sql << " WHERE " << whereString << " = $" << j;

  return sql.str().c_str();
}

Prepare::~Prepare() {
  if (lastResult)
    PQclear(lastResult);
}

#endif
