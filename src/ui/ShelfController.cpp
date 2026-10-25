/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ShelfController.h"

#include "app/Autostart.h"
#include "core/AppSettings.h"
#include "core/ShelfModel.h"
#include "core/ShelfRepository.h"
#include "core/Theme.h"
#include "ui/DropSurface.h"
#include "ui/ShelfSurface.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDrag>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QUrl>

#include <algorithm>
#include <utility>

namespace
{
ShelfGeometry::Edge geometryEdge(int edge)
{
    switch (edge) {
    case AppSettings::Left:
        return ShelfGeometry::Edge::Left;
    case AppSettings::Top:
        return ShelfGeometry::Edge::Top;
    case AppSettings::Bottom:
        return ShelfGeometry::Edge::Bottom;
    case AppSettings::Right:
    default:
        return ShelfGeometry::Edge::Right;
    }
}

QString screenLabel(QScreen *screen, int index)
{
    if (screen && !screen->name().isEmpty()) {
        return screen->name();
    }
    return QCoreApplication::translate("ShelfController", "Screen %1").arg(index + 1);
}

QIcon iconForItem(const ShelfItem &item)
{
    switch (item.kind) {
    case ShelfItem::Kind::File: {
        QFileIconProvider provider;
        return provider.icon(QFileInfo(item.path));
    }
    case ShelfItem::Kind::Folder:
        return QIcon::fromTheme(QStringLiteral("folder"));
    case ShelfItem::Kind::Text:
        return QIcon::fromTheme(QStringLiteral("text-x-generic"));
    case ShelfItem::Kind::Url:
        return QIcon::fromTheme(QStringLiteral("internet-web-browser"));
    case ShelfItem::Kind::Image:
        return QIcon::fromTheme(QStringLiteral("image-x-generic"));
    }
    return QIcon::fromTheme(QStringLiteral("text-x-generic"));
}
} // namespace

ShelfController::ShelfController(ShelfSurface *view, QObject *parent)
    : QObject(parent)
    , m_view(view)
    , m_settings(new AppSettings(this))
    , m_theme(new Theme(this))
    , m_repository(std::make_unique<ShelfRepository>())
{
    const ShelfRepository::LoadResult loaded = m_repository->load();
    m_readOnly = loaded.readOnly;
    m_startupError = loaded.error;
    m_startupWarning = loaded.warning;

    m_model = new ShelfModel(m_repository->imagesDirectory(), this);
    m_model->setItems(loaded.items);
    m_autostartEnabled = Autostart::isEnabled();

    m_closeTimer.setSingleShot(true);
    m_maskTimer.setSingleShot(true);
    m_toastTimer.setSingleShot(true);
    connect(&m_closeTimer, &QTimer::timeout, this, &ShelfController::onCloseTimeout);
    connect(&m_maskTimer, &QTimer::timeout, this, &ShelfController::onMaskTimeout);
    connect(&m_toastTimer, &QTimer::timeout, this, &ShelfController::onToastTimeout);

    connect(m_model, &ShelfModel::contentsChanged, this, &ShelfController::onModelContentsChanged);
    connect(m_model, &ShelfModel::countChanged, this, &ShelfController::onModelCountChanged);
    connect(m_model, &ShelfModel::countChanged, this, [this] {
        if (m_model->count() <= 0) {
            clearSelection();
        }
    });

    connect(m_settings, &AppSettings::pinnedChanged, this, &ShelfController::onSettingsPinnedChanged);
    connect(m_settings, &AppSettings::edgeChanged, this, &ShelfController::onSettingsEdgeChanged);
    connect(m_settings, &AppSettings::positionChanged, this, &ShelfController::onSettingsPositionChanged);
    connect(m_settings, &AppSettings::screenNameChanged, this, &ShelfController::onSettingsScreenNameChanged);
    connect(m_settings, &AppSettings::saveFailed, this, &ShelfController::onSettingsSaveFailed);

    if (QGuiApplication *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        connect(app, &QGuiApplication::screenAdded, this, &ShelfController::refreshScreens);
        connect(app, &QGuiApplication::screenRemoved, this, &ShelfController::refreshScreens);
        connect(app, &QGuiApplication::primaryScreenChanged, this, &ShelfController::refreshScreens);
    }

    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        m_layout = ShelfGeometry::computeLayout(screen->geometry(), screen->availableGeometry(), geometryEdge(m_settings->edge()), m_settings->position());
    } else {
        const QRect fallback(0, 0, 356, 456);
        m_layout = ShelfGeometry::computeLayout(fallback, fallback, geometryEdge(m_settings->edge()), m_settings->position());
    }

    refreshScreens();
}

