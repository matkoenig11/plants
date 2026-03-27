#include "PlantLibraryViewModel.h"

#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {
const QString kProviderPerenual = QStringLiteral("perenual");
const QString kProviderTrefle = QStringLiteral("trefle");
const QString kPerenualBaseUrl = QStringLiteral("https://perenual.com/api/v2");
const QString kPerenualCareGuideBaseUrl = QStringLiteral("https://perenual.com/api");
const QString kTrefleBaseUrl = QStringLiteral("https://trefle.io/api/v1");
const QString kLibrarySettingsGroup = QStringLiteral("plantLibrary");
const QString kProviderKey = QStringLiteral("provider");
const QString kDefaultPerenualToken = QStringLiteral("sk-MEqk69c54942523f615818");
const QString kDefaultTrefleToken = QStringLiteral("usr-Aj_F5Iuo3w555IqUkYfvizPNXUi3IBV40-lTWWkPXLA");

QString tokenSettingsKey(const QString &provider)
{
    return provider.trimmed().toLower() + QStringLiteral("Token");
}

QString normalizedProvider(const QString &provider)
{
    const QString normalized = provider.trimmed().toLower();
    if (normalized == kProviderTrefle) {
        return kProviderTrefle;
    }
    return kProviderPerenual;
}

QString tokenEnvKey(const QString &provider)
{
    if (normalizedProvider(provider) == kProviderTrefle) {
        return QStringLiteral("TREFLE_TOKEN");
    }
    return QStringLiteral("PERENUAL_TOKEN");
}

QString providerDisplayName(const QString &provider)
{
    if (normalizedProvider(provider) == kProviderTrefle) {
        return QStringLiteral("Trefle");
    }
    return QStringLiteral("Perenual");
}

QString readLocalEnvValue(const QString &keyName)
{
    const QString envValue = qEnvironmentVariable(keyName.toUtf8().constData()).trimmed();
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
        if (key == keyName) {
            return value;
        }
    }

    return {};
}

QString loadSavedProvider()
{
    QSettings settings;
    settings.beginGroup(kLibrarySettingsGroup);
    const QString provider = normalizedProvider(settings.value(kProviderKey, kProviderPerenual).toString());
    settings.endGroup();
    return provider;
}

QString loadSavedToken(const QString &provider)
{
    QSettings settings;
    settings.beginGroup(kLibrarySettingsGroup);
    const QString token = settings.value(tokenSettingsKey(provider)).toString().trimmed();
    settings.endGroup();
    return token;
}

void saveProvider(const QString &provider)
{
    QSettings settings;
    settings.beginGroup(kLibrarySettingsGroup);
    settings.setValue(kProviderKey, normalizedProvider(provider));
    settings.endGroup();
    settings.sync();
}

void saveToken(const QString &provider, const QString &token)
{
    QSettings settings;
    settings.beginGroup(kLibrarySettingsGroup);
    settings.setValue(tokenSettingsKey(provider), token.trimmed());
    settings.endGroup();
    settings.sync();
}

