#pragma once

#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QStringList>

class TagCatalogViewModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QStringList availableTags READ availableTags NOTIFY tagsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1
    };

    explicit TagCatalogViewModel(QSqlDatabase db, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QStringList availableTags() const;
    QString lastError() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool addTag(const QString &name);
    Q_INVOKABLE bool updateTag(const QString &oldName, const QString &newName);
    Q_INVOKABLE bool removeTag(const QString &name);
    Q_INVOKABLE QStringList matchingTags(const QString &query) const;

signals:
    void tagsChanged();
    void lastErrorChanged();

private:
    void setLastError(const QString &message);

    QSqlDatabase m_db;
    QStringList m_tags;
    QString m_lastError;
};