ShelfController::~ShelfController() = default;

void ShelfController::start(bool quiet)
{
    Q_UNUSED(quiet);
    if (!m_view) {
        return;
    }

    connectSurface();
    refreshScreens();
    applyLayout();
    m_view->show();
    m_view->setInputExpanded(false);

    if (m_settings->pinned()) {
        openShelf();
    }
}

void ShelfController::shutdown()
{
    m_shuttingDown = true;
    m_closeTimer.stop();
    m_maskTimer.stop();
    m_toastTimer.stop();
    if (m_dirty) {
        persist();
    }
    if (m_settings) {
        m_settings->flush();
    }
}

void ShelfController::connectSurface()
{
    m_dropSurface = qobject_cast<DropSurface *>(m_view->rootObject());
    if (!m_dropSurface) {
        qWarning() << "Yoink: the QML root object is not a DropSurface; external drops are disabled.";
        return;
    }

    connect(m_dropSurface, &DropSurface::externalDragEntered, this, &ShelfController::onExternalDragEntered);
    connect(m_dropSurface, &DropSurface::externalDragLeft, this, &ShelfController::onExternalDragLeft);
    connect(m_dropSurface, &DropSurface::mimeDropped, this, &ShelfController::onMimeDropped);
    connect(m_view, &ShelfSurface::pointerInsideChanged, this, &ShelfController::onPointerInsideChanged);
}

void ShelfController::refreshScreens()
{
    if (m_updatingScreens) {
        return;
    }
    m_updatingScreens = true;

    const QList<QScreen *> screens = QGuiApplication::screens();
    QStringList names;
    names.reserve(screens.size());
    for (int i = 0; i < screens.size(); ++i) {
        names.append(screenLabel(screens.at(i), i));
    }
    names.sort(Qt::CaseInsensitive);

    const QString previousName = m_settings->screenName();
    QScreen *chosen = nullptr;
    for (int i = 0; i < screens.size(); ++i) {
        if (screenLabel(screens.at(i), i) == previousName) {
            chosen = screens.at(i);
            break;
        }
    }
    if (!chosen) {
        chosen = QGuiApplication::primaryScreen();
    }
    if (!chosen && !screens.isEmpty()) {
        chosen = screens.first();
    }

    if (chosen) {
        const int index = int(screens.indexOf(chosen));
        const QString label = screenLabel(chosen, index);
        if (m_settings->screenName() != label) {
            m_settings->setScreenName(label);
        }
    }

    if (m_screenNames != names) {
        m_screenNames = names;
        Q_EMIT screensChanged();
    }

    if (m_screen != chosen) {
        disconnect(m_screenGeometryConnection);
        disconnect(m_screenAvailableConnection);
        m_screen = chosen;
        if (m_screen) {
            m_screenGeometryConnection = connect(m_screen, &QScreen::geometryChanged, this, [this](const QRect &) { applyLayout(); });
            m_screenAvailableConnection = connect(m_screen, &QScreen::availableGeometryChanged, this, [this](const QRect &) { applyLayout(); });
        }
    }

    m_updatingScreens = false;
    applyLayout();
}

void ShelfController::applyLayout()
{
    if (!m_screen || !m_view) {
        return;
    }

    m_layout = ShelfGeometry::computeLayout(m_screen->geometry(), m_screen->availableGeometry(), geometryEdge(m_settings->edge()), m_settings->position());
    m_view->applyLayout(m_layout, m_screen);
    Q_EMIT layoutChanged();
}

void ShelfController::updateInputMask()
{
    if (!m_view) {
        return;
    }
    m_view->setInputExpanded(m_expanded);
}

void ShelfController::openShelf()
{
    if (!m_view) {
        return;
    }
    m_closeTimer.stop();
    if (!m_view->isVisible()) {
        m_view->show();
    }
    setExpanded(true);
    m_view->setInputExpanded(true);
    showStartupMessage();
}

void ShelfController::setExpanded(bool expanded)
{
    if (m_expanded == expanded) {
        if (expanded && m_view) {
            m_view->setInputExpanded(true);
        }
        return;
    }

    m_expanded = expanded;
    if (m_view) {
        if (expanded) {
            m_maskTimer.stop();
            m_view->setInputExpanded(true);
        } else {
            m_maskTimer.start(qMax(0, m_theme ? m_theme->duration(220) : 220));
        }
    }
    Q_EMIT expandedChanged();
}

