/*
    SPDX-FileCopyrightText: 2026 Yoink contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "Theme.h"

#include <KColorScheme>

#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>
#include <QTimer>

namespace
{
QColor mix(const QColor &first, const QColor &second, qreal amount)
{
    const qreal t = qBound(0.0, amount, 1.0);
    return QColor::fromRgbF(first.redF() * (1.0 - t) + second.redF() * t,
                            first.greenF() * (1.0 - t) + second.greenF() * t,
                            first.blueF() * (1.0 - t) + second.blueF() * t,
                            first.alphaF() * (1.0 - t) + second.alphaF() * t);
}

QColor ensureContrast(const QColor &color, const QColor &background, qreal minimumLightnessDifference)
{
    if (qAbs(color.lightnessF() - background.lightnessF()) >= minimumLightnessDifference) {
        return color;
    }
    return mix(color, background.lightnessF() < 0.5 ? Qt::white : Qt::black, 0.35);
}
} // namespace

Theme::Theme(QObject *parent)
    : QObject(parent)
    , m_config(KSharedConfig::openConfig(QStringLiteral("kdeglobals")))
    , m_watcher(KConfigWatcher::create(m_config))
{
    connect(m_watcher.get(), &KConfigWatcher::configChanged, this, &Theme::scheduleReload);
    if (QStyleHints *hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged, this, &Theme::scheduleReload);
    }
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->installEventFilter(this);
    }
    reload();
}

Theme::~Theme()
{
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->removeEventFilter(this);
    }
}

void Theme::reload()
{
    const QPalette palette = QGuiApplication::palette();
    const KColorScheme windowScheme(QPalette::Normal, KColorScheme::Window, m_config);

    m_surface = palette.color(QPalette::Active, QPalette::Window);
    m_text = palette.color(QPalette::Active, QPalette::WindowText);
    m_accent = palette.color(QPalette::Active, QPalette::Highlight);
    m_accentText = palette.color(QPalette::Active, QPalette::HighlightedText);
    m_menu = palette.color(QPalette::Active, QPalette::ToolTipBase);
    m_menuText = palette.color(QPalette::Active, QPalette::ToolTipText);

    m_dark = m_surface.lightnessF() < 0.5;
    if (m_menu == m_surface) {
        m_menu = mix(m_surface, m_dark ? Qt::white : Qt::black, 0.04);
    }
    if (m_menuText == m_text && m_menu != m_surface) {
        m_menuText = ensureContrast(m_text, m_menu, 0.35);
    }

    m_textMuted = palette.color(QPalette::Disabled, QPalette::WindowText);
    if (m_textMuted == m_text) {
        m_textMuted = mix(m_text, m_surface, 0.42);
    }

    m_header = mix(m_surface, m_text, m_dark ? 0.035 : 0.025);
    m_border = mix(m_surface, m_text, m_dark ? 0.18 : 0.16);
    m_divider = mix(m_surface, m_text, m_dark ? 0.10 : 0.08);
    m_hover = mix(m_surface, m_text, m_dark ? 0.09 : 0.06);
    m_pressed = mix(m_surface, m_text, m_dark ? 0.16 : 0.11);
    m_accentSoft = mix(m_surface, m_accent, m_dark ? 0.22 : 0.14);

    m_danger = windowScheme.foreground(KColorScheme::NegativeText).color();
    if (!m_danger.isValid()) {
        m_danger = QColor(218, 68, 83);
    }
    m_warning = windowScheme.foreground(KColorScheme::NeutralText).color();
    if (!m_warning.isValid()) {
        m_warning = QColor(246, 116, 0);
    }
    m_positive = windowScheme.foreground(KColorScheme::PositiveText).color();
    if (!m_positive.isValid()) {
        m_positive = QColor(39, 174, 96);
    }
    m_dangerSoft = mix(m_surface, m_danger, 0.16);

    m_shadow = QColor(0, 0, 0, m_dark ? 160 : 55);
    m_scrollbar = mix(m_surface, m_text, m_dark ? 0.35 : 0.25);
    m_scrim = QColor(0, 0, 0, m_dark ? 80 : 30);

    const qreal factor = m_config->group(QStringLiteral("KDE")).readEntry(QStringLiteral("AnimationDurationFactor"), 1.0);
    m_motionFactor = qBound(0.0, factor, 2.0);
    m_reducedMotion = m_motionFactor <= 0.001;

    const QFont font = QGuiApplication::font();
    m_fontFamily = font.family();
    m_bodyPointSize = font.pointSizeF() > 0 ? font.pointSizeF() : 10.0;
    m_smallPointSize = qMax(7.0, m_bodyPointSize - 1.0);
    m_titlePointSize = m_bodyPointSize + 2.0;

    m_reloadPending = false;
    Q_EMIT changed();
}

void Theme::scheduleReload()
{
    if (m_reloadPending) {
        return;
    }
    m_reloadPending = true;
    QTimer::singleShot(0, this, &Theme::reload);
}

bool Theme::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::ApplicationPaletteChange:
    case QEvent::ApplicationFontChange:
        scheduleReload();
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

QColor Theme::surface() const
{
    return m_surface;
}

QColor Theme::header() const
{
    return m_header;
}

QColor Theme::menu() const
{
    return m_menu;
}

QColor Theme::menuText() const
{
    return m_menuText;
}

QColor Theme::text() const
{
    return m_text;
}

QColor Theme::textMuted() const
{
    return m_textMuted;
}

QColor Theme::accent() const
{
    return m_accent;
}

QColor Theme::accentText() const
{
    return m_accentText;
}

QColor Theme::accentSoft() const
{
    return m_accentSoft;
}

QColor Theme::border() const
{
    return m_border;
}

QColor Theme::divider() const
{
    return m_divider;
}

QColor Theme::hover() const
{
    return m_hover;
}

QColor Theme::pressed() const
{
    return m_pressed;
}

QColor Theme::danger() const
{
    return m_danger;
}

QColor Theme::dangerSoft() const
{
    return m_dangerSoft;
}

QColor Theme::warning() const
{
    return m_warning;
}

QColor Theme::positive() const
{
    return m_positive;
}

QColor Theme::shadow() const
{
    return m_shadow;
}

QColor Theme::scrollbar() const
{
    return m_scrollbar;
}

QColor Theme::scrim() const
{
    return m_scrim;
}

bool Theme::isDark() const
{
    return m_dark;
}

qreal Theme::motionFactor() const
{
    return m_motionFactor;
}

bool Theme::reducedMotion() const
{
    return m_reducedMotion;
}

QString Theme::fontFamily() const
{
    return m_fontFamily;
}

qreal Theme::bodyPointSize() const
{
    return m_bodyPointSize;
}

qreal Theme::smallPointSize() const
{
    return m_smallPointSize;
}

qreal Theme::titlePointSize() const
{
    return m_titlePointSize;
}

int Theme::duration(int milliseconds) const
{
    if (m_reducedMotion) {
        return 0;
    }
    return qRound(milliseconds * m_motionFactor);
}
