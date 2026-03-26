#include <QtTest/QtTest>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUrl>

#include "PlantListViewModel.h"

namespace {
bool createPlantsTable(QSqlDatabase &db)
{
    QSqlQuery query(db);
    return query.exec(
        "CREATE TABLE plants ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "species TEXT,"
        "location TEXT,"
        "scientific_name TEXT,"
        "plant_type TEXT,"
        "light_requirement TEXT,"
        "watering_frequency TEXT,"
        "watering_notes TEXT,"
        "humidity_preference TEXT,"
        "soil_type TEXT,"
        "last_watered TEXT,"
        "fertilizing_schedule TEXT,"
        "last_fertilized TEXT,"
        "pruning_time TEXT,"
        "pruning_notes TEXT,"
        "last_pruned TEXT,"
        "growth_rate TEXT,"
        "issues_pests TEXT,"
        "temperature_tolerance TEXT,"
        "toxic_to_pets TEXT,"
        "poisonous_to_humans TEXT,"
        "poisonous_to_pets TEXT,"
        "indoor TEXT,"
        "flowering_season TEXT,"
        "acquired_on TEXT,"
        "source TEXT,"
        "notes TEXT,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "updated_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ");"
    );
}

bool createPlantImagesTable(QSqlDatabase &db)
{
    QSqlQuery query(db);
    return query.exec(
        "CREATE TABLE plant_images ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "plant_id INTEGER NOT NULL,"
        "file_path TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "FOREIGN KEY (plant_id) REFERENCES plants(id) ON DELETE CASCADE,"
        "UNIQUE (plant_id, file_path)"
        ");"
    );
}

bool createJournalEntriesTable(QSqlDatabase &db)
{
    QSqlQuery query(db);
    return query.exec(
        "CREATE TABLE journal_entries ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "plant_id INTEGER NOT NULL,"
        "entry_type TEXT NOT NULL,"
        "entry_date TEXT NOT NULL,"
        "notes TEXT,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ");"
    );
}

bool createPlantCareSchedulesTable(QSqlDatabase &db)
{
    QSqlQuery query(db);
    return query.exec(
        "CREATE TABLE plant_care_schedules ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "plant_id INTEGER NOT NULL,"
        "care_type TEXT NOT NULL,"
        "season_name TEXT NOT NULL,"
        "interval_days INTEGER,"
        "is_enabled INTEGER NOT NULL DEFAULT 1,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "updated_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "UNIQUE (plant_id, care_type, season_name)"
        ");"
    );
}
}

class PlantListViewModelTest : public QObject
{
    Q_OBJECT

private slots:
    void addPlantCreatesSqlRow();
    void addPlantImageStoresSqlRow();
    void saveAndLoadCareSchedule();
    void journalEntriesBackfillLastCareDates();
    void importSqliteAddsAllPlants();
};

