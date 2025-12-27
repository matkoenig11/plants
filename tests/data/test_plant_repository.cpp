#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "PlantRepository.h"

class PlantRepositoryTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void createAndList();

private:
    QSqlDatabase m_db;
};

void PlantRepositoryTest::initTestCase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "plants_test");
    m_db.setDatabaseName(":memory:");
    QVERIFY(m_db.open());

    QSqlQuery query(m_db);
    QVERIFY(query.exec(
        "CREATE TABLE plants ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "scientific_name TEXT,"
        "plant_type TEXT,"
        "winter_location TEXT,"
        "summer_location TEXT,"
        "light_requirement TEXT,"
        "watering_frequency TEXT,"
        "watering_notes TEXT,"
        "humidity_preference TEXT,"
        "soil_type TEXT,"
        "pot_size TEXT,"
        "last_watered TEXT,"
        "fertilizing_schedule TEXT,"
        "last_fertilized TEXT,"
        "pruning_time TEXT,"
        "pruning_notes TEXT,"
        "last_pruned TEXT,"
        "growth_rate TEXT,"
        "current_health_status TEXT,"
        "issues_pests TEXT,"
        "temperature_tolerance TEXT,"
        "toxic_to_pets TEXT,"
        "acquired_on TEXT,"
        "source TEXT,"
        "notes TEXT,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "updated_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ");"
    ));
}

void PlantRepositoryTest::createAndList()
{
    PlantRepository repo(m_db);
    Plant plant;
    plant.name = "Monstera";
    plant.scientificName = "Monstera deliciosa";
    plant.winterLocation = "Indoors";
    plant.acquiredDate = QDate(2024, 5, 1);
    plant.notes = "Test";

    const int id = repo.create(plant);
    QVERIFY(id > 0);

    const QVector<Plant> plants = repo.listAll();
    QCOMPARE(plants.size(), 1);
    QCOMPARE(plants.first().name, QString("Monstera"));
    QCOMPARE(plants.first().scientificName, QString("Monstera deliciosa"));
    QCOMPARE(plants.first().winterLocation, QString("Indoors"));
}

QTEST_MAIN(PlantRepositoryTest)
#include "test_plant_repository.moc"