void ShelfController::scheduleAutoHide()
{
    if (!m_expanded) {
        return;
    }
    if (m_settings->pinned() || m_holdOpen || m_externalDragActive || m_internalDragActive || overlayOpen() || m_hovered) {
        m_closeTimer.stop();
        return;
    }
    m_closeTimer.start(m_toastVisible ? 1200 : 120);
}

void ShelfController::closeOverlays()
{
    if (!m_settingsOpen && !m_menuOpen && m_itemMenuIndex < 0) {
        return;
    }
    m_settingsOpen = false;
    m_menuOpen = false;
    m_itemMenuIndex = -1;
    Q_EMIT overlayChanged();
}

void ShelfController::showStartupMessage()
{
    if (m_startupToastShown) {
        return;
    }

    QString message = m_startupError;
    bool isError = !message.isEmpty() || m_readOnly;
    if (message.isEmpty() && m_readOnly) {
        message = tr("The shelf is read-only because its data was created by a newer version of Yoink.");
    }
    if (message.isEmpty() && !m_startupWarning.isEmpty()) {
        message = m_startupWarning;
        isError = false;
    }
    if (message.isEmpty()) {
        return;
    }

    m_startupToastShown = true;
    showToast(message, false, isError, 9000);
}

void ShelfController::showToast(const QString &message, bool undoAvailable, bool isError, int durationMs)
{
    if (m_shuttingDown) {
        return;
    }
    m_toastMessage = message;
    m_toastUndoAvailable = undoAvailable;
    m_toastIsError = isError;
    m_toastVisible = true;
    m_toastTimer.start(qMax(1000, durationMs));
    Q_EMIT toastChanged();
}

void ShelfController::persist()
{
    if (m_readOnly || !m_repository) {
        return;
    }

    QString error;
    const bool saved = m_repository->save(m_model->items(), &error);
    m_lastSaveFailed = !saved;
    if (saved) {
        m_dirty = false;
        m_saveErrorShown = false;
        return;
    }

    m_dirty = true;
    qWarning() << "Yoink: couldn't save the shelf:" << error;
    if (!m_saveErrorShown) {
        m_saveErrorShown = true;
        showToast(tr("Couldn't save shelf changes: %1").arg(error), false, true, 8000);
    }
}

bool ShelfController::ensureModifiable()
{
    if (!m_readOnly) {
        return true;
    }
    showToast(tr("The shelf is read-only because its data was created by a newer version of Yoink."), false, true, 8000);
    return false;
}

void ShelfController::invalidateUndo()
{
    m_undo = {};
}

ShelfItem ShelfController::itemAt(int index) const
{
    if (!m_model || index < 0 || index >= m_model->count()) {
        return {};
    }
    return m_model->items().at(index);
}

QPixmap ShelfController::createDragPixmap(const ShelfItem &item, int count) const
{
    const qreal dpr = m_view ? m_view->devicePixelRatio() : 1.0;
    const QSize logicalSize(240, 64);
    QPixmap pixmap(qRound(logicalSize.width() * dpr), qRound(logicalSize.height() * dpr));
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setPen(QPen(m_theme->border(), 1));
    painter.setBrush(m_theme->menu());
    painter.drawRoundedRect(QRectF(0.5, 0.5, logicalSize.width() - 1, logicalSize.height() - 1), 12, 12);

    QIcon icon = iconForItem(item);
    if (icon.isNull()) {
        icon = QIcon::fromTheme(QStringLiteral("document-multiple"));
    }
    const QPixmap iconPixmap = icon.pixmap(QSize(36, 36), dpr);
    if (!iconPixmap.isNull()) {
        painter.drawPixmap(QPointF(14, 14), iconPixmap);
    }

    QFont nameFont = QGuiApplication::font();
    nameFont.setPointSizeF(m_theme->bodyPointSize());
    nameFont.setWeight(QFont::DemiBold);
    QFont detailFont = QGuiApplication::font();
    detailFont.setPointSizeF(m_theme->smallPointSize());

    painter.setFont(nameFont);
    painter.setPen(m_theme->text());
    QString name = QFontMetricsF(nameFont).elidedText(item.displayName(), Qt::ElideMiddle, 164);
    if (count > 1) {
        name = QFontMetricsF(nameFont).elidedText(tr("%1 and %2 more").arg(item.displayName()).arg(count - 1), Qt::ElideMiddle, 164);
    }
    painter.drawText(QRectF(62, 11, 166, 21), Qt::AlignLeft | Qt::AlignVCenter, name);

    painter.setFont(detailFont);
    painter.setPen(m_theme->textMuted());
    QString detail = QFontMetricsF(detailFont).elidedText(item.secondaryText(), Qt::ElideMiddle, 166);
    if (count > 1) {
        detail = QFontMetricsF(detailFont).elidedText(tr("%n item(s) selected", nullptr, count), Qt::ElideMiddle, 166);
    }
    painter.drawText(QRectF(62, 33, 166, 19), Qt::AlignLeft | Qt::AlignVCenter, detail);
    painter.end();
    return pixmap;
}

