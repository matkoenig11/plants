#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class PlantLibraryViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(QString detailText READ detailText NOTIFY detailTextChanged)
    Q_PROPERTY(QVariantMap selectedPlantData READ selectedPlantData NOTIFY selectedPlantDataChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit PlantLibraryViewModel(QObject *parent = nullptr);

    QString token() const;
    QVariantList results() const;
    QString detailText() const;
    QVariantMap selectedPlantData() const;
    QString lastError() const;
    bool busy() const;

    void setToken(const QString &value);

    Q_INVOKABLE bool search(const QString &query);
    Q_INVOKABLE bool selectResult(int index);
    Q_INVOKABLE void clear();

signals:
    void tokenChanged();
    void resultsChanged();
    void detailTextChanged();
    void selectedPlantDataChanged();
    void lastErrorChanged();
    void busyChanged();

private:
    void setLastError(const QString &message);
    void setDetailText(const QString &value);
    void setSelectedPlantData(const QVariantMap &value);
    void setBusy(bool value);

    QString m_token;
    QVariantList m_results;
    QString m_detailText;
    QVariantMap m_selectedPlantData;
    QString m_lastError;
    bool m_busy = false;
};
