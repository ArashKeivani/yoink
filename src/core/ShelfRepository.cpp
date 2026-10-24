/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ShelfRepository.h"

#include "ShelfLimits.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>

#include <utility>

namespace
{
constexpr int CurrentSchemaVersion = 1;

QString translated(const char *text)
{
    return QCoreApplication::translate("ShelfRepository", text);
}

QDateTime parseDateTime(const QJsonValue &value)
{
    const QString text = value.toString();
    if (text.isEmpty()) {
        return {};
    }
    QDateTime date = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (!date.isValid()) {
        date = QDateTime::fromString(text, Qt::ISODate);
    }
    return date;
}

bool isSafeImageFileName(const QString &name)
{
    if (name.isEmpty() || name == QLatin1String(".") || name == QLatin1String("..")) {
        return false;
    }
    if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\'))) {
        return false;
    }
    return QDir::cleanPath(name) == name;
}
} // namespace

ShelfRepository::ShelfRepository(QString dataDirectory)
    : m_dataDirectory(std::move(dataDirectory))
{
}

ShelfRepository::LoadResult ShelfRepository::load()
{
    m_readOnly = false;
    LoadResult result;
    const QString path = itemsFilePath();
    QFile file(path);
    if (!file.exists()) {
        return result;
    }
    if (file.size() > ShelfLimits::MaxDataFileBytes) {
        m_readOnly = true;
        result.readOnly = true;
        result.error = translated("The shelf data file is unexpectedly large, so it was left untouched.");
        return result;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        m_readOnly = true;
        result.readOnly = true;
        result.error = translated("Couldn't read the shelf data: %1").arg(file.errorString());
        return result;
    }

    const QByteArray data = file.readAll();
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        QString backup;
        const QString backupError = backupDamagedFile(&backup);
        result.error = backup.isEmpty() ? translated("The shelf data was damaged and couldn't be backed up: %1").arg(backupError)
                                        : translated("The shelf data was damaged. A backup was saved to %1.").arg(backup);
        return result;
    }

    const QJsonObject root = document.object();
    const int schemaVersion = root.value(QStringLiteral("schemaVersion")).toInt();
    if (schemaVersion > CurrentSchemaVersion) {
        m_readOnly = true;
        result.readOnly = true;
        result.error = translated("The shelf data was created by a newer version of Yoink. It will not be modified.");
        return result;
    }
    if (schemaVersion != CurrentSchemaVersion) {
        QString backup;
        const QString backupError = backupDamagedFile(&backup);
        result.error = backup.isEmpty() ? translated("The shelf data has an unknown format and couldn't be backed up: %1").arg(backupError)
                                        : translated("The shelf data has an unknown format. A backup was saved to %1.").arg(backup);
        return result;
    }

    const QJsonValue itemsValue = root.value(QStringLiteral("items"));
    if (!itemsValue.isArray()) {
        QString backup;
        const QString backupError = backupDamagedFile(&backup);
        result.error = backup.isEmpty() ? translated("The shelf data has an invalid structure and couldn't be backed up: %1").arg(backupError)
                                        : translated("The shelf data has an invalid structure. A backup was saved to %1.").arg(backup);
        return result;
    }

    const QJsonArray items = itemsValue.toArray();
    int skipped = 0;
    QSet<QString> ids;
    for (const QJsonValue &value : items) {
        if (result.items.size() >= ShelfLimits::MaxItems) {
            skipped += int(items.size()) - result.items.size();
            break;
        }
        if (!value.isObject()) {
            ++skipped;
            continue;
        }

        const QJsonObject object = value.toObject();
        const QString kindName = object.value(QStringLiteral("kind")).toString();
        if (kindName.isEmpty()) {
            ++skipped;
            continue;
        }

        QString id = object.value(QStringLiteral("id")).toString();
        if (id.isEmpty() || ids.contains(id)) {
            id.clear();
        }

        const QDateTime addedAt = parseDateTime(object.value(QStringLiteral("addedAt")));
        const ShelfItem::Kind persistedKind = ShelfItem::kindFromString(kindName, ShelfItem::Kind::File);
        ShelfItem item;
        bool valid = true;

        switch (persistedKind) {
        case ShelfItem::Kind::File:
        case ShelfItem::Kind::Folder: {
            const QString path = object.value(QStringLiteral("path")).toString();
            if (path.isEmpty() || !QDir::isAbsolutePath(path)) {
                valid = false;
                break;
            }
            item = ShelfItem::makeFile(path, id);
            item.kind = persistedKind;
            item.refreshFromFileSystem();
            break;
        }
        case ShelfItem::Kind::Text: {
            const QString text = object.value(QStringLiteral("text")).toString();
            if (text.isEmpty() || text.size() > ShelfLimits::MaxTextLength || text.toUtf8().size() > ShelfLimits::MaxTextLength) {
                valid = false;
                break;
            }
            item = ShelfItem::makeText(text, id);
            break;
        }
        case ShelfItem::Kind::Url: {
            const QString urlText = object.value(QStringLiteral("url")).toString();
            const QUrl url(urlText);
            if (urlText.isEmpty() || !url.isValid() || url.scheme().isEmpty()) {
                valid = false;
                break;
            }
            item = ShelfItem::makeUrl(url, id);
            break;
        }
        case ShelfItem::Kind::Image: {
            const QString fileName = object.value(QStringLiteral("imageFile")).toString();
            if (!isSafeImageFileName(fileName)) {
                valid = false;
                break;
            }
            const QString imagePath = QDir(imagesDirectory()).filePath(fileName);
            const QSize dimensions(object.value(QStringLiteral("width")).toInt(), object.value(QStringLiteral("height")).toInt());
            const QString hash = object.value(QStringLiteral("imageHash")).toString();
            const qint64 byteSize = qint64(object.value(QStringLiteral("byteSize")).toDouble(-1));
            item = ShelfItem::makeImage(imagePath, dimensions, byteSize, hash, id);
            break;
        }
        }

        if (!valid) {
            ++skipped;
            continue;
        }

        item.addedAt = addedAt.isValid() ? addedAt : QDateTime::currentDateTimeUtc();
        if (item.id.isEmpty()) {
            item.id = ShelfItem::newId();
        }
        if (ids.contains(item.id)) {
            item.id = ShelfItem::newId();
        }
        ids.insert(item.id);
        result.items.append(item);
    }

    if (skipped > 0) {
        result.warning = translated("Some shelf items couldn't be restored and were skipped.");
    }
    return result;
}

