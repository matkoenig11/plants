#include "PlantLibraryViewModel.h"

#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {
const QString kPerenualBaseUrl = QStringLiteral("https://perenual.com/api/v2");
const QString kPerenualCareGuideBaseUrl = QStringLiteral("https://perenual.com/api");

QString readLocalEnvToken()
{
    const QString envValue = qEnvironmentVariable("PERENUAL_TOKEN").trimmed();
    if (!envValue.isEmpty()) {
        return envValue;
    }

    QFile envFile(QStringLiteral("tests/API/.env"));
    if (!envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    while (!envFile.atEnd()) {
        const QString line = QString::fromUtf8(envFile.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const int separator = line.indexOf(QLatin1Char('='));
        if (separator <= 0) {
            continue;
        }
        const QString key = line.left(separator).trimmed();
        const QString value = line.mid(separator + 1).trimmed();
        if (key == QStringLiteral("PERENUAL_TOKEN")) {
            return value;
        }
    }

    return {};
}

bool executeGetJson(const QUrl &url, QJsonDocument *document, QString *errorMessage)
{
    if (!url.isValid()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Library URL is invalid.");
        }
        return false;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkAccessManager manager;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
    timeout.start(30000);
    loop.exec();
    timeout.stop();

    const QByteArray payload = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkMessage = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError && statusCode == 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Library request failed: %1").arg(networkMessage);
        }
        return false;
    }

    if (statusCode < 200 || statusCode >= 300) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Library request returned HTTP %1.").arg(statusCode);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Library returned invalid JSON.");
        }
        return false;
    }

    if (document) {
        *document = json;
    }
    return true;
}

QString firstString(const QJsonValue &value)
{
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &item : array) {
            if (item.isString() && !item.toString().trimmed().isEmpty()) {
                return item.toString().trimmed();
            }
        }
        return {};
    }

    if (value.isString()) {
        return value.toString().trimmed();
    }

    return {};
}

QString joinStringArray(const QJsonValue &value)
{
    if (value.isArray()) {
        QStringList items;
        const QJsonArray array = value.toArray();
        for (const QJsonValue &item : array) {
            const QString text = firstString(item);
            if (!text.isEmpty()) {
                items.append(text);
            }
        }
        items.removeDuplicates();
        return items.join(QStringLiteral(", "));
    }

    return firstString(value);
}

QString boolToText(const QJsonValue &value)
{
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("Yes") : QStringLiteral("No");
    }

    const QString text = firstString(value);
    if (text.isEmpty()) {
        return {};
    }
    return text;
}

QString hardinessText(const QJsonObject &detailObject)
{
    const QJsonObject hardiness = detailObject.value(QStringLiteral("hardiness")).toObject();
    if (hardiness.isEmpty()) {
        return {};
    }

    const QString minValue = hardiness.value(QStringLiteral("min")).toString().trimmed();
    const QString maxValue = hardiness.value(QStringLiteral("max")).toString().trimmed();
    if (minValue.isEmpty() && maxValue.isEmpty()) {
        return {};
    }
    if (!minValue.isEmpty() && !maxValue.isEmpty()) {
        return QStringLiteral("USDA zones %1-%2").arg(minValue, maxValue);
    }
    return QStringLiteral("USDA zone %1").arg(!minValue.isEmpty() ? minValue : maxValue);
}

QString wateringBenchmarkText(const QJsonObject &detailObject)
{
    const QJsonObject benchmark = detailObject.value(QStringLiteral("watering_general_benchmark")).toObject();
    if (benchmark.isEmpty()) {
        return {};
    }

    QString value = benchmark.value(QStringLiteral("value")).toString().trimmed();
    value.remove(QLatin1Char('"'));
    const QString unit = benchmark.value(QStringLiteral("unit")).toString().trimmed();
    return QStringLiteral("%1 %2").arg(value, unit).trimmed();
}

QString careGuideText(const QJsonObject &careGuideObject, const QString &type)
{
    const QString normalizedType = type.trimmed().toLower();
    if (normalizedType.isEmpty()) {
        return {};
    }

    const QJsonArray sections = careGuideObject.value(QStringLiteral("data")).toArray();
    for (const QJsonValue &entryValue : sections) {
        const QJsonObject entryObject = entryValue.toObject();
        const QJsonArray sectionArray = entryObject.value(QStringLiteral("section")).toArray();
        for (const QJsonValue &sectionValue : sectionArray) {
            const QJsonObject sectionObject = sectionValue.toObject();
            if (sectionObject.value(QStringLiteral("type")).toString().trimmed().toLower() == normalizedType) {
                return sectionObject.value(QStringLiteral("description")).toString().trimmed();
            }
        }
    }

    return {};
}