void ShelfController::onPointerInsideChanged(bool inside)
{
    m_hovered = inside;
    if (inside) {
        m_closeTimer.stop();
        if (!m_expanded) {
            openShelf();
        }
        return;
    }
    scheduleAutoHide();
}

void ShelfController::onExternalDragEntered(const QString &summary, int count)
{
    m_externalDragActive = true;
    m_externalDragSummary = summary;
    m_externalDragCount = count;
    m_closeTimer.stop();
    openShelf();
    Q_EMIT externalDragChanged();
}

void ShelfController::onExternalDragLeft()
{
    if (!m_externalDragActive) {
        return;
    }
    m_externalDragActive = false;
    m_externalDragSummary.clear();
    m_externalDragCount = 0;
    Q_EMIT externalDragChanged();
    scheduleAutoHide();
}

void ShelfController::onMimeDropped(const QMimeData *mimeData)
{
    if (m_externalDragActive) {
        m_externalDragActive = false;
        m_externalDragSummary.clear();
        m_externalDragCount = 0;
        Q_EMIT externalDragChanged();
    }
    openShelf();

    if (!ensureModifiable()) {
        scheduleAutoHide();
        return;
    }

    const ShelfModel::ImportResult result = m_model->importMimeData(mimeData);
    if (result.added > 0) {
        invalidateUndo();
    }

    QStringList parts;
    if (result.added > 0) {
        parts.append(tr("Added %n item(s)", nullptr, result.added));
    }
    if (result.duplicates > 0) {
        parts.append(tr("%n duplicate(s) skipped", nullptr, result.duplicates));
    }
    if (result.rejected > 0) {
        parts.append(tr("%n item(s) not added", nullptr, result.rejected));
    }
    QString message = parts.join(QStringLiteral(" · "));
    if (!result.error.isEmpty()) {
        message = message.isEmpty() ? result.error : message + QStringLiteral(" — ") + result.error;
    }
    if (!message.isEmpty() && !m_lastSaveFailed) {
        showToast(message, false, result.added == 0 && !result.error.isEmpty(), result.added == 0 ? 2500 : 4000);
    }
    scheduleAutoHide();
}

void ShelfController::onModelContentsChanged()
{
    clearSelection();
    m_dirty = true;
    persist();
}

void ShelfController::onModelCountChanged()
{
    Q_EMIT itemCountChanged();
    if (m_itemMenuIndex >= m_model->count()) {
        m_itemMenuIndex = -1;
        Q_EMIT overlayChanged();
    }
}

void ShelfController::onSettingsPinnedChanged()
{
    if (m_settings->pinned()) {
        m_holdOpen = false;
        openShelf();
    } else {
        scheduleAutoHide();
    }
}

void ShelfController::onSettingsEdgeChanged()
{
    applyLayout();
}

void ShelfController::onSettingsPositionChanged()
{
    applyLayout();
}

void ShelfController::onSettingsScreenNameChanged()
{
    refreshScreens();
}

void ShelfController::onSettingsSaveFailed(const QString &message)
{
    showToast(message, false, true, 7000);
}

void ShelfController::onCloseTimeout()
{
    if (!m_hovered && !m_settings->pinned() && !m_holdOpen && !m_externalDragActive && !m_internalDragActive && !overlayOpen()) {
        setExpanded(false);
    }
}

void ShelfController::onMaskTimeout()
{
    if (!m_expanded && m_view) {
        m_view->setInputExpanded(false);
    }
}

void ShelfController::onToastTimeout()
{
    m_toastVisible = false;
    m_toastUndoAvailable = false;
    Q_EMIT toastChanged();
}

