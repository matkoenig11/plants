#pragma once

#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QVector>

#include "Reminder.h"
#include "ReminderRepository.h"

class ReminderListViewModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    enum ReminderRoles {
        IdRole = Qt::UserRole + 1,
        CustomNameRole,
        TaskTypeRole,
        ScheduleTypeRole,
        IntervalDaysRole,
        StartDateRole,
        NextDueDateRole,
        NotesRole,
        PlantIdsRole
    };

    explicit ReminderListViewModel(QSqlDatabase db, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE int addReminder(const QVariantMap &data, const QVariantList &plantIds);
    Q_INVOKABLE bool updateReminder(int id, const QVariantMap &data, const QVariantList &plantIds);
    Q_INVOKABLE bool removeReminder(int id);

    QString lastError() const;

signals:
    void lastErrorChanged();

private:
    bool validateInput(const QVariantMap &data, const QVariantList &plantIds);
    QDate computeNextDueDate(const QVariantMap &data) const;
    void setLastError(const QString &message);
    QVector<int> toIntList(const QVariantList &ids) const;

    ReminderRepository m_repository;
    QVector<Reminder> m_reminders;
    QHash<int, QVector<int>> m_reminderPlantIds;
    QString m_lastError;
};
