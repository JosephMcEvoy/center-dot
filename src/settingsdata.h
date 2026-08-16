#ifndef SETTINGSDATA_H
#define SETTINGSDATA_H

#include <QColor>

// The available crosshair shapes the user can pick from.
// The numeric values are not persisted, settings are stored under the stable
// string keys defined in crosshairrenderer.cpp, so this list may be reordered.
enum class CrosshairStyle {
    Dot,
    Circle,
    DotInCircle,
    Cross,
    CrossWithGap,
    CrossWithGapAndDot,
    TShape,
    XShape,
    Chevron,
    Brackets
};

struct SettingsData {
    // screen coordinates of the point the crosshair is centered on
    int xPosition = 0;
    int yPosition = 0;

    CrosshairStyle style = CrosshairStyle::Dot;

    // diameter of the dot resp. of the surrounding circle
    int crosshairSize = 5;

    // stroke width of the lines a crosshair is built from
    int lineThickness = 2;

    // distance between the center and where the lines start
    int gapSize = 4;

    // length of a single line of the crosshair
    int armLength = 7;

    QColor contourColor = QColor(Qt::black);
    QColor fillColor = QColor(Qt::white);
};

#endif // SETTINGSDATA_H
