/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ShelfModel.h"

#include "MimeImport.h"
#include "ShelfLimits.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMimeData>
#include <QSaveFile>
#include <QSet>
#include <QUrl>

#include <utility>

namespace
{
QString translated(const char *text)
{
    return QCoreApplication::translate("ShelfModel", text);
}

QString providerSource(const QString &type, const QString &value)
{
    return QStringLiteral("image://yoink/") + type + QLatin1Char('/') + QString::fromLatin1(QUrl::toPercentEncoding(value));
}
} // namespace

ShelfModel::ShelfModel(QString imageDirectory, QObject *parent)
    : QAbstractListModel(parent)
    , m_imageDirectory(std::move(imageDirectory))
{
}

int ShelfModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return int(m_items.size());
}

QVariant ShelfModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }

    const ShelfItem &item = m_items.at(index.row());
    switch (role) {
    case IdRole:
        return item.id;
    case KindRole:
        return int(item.kind);
    case KindNameRole:
        return ShelfItem::kindName(item.kind);
    case NameRole:
        return item.displayName();
    case DetailRole:
        return item.secondaryText();
    case DescriptionRole:
        return item.accessibleDescription();
    case IconSourceRole:
        return providerSource(QStringLiteral("icon"), item.iconKey());
    case PreviewSourceRole:
        return item.hasPreview() ? providerSource(QStringLiteral("preview"), item.kind == ShelfItem::Kind::Image ? item.imageFile : item.path) : QString();
    case AvailableRole:
        return item.available;
    case PathRole:
        return item.path;
    case UrlRole:
        return item.url;
    case TextRole:
        return item.text;
    case SourceRole:
        switch (item.kind) {
        case ShelfItem::Kind::File:
        case ShelfItem::Kind::Folder:
            return item.path;
        case ShelfItem::Kind::Text:
            return item.text;
        case ShelfItem::Kind::Url:
            return item.url;
        case ShelfItem::Kind::Image:
            return item.imageFile;
        }
        return {};
    case AddedAtRole:
        return item.addedAt;
    case ByteSizeRole:
        return item.byteSize;
    case DimensionsRole:
        return item.imageSize.isValid() ? QStringLiteral("%1 × %2").arg(item.imageSize.width()).arg(item.imageSize.height()) : QString();
    case IsFileRole:
        return item.kind == ShelfItem::Kind::File;
    case IsFolderRole:
        return item.kind == ShelfItem::Kind::Folder;
    case IsImageRole:
        return item.kind == ShelfItem::Kind::Image;
    case HasPreviewRole:
        return item.hasPreview();
    default:
        return {};
    }
}

QHash<int, QByteArray> ShelfModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {KindRole, "kind"},
        {KindNameRole, "kindName"},
        {NameRole, "name"},
        {DetailRole, "detail"},
        {DescriptionRole, "description"},
        {IconSourceRole, "iconSource"},
        {PreviewSourceRole, "previewSource"},
        {AvailableRole, "available"},
        {PathRole, "path"},
        {UrlRole, "url"},
        {TextRole, "text"},
        {SourceRole, "source"},
        {AddedAtRole, "addedAt"},
        {ByteSizeRole, "byteSize"},
        {DimensionsRole, "dimensions"},
        {IsFileRole, "isFile"},
        {IsFolderRole, "isFolder"},
        {IsImageRole, "isImage"},
        {HasPreviewRole, "hasPreview"},
    };
}

int ShelfModel::count() const
{
    return int(m_items.size());
}

QList<ShelfItem> ShelfModel::items() const
{
    return m_items;
}

void ShelfModel::setItems(QList<ShelfItem> items)
{
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    Q_EMIT countChanged();
}

ShelfModel::ImportResult ShelfModel::importMimeData(const QMimeData *mimeData)
{
    ImportResult result;
    const MimeImport::Plan plan = MimeImport::plan(mimeData);
    if (!plan.isSupported()) {
        result.rejected = 1;
        result.error = translated("That content can't be collected by Yoink.");
        return result;
    }

    QList<ShelfItem> candidates;
    switch (plan.kind) {
    case MimeImport::Plan::Kind::Files: {
        for (const QString &path : plan.localPaths) {
            if (!QFileInfo::exists(path)) {
                ++result.rejected;
                result.error = translated("Some items are no longer available.");
                continue;
            }
            candidates.append(ShelfItem::makeFile(path));
        }
        for (const QUrl &url : plan.remoteUrls) {
            candidates.append(ShelfItem::makeUrl(url));
        }
        break;
    }
    case MimeImport::Plan::Kind::Image: {
        if (plan.image.isNull()) {
            ++result.rejected;
            result.error = translated("The image could not be read.");
            break;
        }
        if (qint64(plan.image.width()) * plan.image.height() > ShelfLimits::MaxImagePixels) {
            ++result.rejected;
            result.error = translated("The image is too large (maximum 20 megapixels).");
            break;
        }

        ShelfItem item;
        QString error;
        if (!storeImage(plan.image, &item, &error)) {
            ++result.rejected;
            result.error = error;
            break;
        }
        candidates.append(item);
        break;
    }
    case MimeImport::Plan::Kind::Text: {
        if (plan.text.size() > ShelfLimits::MaxTextLength || plan.text.toUtf8().size() > ShelfLimits::MaxTextLength) {
            ++result.rejected;
            result.error = translated("The text is too large (maximum 256 KiB).");
            break;
        }
        candidates.append(plan.textIsUrl ? ShelfItem::makeUrl(plan.textUrl) : ShelfItem::makeText(plan.text));
        break;
    }
    case MimeImport::Plan::Kind::None:
        break;
    }

    if (candidates.isEmpty()) {
        if (result.error.isEmpty()) {
            result.error = translated("There was nothing to collect.");
        }
        return result;
    }

    ImportResult added = addCandidates(std::move(candidates));
    added.rejected += result.rejected;
    if (!result.error.isEmpty()) {
        added.error = result.error;
    }
    return added;
}

