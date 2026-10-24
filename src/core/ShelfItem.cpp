/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ShelfItem.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUrl>
#include <QUuid>

namespace
{
QString translated(const char *text)
{
    return QCoreApplication::translate("ShelfItem", text);
}

QString formatSize(qint64 bytes)
{
    if (bytes < 0) {
        return {};
    }
    return QLocale::system().formattedDataSize(bytes, 1, QLocale::DataSizeSIFormat);
}

QString elideText(const QString &text, int maximumCharacters)
{
    QString compact = text.simplified();
    if (compact.size() <= maximumCharacters) {
        return compact;
    }
    return compact.left(maximumCharacters - 1) + QChar(0x2026);
}

QString urlDisplayString(const QString &value)
{
    const QUrl url(value);
    return url.toDisplayString(QUrl::RemovePassword);
}

QString normalizedUrl(const QUrl &url)
{
    return url.adjusted(QUrl::NormalizePathSegments).toString(QUrl::FullyEncoded);
}
} // namespace

bool ShelfItem::isFileSystemItem() const
{
    return kind == Kind::File || kind == Kind::Folder;
}

bool ShelfItem::hasPreview() const
{
    if (!available) {
        return false;
    }
    if (kind == Kind::Image) {
        return !imageFile.isEmpty();
    }
    return kind == Kind::File && mimeTypeName.startsWith(QLatin1String("image/"));
}

QString ShelfItem::displayName() const
{
    switch (kind) {
    case Kind::File:
    case Kind::Folder: {
        const QFileInfo info(path);
        const QString name = info.fileName();
        return name.isEmpty() ? path : name;
    }
    case Kind::Text: {
        const QStringList lines = text.split(QLatin1Char('\n'));
        for (const QString &line : lines) {
            const QString compact = line.simplified();
            if (!compact.isEmpty()) {
                return elideText(compact, 90);
            }
        }
        return translated("Text snippet");
    }
    case Kind::Url: {
        const QUrl parsed(url);
        QString name = parsed.host();
        if (!parsed.path().isEmpty() && parsed.path() != QLatin1String("/")) {
            name += parsed.path();
        }
        if (name.isEmpty()) {
            name = parsed.toDisplayString(QUrl::RemovePassword | QUrl::RemoveScheme);
        }
        if (name.isEmpty()) {
            name = url;
        }
        return elideText(name, 72);
    }
    case Kind::Image:
        return translated("Image");
    }
    return {};
}

QString ShelfItem::secondaryText() const
{
    if (!available && !unavailableReason.isEmpty()) {
        return unavailableReason;
    }

    switch (kind) {
    case Kind::File: {
        QStringList parts;
        if (!mimeDescription.isEmpty()) {
            parts.append(mimeDescription);
        }
        if (byteSize >= 0) {
            parts.append(formatSize(byteSize));
        }
        return parts.join(QStringLiteral(" · "));
    }
    case Kind::Folder:
        return translated("Folder");
    case Kind::Text: {
        const qsizetype characters = text.size();
        return translated("Text") + QStringLiteral(" · ") + translated("%1 characters").arg(characters);
    }
    case Kind::Url:
        return urlDisplayString(url);
    case Kind::Image: {
        QStringList parts;
        if (imageSize.isValid()) {
            parts.append(QStringLiteral("%1 × %2").arg(imageSize.width()).arg(imageSize.height()));
        }
        if (byteSize >= 0) {
            parts.append(formatSize(byteSize));
        }
        return parts.join(QStringLiteral(" · "));
    }
    }
    return {};
}

QString ShelfItem::accessibleDescription() const
{
    QString description;
    switch (kind) {
    case Kind::File:
    case Kind::Folder:
        description = path;
        break;
    case Kind::Text:
        description = elideText(text, 1000);
        break;
    case Kind::Url:
        description = url;
        break;
    case Kind::Image:
        description = imageFile;
        break;
    }

    if (!available && !unavailableReason.isEmpty()) {
        if (!description.isEmpty()) {
            description += QStringLiteral(". ");
        }
        description += unavailableReason;
    }
    return description;
}

QString ShelfItem::dedupeKey() const
{
    switch (kind) {
    case Kind::File:
    case Kind::Folder: {
        const QFileInfo info(path);
        QString canonical = info.canonicalFilePath();
        if (canonical.isEmpty()) {
            canonical = info.absoluteFilePath();
        }
        return QStringLiteral("path:") + QDir::cleanPath(canonical);
    }
    case Kind::Text:
        return QStringLiteral("text:") + QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256).toHex());
    case Kind::Url:
        return QStringLiteral("url:") + normalizedUrl(QUrl(url));
    case Kind::Image:
        return QStringLiteral("image:") + imageHash;
    }
    return {};
}

