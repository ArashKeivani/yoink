/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "app/SingleInstance.h"
#include "app/TrayIcon.h"
#include "ui/DropSurface.h"
#include "ui/ShelfController.h"
#include "ui/ShelfSurface.h"
#include "ui/YoinkImageProvider.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QIcon>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>

#include <cstdio>
#include <optional>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Yoink"));
    QApplication::setApplicationDisplayName(QStringLiteral("Yoink"));
    QApplication::setOrganizationName(QStringLiteral("Yoink"));
    QApplication::setOrganizationDomain(QStringLiteral("yoink.org"));
    QApplication::setApplicationVersion(QStringLiteral(YOINK_VERSION));
    QApplication::setDesktopFileName(QStringLiteral("org.yoink.Yoink"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/org.yoink.Yoink.svg")));
    QApplication::setQuitOnLastWindowClosed(false);
    QIcon::setFallbackThemeName(QStringLiteral("breeze"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QQuickWindow::setDefaultAlphaBuffer(true);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Yoink — a drag and drop edge shelf for KDE Plasma"));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption showOption(QStringLiteral("show"), QStringLiteral("Open the shelf."));
    const QCommandLineOption settingsOption(QStringLiteral("settings"), QStringLiteral("Open the shelf settings."));
    const QCommandLineOption toggleOption(QStringLiteral("toggle"), QStringLiteral("Toggle the shelf."));
    const QCommandLineOption quitOption(QStringLiteral("quit"), QStringLiteral("Quit a running Yoink instance."));
    const QCommandLineOption autostartOption(QStringLiteral("autostart"), QStringLiteral("Start quietly, as at login."));
    const QCommandLineOption screenshotOption(
        QStringList{QStringLiteral("screenshot")}, QStringLiteral("Render a view to a PNG and exit; for documentation."), QStringLiteral("file"));
    const QCommandLineOption screenshotViewOption(QStringList{QStringLiteral("screenshot-view")},
                                                  QStringLiteral("View to render with --screenshot: shelf, selection, menu, settings, toast."),
                                                  QStringLiteral("view"),
                                                  QStringLiteral("shelf"));
    parser.addOption(showOption);
    parser.addOption(settingsOption);
    parser.addOption(toggleOption);
    parser.addOption(quitOption);
    parser.addOption(autostartOption);
    parser.addOption(screenshotOption);
    parser.addOption(screenshotViewOption);
    parser.process(app);

    const bool autostart = parser.isSet(autostartOption);
    const bool screenshotting = parser.isSet(screenshotOption);
    QString message;
    if (parser.isSet(quitOption)) {
        message = QStringLiteral("quit");
    } else if (parser.isSet(settingsOption)) {
        message = QStringLiteral("settings");
    } else if (parser.isSet(toggleOption)) {
        message = QStringLiteral("toggle");
    } else if (parser.isSet(showOption) || !autostart) {
        message = QStringLiteral("show");
    }

    SingleInstance instance(QStringLiteral("org.yoink.Yoink"));
    // Screenshot runs always make a real instance so they never forward to
    // (and never disturb) the user's live shelf.
    if (!screenshotting && !instance.tryAcquire()) {
        if (!message.isEmpty()) {
            if (instance.sendMessage(message)) {
                return 0;
            }
            qWarning() << "Yoink: another instance is running but didn't respond.";
            return 1;
        }
        return 0;
    }

    if (parser.isSet(quitOption)) {
        return 0;
    }

    if (QGuiApplication::platformName() != QLatin1String("wayland")) {
        qWarning() << "Yoink: this session is not Wayland; the edge shelf requires a layer-shell compositor.";
    }

    qmlRegisterType<DropSurface>("Yoink", 1, 0, "DropSurface");

    ShelfSurface view;
    view.engine()->addImageProvider(QStringLiteral("yoink"), new YoinkImageProvider);

    ShelfController controller(&view);
    view.rootContext()->setContextProperty(QStringLiteral("shelf"), &controller);

    view.setSource(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (view.status() == QQuickView::Error) {
        const QList<QQmlError> errors = view.errors();
        std::fprintf(stderr, "Yoink: QML failed to load (%lld errors)\n", static_cast<long long>(errors.size()));
        for (const QQmlError &error : errors) {
            std::fprintf(stderr, "%s\n", qPrintable(error.toString()));
        }
        return 1;
    }

    if (!view.initializeLayerShell()) {
        qWarning() << "Yoink: layer-shell is unavailable; showing a regular window instead.";
        view.setFlags(view.flags() | Qt::WindowStaysOnTopHint);
    }

    std::optional<TrayIcon> tray;
    if (!screenshotting) {
        tray.emplace();
        QObject::connect(&*tray, &TrayIcon::toggleRequested, &controller, &ShelfController::toggleFromTray);
        QObject::connect(&*tray, &TrayIcon::settingsRequested, &controller, &ShelfController::openSettings);
        QObject::connect(&*tray, &TrayIcon::quitRequested, &controller, &ShelfController::quit);
        QObject::connect(&*tray, &TrayIcon::autostartToggleRequested, &controller, &ShelfController::setAutostartEnabled);
        QObject::connect(&controller, &ShelfController::expandedChanged, &*tray, [&tray, &controller] { tray->setExpanded(controller.expanded()); });
        QObject::connect(
            &controller, &ShelfController::autostartChanged, &*tray, [&tray, &controller] { tray->setAutostartEnabled(controller.autostartEnabled()); });
        tray->setAutostartEnabled(controller.autostartEnabled());
    }

    QObject::connect(&instance, &SingleInstance::messageReceived, &controller, &ShelfController::handleIpcMessage);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &controller, &ShelfController::shutdown);

    controller.start(autostart);
    if (parser.isSet(showOption)) {
        controller.showFromUser();
    } else if (parser.isSet(toggleOption)) {
        controller.toggleFromTray();
    } else if (parser.isSet(settingsOption)) {
        controller.openSettings();
    }

    if (screenshotting) {
        const QString file = parser.value(screenshotOption);
        const QString view = parser.value(screenshotViewOption);
        QTimer::singleShot(1150, &app, [&app, &view, &controller, file] {
            if (view == QLatin1String("selection")) {
                controller.selectAllItems();
            } else if (view == QLatin1String("menu")) {
                controller.selectAllItems();
                controller.openItemMenu(0);
            } else if (view == QLatin1String("settings")) {
                controller.openSettings();
            } else if (view == QLatin1String("toast")) {
                controller.removeItem(1);
            }
            QTimer::singleShot(350, &app, [&app, file] {
                QImage grabbed;
                const QList<QWindow *> windows = app.topLevelWindows();
                for (QWindow *window : windows) {
                    if (auto *quick = qobject_cast<QQuickWindow *>(window)) {
                        grabbed = quick->grabWindow();
                        break;
                    }
                }
                if (grabbed.isNull()) {
                    qWarning() << "Yoink: could not capture the window.";
                    QCoreApplication::exit(2);
                    return;
                }
                if (!grabbed.save(file)) {
                    qWarning() << "Yoink: could not save" << file;
                    QCoreApplication::exit(2);
                    return;
                }
                QCoreApplication::exit(0);
            });
        });
    }

    return app.exec();
}
