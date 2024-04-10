#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QMap>
#include <QString>

class Table;
class Column;
class Database
{
public:
  Database(const QString &name = "", const QString &hostname = "", const QString &type = "QPSQL");
  QSqlDatabase getQSqlDatabase(void) { return database_; }
  QString getName(void) { return name_; }
  QString getHostname(void) { return hostname_; }
  QString getType(void) { return type_; }
  Table table(QString name);

  QMap<QString, Table> tables;

private:
  QString name_;
  QString hostname_;
  QString type_;
  QSqlDatabase database_;
};

class Table
{
public:
  Table(Database &database, const QString &name = "");
  Database &getDatabase(void) { return database_; }
  QString getName(void) { return name_; }
  Column column(QString name, QString type);

private:
  Database database_;
  QString name_;
};

class Column
{
public:
  Column(Table &table, const QString &name = "", const QString &type = "");
  Table &getTable(void) { return table_; }
  QString getName(void) { return name_; }
  QString getType(void) { return type_; }
  Column column(QString name, QString type);

private:
  Table table_;
  QString name_;
  QString type_;
};

#endif