ShelfModel *ShelfController::model() const
{
    return m_model;
}

AppSettings *ShelfController::settings() const
{
    return m_settings;
}

Theme *ShelfController::theme() const
{
    return m_theme;
}

bool ShelfController::expanded() const
{
    return m_expanded;
}

bool ShelfController::externalDragActive() const
{
    return m_externalDragActive;
}

QString ShelfController::externalDragSummary() const
{
    return m_externalDragSummary;
}

bool ShelfController::internalDragActive() const
{
    return m_internalDragActive;
}

bool ShelfController::settingsOpen() const
{
    return m_settingsOpen;
}

bool ShelfController::menuOpen() const
{
    return m_menuOpen;
}

int ShelfController::itemMenuIndex() const
{
    return m_itemMenuIndex;
}

bool ShelfController::itemMenuOpen() const
{
    return m_itemMenuIndex >= 0;
}

bool ShelfController::overlayOpen() const
{
    return m_settingsOpen || m_menuOpen || m_itemMenuIndex >= 0;
}

QString ShelfController::toastMessage() const
{
    return m_toastMessage;
}

bool ShelfController::toastVisible() const
{
    return m_toastVisible;
}

bool ShelfController::toastUndoAvailable() const
{
    return m_toastUndoAvailable;
}

bool ShelfController::toastIsError() const
{
    return m_toastIsError;
}

bool ShelfController::readOnly() const
{
    return m_readOnly;
}

bool ShelfController::autostartEnabled() const
{
    return m_autostartEnabled;
}

QStringList ShelfController::screenNames() const
{
    return m_screenNames;
}

int ShelfController::itemCount() const
{
    return m_model ? m_model->count() : 0;
}

int ShelfController::stageWidth() const
{
    return m_layout.stageSize.width();
}

int ShelfController::stageHeight() const
{
    return m_layout.stageSize.height();
}

int ShelfController::cardWidth() const
{
    return m_layout.cardSize.width();
}

int ShelfController::cardHeight() const
{
    return m_layout.cardSize.height();
}

QRectF ShelfController::handleRect() const
{
    return QRectF(m_layout.handleRect);
}

QRectF ShelfController::handleHitRect() const
{
    return QRectF(m_layout.handleHitRect);
}

QRectF ShelfController::cardRect() const
{
    return QRectF(m_layout.cardRect);
}

bool ShelfController::verticalEdge() const
{
    return m_layout.verticalEdge;
}

int ShelfController::edge() const
{
    return int(m_layout.edge);
}

QString ShelfController::version() const
{
    const QString appVersion = QCoreApplication::applicationVersion();
    return appVersion.isEmpty() ? QStringLiteral(YOINK_VERSION) : appVersion;
}

QString ShelfController::storageLocation() const
{
    return m_repository ? m_repository->dataDirectory() : QString();
}

void ShelfController::showFromUser()
{
    openShelf();
}

void ShelfController::toggleFromTray()
{
    if (m_expanded) {
        closeShelf();
        return;
    }
    m_holdOpen = true;
    openShelf();
}

void ShelfController::openSettings()
{
    openShelf();
    m_settingsOpen = true;
    m_menuOpen = false;
    m_itemMenuIndex = -1;
    m_closeTimer.stop();
    Q_EMIT overlayChanged();
    if (m_view) {
        m_view->requestActivate();
    }
}

void ShelfController::closeSettings()
{
    if (!m_settingsOpen) {
        return;
    }
    m_settingsOpen = false;
    Q_EMIT overlayChanged();
    scheduleAutoHide();
}

void ShelfController::openMenu()
{
    openShelf();
    m_menuOpen = true;
    m_settingsOpen = false;
    m_itemMenuIndex = -1;
    m_closeTimer.stop();
    Q_EMIT overlayChanged();
    if (m_view) {
        m_view->requestActivate();
    }
}

void ShelfController::closeMenu()
{
    if (!m_menuOpen) {
        return;
    }
    m_menuOpen = false;
    Q_EMIT overlayChanged();
    scheduleAutoHide();
}

void ShelfController::openItemMenu(int index)
{
    if (!m_model || index < 0 || index >= m_model->count()) {
        return;
    }
    openShelf();
    m_itemMenuIndex = index;
    m_menuOpen = false;
    m_settingsOpen = false;
    m_closeTimer.stop();
    Q_EMIT overlayChanged();
    if (m_view) {
        m_view->requestActivate();
    }
}