QString combinedText(const QStringList &parts)
{
    QStringList nonEmptyParts;
    for (const QString &part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            nonEmptyParts.append(trimmed);
        }
    }
    return nonEmptyParts.join(QStringLiteral("\n\n"));
}

void appendLine(QStringList *lines, const QString &label, const QString &value)
{
    if (!lines || value.trimmed().isEmpty()) {
        return;
    }
    lines->append(QStringLiteral("%1: %2").arg(label, value.trimmed()));
}

QVariantMap resultItemFromJson(const QJsonObject &object)
{
    const QJsonObject defaultImage = object.value(QStringLiteral("default_image")).toObject();
    QVariantMap result;
    result.insert(QStringLiteral("id"), object.value(QStringLiteral("id")).toInt());
    result.insert(QStringLiteral("commonName"), object.value(QStringLiteral("common_name")).toString());
    result.insert(QStringLiteral("scientificName"), firstString(object.value(QStringLiteral("scientific_name"))));
    result.insert(QStringLiteral("family"), object.value(QStringLiteral("family")).toString());
    result.insert(QStringLiteral("imageUrl"), defaultImage.value(QStringLiteral("regular_url")).toString());
    return result;
}

QVariantMap buildPlantDraft(const QJsonObject &detailObject, const QJsonObject &careGuideObject)
{
    const QString commonName = detailObject.value(QStringLiteral("common_name")).toString().trimmed();
    const QString scientificName = firstString(detailObject.value(QStringLiteral("scientific_name")));
    const QString wateringGuide = careGuideText(careGuideObject, QStringLiteral("watering"));
    const QString pruningGuide = careGuideText(careGuideObject, QStringLiteral("pruning"));
    const QString fertilizingGuide = careGuideText(careGuideObject, QStringLiteral("fertilizing"));
    const QString description = detailObject.value(QStringLiteral("description")).toString().trimmed();

    QStringList wateringNotesParts;
    const QString wateringBenchmark = wateringBenchmarkText(detailObject);
    if (!wateringBenchmark.isEmpty()) {
        wateringNotesParts.append(QStringLiteral("Benchmark: %1").arg(wateringBenchmark));
    }
    if (!wateringGuide.isEmpty()) {
        wateringNotesParts.append(wateringGuide);
    }

    QVariantMap plantData;
    plantData.insert(QStringLiteral("name"), !commonName.isEmpty() ? commonName : scientificName);
    plantData.insert(QStringLiteral("scientificName"), scientificName);
    plantData.insert(QStringLiteral("plantType"), detailObject.value(QStringLiteral("type")).toString().trimmed());
    plantData.insert(QStringLiteral("lightRequirement"), joinStringArray(detailObject.value(QStringLiteral("sunlight"))));
    plantData.insert(QStringLiteral("indoor"), boolToText(detailObject.value(QStringLiteral("indoor"))));
    plantData.insert(QStringLiteral("floweringSeason"), joinStringArray(detailObject.value(QStringLiteral("flowering_season"))));
    plantData.insert(QStringLiteral("temperatureTolerance"), hardinessText(detailObject));
    plantData.insert(QStringLiteral("wateringFrequency"), detailObject.value(QStringLiteral("watering")).toString().trimmed());
    plantData.insert(QStringLiteral("wateringNotes"), combinedText(wateringNotesParts));
    plantData.insert(QStringLiteral("humidityPreference"), QString());
    plantData.insert(QStringLiteral("soilType"), joinStringArray(detailObject.value(QStringLiteral("soil"))));
    plantData.insert(QStringLiteral("fertilizingSchedule"), fertilizingGuide);
    plantData.insert(QStringLiteral("pruningTime"), joinStringArray(detailObject.value(QStringLiteral("pruning_month"))));
    plantData.insert(QStringLiteral("pruningNotes"), pruningGuide);
    plantData.insert(QStringLiteral("growthRate"), detailObject.value(QStringLiteral("growth_rate")).toString().trimmed());
    plantData.insert(QStringLiteral("issuesPests"), QString());
    plantData.insert(QStringLiteral("toxicToPets"), boolToText(detailObject.value(QStringLiteral("poisonous_to_pets"))));
    plantData.insert(QStringLiteral("poisonousToPets"), boolToText(detailObject.value(QStringLiteral("poisonous_to_pets"))));
    plantData.insert(QStringLiteral("poisonousToHumans"), boolToText(detailObject.value(QStringLiteral("poisonous_to_humans"))));
    plantData.insert(QStringLiteral("source"), QStringLiteral("Perenual"));
    plantData.insert(QStringLiteral("notes"), description);
    plantData.insert(QStringLiteral("imageUrl"), detailObject.value(QStringLiteral("default_image")).toObject().value(QStringLiteral("regular_url")).toString());
    return plantData;
}

