#pragma once

#include <QPushButton>
#include <initializer_list>

// Spacing values from the KDE Human Interface Guidelines, mirroring the
// Kirigami.Units the guidelines are written against:
// https://develop.kde.org/hig/layout/
//
//   small  – between a title/label and its content, and between rows of
//            closely related controls
//   medium – between items in a toolbar-like row of buttons
//   large  – window edge padding, padding inside group boxes, and between
//            separate groups of controls
//   gridUnit – base unit for fixed sizes (minimum/default window size, etc.)
namespace Spacing {
constexpr int small = 4;
constexpr int medium = 6;
constexpr int large = 8;
constexpr int gridUnit = 18;
} // namespace Spacing

namespace Layout {

// Buttons normally keep their own height, which depends on their label's
// font. Japanese labels can be drawn with a fallback font of other metrics,
// so buttons side by side ended up different heights. Let the buttons in a
// row stretch to the row's height instead, so they always line up.
inline void lineUp(std::initializer_list<QPushButton *> buttons)
{
    for (QPushButton *b : buttons)
        b->setSizePolicy(b->sizePolicy().horizontalPolicy(), QSizePolicy::Preferred);
}

} // namespace Layout