void ShelfController::closeItemMenu()
{
    if (m_itemMenuIndex < 0) {
        return;
    }
    m_itemMenuIndex = -1;
    Q_EMIT overlayChanged();
    scheduleAutoHide();
}

void ShelfController::closeShelf()
{
    closeOverlays();
    m_holdOpen = false;
    setExpanded(false);
}

void ShelfController::handleEscape()
{
    if (m_settingsOpen) {
        closeSettings();
        return;
    }
    if (m_menuOpen) {
        closeMenu();
        return;
    }
    if (m_itemMenuIndex >= 0) {
        closeItemMenu();
        return;
    }
    if (m_selectedRows.size() > 0) {
        clearSelection();
        return;
    }
    if (m_expanded) {
        closeShelf();
    }
}

void ShelfController::togglePinned()
{
    m_settings->setPinned(!m_settings->pinned());
}

void ShelfController::setPinned(bool pinned)
{
    m_settings->setPinned(pinned);
}

void ShelfController::activateItem(int index)
{
    m_model->refreshAvailability();
    const ShelfItem item = itemAt(index);
    if (item.id.isEmpty()) {
        return;
    }

    if ((item.kind == ShelfItem::Kind::File || item.kind == ShelfItem::Kind::Folder || item.kind == ShelfItem::Kind::Image) && !item.available) {
        showToast(item.unavailableReason.isEmpty() ? tr("That item is no longer available.") : item.unavailableReason, false, true, 6000);
        return;
    }

    if (item.kind == ShelfItem::Kind::Text) {
        copyItem(index);
        return;
    }

    QUrl target;
    switch (item.kind) {
    case ShelfItem::Kind::File:
    case ShelfItem::Kind::Folder:
    case ShelfItem::Kind::Image:
        target = QUrl::fromLocalFile(item.path.isEmpty() ? item.imageFile : item.path);
        break;
    case ShelfItem::Kind::Url:
        target = QUrl(item.url);
        break;
    case ShelfItem::Kind::Text:
        break;
    }
    if (!target.isValid() || !QDesktopServices::openUrl(target)) {
        showToast(tr("Couldn't open that item."), false, true, 6000);
    }
}

void ShelfController::startItemDrag(int index)
{
    if (index >= 0 && m_model && index < m_model->count()) {
        startItemsDrag(QList<int>{index});
    }
}

void ShelfController::startItemsDrag(const QList<int> &indices)
{
    if (indices.isEmpty() || m_internalDragActive || !m_view || !m_model) {
        return;
    }

    m_model->refreshAvailability();
    QList<ShelfItem> items;
    items.reserve(indices.size());
    int skippedUnavailable = 0;
    for (int index : indices) {
        ShelfItem item = itemAt(index);
        if (item.id.isEmpty()) {
            continue;
        }
        if ((item.kind == ShelfItem::Kind::File || item.kind == ShelfItem::Kind::Folder || item.kind == ShelfItem::Kind::Image) && !item.available) {
            if (indices.size() == 1) {
                showToast(item.unavailableReason.isEmpty() ? tr("That item is no longer available.") : item.unavailableReason, false, true, 6000);
                return;
            }
            ++skippedUnavailable;
            continue;
        }
        items.append(item);
    }
    if (items.isEmpty()) {
        showToast(tr("Nothing to drag."), false, false, 2500);
        return;
    }

    auto mimeData = std::make_unique<QMimeData>();
    Qt::DropActions actions = Qt::CopyAction;
    QList<QUrl> urls;
    QStringList textParts;
    for (const ShelfItem &item : items) {
        switch (item.kind) {
        case ShelfItem::Kind::File:
        case ShelfItem::Kind::Folder:
            urls.append(QUrl::fromLocalFile(item.path));
            actions |= Qt::MoveAction;
            break;
        case ShelfItem::Kind::Image:
            urls.append(QUrl::fromLocalFile(item.imageFile));
            actions |= Qt::MoveAction;
            break;
        case ShelfItem::Kind::Url:
            urls.append(QUrl(item.url));
            textParts.append(item.url);
            break;
        case ShelfItem::Kind::Text:
            textParts.append(item.text);
            break;
        }
    }

    if (!urls.isEmpty()) {
        mimeData->setUrls(urls);
    }
    if (!textParts.isEmpty()) {
        mimeData->setText(textParts.join(QStringLiteral("\n")));
    }
    if (items.count() == 1 && items.first().kind == ShelfItem::Kind::Image) {
        const QImage image(items.first().imageFile);
        if (!image.isNull()) {
            mimeData->setImageData(image);
        }
    }
    if (mimeData->urls().isEmpty() && mimeData->text().isEmpty()) {
        showToast(tr("There is nothing to drag."), false, false, 2500);
        return;
    }
    if (skippedUnavailable > 0) {
        showToast(tr("Skipped %n unavailable item(s)", nullptr, skippedUnavailable), false, false, 2500);
    }

    m_internalDragActive = true;
    m_closeTimer.stop();
    if (m_dropSurface) {
        m_dropSurface->setInternalDragActive(true);
    }
    m_view->setInputExpanded(true);
    Q_EMIT internalDragChanged();

    QDrag drag(m_view);
    drag.setMimeData(mimeData.release());
    const QPixmap pixmap = createDragPixmap(items.first(), items.size());
    drag.setPixmap(pixmap);
    drag.setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
    drag.exec(actions, Qt::CopyAction);

    m_internalDragActive = false;
    if (m_dropSurface) {
        m_dropSurface->setInternalDragActive(false);
    }
    m_model->refreshAvailability();
    Q_EMIT internalDragChanged();
    scheduleAutoHide();
}

