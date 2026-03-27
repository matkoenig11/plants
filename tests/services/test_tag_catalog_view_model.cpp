#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "TagCatalogViewModel.h"

namespace {
bool createPlantsTable(QSqlDatabase &db)
{
    QSqlQuery query(db);
    return query.exec(
        "CREATE TABLE plants ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "tags TEXT"
        ");");
}
}

class TagCatalogViewModelTest : public QObject
{
    Q_OBJECT

private slots:
    void refresh_discoversTagsFromPlants();
    void updateTag_renamesPlants();
    void removeTag_updatesPlants();
};

void TagCatalogViewModelTest::refresh_discoversTagsFromPlants()
{
    const QString connectionName = QStringLiteral("tag_catalog_refresh_test");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QVERIFY(createPlantsTable(db));

        QSqlQuery query(db);
        QVERIFY(query.exec("INSERT INTO plants (name, tags) VALUES ('Basil', 'indoor, herbs');"));

        TagCatalogViewModel viewModel(db);
        QVERIFY(viewModel.availableTags().contains(QStringLiteral("indoor")));
        QVERIFY(viewModel.availableTags().contains(QStringLiteral("herbs")));
        QCOMPARE(viewModel.rowCount(), 2);

        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

void TagCatalogViewModelTest::updateTag_renamesPlants()
{
    const QString connectionName = QStringLiteral("tag_catalog_update_test");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QVERIFY(createPlantsTable(db));

        QSqlQuery query(db);
        QVERIFY(query.exec("INSERT INTO plants (name, tags) VALUES ('Rose', 'outdoor, flowers');"));

        TagCatalogViewModel viewModel(db);
        QVERIFY(viewModel.updateTag(QStringLiteral("flowers"), QStringLiteral("blooms")));

        QVERIFY(query.exec("SELECT tags FROM plants WHERE name = 'Rose';"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("outdoor, blooms"));

        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

void TagCatalogViewModelTest::removeTag_updatesPlants()
{
    const QString connectionName = QStringLiteral("tag_catalog_remove_test");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());
        QVERIFY(createPlantsTable(db));

        QSqlQuery query(db);
        QVERIFY(query.exec("INSERT INTO plants (name, tags) VALUES ('Apple', 'fruits, outdoor');"));

        TagCatalogViewModel viewModel(db);
        QVERIFY(viewModel.removeTag(QStringLiteral("fruits")));

        QVERIFY(query.exec("SELECT tags FROM plants WHERE name = 'Apple';"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("outdoor"));

        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

QTEST_MAIN(TagCatalogViewModelTest)
#include "test_tag_catalog_view_model.moc"
