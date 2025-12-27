#include "ReminderListViewModel.h"

#include <QDate>

ReminderListViewModel::ReminderListViewModel(QSqlDatabase db, QObject *parent)
    : QAbstractListModel(parent)
    , m_repository(db)
{
    refresh();
}

int ReminderListViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_reminders.size();
}

QVariant ReminderListViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_reminders.size()) {
        return {};
    }

    const Reminder &reminder = m_reminders.at(index.row());
    switch (role) {
    case IdRole:
        return reminder.id;
    case CustomNameRole:
        return reminder.customName;
    case TaskTypeRole:
        return reminder.taskType;
    case ScheduleTypeRole:
        return reminder.scheduleType;
    case IntervalDaysRole:
        return reminder.intervalDays;
    case StartDateRole:
        return reminder.startDate.isValid() ? reminder.startDate.toString(Qt::ISODate) : QString();
    case NextDueDateRole:
        return reminder.nextDueDate.isValid() ? reminder.nextDueDate.toString(Qt::ISODate) : QString();
    case NotesRole:
        return reminder.notes;
    case PlantIdsRole: {
        QVariantList list;
        const QVector<int> ids = m_reminderPlantIds.value(reminder.id);
        for (int id : ids) {
            list.append(id);
        }
        return list;
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> ReminderListViewModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {CustomNameRole, "customName"},
        {TaskTypeRole, "taskType"},
        {ScheduleTypeRole, "scheduleType"},
        {IntervalDaysRole, "intervalDays"},
        {StartDateRole, "startDate"},
        {NextDueDateRole, "nextDueDate"},
        {NotesRole, "notes"},
        {PlantIdsRole, "plantIds"}
    };
}

void ReminderListViewModel::refresh()
{
    beginResetModel();
    m_reminders = m_repository.listAll();
    m_reminderPlantIds.clear();
    for (const Reminder &reminder : m_reminders) {
        m_reminderPlantIds.insert(reminder.id, m_repository.plantIdsForReminder(reminder.id));
    }
    endResetModel();
    setLastError(QString());
}

int ReminderListViewModel::addReminder(const QVariantMap &data, const QVariantList &plantIds)
{
    if (!validateInput(data, plantIds)) {
        return 0;
    }

    Reminder reminder;
    reminder.customName = data.value("customName").toString();
    reminder.taskType = data.value("taskType").toString();
    reminder.scheduleType = data.value("scheduleType").toString();
    reminder.intervalDays = data.value("intervalDays").toInt();
    reminder.startDate = QDate::fromString(data.value("startDate").toString(), Qt::ISODate);
    reminder.nextDueDate = computeNextDueDate(data);
    reminder.notes = data.value("notes").toString();

    const QVector<int> ids = toIntList(plantIds);
    const int id = m_repository.create(reminder, ids);
    if (id <= 0) {
        setLastError(QStringLiteral("Failed to add reminder."));
        return 0;
    }

    reminder.id = id;
    beginInsertRows(QModelIndex(), m_reminders.size(), m_reminders.size());
    m_reminders.append(reminder);
    endInsertRows();
    m_reminderPlantIds.insert(id, ids);
    setLastError(QString());
    return id;
}

bool ReminderListViewModel::updateReminder(int id, const QVariantMap &data, const QVariantList &plantIds)
{
    if (id <= 0) {
        setLastError(QStringLiteral("Select a reminder to update."));
        return false;
    }

    if (!validateInput(data, plantIds)) {
        return false;
    }

    Reminder reminder;
    reminder.id = id;
    reminder.customName = data.value("customName").toString();
    reminder.taskType = data.value("taskType").toString();
    reminder.scheduleType = data.value("scheduleType").toString();
    reminder.intervalDays = data.value("intervalDays").toInt();
    reminder.startDate = QDate::fromString(data.value("startDate").toString(), Qt::ISODate);
    reminder.nextDueDate = computeNextDueDate(data);
    reminder.notes = data.value("notes").toString();

    const QVector<int> ids = toIntList(plantIds);
    if (!m_repository.update(reminder, ids)) {
        setLastError(QStringLiteral("Failed to update reminder."));
        return false;
    }

    for (int i = 0; i < m_reminders.size(); ++i) {
        if (m_reminders[i].id == id) {
            m_reminders[i] = reminder;
            m_reminderPlantIds.insert(id, ids);
            const QModelIndex modelIndex = index(i);
            emit dataChanged(modelIndex, modelIndex);
            break;
        }
    }

    setLastError(QString());
    return true;
}