QString defaultTokenForProvider(const QString &provider)
{
    if (normalizedProvider(provider) == kProviderTrefle) {
        return kDefaultTrefleToken;
    }
    if (normalizedProvider(provider) == kProviderPerenual) {
        return kDefaultPerenualToken;
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
    result.insert(QStringLiteral("identifier"), QString::number(object.value(QStringLiteral("id")).toInt()));
    result.insert(QStringLiteral("commonName"), object.value(QStringLiteral("common_name")).toString());
    result.insert(QStringLiteral("scientificName"), firstString(object.value(QStringLiteral("scientific_name"))));
    result.insert(QStringLiteral("family"), object.value(QStringLiteral("family")).toString());
    result.insert(QStringLiteral("imageUrl"), defaultImage.value(QStringLiteral("regular_url")).toString());
    return result;
}

QVariantMap resultItemFromTrefleJson(const QJsonObject &object)
{
    QVariantMap result;
    result.insert(QStringLiteral("identifier"), object.value(QStringLiteral("slug")).toString());
    result.insert(QStringLiteral("commonName"), object.value(QStringLiteral("common_name")).toString());
    result.insert(QStringLiteral("scientificName"), object.value(QStringLiteral("scientific_name")).toString());
    result.insert(QStringLiteral("family"), object.value(QStringLiteral("family")).toString());
    result.insert(QStringLiteral("imageUrl"), object.value(QStringLiteral("image_url")).toString());
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

QString trefleScaleText(const QString &label, const QJsonValue &value)
{
    if (!value.isDouble()) {
        return {};
    }
    const int numericValue = value.toInt();
    return QStringLiteral("%1 level %2/10").arg(label, QString::number(numericValue));
}

QString trefleLightRequirementText(const QJsonValue &value)
{
    if (!value.isDouble()) {
        return {};
    }

    const int numericValue = value.toInt();
    QString description;
    if (numericValue <= 2) {
        description = QStringLiteral("Low light");
    } else if (numericValue <= 4) {
        description = QStringLiteral("Filtered light");
    } else if (numericValue <= 6) {
        description = QStringLiteral("Bright indirect light");
    } else if (numericValue <= 8) {
        description = QStringLiteral("Part sun");
    } else {
        description = QStringLiteral("Full sun");
    }

    return QStringLiteral("%1 (%2/10)").arg(description, QString::number(numericValue));
}

QString trefleHumidityText(const QJsonValue &value)
{
    if (!value.isDouble()) {
        return {};
    }

    const int numericValue = value.toInt();
    QString description;
    if (numericValue <= 2) {
        description = QStringLiteral("Low humidity");
    } else if (numericValue <= 4) {
        description = QStringLiteral("Moderate humidity");
    } else if (numericValue <= 6) {
        description = QStringLiteral("Medium-high humidity");
    } else if (numericValue <= 8) {
        description = QStringLiteral("High humidity");
    } else {
        description = QStringLiteral("Very high humidity");
    }

    return QStringLiteral("%1 (%2/10)").arg(description, QString::number(numericValue));
}

QString trefleMonthsText(const QJsonValue &value)
{
    if (!value.isArray()) {
        return {};
    }
    const QJsonArray array = value.toArray();
    QStringList months;
    for (const QJsonValue &entry : array) {
        const QString month = entry.toString().trimmed();
        if (!month.isEmpty()) {
            months.append(month);
        }
    }
    return months.join(QStringLiteral(", "));
}

QString trefleTemperatureToleranceText(const QJsonObject &growth)
{
    const QJsonObject minimumTemperature = growth.value(QStringLiteral("minimum_temperature")).toObject();
    const QJsonObject maximumTemperature = growth.value(QStringLiteral("maximum_temperature")).toObject();

    const auto formatTemperature = [](const QJsonObject &object) -> QString {
        if (object.value(QStringLiteral("deg_c")).isDouble()) {
            return QStringLiteral("%1 C").arg(QString::number(object.value(QStringLiteral("deg_c")).toDouble(), 'f', 1));
        }
        if (object.value(QStringLiteral("deg_f")).isDouble()) {
            return QStringLiteral("%1 F").arg(QString::number(object.value(QStringLiteral("deg_f")).toDouble(), 'f', 1));
        }
        return {};
    };

    const QString minimum = formatTemperature(minimumTemperature);
    const QString maximum = formatTemperature(maximumTemperature);
    if (minimum.isEmpty() && maximum.isEmpty()) {
        return {};
    }
    if (!minimum.isEmpty() && !maximum.isEmpty()) {
        return QStringLiteral("%1 to %2").arg(minimum, maximum);
    }
    return !minimum.isEmpty() ? minimum : maximum;
}

QString trefleToxicityText(const QJsonValue &value)
{
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("Yes") : QStringLiteral("No");
    }
    if (value.isDouble()) {
        return value.toDouble() > 0.0 ? QStringLiteral("Yes") : QStringLiteral("No");
    }

    const QString text = firstString(value);
    if (text.isEmpty()) {
        return {};
    }

    const QString normalized = text.trimmed().toLower();
    if (normalized == QStringLiteral("none")
        || normalized == QStringLiteral("no")
        || normalized == QStringLiteral("false")
        || normalized == QStringLiteral("non-toxic")
        || normalized == QStringLiteral("nontoxic")) {
        return QStringLiteral("No");
    }
    if (normalized == QStringLiteral("yes")
        || normalized == QStringLiteral("true")
        || normalized == QStringLiteral("toxic")) {
        return QStringLiteral("Yes");
    }

    return text.trimmed();
}

QVariantMap buildTreflePlantDraft(const QJsonObject &detailObject)
{
    const QJsonObject growth = detailObject.value(QStringLiteral("growth")).toObject();
    const QJsonObject specifications = detailObject.value(QStringLiteral("specifications")).toObject();
    const QJsonObject distribution = detailObject.value(QStringLiteral("distribution")).toObject();

    QStringList notesParts;
    const QString observations = detailObject.value(QStringLiteral("observations")).toString().trimmed();
    if (!observations.isEmpty()) {
        notesParts.append(observations);
    }

    const QString nativeDistribution = joinStringArray(distribution.value(QStringLiteral("native")));
    if (!nativeDistribution.isEmpty()) {
        notesParts.append(QStringLiteral("Native distribution: %1").arg(nativeDistribution));
    }

    const QString introducedDistribution = joinStringArray(distribution.value(QStringLiteral("introduced")));
    if (!introducedDistribution.isEmpty()) {
        notesParts.append(QStringLiteral("Introduced distribution: %1").arg(introducedDistribution));
    }

    if (growth.value(QStringLiteral("ph_minimum")).isDouble() || growth.value(QStringLiteral("ph_maximum")).isDouble()) {
        const QString minimum = growth.value(QStringLiteral("ph_minimum")).isDouble()
            ? QString::number(growth.value(QStringLiteral("ph_minimum")).toDouble(), 'f', 1)
            : QString();
        const QString maximum = growth.value(QStringLiteral("ph_maximum")).isDouble()
            ? QString::number(growth.value(QStringLiteral("ph_maximum")).toDouble(), 'f', 1)
            : QString();
        notesParts.append(QStringLiteral("Soil pH: %1-%2").arg(minimum, maximum));
    }

    const QString soilNutriments = trefleScaleText(QStringLiteral("Soil nutriment"), growth.value(QStringLiteral("soil_nutriments")));
    if (!soilNutriments.isEmpty()) {
        notesParts.append(soilNutriments);
    }

    const QString soilSalinity = growth.value(QStringLiteral("soil_salinity")).isDouble()
        ? QStringLiteral("Soil salinity level %1/10").arg(QString::number(growth.value(QStringLiteral("soil_salinity")).toInt()))
        : QString();
    if (!soilSalinity.isEmpty()) {
        notesParts.append(soilSalinity);
    }

    const QString toxicity = trefleToxicityText(specifications.value(QStringLiteral("toxicity")));
    if (!toxicity.isEmpty()) {
        notesParts.append(QStringLiteral("Toxicity: %1").arg(toxicity));
    }

    QVariantMap plantData;
    const QString commonName = detailObject.value(QStringLiteral("common_name")).toString().trimmed();
    const QString scientificName = detailObject.value(QStringLiteral("scientific_name")).toString().trimmed();
    plantData.insert(QStringLiteral("name"), !commonName.isEmpty() ? commonName : scientificName);
    plantData.insert(QStringLiteral("scientificName"), scientificName);
    plantData.insert(QStringLiteral("plantType"), specifications.value(QStringLiteral("growth_habit")).toString().trimmed());
    plantData.insert(QStringLiteral("lightRequirement"), trefleLightRequirementText(growth.value(QStringLiteral("light"))));
    plantData.insert(QStringLiteral("indoor"), QString());
    plantData.insert(QStringLiteral("floweringSeason"), trefleMonthsText(growth.value(QStringLiteral("bloom_months"))));
    plantData.insert(QStringLiteral("temperatureTolerance"), trefleTemperatureToleranceText(growth));
    plantData.insert(QStringLiteral("wateringFrequency"), QString());
    plantData.insert(QStringLiteral("wateringNotes"), QString());
    plantData.insert(QStringLiteral("humidityPreference"), trefleHumidityText(growth.value(QStringLiteral("atmospheric_humidity"))));
    plantData.insert(QStringLiteral("soilType"), joinStringArray(growth.value(QStringLiteral("soil_texture"))));
    plantData.insert(QStringLiteral("fertilizingSchedule"), QString());
    plantData.insert(QStringLiteral("pruningTime"), QString());
    plantData.insert(QStringLiteral("pruningNotes"), QString());
    plantData.insert(QStringLiteral("growthRate"), specifications.value(QStringLiteral("growth_rate")).toString().trimmed());
    plantData.insert(QStringLiteral("issuesPests"), QString());
    plantData.insert(QStringLiteral("toxicToPets"), toxicity);
    plantData.insert(QStringLiteral("poisonousToPets"), toxicity);
    plantData.insert(QStringLiteral("poisonousToHumans"), toxicity);
    plantData.insert(QStringLiteral("source"), QStringLiteral("Trefle"));
    plantData.insert(QStringLiteral("notes"), combinedText(notesParts));
    plantData.insert(QStringLiteral("imageUrl"), detailObject.value(QStringLiteral("image_url")).toString());
    return plantData;
}

QString buildTrefleDetailText(const QJsonObject &detailObject)
{
    QStringList lines;
    appendLine(&lines, QStringLiteral("Common name"), detailObject.value(QStringLiteral("common_name")).toString());
    appendLine(&lines, QStringLiteral("Scientific name"), detailObject.value(QStringLiteral("scientific_name")).toString());
    appendLine(&lines, QStringLiteral("Family"), detailObject.value(QStringLiteral("family")).toString());
    appendLine(&lines, QStringLiteral("Family common name"), detailObject.value(QStringLiteral("family_common_name")).toString());
    appendLine(&lines, QStringLiteral("Genus"), detailObject.value(QStringLiteral("genus")).toString());
    appendLine(&lines, QStringLiteral("Rank"), detailObject.value(QStringLiteral("rank")).toString());
    appendLine(&lines, QStringLiteral("Status"), detailObject.value(QStringLiteral("status")).toString());
    appendLine(&lines, QStringLiteral("Observations"), detailObject.value(QStringLiteral("observations")).toString());

    const QJsonObject specifications = detailObject.value(QStringLiteral("specifications")).toObject();
    appendLine(&lines, QStringLiteral("Growth habit"), specifications.value(QStringLiteral("growth_habit")).toString());
    appendLine(&lines, QStringLiteral("Growth rate"), specifications.value(QStringLiteral("growth_rate")).toString());
    appendLine(&lines, QStringLiteral("Toxicity"), specifications.value(QStringLiteral("toxicity")).toString());

    const QJsonObject growth = detailObject.value(QStringLiteral("growth")).toObject();
    appendLine(&lines, QStringLiteral("Light"), trefleLightRequirementText(growth.value(QStringLiteral("light"))));
    appendLine(&lines, QStringLiteral("Humidity"), trefleHumidityText(growth.value(QStringLiteral("atmospheric_humidity"))));
    appendLine(&lines, QStringLiteral("Bloom months"), trefleMonthsText(growth.value(QStringLiteral("bloom_months"))));
    appendLine(&lines, QStringLiteral("Soil texture"), joinStringArray(growth.value(QStringLiteral("soil_texture"))));
    appendLine(&lines, QStringLiteral("Soil humidity"), trefleScaleText(QStringLiteral("Soil humidity"), growth.value(QStringLiteral("soil_humidity"))));
    appendLine(&lines, QStringLiteral("Soil nutriments"), trefleScaleText(QStringLiteral("Soil nutriment"), growth.value(QStringLiteral("soil_nutriments"))));
    appendLine(&lines, QStringLiteral("Temperature tolerance"), trefleTemperatureToleranceText(growth));

    if (growth.value(QStringLiteral("ph_minimum")).isDouble() || growth.value(QStringLiteral("ph_maximum")).isDouble()) {
        lines.append(QStringLiteral("Soil pH: %1-%2")
                         .arg(growth.value(QStringLiteral("ph_minimum")).isDouble()
                                  ? QString::number(growth.value(QStringLiteral("ph_minimum")).toDouble(), 'f', 1)
                                  : QString(),
                              growth.value(QStringLiteral("ph_maximum")).isDouble()
                                  ? QString::number(growth.value(QStringLiteral("ph_maximum")).toDouble(), 'f', 1)
                                  : QString()));
    }

    const QJsonObject distribution = detailObject.value(QStringLiteral("distribution")).toObject();
    appendLine(&lines, QStringLiteral("Native distribution"), joinStringArray(distribution.value(QStringLiteral("native"))));
    appendLine(&lines, QStringLiteral("Introduced distribution"), joinStringArray(distribution.value(QStringLiteral("introduced"))));

    return lines.join(QLatin1Char('\n')).trimmed();
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
{
    m_provider = loadSavedProvider();

    const QStringList providers = {kProviderPerenual, kProviderTrefle};
    for (const QString &provider : providers) {
        QString token = loadSavedToken(provider);
        if (token.isEmpty()) {
            token = readLocalEnvValue(tokenEnvKey(provider));
        }
        if (token.isEmpty()) {
            token = defaultTokenForProvider(provider);
        }
        if (!token.isEmpty()) {
            m_tokens.insert(provider, token);
            saveToken(provider, token);
        }
    }
}

QString PlantLibraryViewModel::provider() const
{
    return m_provider;
}

QVariantList PlantLibraryViewModel::providerOptions() const
{
    return {
        QVariantMap{
            {QStringLiteral("providerId"), kProviderPerenual},
            {QStringLiteral("displayName"), providerDisplayName(kProviderPerenual)}
        },
        QVariantMap{
            {QStringLiteral("providerId"), kProviderTrefle},
            {QStringLiteral("displayName"), providerDisplayName(kProviderTrefle)}
        }
    };
}

QString PlantLibraryViewModel::token() const
{
    return m_tokens.value(m_provider);
}

QString PlantLibraryViewModel::tokenLabel() const
{
    return providerDisplayName(m_provider) + QStringLiteral(" token");
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

void PlantLibraryViewModel::setProvider(const QString &value)
{
    const QString normalized = normalizedProvider(value);
    if (m_provider == normalized) {
        return;
    }
    m_provider = normalized;
    saveProvider(m_provider);
    clear();
    emit providerChanged();
    emit tokenChanged();
}

void PlantLibraryViewModel::setToken(const QString &value)
{
    const QString trimmedValue = value.trimmed();
    if (m_tokens.value(m_provider) == trimmedValue) {
        return;
    }
    m_tokens.insert(m_provider, trimmedValue);
    saveToken(m_provider, trimmedValue);
    emit tokenChanged();
}

bool PlantLibraryViewModel::search(const QString &query)
{
    const QString currentToken = token().trimmed();
    if (currentToken.isEmpty()) {
        setLastError(QStringLiteral("%1 token is required.").arg(providerDisplayName(m_provider)));
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

    QUrl url;
    QUrlQuery urlQuery;
    if (m_provider == kProviderTrefle) {
        url = QUrl(QStringLiteral("%1/species/search").arg(kTrefleBaseUrl));
        urlQuery.addQueryItem(QStringLiteral("token"), currentToken);
        urlQuery.addQueryItem(QStringLiteral("q"), query.trimmed());
    } else {
        url = QUrl(QStringLiteral("%1/species-list").arg(kPerenualBaseUrl));
        urlQuery.addQueryItem(QStringLiteral("key"), currentToken);
        urlQuery.addQueryItem(QStringLiteral("q"), query.trimmed());
    }
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
            if (m_provider == kProviderTrefle) {
                parsedResults.append(resultItemFromTrefleJson(value.toObject()));
            } else {
                parsedResults.append(resultItemFromJson(value.toObject()));
            }
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

    const QString currentToken = token().trimmed();
    if (currentToken.isEmpty()) {
        setLastError(QStringLiteral("%1 token is required.").arg(providerDisplayName(m_provider)));
        return false;
    }

    const QVariantMap selected = m_results.at(index).toMap();
    const QString identifier = selected.value(QStringLiteral("identifier")).toString().trimmed();
    if (identifier.isEmpty()) {
        setLastError(QStringLiteral("Selected library item does not have a valid id."));
        return false;
    }

    setBusy(true);
    setLastError(QString());

    QJsonDocument detailDocument;
    QString errorMessage;

    QUrl detailUrl;
    QUrlQuery detailQuery;
    if (m_provider == kProviderTrefle) {
        detailUrl = QUrl(QStringLiteral("%1/species/%2").arg(kTrefleBaseUrl, identifier));
        detailQuery.addQueryItem(QStringLiteral("token"), currentToken);
    } else {
        detailUrl = QUrl(QStringLiteral("%1/species/details/%2").arg(kPerenualBaseUrl, identifier));
        detailQuery.addQueryItem(QStringLiteral("key"), currentToken);
    }
    detailUrl.setQuery(detailQuery);

    if (!executeGetJson(detailUrl, &detailDocument, &errorMessage) || !detailDocument.isObject()) {
        setBusy(false);
        setLastError(errorMessage.isEmpty() ? QStringLiteral("Loading plant details failed.") : errorMessage);
        return false;
    }

    if (m_provider == kProviderTrefle) {
        const QJsonObject detailObject = detailDocument.object().value(QStringLiteral("data")).toObject();
        setSelectedPlantData(buildTreflePlantDraft(detailObject));
        setDetailText(buildTrefleDetailText(detailObject));
        setBusy(false);
        return true;
    }

    QJsonDocument careGuideDocument;
    QUrl careGuideUrl(QStringLiteral("%1/species-care-guide-list").arg(kPerenualCareGuideBaseUrl));
    QUrlQuery careGuideQuery;
    careGuideQuery.addQueryItem(QStringLiteral("species_id"), identifier);
    careGuideQuery.addQueryItem(QStringLiteral("key"), currentToken);
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
