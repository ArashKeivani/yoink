/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "ShelfItem.h"

#include <QList>
#include <QString>

class QFileInfo;

class ShelfRepository
{
public:
    struct LoadResult {
        QList<ShelfItem> items;
        QString error;
        QString warning;
        bool readOnly = false;
    };

    explicit ShelfRepository(QString dataDirectory = defaultDataDirectory());

    [[nodiscard]] LoadResult load();
    bool save(const QList<ShelfItem> &items, QString *error = nullptr);

    [[nodiscard]] QString dataDirectory() const;
    [[nodiscard]] QString itemsFilePath() const;
    [[nodiscard]] QString imagesDirectory() const;
    [[nodiscard]] bool isReadOnly() const;

    static QString defaultDataDirectory();

private:
    QString backupDamagedFile(QString *backupPath);
    void cleanOrphanedImages(const QList<ShelfItem> &items);
    void migrateLegacyDataDirectory();
    static bool moveEntryInto(const QFileInfo &entry, const QString &targetDirectory);

    QString m_dataDirectory;
    bool m_readOnly = false;
};
