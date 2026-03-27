#include "TagCatalogViewModel.h"

#include <QSqlError>
#include <QSqlQuery>

#include <algorithm>

namespace {
QString normalizeTagName(const QString &value)
{
    return value.simplified().trimmed().toLower();
}

QStringList parseTagList(const QString &value)
{
    const QStringList rawParts = value.split(QLatin1Char(','), Qt::SkipEmptyParts);
    QStringList tags;
    for (const QString &rawPart : rawParts) {
        const QString tag = normalizeTagName(rawPart);
        if (!tag.isEmpty() && !tags.contains(tag, Qt::CaseInsensitive)) {
            tags.append(tag);
        }
    }
    return tags;
}

QString serializeTagList(const QStringList &tags)
{
    QStringList normalized;
    for (const QString &rawTag : tags) {
        const QString tag = normalizeTagName(rawTag);
        if (!tag.isEmpty() && !normalized.contains(tag, Qt::CaseInsensitive)) {
            normalized.append(tag);
        }
    }
    return normalized.join(QStringLiteral(", "));
}

bool execPrepared(QSqlQuery &query, QString *errorMessage)
{
    if (query.exec()) {
        return true;
    }
    if (errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return false;
}

bool ensureTagCatalogTable(QSqlDatabase &db, QString *errorMessage)
{
    QSqlQuery query(db);
    if (query.exec(
            "CREATE TABLE IF NOT EXISTS plant_tags_catalog ("
            "name TEXT PRIMARY KEY,"
            "created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))"
            ");")) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return false;
}
}

TagCatalogViewModel::TagCatalogViewModel(QSqlDatabase db, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(db)
{
    refresh();
}

int TagCatalogViewModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_tags.size();
}

QVariant TagCatalogViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tags.size()) {
        return {};
    }

    switch (role) {
    case NameRole:
    case Qt::DisplayRole:
        return m_tags.at(index.row());
    default:
        return {};
    }
}

QHash<int, QByteArray> TagCatalogViewModel::roleNames() const
{
    return {
        {NameRole, "name"}
    };
}

QStringList TagCatalogViewModel::availableTags() const
{
    return m_tags;
}

QString TagCatalogViewModel::lastError() const
{
    return m_lastError;
}

void TagCatalogViewModel::refresh()
{
    QString error;
    if (!ensureTagCatalogTable(m_db, &error)) {
        setLastError(error);
        return;
    }

    QSqlQuery selectPlantTags(m_db);
    if (!selectPlantTags.exec("SELECT tags FROM plants WHERE tags IS NOT NULL AND trim(tags) <> '';")) {
        setLastError(selectPlantTags.lastError().text());
        return;
    }

    QSqlQuery insertTag(m_db);
    insertTag.prepare("INSERT OR IGNORE INTO plant_tags_catalog (name) VALUES (:name);");
    while (selectPlantTags.next()) {
        const QStringList tags = parseTagList(selectPlantTags.value(0).toString());
        for (const QString &tag : tags) {
            insertTag.bindValue(QStringLiteral(":name"), tag);
            if (!execPrepared(insertTag, &error)) {
                setLastError(error);
                return;
            }
        }
    }

    QStringList tags;
    QSqlQuery query(m_db);
    if (!query.exec("SELECT name FROM plant_tags_catalog ORDER BY name COLLATE NOCASE;")) {
        setLastError(query.lastError().text());
        return;
    }

    while (query.next()) {
        const QString tag = normalizeTagName(query.value(0).toString());
        if (!tag.isEmpty()) {
            tags.append(tag);
        }
    }

    beginResetModel();
    m_tags = tags;
    endResetModel();
    setLastError(QString());
    emit tagsChanged();
}

