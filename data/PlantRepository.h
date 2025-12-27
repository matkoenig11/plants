#pragma once

#include <QSqlDatabase>
#include <QVector>

#include "Plant.h"

class QSqlQuery;

class PlantRepository
{
public:
    explicit PlantRepository(QSqlDatabase db);

    int create(const Plant &plant);
    bool update(const Plant &plant);
    bool remove(int id);
    Plant findById(int id);
    QVector<Plant> listAll();

private:
    Plant mapRow(const QSqlQuery &query) const;

    QSqlDatabase m_db;
};
