#pragma once

#include <QList>
#include <QSqlDatabase>
#include <QStringList>

class MigrationRunner
{
public:
    bool run(QSqlDatabase &db);

private:
    bool ensureSchemaVersionTable(QSqlDatabase &db);
    int currentVersion(QSqlDatabase &db);
    bool setVersion(QSqlDatabase &db, int version);
    bool applyScript(QSqlDatabase &db, const QString &resourcePath);
    QList<QStringList> migrations() const;
};
