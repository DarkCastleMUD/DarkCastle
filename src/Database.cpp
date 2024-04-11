#include "Database.h"
#include "utility.h" // logentry
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

Database::Database(const QString &name, const QString &hostname, const QString &type)
    : name_(name), hostname_(hostname), type_(type)
{
  if (QSqlDatabase::contains("qt_sql_default_connection"))
  {
    database_ = QSqlDatabase::database("qt_sql_default_connection");
    database_.close();
  }
  else
  {
    database_ = QSqlDatabase::addDatabase(type);
  }

  if (!hostname.isEmpty())
  {
    database_.setHostName(hostname);
  }

  if (!name.isEmpty())
  {
    database_.setDatabaseName(name);
  }

  if (!database_.open())
  {
    logentry(QStringLiteral("Failed to open database %1.").arg(name));
    qWarning() << database_ << database_.isOpen();
    logentry(database_.lastError().databaseText());
    logentry(database_.lastError().driverText());
  }
}

Table Database::table(QString name)
{
  if (!tables.contains(name))
  {
    Table table(*this, name);
    tables.insert(name, table);
  }

  return tables.value(name, Table(*this, name));
}

Table::Table(Database &database, const QString &name)
    : database_(database), name_(name)
{
  auto db = database.getQSqlDatabase();
  if (db.isOpen())
  {
    if (!db.tables().contains(name))
    {
      QSqlQuery query;
      if (query.exec(QString("CREATE TABLE %1 (id BIGINT GENERATED ALWAYS AS IDENTITY)").arg(name)))
      {
        logentry(QStringLiteral("Created database table %1.").arg(name));
        if (!db.tables().contains(name))
        {
          logentry(QStringLiteral("Failed to find database table %1 after creating it.").arg(name));
          return;
        }
      }
      else
      {
        logentry(QStringLiteral("Failed to create database table %1.").arg(name));
        logentry(query.lastError().databaseText());
        logentry(query.lastError().driverText());
      }
    }
  }
}
Column Table::column(QString name, QString type)
{
  return Column(*this, name, type);
}

Column::Column(Table &table, const QString &name, const QString &type)
    : table_(table), name_(name), type_(type)
{
  QSqlRecord table_fields = table.getDatabase().getQSqlDatabase().record(table.getName());
  if (!table_fields.contains(name))
  {
    QSqlQuery query;
    if (query.exec(QStringLiteral("ALTER TABLE %1 ADD %2 %3").arg(table.getName()).arg(name).arg(type)))
    {
      logentry(QStringLiteral("Created table %1 column %2.").arg(table.getName()).arg(name));

      // Check again
      QSqlRecord table_fields = table.getDatabase().getQSqlDatabase().record(table.getName());
      if (!table_fields.contains(name))
      {
        logentry(QStringLiteral("Failed to find table %1 column %2 after creating it.").arg(table.getName()).arg(name));
        return;
      }
    }
    else
    {
      logentry(QStringLiteral("Failed to create table %1 column %2.").arg(table.getName()).arg(name));
      logentry(query.lastError().databaseText());
      logentry(query.lastError().driverText());
    }
  }
}

Column Column::column(QString name, QString type)
{
  return Column(table_, name, type);
}