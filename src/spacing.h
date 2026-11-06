#pragma once

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