void ShelfController::toggleItemSelection(int index)
{
    const int itemCount = m_model ? m_model->count() : 0;
    if (index < 0 || index >= itemCount) {
        return;
    }
    if (m_selectedRows.contains(index)) {
        m_selectedRows.remove(index);
        if (m_selectionAnchor == index) {
            m_selectionAnchor = -1;
        }
    } else {
        m_selectedRows.insert(index);
        if (m_selectionAnchor < 0) {
            m_selectionAnchor = index;
        }
    }
    Q_EMIT selectedRowsChanged();
}

void ShelfController::extendItemSelection(int index)
{
    const int itemCount = m_model ? m_model->count() : 0;
    if (index < 0 || index >= itemCount) {
        return;
    }

    if (m_selectionAnchor < 0) {
        m_selectionAnchor = index;
        m_selectedRows.clear();
        m_selectedRows.insert(index);
    } else {
        const int first = qMin(m_selectionAnchor, index);
        const int last = qMax(m_selectionAnchor, index);
        m_selectedRows.clear();
        for (int row = first; row <= last; ++row) {
            m_selectedRows.insert(row);
        }
    }
    Q_EMIT selectedRowsChanged();
}

void ShelfController::clearSelection()
{
    if (m_selectedRows.isEmpty() && m_selectionAnchor < 0) {
        return;
    }
    m_selectedRows.clear();
    m_selectionAnchor = -1;
    Q_EMIT selectedRowsChanged();
}

void ShelfController::selectAllItems()
{
    const int itemCount = m_model ? m_model->count() : 0;
    if (itemCount <= 0) {
        return;
    }
    m_selectedRows.clear();
    for (int row = 0; row < itemCount; ++row) {
        m_selectedRows.insert(row);
    }
    m_selectionAnchor = 0;
    Q_EMIT selectedRowsChanged();
}

QVariantList ShelfController::selectedRows() const
{
    QList<int> rows = m_selectedRows.values();
    std::sort(rows.begin(), rows.end());
    QVariantList list;
    list.reserve(rows.size());
    for (int row : rows) {
        list.append(QVariant::fromValue(row));
    }
    return list;
}

int ShelfController::selectedCount() const
{
    return int(m_selectedRows.size());
}

void ShelfController::removeItem(int index)
{
    if (!ensureModifiable()) {
        return;
    }
    const ShelfItem item = itemAt(index);
    if (item.id.isEmpty()) {
        return;
    }

    closeItemMenu();
    m_undo.row = index;
    m_undo.items = {item};
    m_undo.description = item.displayName();
    m_undo.valid = true;

    m_model->removeRow(index);
    if (!m_lastSaveFailed) {
        showToast(tr("Removed “%1”").arg(m_undo.description), true, false, 6000);
    }
}

void ShelfController::clearShelf()
{
    if (!ensureModifiable()) {
        return;
    }
    const QList<ShelfItem> items = m_model->items();
    if (items.isEmpty()) {
        showToast(tr("The shelf is already empty."), false, false, 3500);
        return;
    }

    m_undo.row = 0;
    m_undo.items = items;
    m_undo.description = tr("%n item(s)", nullptr, int(items.size()));
    m_undo.valid = true;

    m_model->clear();
    if (!m_lastSaveFailed) {
        showToast(tr("Cleared %n item(s)", nullptr, int(items.size())), true, false, 3000);
    }
}

