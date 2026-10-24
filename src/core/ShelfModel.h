/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "ShelfItem.h"

#include <QAbstractListModel>
#include <QList>
#include <QString>

class QMimeData;

class ShelfModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct ImportResult {
        int added = 0;
        int duplicates = 0;
        int rejected = 0;
        QString error;

        [[nodiscard]] bool hasChanges() const
        {
            return added > 0;
        }
    };

    enum Role {
        IdRole = Qt::UserRole + 1,
        KindRole,
        KindNameRole,
        NameRole,
        DetailRole,
        DescriptionRole,
        IconSourceRole,
        PreviewSourceRole,
        AvailableRole,
        PathRole,
        UrlRole,
        TextRole,
        SourceRole,
        AddedAtRole,
        ByteSizeRole,
        DimensionsRole,
        IsFileRole,
        IsFolderRole,
        IsImageRole,
        HasPreviewRole,
    };
    Q_ENUM(Role)

    explicit ShelfModel(QString imageDirectory, QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int count() const;
    [[nodiscard]] QList<ShelfItem> items() const;
    void setItems(QList<ShelfItem> items);

    ImportResult importMimeData(const QMimeData *mimeData);
    bool removeRow(int row);
    void clear();
    void insertItemsAt(int row, const QList<ShelfItem> &items);
    void refreshAvailability();

Q_SIGNALS:
    void contentsChanged();
    void countChanged();

private:
    ImportResult addCandidates(QList<ShelfItem> candidates);
    bool storeImage(const QImage &image, ShelfItem *item, QString *error);
    QString imagePath(const QString &id) const;

    QList<ShelfItem> m_items;
    QString m_imageDirectory;
};