QString ShelfItem::iconKey() const
{
    if (!available) {
        return QStringLiteral("theme:dialog-warning");
    }

    switch (kind) {
    case Kind::File:
        return QStringLiteral("file:") + path;
    case Kind::Folder:
        return QStringLiteral("theme:folder");
    case Kind::Text:
        return QStringLiteral("theme:text-x-generic");
    case Kind::Url:
        return QStringLiteral("theme:internet-web-browser");
    case Kind::Image:
        return QStringLiteral("theme:image-x-generic");
    }
    return QStringLiteral("theme:text-x-generic");
}

void ShelfItem::refreshFromFileSystem()
{
    switch (kind) {
    case Kind::File:
    case Kind::Folder: {
        const QFileInfo info(path);
        if (!info.exists()) {
            available = false;
            unavailableReason = kind == Kind::Folder ? translated("Folder is no longer available") : translated("File is no longer available");
            byteSize = -1;
            modified = {};
            mimeDescription = kind == Kind::Folder ? translated("Folder") : translated("File");
            mimeTypeName.clear();
            return;
        }

        kind = info.isDir() ? Kind::Folder : Kind::File;
        available = true;
        unavailableReason.clear();
        byteSize = info.isDir() ? -1 : info.size();
        modified = info.lastModified();

        if (info.isDir()) {
            mimeDescription = translated("Folder");
            mimeTypeName = QStringLiteral("inode/directory");
        } else {
            const QMimeType mime = QMimeDatabase().mimeTypeForFile(info, QMimeDatabase::MatchDefault);
            mimeDescription = mime.comment();
            if (mimeDescription.isEmpty()) {
                mimeDescription = translated("File");
            }
            mimeTypeName = mime.name();
        }
        return;
    }
    case Kind::Image: {
        const QFileInfo info(imageFile);
        available = !imageFile.isEmpty() && info.exists() && info.isFile();
        unavailableReason = available ? QString() : translated("Image data is missing");
        if (available) {
            byteSize = info.size();
        }
        mimeDescription = translated("Image");
        mimeTypeName = QStringLiteral("image/png");
        return;
    }
    case Kind::Text:
    case Kind::Url:
        available = true;
        unavailableReason.clear();
        return;
    }
}

ShelfItem ShelfItem::makeFile(const QString &path, const QString &id)
{
    ShelfItem item;
    item.id = id.isEmpty() ? newId() : id;
    item.kind = Kind::File;
    item.path = path;
    item.addedAt = QDateTime::currentDateTimeUtc();
    item.refreshFromFileSystem();
    return item;
}

ShelfItem ShelfItem::makeText(const QString &text, const QString &id)
{
    ShelfItem item;
    item.id = id.isEmpty() ? newId() : id;
    item.kind = Kind::Text;
    item.text = text;
    item.addedAt = QDateTime::currentDateTimeUtc();
    item.mimeDescription = translated("Text");
    item.mimeTypeName = QStringLiteral("text/plain");
    return item;
}

ShelfItem ShelfItem::makeUrl(const QUrl &url, const QString &id)
{
    ShelfItem item;
    item.id = id.isEmpty() ? newId() : id;
    item.kind = Kind::Url;
    item.url = normalizedUrl(url);
    item.addedAt = QDateTime::currentDateTimeUtc();
    item.mimeDescription = translated("Link");
    item.mimeTypeName = QStringLiteral("text/uri-list");
    return item;
}

ShelfItem ShelfItem::makeImage(const QString &imageFile, const QSize &imageSize, qint64 byteSize, const QString &imageHash, const QString &id)
{
    ShelfItem item;
    item.id = id.isEmpty() ? newId() : id;
    item.kind = Kind::Image;
    item.imageFile = imageFile;
    item.imageSize = imageSize;
    item.byteSize = byteSize;
    item.imageHash = imageHash;
    item.addedAt = QDateTime::currentDateTimeUtc();
    item.refreshFromFileSystem();
    return item;
}

QString ShelfItem::newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString ShelfItem::kindName(Kind kind)
{
    switch (kind) {
    case Kind::File:
        return QStringLiteral("file");
    case Kind::Folder:
        return QStringLiteral("folder");
    case Kind::Text:
        return QStringLiteral("text");
    case Kind::Url:
        return QStringLiteral("url");
    case Kind::Image:
        return QStringLiteral("image");
    }
    return QStringLiteral("file");
}

ShelfItem::Kind ShelfItem::kindFromString(QStringView name, Kind fallback)
{
    if (name == QLatin1String("file")) {
        return Kind::File;
    }
    if (name == QLatin1String("folder")) {
        return Kind::Folder;
    }
    if (name == QLatin1String("text")) {
        return Kind::Text;
    }
    if (name == QLatin1String("url")) {
        return Kind::Url;
    }
    if (name == QLatin1String("image")) {
        return Kind::Image;
    }
    return fallback;
}