QString buildDetailText(const QJsonObject &detailObject, const QJsonObject &careGuideObject)
{
    QStringList lines;

    appendLine(&lines, QStringLiteral("Common name"), detailObject.value(QStringLiteral("common_name")).toString());
    appendLine(&lines, QStringLiteral("Scientific name"), joinStringArray(detailObject.value(QStringLiteral("scientific_name"))));
    appendLine(&lines, QStringLiteral("Other names"), joinStringArray(detailObject.value(QStringLiteral("other_name"))));
    appendLine(&lines, QStringLiteral("Family"), detailObject.value(QStringLiteral("family")).toString());
    appendLine(&lines, QStringLiteral("Genus"), detailObject.value(QStringLiteral("genus")).toString());
    appendLine(&lines, QStringLiteral("Origin"), joinStringArray(detailObject.value(QStringLiteral("origin"))));
    appendLine(&lines, QStringLiteral("Type"), detailObject.value(QStringLiteral("type")).toString());
    appendLine(&lines, QStringLiteral("Cycle"), detailObject.value(QStringLiteral("cycle")).toString());
    appendLine(&lines, QStringLiteral("Sunlight"), joinStringArray(detailObject.value(QStringLiteral("sunlight"))));
    appendLine(&lines, QStringLiteral("Soil"), joinStringArray(detailObject.value(QStringLiteral("soil"))));
    appendLine(&lines, QStringLiteral("Watering"), detailObject.value(QStringLiteral("watering")).toString());

    const QJsonObject wateringBenchmark = detailObject.value(QStringLiteral("watering_general_benchmark")).toObject();
    if (!wateringBenchmark.isEmpty()) {
        appendLine(&lines,
                   QStringLiteral("Watering benchmark"),
                   QStringLiteral("%1 %2")
                       .arg(wateringBenchmark.value(QStringLiteral("value")).toString().remove(QLatin1Char('"')),
                            wateringBenchmark.value(QStringLiteral("unit")).toString())
                       .trimmed());
    }

    appendLine(&lines, QStringLiteral("Maintenance"), detailObject.value(QStringLiteral("maintenance")).toString());
    appendLine(&lines, QStringLiteral("Care level"), detailObject.value(QStringLiteral("care_level")).toString());
    appendLine(&lines, QStringLiteral("Growth rate"), detailObject.value(QStringLiteral("growth_rate")).toString());
    appendLine(&lines, QStringLiteral("Pruning months"), joinStringArray(detailObject.value(QStringLiteral("pruning_month"))));

    const QJsonObject hardiness = detailObject.value(QStringLiteral("hardiness")).toObject();
    if (!hardiness.isEmpty()) {
        appendLine(&lines,
                   QStringLiteral("Hardiness"),
                   QStringLiteral("%1 to %2").arg(hardiness.value(QStringLiteral("min")).toString(),
                                                   hardiness.value(QStringLiteral("max")).toString()));
    }

    appendLine(&lines, QStringLiteral("Description"), detailObject.value(QStringLiteral("description")).toString());

    const QJsonArray sections = careGuideObject.value(QStringLiteral("data")).toArray();
    if (!sections.isEmpty()) {
        lines.append(QString());
        lines.append(QStringLiteral("Care guides"));
        lines.append(QStringLiteral("----------"));

        for (const QJsonValue &entryValue : sections) {
            const QJsonObject entryObject = entryValue.toObject();
            const QJsonArray sectionArray = entryObject.value(QStringLiteral("section")).toArray();
            for (const QJsonValue &sectionValue : sectionArray) {
                const QJsonObject sectionObject = sectionValue.toObject();
                const QString type = sectionObject.value(QStringLiteral("type")).toString();
                const QString description = sectionObject.value(QStringLiteral("description")).toString().trimmed();
                if (type.isEmpty() || description.isEmpty()) {
                    continue;
                }

                lines.append(QStringLiteral("%1").arg(type.left(1).toUpper() + type.mid(1)));
                lines.append(description);
                lines.append(QString());
            }
        }
    }

    return lines.join(QLatin1Char('\n')).trimmed();
}
}

PlantLibraryViewModel::PlantLibraryViewModel(QObject *parent)
    : QObject(parent)
    , m_token(readLocalEnvToken())
{
}

QString PlantLibraryViewModel::token() const
{
    return m_token;
}

QVariantList PlantLibraryViewModel::results() const
{
    return m_results;
}

QString PlantLibraryViewModel::detailText() const
{
    return m_detailText;
}

QVariantMap PlantLibraryViewModel::selectedPlantData() const
{
    return m_selectedPlantData;
}

