/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QMetaType>
#include <QSize>
#include <QString>
#include <QStringView>
#include <QUrl>

struct ShelfItem {
    enum class Kind {
        File,
        Folder,
        Text,
        Url,
        Image,
    };

    QString id;
    Kind kind = Kind::File;

    // File and folder items.
    QString path;

    // Text items.
    QString text;

    // URL items.
    QString url;

    // Image items. imageFile is an absolute path below Yoink's data directory.
    QString imageFile;
    QString imageHash;
    QSize imageSize;

    qint64 byteSize = -1;
    QDateTime modified;
    QDateTime addedAt;

    bool available = true;
    QString unavailableReason;

    // Derived, non-persistent metadata.
    QString mimeDescription;
    QString mimeTypeName;

    [[nodiscard]] bool isFileSystemItem() const;
    [[nodiscard]] bool hasPreview() const;
    [[nodiscard]] QString displayName() const;
    [[nodiscard]] QString secondaryText() const;
    [[nodiscard]] QString accessibleDescription() const;
    [[nodiscard]] QString dedupeKey() const;
    [[nodiscard]] QString iconKey() const;

    void refreshFromFileSystem();

    static ShelfItem makeFile(const QString &path, const QString &id = {});
    static ShelfItem makeText(const QString &text, const QString &id = {});
    static ShelfItem makeUrl(const QUrl &url, const QString &id = {});
    static ShelfItem makeImage(const QString &imageFile, const QSize &imageSize, qint64 byteSize, const QString &imageHash, const QString &id = {});

    static QString newId();
    static QString kindName(Kind kind);
    static Kind kindFromString(QStringView name, Kind fallback = Kind::File);
};

Q_DECLARE_METATYPE(ShelfItem)
