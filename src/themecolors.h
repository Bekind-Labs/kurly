#pragma once

#include <QColor>
#include <QPalette>
#include <QString>
#include <QWidget>

// Status colors that stay readable in both light and dark themes. The darker
// shades are for light backgrounds, the lighter ones for dark backgrounds;
// each has a contrast of at least 4.5:1 against Qt's standard window colors.
namespace ThemeColors {

inline bool isDark(const QWidget *widget)
{
    return widget->palette().color(QPalette::Window).lightness() < 128;
}

inline QString success(const QWidget *widget)
{
    return isDark(widget) ? QStringLiteral("#81c784") : QStringLiteral("#2e7d32");
}

inline QString error(const QWidget *widget)
{
    return isDark(widget) ? QStringLiteral("#ef9a9a") : QStringLiteral("#c62828");
}

// A style sheet setting the text color, or none for an empty color.
inline QString textColorStyle(const QString &color)
{
    return color.isEmpty() ? QString() : QStringLiteral("color: %1;").arg(color);
}

} // namespace ThemeColors