bool TagCatalogViewModel::addTag(const QString &name)
{
    const QString normalized = normalizeTagName(name);
    if (normalized.isEmpty()) {
        setLastError(QStringLiteral("Tag name is required."));
        return false;
    }

    QString error;
    if (!ensureTagCatalogTable(m_db, &error)) {
        setLastError(error);
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT OR IGNORE INTO plant_tags_catalog (name) VALUES (:name);");
    query.bindValue(QStringLiteral(":name"), normalized);
    if (!execPrepared(query, &error)) {
        setLastError(error);
        return false;
    }

    refresh();
    return true;
}

bool TagCatalogViewModel::updateTag(const QString &oldName, const QString &newName)
{
    const QString normalizedOld = normalizeTagName(oldName);
    const QString normalizedNew = normalizeTagName(newName);
    if (normalizedOld.isEmpty() || normalizedNew.isEmpty()) {
        setLastError(QStringLiteral("Both old and new tag names are required."));
        return false;
    }

    if (normalizedOld == normalizedNew) {
        setLastError(QString());
        return true;
    }

    QString error;
    if (!ensureTagCatalogTable(m_db, &error)) {
        setLastError(error);
        return false;
    }

    if (!m_db.transaction()) {
        setLastError(m_db.lastError().text());
        return false;
    }

    QSqlQuery createNew(m_db);
    createNew.prepare("INSERT OR IGNORE INTO plant_tags_catalog (name) VALUES (:name);");
    createNew.bindValue(QStringLiteral(":name"), normalizedNew);
    if (!execPrepared(createNew, &error)) {
        m_db.rollback();
        setLastError(error);
        return false;
    }

    QSqlQuery selectPlants(m_db);
    if (!selectPlants.exec("SELECT id, tags FROM plants WHERE tags IS NOT NULL AND trim(tags) <> '';")) {
        m_db.rollback();
        setLastError(selectPlants.lastError().text());
        return false;
    }

    while (selectPlants.next()) {
        const int plantId = selectPlants.value(0).toInt();
        QStringList tags = parseTagList(selectPlants.value(1).toString());
        bool changed = false;
        for (QString &tag : tags) {
            if (QString::compare(tag, normalizedOld, Qt::CaseInsensitive) == 0) {
                tag = normalizedNew;
                changed = true;
            }
        }
        if (!changed) {
            continue;
        }

        QSqlQuery updatePlant(m_db);
        updatePlant.prepare("UPDATE plants SET tags = :tags WHERE id = :id;");
        updatePlant.bindValue(QStringLiteral(":tags"), serializeTagList(tags));
        updatePlant.bindValue(QStringLiteral(":id"), plantId);
        if (!execPrepared(updatePlant, &error)) {
            m_db.rollback();
            setLastError(error);
            return false;
        }
    }

    QSqlQuery deleteOld(m_db);
    deleteOld.prepare("DELETE FROM plant_tags_catalog WHERE lower(name) = lower(:name);");
    deleteOld.bindValue(QStringLiteral(":name"), normalizedOld);
    if (!execPrepared(deleteOld, &error)) {
        m_db.rollback();
        setLastError(error);
        return false;
    }

    if (!m_db.commit()) {
        setLastError(m_db.lastError().text());
        return false;
    }

    refresh();
    return true;
}

bool TagCatalogViewModel::removeTag(const QString &name)
{
    const QString normalized = normalizeTagName(name);
    if (normalized.isEmpty()) {
        setLastError(QStringLiteral("Tag name is required."));
        return false;
    }

    QString error;
    if (!ensureTagCatalogTable(m_db, &error)) {
        setLastError(error);
        return false;
    }

    if (!m_db.transaction()) {
        setLastError(m_db.lastError().text());
        return false;
    }

    QSqlQuery selectPlants(m_db);
    if (!selectPlants.exec("SELECT id, tags FROM plants WHERE tags IS NOT NULL AND trim(tags) <> '';")) {
        m_db.rollback();
        setLastError(selectPlants.lastError().text());
        return false;
    }

    while (selectPlants.next()) {
        const int plantId = selectPlants.value(0).toInt();
        QStringList tags = parseTagList(selectPlants.value(1).toString());
        const int beforeSize = tags.size();
        tags.erase(std::remove_if(tags.begin(), tags.end(), [&normalized](const QString &tag) {
            return QString::compare(tag, normalized, Qt::CaseInsensitive) == 0;
        }), tags.end());
        if (beforeSize == tags.size()) {
            continue;
        }

        QSqlQuery updatePlant(m_db);
        updatePlant.prepare("UPDATE plants SET tags = :tags WHERE id = :id;");
        updatePlant.bindValue(QStringLiteral(":tags"), serializeTagList(tags));
        updatePlant.bindValue(QStringLiteral(":id"), plantId);
        if (!execPrepared(updatePlant, &error)) {
            m_db.rollback();
            setLastError(error);
            return false;
        }
    }

    QSqlQuery deleteQuery(m_db);
    deleteQuery.prepare("DELETE FROM plant_tags_catalog WHERE lower(name) = lower(:name);");
    deleteQuery.bindValue(QStringLiteral(":name"), normalized);
    if (!execPrepared(deleteQuery, &error)) {
        m_db.rollback();
        setLastError(error);
        return false;
    }

    if (!m_db.commit()) {
        setLastError(m_db.lastError().text());
        return false;
    }

    refresh();
    return true;
}

QStringList TagCatalogViewModel::matchingTags(const QString &query) const
{
    const QString normalizedQuery = normalizeTagName(query);
    if (normalizedQuery.isEmpty()) {
        return m_tags;
    }

    QStringList matches;
    for (const QString &tag : m_tags) {
        if (tag.contains(normalizedQuery, Qt::CaseInsensitive)) {
            matches.append(tag);
        }
    }
    return matches;
}

void TagCatalogViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}