bool ShelfModel::removeRow(int row)
{
    if (row < 0 || row >= m_items.size()) {
        return false;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_items.removeAt(row);
    endRemoveRows();
    Q_EMIT contentsChanged();
    Q_EMIT countChanged();
    return true;
}

void ShelfModel::clear()
{
    if (m_items.isEmpty()) {
        return;
    }

    beginResetModel();
    m_items.clear();
    endResetModel();
    Q_EMIT contentsChanged();
    Q_EMIT countChanged();
}

void ShelfModel::insertItemsAt(int row, const QList<ShelfItem> &items)
{
    if (items.isEmpty()) {
        return;
    }

    const int boundedRow = qBound(0, row, int(m_items.size()));
    const int available = ShelfLimits::MaxItems - int(m_items.size());
    if (available <= 0) {
        return;
    }

    QList<ShelfItem> inserted = items.mid(0, available);
    beginInsertRows(QModelIndex(), boundedRow, boundedRow + int(inserted.size()) - 1);
    for (int i = 0; i < inserted.size(); ++i) {
        m_items.insert(boundedRow + i, inserted.at(i));
    }
    endInsertRows();
    Q_EMIT contentsChanged();
    Q_EMIT countChanged();
}

void ShelfModel::refreshAvailability()
{
    for (int row = 0; row < m_items.size(); ++row) {
        ShelfItem &item = m_items[row];
        const bool wasAvailable = item.available;
        const QString oldReason = item.unavailableReason;
        const qint64 oldSize = item.byteSize;
        const QString oldMime = item.mimeDescription;
        item.refreshFromFileSystem();
        if (wasAvailable != item.available || oldReason != item.unavailableReason || oldSize != item.byteSize || oldMime != item.mimeDescription) {
            const QModelIndex modelIndex = index(row, 0);
            Q_EMIT dataChanged(modelIndex, modelIndex);
        }
    }
}

ShelfModel::ImportResult ShelfModel::addCandidates(QList<ShelfItem> candidates)
{
    ImportResult result;
    QSet<QString> keys;
    keys.reserve(m_items.size() + candidates.size());
    for (const ShelfItem &item : std::as_const(m_items)) {
        keys.insert(item.dedupeKey());
    }

    QList<ShelfItem> accepted;
    accepted.reserve(candidates.size());
    for (ShelfItem &candidate : candidates) {
        const QString key = candidate.dedupeKey();
        if (key.isEmpty()) {
            ++result.rejected;
            continue;
        }
        if (keys.contains(key)) {
            ++result.duplicates;
            continue;
        }
        if (m_items.size() + accepted.size() >= ShelfLimits::MaxItems) {
            ++result.rejected;
            result.error = translated("The shelf is full (maximum 60 items).");
            continue;
        }
        keys.insert(key);
        accepted.append(std::move(candidate));
    }

    if (accepted.isEmpty()) {
        if (result.duplicates > 0 && result.rejected == 0) {
            result.error = translated("Already on the shelf.");
        }
        return result;
    }

    beginInsertRows(QModelIndex(), 0, int(accepted.size()) - 1);
    for (int i = accepted.size() - 1; i >= 0; --i) {
        m_items.prepend(accepted.at(i));
    }
    endInsertRows();
    result.added = int(accepted.size());
    Q_EMIT contentsChanged();
    Q_EMIT countChanged();
    return result;
}

bool ShelfModel::storeImage(const QImage &image, ShelfItem *item, QString *error)
{
    if (!item) {
        return false;
    }
    if (m_imageDirectory.isEmpty()) {
        if (error) {
            *error = translated("Image storage is unavailable.");
        }
        return false;
    }
    if (!QDir().mkpath(m_imageDirectory)) {
        if (error) {
            *error = translated("Couldn't create the image storage directory.");
        }
        return false;
    }

    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        if (error) {
            *error = translated("The image could not be encoded.");
        }
        return false;
    }
    if (png.size() > ShelfLimits::MaxImageBytes) {
        if (error) {
            *error = translated("The encoded image is too large (maximum 6 MiB).");
        }
        return false;
    }

    const QString id = ShelfItem::newId();
    const QString filePath = imagePath(id);
    QSaveFile file(filePath);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly) || file.write(png) != png.size() || !file.commit()) {
        if (error) {
            *error = translated("Couldn't store the image.");
        }
        return false;
    }
    QFile::setPermissions(filePath, QFile::ReadOwner | QFile::WriteOwner);

    const QString hash = QString::fromLatin1(QCryptographicHash::hash(png, QCryptographicHash::Sha256).toHex());
    *item = ShelfItem::makeImage(filePath, image.size(), png.size(), hash, id);
    return true;
}

QString ShelfModel::imagePath(const QString &id) const
{
    return QDir(m_imageDirectory).filePath(id + QStringLiteral(".png"));
}
