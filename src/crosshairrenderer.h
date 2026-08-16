#ifndef CROSSHAIRRENDERER_H
#define CROSSHAIRRENDERER_H

#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QVector>

#include "settingsdata.h"

// Draws the crosshair shapes. Used by MainWindow to paint the on screen overlay
// and by SettingsDialog to paint the preview, so both always agree.
namespace CrosshairRenderer {

// Describes one selectable style and which of the size settings it reacts to.
// A style that does not use a setting leaves the corresponding control disabled
// in the settings dialog.
struct StyleInfo {
    CrosshairStyle style;
    QString        key;         // stable identifier written to the settings file
    QString        displayName; // translated name shown in the settings dialog
    bool           usesSize;
    bool           usesThickness;
    bool           usesGap;
    bool           usesArmLength;
};

// allowed ranges of the size related settings, shared by the settings dialog
// (slider bounds) and the settings file reader (validation)
const int MIN_SIZE          = 1;
const int MAX_SIZE          = 30;
const int MIN_LINE_THICKNESS = 1;
const int MAX_LINE_THICKNESS = 10;
const int MIN_GAP_SIZE      = 0;
const int MAX_GAP_SIZE      = 30;
const int MIN_ARM_LENGTH    = 1;
const int MAX_ARM_LENGTH    = 40;

// All styles in the order they are offered to the user.
const QVector<StyleInfo> &styles();

// Never fails, falls back to the description of CrosshairStyle::Dot.
const StyleInfo &styleInfo(CrosshairStyle style);

// Conversion between a style and the identifier stored in the settings file.
QString         styleKey(CrosshairStyle style);
CrosshairStyle  styleFromKey(const QString &key, CrosshairStyle fallback);

// Edge length of the square area the crosshair needs, contour and a little
// room for antialiasing included. The crosshair is always centered in it.
int boundingSide(const SettingsData &settings);

// Draws the crosshair centered on the given point of the painter's device.
void paint(QPainter &painter, const SettingsData &settings, const QPointF &center);

// Renders the crosshair centered on a transparent pixmap of the given size.
// Used for the settings preview.
QPixmap renderPixmap(const SettingsData &settings, const QSize &size, qreal devicePixelRatio = 1.0);

}

#endif // CROSSHAIRRENDERER_H
