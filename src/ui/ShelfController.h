/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "core/AppSettings.h"
#include "core/ShelfItem.h"
#include "core/ShelfModel.h"
#include "core/ShelfPlacement.h"
#include "core/Theme.h"

#include <QList>
#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QStringList>
#include <QTimer>
#include <QVariant>

#include <memory>

class AppSettings;
class DropSurface;
class QMimeData;
class QPixmap;
class QScreen;
class ShelfModel;
class ShelfRepository;
class ShelfSurface;
class Theme;

class ShelfController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ShelfModel *model READ model CONSTANT)
    Q_PROPERTY(AppSettings *settings READ settings CONSTANT)
    Q_PROPERTY(Theme *theme READ theme CONSTANT)

    Q_PROPERTY(bool expanded READ expanded NOTIFY expandedChanged)
    Q_PROPERTY(bool externalDragActive READ externalDragActive NOTIFY externalDragChanged)
    Q_PROPERTY(QString externalDragSummary READ externalDragSummary NOTIFY externalDragChanged)
    Q_PROPERTY(bool internalDragActive READ internalDragActive NOTIFY internalDragChanged)

    Q_PROPERTY(bool settingsOpen READ settingsOpen NOTIFY overlayChanged)
    Q_PROPERTY(bool menuOpen READ menuOpen NOTIFY overlayChanged)
    Q_PROPERTY(int itemMenuIndex READ itemMenuIndex NOTIFY overlayChanged)
    Q_PROPERTY(bool itemMenuOpen READ itemMenuOpen NOTIFY overlayChanged)
    Q_PROPERTY(bool overlayOpen READ overlayOpen NOTIFY overlayChanged)

    Q_PROPERTY(QString toastMessage READ toastMessage NOTIFY toastChanged)
    Q_PROPERTY(bool toastVisible READ toastVisible NOTIFY toastChanged)
    Q_PROPERTY(bool toastUndoAvailable READ toastUndoAvailable NOTIFY toastChanged)
    Q_PROPERTY(bool toastIsError READ toastIsError NOTIFY toastChanged)

    Q_PROPERTY(bool readOnly READ readOnly CONSTANT)
    Q_PROPERTY(bool autostartEnabled READ autostartEnabled NOTIFY autostartChanged)
    Q_PROPERTY(QStringList screenNames READ screenNames NOTIFY screensChanged)
    Q_PROPERTY(int itemCount READ itemCount NOTIFY itemCountChanged)

    Q_PROPERTY(int stageWidth READ stageWidth NOTIFY layoutChanged)
    Q_PROPERTY(int stageHeight READ stageHeight NOTIFY layoutChanged)
    Q_PROPERTY(int cardWidth READ cardWidth NOTIFY layoutChanged)
    Q_PROPERTY(int cardHeight READ cardHeight NOTIFY layoutChanged)
    Q_PROPERTY(QRectF handleRect READ handleRect NOTIFY layoutChanged)
    Q_PROPERTY(QRectF handleHitRect READ handleHitRect NOTIFY layoutChanged)
    Q_PROPERTY(bool verticalEdge READ verticalEdge NOTIFY layoutChanged)
    Q_PROPERTY(int edge READ edge NOTIFY layoutChanged)

    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString storageLocation READ storageLocation CONSTANT)