void PlantListViewModelTest::addPlantCreatesSqlRow()
{
    const QString connectionName = QStringLiteral("plant_list_view_model_add_test");

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QVERIFY(createPlantsTable(db));
        QVERIFY(createJournalEntriesTable(db));

        PlantListViewModel viewModel(db);
        QVariantMap data;
        data.insert(QStringLiteral("name"), QStringLiteral("Monstera"));
        data.insert(QStringLiteral("scientificName"), QStringLiteral("Monstera deliciosa"));
        data.insert(QStringLiteral("poisonousToPets"), QStringLiteral("Yes"));
        data.insert(QStringLiteral("indoor"), QStringLiteral("Yes"));
        data.insert(QStringLiteral("acquiredDate"), QStringLiteral("2024-05-01"));
        data.insert(QStringLiteral("notes"), QStringLiteral("Test plant"));

        const int id = viewModel.addPlant(data);
        QVERIFY(id > 0);
        QCOMPARE(viewModel.rowCount(), 1);

        QSqlQuery query(db);
        QVERIFY(query.exec("SELECT name, scientific_name, poisonous_to_pets, indoor FROM plants;"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("Monstera"));
        QCOMPARE(query.value(1).toString(), QStringLiteral("Monstera deliciosa"));
        QCOMPARE(query.value(2).toString(), QStringLiteral("Yes"));
        QCOMPARE(query.value(3).toString(), QStringLiteral("Yes"));

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

void PlantListViewModelTest::addPlantImageStoresSqlRow()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString imagePath = tempDir.filePath(QStringLiteral("leaf.jpg"));
    QFile imageFile(imagePath);
    QVERIFY(imageFile.open(QIODevice::WriteOnly));
    imageFile.write("not-a-real-image-but-good-enough-for-path-tests");
    imageFile.close();

    const QString connectionName = QStringLiteral("plant_list_view_model_image_test");

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QVERIFY(createPlantsTable(db));
        QVERIFY(createJournalEntriesTable(db));
        QVERIFY(createPlantImagesTable(db));

        PlantListViewModel viewModel(db);
        QVariantMap data;
        data.insert(QStringLiteral("name"), QStringLiteral("Ficus"));

        const int plantId = viewModel.addPlant(data);
        QVERIFY(plantId > 0);
        QVERIFY2(viewModel.addPlantImage(plantId, QUrl::fromLocalFile(imagePath).toString()),
                 qPrintable(viewModel.lastError()));

        const QVariantList images = viewModel.plantImages(plantId);
        QCOMPARE(images.size(), 1);
        QCOMPARE(images.first().toMap().value(QStringLiteral("path")).toString(),
                 QFileInfo(imagePath).absoluteFilePath());

        QSqlQuery query(db);
        QVERIFY(query.exec("SELECT plant_id, file_path FROM plant_images;"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), plantId);
        QCOMPARE(query.value(1).toString(), QFileInfo(imagePath).absoluteFilePath());

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

void PlantListViewModelTest::saveAndLoadCareSchedule()
{
    const QString connectionName = QStringLiteral("plant_list_view_model_care_schedule_test");

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QVERIFY(createPlantsTable(db));
        QVERIFY(createJournalEntriesTable(db));
        QVERIFY(createPlantCareSchedulesTable(db));

        PlantListViewModel viewModel(db);
        QVariantMap plantData;
        plantData.insert(QStringLiteral("name"), QStringLiteral("Calathea"));
        const int plantId = viewModel.addPlant(plantData);
        QVERIFY(plantId > 0);

        QVariantMap winterWater;
        winterWater.insert(QStringLiteral("useOverride"), true);
        winterWater.insert(QStringLiteral("enabled"), false);
        winterWater.insert(QStringLiteral("intervalDays"), 0);

        QVariantMap summerWater;
        summerWater.insert(QStringLiteral("useOverride"), true);
        summerWater.insert(QStringLiteral("enabled"), true);
        summerWater.insert(QStringLiteral("intervalDays"), 4);

        QVariantMap waterSeasonal;
        waterSeasonal.insert(QStringLiteral("winter"), winterWater);
        waterSeasonal.insert(QStringLiteral("summer"), summerWater);

        QVariantMap water;
        water.insert(QStringLiteral("defaultIntervalDays"), 7);
        water.insert(QStringLiteral("seasonal"), waterSeasonal);

        QVariantMap winterFertilize;
        winterFertilize.insert(QStringLiteral("useOverride"), true);
        winterFertilize.insert(QStringLiteral("enabled"), false);
        winterFertilize.insert(QStringLiteral("intervalDays"), 0);

        QVariantMap fertilizeSeasonal;
        fertilizeSeasonal.insert(QStringLiteral("winter"), winterFertilize);

        QVariantMap fertilize;
        fertilize.insert(QStringLiteral("defaultIntervalDays"), 14);
        fertilize.insert(QStringLiteral("seasonal"), fertilizeSeasonal);

        QVariantMap schedule;
        schedule.insert(QStringLiteral("water"), water);
        schedule.insert(QStringLiteral("fertilize"), fertilize);

        QVERIFY2(viewModel.saveCareSchedule(plantId, schedule), qPrintable(viewModel.lastError()));

        const QVariantMap loaded = viewModel.careSchedule(plantId);
        const QVariantMap loadedWater = loaded.value(QStringLiteral("water")).toMap();
        const QVariantMap loadedWaterSeasonal = loadedWater.value(QStringLiteral("seasonal")).toMap();
        const QVariantMap loadedSummerWater = loadedWaterSeasonal.value(QStringLiteral("summer")).toMap();
        const QVariantMap loadedWinterWater = loadedWaterSeasonal.value(QStringLiteral("winter")).toMap();

        QCOMPARE(loadedWater.value(QStringLiteral("defaultIntervalDays")).toInt(), 7);
        QVERIFY(loadedSummerWater.value(QStringLiteral("useOverride")).toBool());
        QCOMPARE(loadedSummerWater.value(QStringLiteral("intervalDays")).toInt(), 4);
        QVERIFY(loadedWinterWater.value(QStringLiteral("useOverride")).toBool());
        QVERIFY(!loadedWinterWater.value(QStringLiteral("enabled")).toBool());

        QSqlQuery query(db);
        QVERIFY(query.exec("SELECT watering_frequency, fertilizing_schedule FROM plants WHERE id = 1;"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("Every 7 days; seasonal overrides"));
        QCOMPARE(query.value(1).toString(), QStringLiteral("Every 14 days; seasonal overrides"));

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

void PlantListViewModelTest::journalEntriesBackfillLastCareDates()
{
    const QString connectionName = QStringLiteral("plant_list_view_model_journal_backfill_test");

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QVERIFY(createPlantsTable(db));
        QVERIFY(createJournalEntriesTable(db));
        QVERIFY(createPlantCareSchedulesTable(db));

        QSqlQuery insertPlant(db);
        QVERIFY(insertPlant.exec(
            "INSERT INTO plants (name, last_watered, last_fertilized) "
            "VALUES ('Orchid', '', '');"
        ));
        const int plantId = insertPlant.lastInsertId().toInt();
        QVERIFY(plantId > 0);

        QSqlQuery insertJournal(db);
        QVERIFY(insertJournal.exec(QString(
            "INSERT INTO journal_entries (plant_id, entry_type, entry_date, notes) VALUES "
            "(%1, 'Water', '2026-03-20', 'Watered orchid'),"
            "(%1, 'Fertilize', '2026-03-22', 'Fed orchid');").arg(plantId)));

        PlantListViewModel viewModel(db);
        QCOMPARE(viewModel.rowCount(), 1);
        QCOMPARE(viewModel.data(viewModel.index(0), PlantListViewModel::LastWateredRole).toString(),
                 QStringLiteral("2026-03-20"));
        QCOMPARE(viewModel.data(viewModel.index(0), PlantListViewModel::LastFertilizedRole).toString(),
                 QStringLiteral("2026-03-22"));

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

void PlantListViewModelTest::importSqliteAddsAllPlants()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString targetConnectionName = QStringLiteral("plant_sqlite_target_test");
    const QString sourceConnectionName = QStringLiteral("plant_sqlite_source_test");
    const QString sourceDatabasePath = tempDir.filePath(QStringLiteral("import.sqlite"));

    {
        QSqlDatabase sourceDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), sourceConnectionName);
        sourceDb.setDatabaseName(sourceDatabasePath);
        QVERIFY(sourceDb.open());
        QVERIFY(createPlantsTable(sourceDb));
        QVERIFY(createJournalEntriesTable(sourceDb));
        QVERIFY(createPlantCareSchedulesTable(sourceDb));

        QSqlQuery sourceQuery(sourceDb);
        QVERIFY(sourceQuery.exec(
            "INSERT INTO plants ("
            "name, scientific_name, plant_type, "
            "light_requirement, watering_frequency, humidity_preference, "
            "poisonous_to_pets, indoor, flowering_season, "
            "source, notes"
            ") VALUES ("
            "'SQLite Monstera', 'Monstera deliciosa', 'Houseplant', "
            "'Bright indirect', 'Weekly', 'Medium', 'Yes', 'Yes', 'Summer', 'SQLite import', 'Imported from sqlite'"
            ");"
        ));
        sourceDb.close();
    }
    QSqlDatabase::removeDatabase(sourceConnectionName);

    {
        QSqlDatabase targetDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), targetConnectionName);
        targetDb.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(targetDb.open());
        QVERIFY(createPlantsTable(targetDb));
        QVERIFY(createJournalEntriesTable(targetDb));
        QVERIFY(createPlantCareSchedulesTable(targetDb));

        PlantListViewModel viewModel(targetDb);
        QVERIFY2(viewModel.importFromSqlite(QUrl::fromLocalFile(sourceDatabasePath).toString()),
                 qPrintable(viewModel.lastError()));

        QCOMPARE(viewModel.rowCount(), 1);
        QCOMPARE(viewModel.data(viewModel.index(0), PlantListViewModel::NameRole).toString(),
                 QStringLiteral("SQLite Monstera"));
        QCOMPARE(viewModel.data(viewModel.index(0), PlantListViewModel::SourceRole).toString(),
                 QStringLiteral("SQLite import"));

        QSqlQuery targetQuery(targetDb);
        QVERIFY(targetQuery.exec("SELECT COUNT(*), MIN(name) FROM plants;"));
        QVERIFY(targetQuery.next());
        QCOMPARE(targetQuery.value(0).toInt(), 1);
        QCOMPARE(targetQuery.value(1).toString(), QStringLiteral("SQLite Monstera"));

        targetDb.close();
    }
    QSqlDatabase::removeDatabase(targetConnectionName);
}

QTEST_MAIN(PlantListViewModelTest)
#include "test_plant_list_view_model.moc"