QString PlantLibraryViewModel::lastError() const
{
    return m_lastError;
}

bool PlantLibraryViewModel::busy() const
{
    return m_busy;
}

void PlantLibraryViewModel::setToken(const QString &value)
{
    if (m_token == value) {
        return;
    }
    m_token = value;
    emit tokenChanged();
}

bool PlantLibraryViewModel::search(const QString &query)
{
    if (m_token.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Perenual token is required."));
        return false;
    }

    if (query.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Enter a plant name to search."));
        return false;
    }

    setBusy(true);
    setLastError(QString());
    setDetailText(QString());
    setSelectedPlantData(QVariantMap());

    QUrl url(QStringLiteral("%1/species-list").arg(kPerenualBaseUrl));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("key"), m_token.trimmed());
    urlQuery.addQueryItem(QStringLiteral("q"), query.trimmed());
    url.setQuery(urlQuery);

    QJsonDocument document;
    QString errorMessage;
    if (!executeGetJson(url, &document, &errorMessage) || !document.isObject()) {
        setBusy(false);
        setLastError(errorMessage.isEmpty() ? QStringLiteral("Plant library search failed.") : errorMessage);
        return false;
    }

    QVariantList parsedResults;
    const QJsonArray resultArray = document.object().value(QStringLiteral("data")).toArray();
    for (const QJsonValue &value : resultArray) {
        if (value.isObject()) {
            parsedResults.append(resultItemFromJson(value.toObject()));
        }
    }

    m_results = parsedResults;
    emit resultsChanged();
    setBusy(false);

    if (m_results.isEmpty()) {
        setLastError(QStringLiteral("No library results found."));
        return false;
    }

    return selectResult(0);
}

bool PlantLibraryViewModel::selectResult(int index)
{
    if (index < 0 || index >= m_results.size()) {
        setLastError(QStringLiteral("Selected library item is out of range."));
        return false;
    }

    if (m_token.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Perenual token is required."));
        return false;
    }

    const QVariantMap selected = m_results.at(index).toMap();
    const int speciesId = selected.value(QStringLiteral("id")).toInt();
    if (speciesId <= 0) {
        setLastError(QStringLiteral("Selected library item does not have a valid id."));
        return false;
    }

    setBusy(true);
    setLastError(QString());

    QJsonDocument detailDocument;
    QString errorMessage;

    QUrl detailUrl(QStringLiteral("%1/species/details/%2").arg(kPerenualBaseUrl).arg(speciesId));
    QUrlQuery detailQuery;
    detailQuery.addQueryItem(QStringLiteral("key"), m_token.trimmed());
    detailUrl.setQuery(detailQuery);

    if (!executeGetJson(detailUrl, &detailDocument, &errorMessage) || !detailDocument.isObject()) {
        setBusy(false);
        setLastError(errorMessage.isEmpty() ? QStringLiteral("Loading plant details failed.") : errorMessage);
        return false;
    }

    QJsonDocument careGuideDocument;
    QUrl careGuideUrl(QStringLiteral("%1/species-care-guide-list").arg(kPerenualCareGuideBaseUrl));
    QUrlQuery careGuideQuery;
    careGuideQuery.addQueryItem(QStringLiteral("species_id"), QString::number(speciesId));
    careGuideQuery.addQueryItem(QStringLiteral("key"), m_token.trimmed());
    careGuideUrl.setQuery(careGuideQuery);

    if (!executeGetJson(careGuideUrl, &careGuideDocument, &errorMessage) || !careGuideDocument.isObject()) {
        careGuideDocument = QJsonDocument(QJsonObject());
    }

    setSelectedPlantData(buildPlantDraft(detailDocument.object(), careGuideDocument.object()));
    setDetailText(buildDetailText(detailDocument.object(), careGuideDocument.object()));
    setBusy(false);
    return true;
}

void PlantLibraryViewModel::clear()
{
    m_results.clear();
    emit resultsChanged();
    setDetailText(QString());
    setSelectedPlantData(QVariantMap());
    setLastError(QString());
}

void PlantLibraryViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}

void PlantLibraryViewModel::setDetailText(const QString &value)
{
    if (m_detailText == value) {
        return;
    }
    m_detailText = value;
    emit detailTextChanged();
}

void PlantLibraryViewModel::setSelectedPlantData(const QVariantMap &value)
{
    if (m_selectedPlantData == value) {
        return;
    }
    m_selectedPlantData = value;
    emit selectedPlantDataChanged();
}

void PlantLibraryViewModel::setBusy(bool value)
{
    if (m_busy == value) {
        return;
    }
    m_busy = value;
    emit busyChanged();
}