public:
    explicit ShelfController(ShelfSurface *view, QObject *parent = nullptr);
    ~ShelfController() override;

    void start(bool quiet);
    void shutdown();
    void handleIpcMessage(const QString &message);

    [[nodiscard]] ShelfModel *model() const;
    [[nodiscard]] AppSettings *settings() const;
    [[nodiscard]] Theme *theme() const;

    [[nodiscard]] bool expanded() const;
    [[nodiscard]] bool externalDragActive() const;
    [[nodiscard]] QString externalDragSummary() const;
    [[nodiscard]] bool internalDragActive() const;

    [[nodiscard]] bool settingsOpen() const;
    [[nodiscard]] bool menuOpen() const;
    [[nodiscard]] int itemMenuIndex() const;
    [[nodiscard]] bool itemMenuOpen() const;
    [[nodiscard]] bool overlayOpen() const;

    [[nodiscard]] QString toastMessage() const;
    [[nodiscard]] bool toastVisible() const;
    [[nodiscard]] bool toastUndoAvailable() const;
    [[nodiscard]] bool toastIsError() const;

    [[nodiscard]] bool readOnly() const;
    [[nodiscard]] bool autostartEnabled() const;
    [[nodiscard]] QStringList screenNames() const;
    [[nodiscard]] int itemCount() const;

    [[nodiscard]] int stageWidth() const;
    [[nodiscard]] int stageHeight() const;
    [[nodiscard]] int cardWidth() const;
    [[nodiscard]] int cardHeight() const;
    [[nodiscard]] QRectF handleRect() const;
    [[nodiscard]] QRectF handleHitRect() const;
    [[nodiscard]] bool verticalEdge() const;
    [[nodiscard]] int edge() const;

    [[nodiscard]] QString version() const;
    [[nodiscard]] QString storageLocation() const;

    Q_INVOKABLE void showFromUser();
    Q_INVOKABLE void toggleFromTray();
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void closeSettings();
    Q_INVOKABLE void openMenu();
    Q_INVOKABLE void closeMenu();
    Q_INVOKABLE void openItemMenu(int index);
    Q_INVOKABLE void closeItemMenu();
    Q_INVOKABLE void closeShelf();
    Q_INVOKABLE void handleEscape();
    Q_INVOKABLE void togglePinned();
    Q_INVOKABLE void setPinned(bool pinned);
    Q_INVOKABLE void activateItem(int index);
    Q_INVOKABLE void startItemDrag(int index);
    Q_INVOKABLE void removeItem(int index);
    Q_INVOKABLE void clearShelf();
    Q_INVOKABLE void undoLastRemoval();
    Q_INVOKABLE void copyItem(int index);
    Q_INVOKABLE void openContainingFolder(int index);
    Q_INVOKABLE void setEdge(int edge);
    Q_INVOKABLE void setPosition(int position);
    Q_INVOKABLE void selectScreen(const QString &screenName);
    Q_INVOKABLE void setAutostartEnabled(bool enabled);
    Q_INVOKABLE void quit();
    Q_INVOKABLE [[nodiscard]] QVariant itemData(int index, const QString &role) const;

Q_SIGNALS:
    void expandedChanged();
    void externalDragChanged();
    void internalDragChanged();
    void overlayChanged();
    void toastChanged();
    void autostartChanged();
    void screensChanged();
    void itemCountChanged();
    void layoutChanged();

private:
    struct UndoEntry {
        int row = 0;
        QList<ShelfItem> items;
        QString description;
        bool valid = false;
    };

    void connectSurface();
    void refreshScreens();
    void applyLayout();
    void updateInputMask();
    void openShelf();
    void setExpanded(bool expanded);
    void scheduleAutoHide();
    void closeOverlays();
    void showStartupMessage();
    void showToast(const QString &message, bool undoAvailable = false, bool isError = false, int durationMs = 4500);
    void persist();
    bool ensureModifiable();
    void invalidateUndo();
    [[nodiscard]] ShelfItem itemAt(int index) const;
    [[nodiscard]] QPixmap createDragPixmap(const ShelfItem &item) const;

    void onPointerInsideChanged(bool inside);
    void onExternalDragEntered(const QString &summary, int count);
    void onExternalDragLeft();
    void onMimeDropped(const QMimeData *mimeData);
    void onModelContentsChanged();
    void onModelCountChanged();
    void onSettingsPinnedChanged();
    void onSettingsEdgeChanged();
    void onSettingsPositionChanged();
    void onSettingsScreenNameChanged();
    void onSettingsSaveFailed(const QString &message);
    void onCloseTimeout();
    void onMaskTimeout();
    void onToastTimeout();

    ShelfSurface *m_view = nullptr;
    AppSettings *m_settings = nullptr;
    Theme *m_theme = nullptr;
    std::unique_ptr<ShelfRepository> m_repository;
    ShelfModel *m_model = nullptr;
    DropSurface *m_dropSurface = nullptr;

    QTimer m_closeTimer;
    QTimer m_maskTimer;
    QTimer m_toastTimer;

    bool m_expanded = false;
    bool m_hovered = false;
    bool m_holdOpen = false;
    bool m_externalDragActive = false;
    QString m_externalDragSummary;
    int m_externalDragCount = 0;
    bool m_internalDragActive = false;

    bool m_settingsOpen = false;
    bool m_menuOpen = false;
    int m_itemMenuIndex = -1;

    QString m_toastMessage;
    bool m_toastVisible = false;
    bool m_toastUndoAvailable = false;
    bool m_toastIsError = false;

    bool m_readOnly = false;
    bool m_autostartEnabled = false;
    bool m_dirty = false;
    bool m_saveErrorShown = false;
    bool m_lastSaveFailed = false;
    bool m_shuttingDown = false;
    bool m_updatingScreens = false;
    bool m_startupToastShown = false;

    QString m_startupError;
    QString m_startupWarning;
    QString m_pendingToast;
    bool m_pendingToastIsError = false;

    QStringList m_screenNames;
    QPointer<QScreen> m_screen;
    QMetaObject::Connection m_screenGeometryConnection;
    QMetaObject::Connection m_screenAvailableConnection;
    ShelfGeometry::Layout m_layout;

    UndoEntry m_undo;
};