bool ReminderListViewModel::removeReminder(int id)
{
    if (id <= 0) {
        setLastError(QStringLiteral("Select a reminder to delete."));
        return false;
    }

    if (!m_repository.remove(id)) {
        setLastError(QStringLiteral("Failed to delete reminder."));
        return false;
    }

    for (int i = 0; i < m_reminders.size(); ++i) {
        if (m_reminders[i].id == id) {
            beginRemoveRows(QModelIndex(), i, i);
            m_reminders.removeAt(i);
            endRemoveRows();
            break;
        }
    }
    m_reminderPlantIds.remove(id);
    setLastError(QString());
    return true;
}

QString ReminderListViewModel::lastError() const
{
    return m_lastError;
}

bool ReminderListViewModel::validateInput(const QVariantMap &data, const QVariantList &plantIds)
{
    const QString taskType = data.value("taskType").toString().trimmed();
    const QString scheduleType = data.value("scheduleType").toString().trimmed();
    const QString startDate = data.value("startDate").toString().trimmed();
    const QString nextDueDate = data.value("nextDueDate").toString().trimmed();
    const int intervalDays = data.value("intervalDays").toInt();

    if (taskType.isEmpty()) {
        setLastError(QStringLiteral("Task type is required."));
        return false;
    }
    if (scheduleType.isEmpty()) {
        setLastError(QStringLiteral("Schedule type is required."));
        return false;
    }
    if (plantIds.isEmpty()) {
        setLastError(QStringLiteral("Select at least one plant."));
        return false;
    }

    if (scheduleType == "interval") {
        if (intervalDays <= 0) {
            setLastError(QStringLiteral("Interval days must be > 0."));
            return false;
        }
        if (!startDate.isEmpty()) {
            if (!QDate::fromString(startDate, Qt::ISODate).isValid()) {
                setLastError(QStringLiteral("Start date must be YYYY-MM-DD."));
                return false;
            }
        }
    } else if (scheduleType == "fixed") {
        if (nextDueDate.isEmpty()) {
            setLastError(QStringLiteral("Next due date is required."));
            return false;
        }
        if (!QDate::fromString(nextDueDate, Qt::ISODate).isValid()) {
            setLastError(QStringLiteral("Next due date must be YYYY-MM-DD."));
            return false;
        }
    }

    setLastError(QString());
    return true;
}

QDate ReminderListViewModel::computeNextDueDate(const QVariantMap &data) const
{
    const QString scheduleType = data.value("scheduleType").toString().trimmed();
    if (scheduleType == "interval") {
        const int intervalDays = data.value("intervalDays").toInt();
        const QString startDate = data.value("startDate").toString().trimmed();
        QDate base = QDate::fromString(startDate, Qt::ISODate);
        if (!base.isValid()) {
            base = QDate::currentDate();
        }
        return base.addDays(intervalDays);
    }

    const QString nextDueDate = data.value("nextDueDate").toString().trimmed();
    return QDate::fromString(nextDueDate, Qt::ISODate);
}

void ReminderListViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}

QVector<int> ReminderListViewModel::toIntList(const QVariantList &ids) const
{
    QVector<int> result;
    for (const QVariant &value : ids) {
        const int id = value.toInt();
        if (id > 0) {
            result.append(id);
        }
    }
    return result;
}
