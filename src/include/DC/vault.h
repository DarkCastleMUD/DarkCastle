#pragma once
#include <QString>
#include <QList>
#include <QMap>
#include <cstdint>
constexpr auto VAULT_UPGRADE_COST = 100; // plats
constexpr auto VAULT_BASE_SIZE = 10;     // weight
constexpr auto VAULT_MAX_SIZE = 10000;   // weight

constexpr auto VAULT_MAX_DEPWITH = 2000000000; // 2 bil max to add/remove from bank at a time

qint32 vault_log_to_string(const QString name, QString buf);
void logvault(QString message, QString name);