void ShelfController::undoLastRemoval()
{
    if (!m_undo.valid) {
        return;
    }

    const UndoEntry entry = m_undo;
    m_undo = {};
    m_model->insertItemsAt(entry.row, entry.items);
    if (!m_lastSaveFailed) {
        showToast(tr("Restored %n item(s)", nullptr, int(entry.items.size())), false, false, 4500);
    }
}

void ShelfController::copyItem(int index)
{
    const ShelfItem item = itemAt(index);
    if (item.id.isEmpty()) {
        return;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    switch (item.kind) {
    case ShelfItem::Kind::File:
    case ShelfItem::Kind::Folder: {
        auto *mimeData = new QMimeData;
        mimeData->setUrls({QUrl::fromLocalFile(item.path)});
        clipboard->setMimeData(mimeData);
        showToast(tr("Copied to clipboard."), false, false, 3000);
        break;
    }
    case ShelfItem::Kind::Text:
        clipboard->setText(item.text);
        showToast(tr("Copied to clipboard."), false, false, 3000);
        break;
    case ShelfItem::Kind::Url:
        clipboard->setText(item.url);
        showToast(tr("Copied to clipboard."), false, false, 3000);
        break;
    case ShelfItem::Kind::Image: {
        const QImage image(item.imageFile);
        if (image.isNull()) {
            showToast(tr("The image could not be read."), false, true, 6000);
            break;
        }
        auto *mimeData = new QMimeData;
        mimeData->setImageData(image);
        mimeData->setUrls({QUrl::fromLocalFile(item.imageFile)});
        clipboard->setMimeData(mimeData);
        showToast(tr("Image copied to clipboard."), false, false, 3000);
        break;
    }
    }
}

void ShelfController::openContainingFolder(int index)
{
    const ShelfItem item = itemAt(index);
    if (item.id.isEmpty()) {
        return;
    }

    QString directory;
    switch (item.kind) {
    case ShelfItem::Kind::File:
    case ShelfItem::Kind::Folder: {
        const QFileInfo info(item.path);
        directory = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
        break;
    }
    case ShelfItem::Kind::Image:
        directory = m_repository->imagesDirectory();
        break;
    case ShelfItem::Kind::Text:
    case ShelfItem::Kind::Url:
        showToast(tr("This item has no file location."), false, false, 3500);
        return;
    }

    if (directory.isEmpty() || !QDesktopServices::openUrl(QUrl::fromLocalFile(directory))) {
        showToast(tr("Couldn't open the containing folder."), false, true, 6000);
    }
}

void ShelfController::setEdge(int edge)
{
    m_settings->setEdge(edge);
}

void ShelfController::setPosition(int position)
{
    m_settings->setPosition(position);
}

void ShelfController::selectScreen(const QString &screenName)
{
    m_settings->setScreenName(screenName);
}

void ShelfController::setAutostartEnabled(bool enabled)
{
    if (enabled == m_autostartEnabled) {
        return;
    }

    QString error;
    if (!Autostart::setEnabled(enabled, &error)) {
        showToast(error, false, true, 7000);
        return;
    }
    m_autostartEnabled = enabled;
    Q_EMIT autostartChanged();
    showToast(enabled ? tr("Yoink will start when you sign in.") : tr("Autostart turned off."), false, false, 3500);
}

void ShelfController::quit()
{
    m_shuttingDown = true;
    if (m_dirty) {
        persist();
    }
    if (m_settings) {
        m_settings->flush();
    }
    QCoreApplication::quit();
}

void ShelfController::handleIpcMessage(const QString &message)
{
    if (message == QLatin1String("settings")) {
        m_holdOpen = true;
        openSettings();
    } else if (message == QLatin1String("toggle")) {
        toggleFromTray();
    } else if (message == QLatin1String("quit")) {
        quit();
    } else {
        m_holdOpen = true;
        openShelf();
    }
}

QVariant ShelfController::itemData(int index, const QString &role) const
{
    if (!m_model || index < 0 || index >= m_model->count()) {
        return {};
    }

    const QHash<int, QByteArray> roles = m_model->roleNames();
    const QByteArray requested = role.toUtf8();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        if (it.value() == requested) {
            return m_model->data(m_model->index(index, 0), it.key());
        }
    }
    return {};
}