bool ShelfRepository::save(const QList<ShelfItem> &items, QString *error)
{
    if (m_readOnly) {
        if (error) {
            *error = translated("The shelf is read-only because its data was created by a newer version of Yoink.");
        }
        return false;
    }

    if (!QDir().mkpath(m_dataDirectory)) {
        if (error) {
            *error = translated("Couldn't create the data directory.");
        }
        return false;
    }

    QJsonArray itemArray;
    QSet<QString> referencedImages;
    for (const ShelfItem &item : items) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), item.id);
        object.insert(QStringLiteral("kind"), ShelfItem::kindName(item.kind));
        object.insert(QStringLiteral("addedAt"), item.addedAt.toUTC().toString(Qt::ISODateWithMs));

        switch (item.kind) {
        case ShelfItem::Kind::File:
        case ShelfItem::Kind::Folder:
            object.insert(QStringLiteral("path"), item.path);
            break;
        case ShelfItem::Kind::Text:
            object.insert(QStringLiteral("text"), item.text);
            break;
        case ShelfItem::Kind::Url:
            object.insert(QStringLiteral("url"), item.url);
            break;
        case ShelfItem::Kind::Image: {
            const QString fileName = QFileInfo(item.imageFile).fileName();
            if (!isSafeImageFileName(fileName)) {
                if (error) {
                    *error = translated("An image path was invalid, so the shelf was not saved.");
                }
                return false;
            }
            object.insert(QStringLiteral("imageFile"), fileName);
            object.insert(QStringLiteral("imageHash"), item.imageHash);
            object.insert(QStringLiteral("width"), item.imageSize.width());
            object.insert(QStringLiteral("height"), item.imageSize.height());
            object.insert(QStringLiteral("byteSize"), double(item.byteSize));
            referencedImages.insert(fileName);
            break;
        }
        }
        itemArray.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), CurrentSchemaVersion);
    root.insert(QStringLiteral("items"), itemArray);

    QSaveFile file(itemsFilePath());
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = translated("Couldn't open the shelf data for writing: %1").arg(file.errorString());
        }
        return false;
    }
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size() || !file.commit()) {
        if (error) {
            *error = translated("Couldn't write the shelf data: %1").arg(file.errorString());
        }
        return false;
    }
    QFile::setPermissions(itemsFilePath(), QFile::ReadOwner | QFile::WriteOwner);

    cleanOrphanedImages(items);
    return true;
}

QString ShelfRepository::dataDirectory() const
{
    return m_dataDirectory;
}

QString ShelfRepository::itemsFilePath() const
{
    return QDir(m_dataDirectory).filePath(QStringLiteral("items.json"));
}

QString ShelfRepository::imagesDirectory() const
{
    return QDir(m_dataDirectory).filePath(QStringLiteral("images"));
}

bool ShelfRepository::isReadOnly() const
{
    return m_readOnly;
}

QString ShelfRepository::defaultDataDirectory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (path.isEmpty()) {
        path = QDir::homePath() + QStringLiteral("/.local/share/yoink");
    }
    return path;
}

QString ShelfRepository::backupDamagedFile(QString *backupPath)
{
    const QString source = itemsFilePath();
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    QString candidate = source + QStringLiteral(".corrupt-") + timestamp;
    int suffix = 1;
    while (QFile::exists(candidate)) {
        candidate = source + QStringLiteral(".corrupt-") + timestamp + QLatin1Char('-') + QString::number(suffix++);
    }

    if (!QFile::copy(source, candidate)) {
        if (backupPath) {
            backupPath->clear();
        }
        return translated("The existing file couldn't be copied.");
    }
    if (backupPath) {
        *backupPath = candidate;
    }
    return {};
}

void ShelfRepository::cleanOrphanedImages(const QList<ShelfItem> &items)
{
    QSet<QString> referenced;
    for (const ShelfItem &item : items) {
        if (item.kind == ShelfItem::Kind::Image) {
            referenced.insert(QFileInfo(item.imageFile).fileName());
        }
    }

    QDir directory(imagesDirectory());
    const QFileInfoList entries = directory.entryInfoList({QStringLiteral("*.png")}, QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        if (!referenced.contains(entry.fileName())) {
            QFile::remove(entry.absoluteFilePath());
        }
    }
}
