#include "crosshairrenderer.h"

#include <QCoreApplication>
#include <QPainterPath>
#include <QRect>
#include <QtMath>

namespace {

// width in pixels of the contour drawn around the crosshair
const int CONTOUR_WIDTH = 1;

// factor between the length of a diagonal arm and its horizontal/vertical reach
const qreal DIAGONAL_REACH = 0.7071067811865476; // cos(45 degrees)

// The shapes a crosshair is built from plus the room they need.
struct Geometry {
    // Straight bars, dots and diagonals. Sub shapes are allowed to overlap,
    // the winding fill rule merges them into one solid area.
    QPainterPath solid;

    // The hollow circle. Kept apart because it needs the odd even fill rule to
    // turn the two ellipses into a ring.
    QPainterPath ring;

    // distance from the center to the outermost drawn pixel, contour excluded
    qreal halfExtent = 0.0;
};

// Everything is laid out in whole pixels, counted outwards from the pixel the
// crosshair is centered on. Integer coordinates sit on pixel edges, not in pixel
// middles, so pixel n covers [n, n + 1). Sticking to whole pixels is what lets a
// measurement of "n px" cover n pixels instead of being smeared over n + 1 of
// them by antialiasing.

// First pixel of a bar of the given thickness laid across the axis pixel. An
// odd thickness sits centered on that pixel, an even one cannot and leans
// towards the smaller coordinate. Every shape uses this same rule, so that a
// dot and the arms of a cross agree on where the middle is.
int barStart(int axis, int thickness)
{
    return axis - thickness / 2;
}

// First pixel of a run of count pixels that starts offset pixels away from the
// center pixel and grows outwards. An offset of 0 starts on the center pixel
// itself, an offset of 1 on the pixel next to it.
//
// The negative direction has to be counted back from the far end. Deriving it
// from the center instead is what used to make an arm reach one pixel further
// than its own mirror image, leaving the crosshair sitting half a pixel up and
// to the left of the point it is supposed to mark.
int runStart(int centerPixel, int direction, int offset, int count)
{
    return direction > 0 ? centerPixel + offset
                         : centerPixel - offset - count + 1;
}

// The square of whole pixels a round shape of the given diameter fills.
QRect pixelSquare(int cx, int cy, int diameter)
{
    return QRect(barStart(cx, diameter), barStart(cy, diameter), diameter, diameter);
}

// Adds a filled disc covering the given square of whole pixels. Below three
// pixels there is no room for a circle to read as round, and antialiasing a
// curve that small only yields a grey smudge, so the square is taken as it is.
// That is what makes a dot of "1 px" a single fully lit pixel rather than a
// faint one at three quarters coverage.
void addDisc(QPainterPath &path, const QRect &square)
{
    if (square.width() < 1) {
        return;
    }

    if (square.width() < 3) {
        path.addRect(square);
    } else {
        path.addEllipse(square);
    }
}

// Adds a diagonal arm running outwards from the center pixel in the (dirX,
// dirY) direction, as a staircase of whole pixels: step k sits on the pixel k
// steps away along both axes and carries a run of thickness pixels laid across
// it by the same rule the straight bars use. Both ends are included, so an arm
// from step 0 grows out of the center pixel itself.
//
// Stroking a line instead is what used to make a diagonal miss its measurement.
// A stroke width is taken perpendicular to the line, and across the screen a 45
// degree band is sqrt(2) times as wide as that, so "1 px" came out 1.41 px wide
// and, sitting on no pixel edge at all, never lit a single pixel fully: the arm
// showed up as a pair of half lit ones the contour underneath shone through.
void addDiagonalArm(QPainterPath &path, int cx, int cy, int dirX, int dirY,
                    int firstStep, int lastStep, int thickness)
{
    for (int step = firstStep; step <= lastStep; ++step) {
        path.addRect(QRect(barStart(cx + dirX * step, thickness),
                           cy + dirY * step, thickness, 1));
    }
}

// How far the outermost pixel of such an arm sits from the center pixel. The
// run laid across the last step reaches thickness / 2 further out sideways.
int diagonalReach(int lastStep, int thickness)
{
    return lastStep + thickness / 2;
}

// The step a point that many pixels along a diagonal falls on. Gap and arm
// length stay measured along the line, the way they read in the settings
// dialog, so switching to whole pixels does not resize anybody's crosshair.
int diagonalStep(int lengthAlongLine)
{
    return qRound(lengthAlongLine * DIAGONAL_REACH);
}

// Adds the four arms of a cross. Every arm is armLength pixels long and begins
// gap pixels away from the center pixel, so a gap of 0 grows out of the aiming
// point itself and keeps a plain cross solid, while a gap of 1 leaves exactly
// the center pixel clear. Directions that are switched off are skipped, that is
// how the T shape drops its upper arm.
void addCrossArms(QPainterPath &path, int cx, int cy, int thickness,
                  int gap, int armLength, bool up, bool down, bool left, bool right)
{
    const int acrossX = barStart(cx, thickness);
    const int acrossY = barStart(cy, thickness);

    if (up) {
        path.addRect(QRect(acrossX, runStart(cy, -1, gap, armLength), thickness, armLength));
    }
    if (down) {
        path.addRect(QRect(acrossX, runStart(cy, 1, gap, armLength), thickness, armLength));
    }
    if (left) {
        path.addRect(QRect(runStart(cx, -1, gap, armLength), acrossY, armLength, thickness));
    }
    if (right) {
        path.addRect(QRect(runStart(cx, 1, gap, armLength), acrossY, armLength, thickness));
    }
}

// How far the outermost pixel of such an arm sits from the center pixel. A
// crosshair whose arms are shorter than they are thick is as wide as the bar.
int armReach(int gap, int armLength, int thickness)
{
    return qMax(gap + armLength - 1, thickness / 2);
}

// Builds all shapes of the configured crosshair around the center pixel (cx, cy).
Geometry buildGeometry(const SettingsData &settings, int cx, int cy)
{
    using namespace CrosshairRenderer;

    // a hand edited settings file must not be able to blow up the window size
    const int size      = qBound(MIN_SIZE, settings.crosshairSize, MAX_SIZE);
    const int thickness = qBound(MIN_LINE_THICKNESS, settings.lineThickness, MAX_LINE_THICKNESS);
    const int gap       = qBound(MIN_GAP_SIZE, settings.gapSize, MAX_GAP_SIZE);
    const int arm       = qBound(MIN_ARM_LENGTH, settings.armLength, MAX_ARM_LENGTH);

    Geometry geometry;
    geometry.ring.setFillRule(Qt::OddEvenFill);
    geometry.solid.setFillRule(Qt::WindingFill);

    switch (settings.style) {
        case CrosshairStyle::Dot: {
            addDisc(geometry.solid, pixelSquare(cx, cy, size));
            geometry.halfExtent = size / 2;
            break;
        }

        case CrosshairStyle::Circle: {
            addDisc(geometry.ring, pixelSquare(cx, cy, size));

            // Punches the hole. Snapping it by the same rule as the outer
            // circle leaves a wall of exactly thickness pixels all around.
            addDisc(geometry.ring, pixelSquare(cx, cy, size - 2 * thickness));
            geometry.halfExtent = size / 2;
            break;
        }

        case CrosshairStyle::DotInCircle: {
            const int hole = size - 2 * thickness;
            addDisc(geometry.ring, pixelSquare(cx, cy, size));
            addDisc(geometry.ring, pixelSquare(cx, cy, hole));

            // a dot that stays clear of the surrounding ring
            addDisc(geometry.solid, pixelSquare(cx, cy, qMax(1, hole / 3)));
            geometry.halfExtent = size / 2;
            break;
        }

        case CrosshairStyle::Cross: {
            addCrossArms(geometry.solid, cx, cy, thickness, 0, arm, true, true, true, true);
            geometry.halfExtent = armReach(0, arm, thickness);
            break;
        }

        case CrosshairStyle::CrossWithGap: {
            addCrossArms(geometry.solid, cx, cy, thickness, gap, arm, true, true, true, true);
            geometry.halfExtent = armReach(gap, arm, thickness);
            break;
        }

        case CrosshairStyle::CrossWithGapAndDot: {
            addCrossArms(geometry.solid, cx, cy, thickness, gap, arm, true, true, true, true);
            addDisc(geometry.solid, pixelSquare(cx, cy, size));
            geometry.halfExtent = qMax(armReach(gap, arm, thickness), size / 2);
            break;
        }

        case CrosshairStyle::TShape: {
            // no upper arm, keeps the view above the aiming point clear
            addCrossArms(geometry.solid, cx, cy, thickness, gap, arm, false, true, true, true);
            geometry.halfExtent = armReach(gap, arm, thickness);
            break;
        }

        case CrosshairStyle::XShape: {
            const int firstStep = diagonalStep(gap);
            const int lastStep  = diagonalStep(gap + arm);

            for (int dx = -1; dx <= 1; dx += 2) {
                for (int dy = -1; dy <= 1; dy += 2) {
                    addDiagonalArm(geometry.solid, cx, cy, dx, dy,
                                   firstStep, lastStep, thickness);
                }
            }
            geometry.halfExtent = diagonalReach(lastStep, thickness);
            break;
        }

        case CrosshairStyle::Chevron: {
            // an arrow head with its tip on the aiming point, opening downwards.
            // Both arms start on the center pixel, which is what keeps the tip
            // closed without a mitre sticking out past the arms.
            const int lastStep = diagonalStep(arm);

            for (int dx = -1; dx <= 1; dx += 2) {
                addDiagonalArm(geometry.solid, cx, cy, dx, 1, 0, lastStep, thickness);
            }
            geometry.halfExtent = diagonalReach(lastStep, thickness);
            break;
        }

        case CrosshairStyle::Brackets: {
            // four corner brackets, gap sets how wide they stand apart
            const int corner = gap + arm;

            for (int dx = -1; dx <= 1; dx += 2) {
                for (int dy = -1; dy <= 1; dy += 2) {
                    // The legs run inwards from the corner, and each bar lies on
                    // the inner side of it, so that opposite brackets end up the
                    // same distance from the aiming point.
                    const int acrossX = runStart(cx + dx * corner, -dx, 0, thickness);
                    const int acrossY = runStart(cy + dy * corner, -dy, 0, thickness);

                    geometry.solid.addRect(
                        QRect(runStart(cx, dx, gap + 1, arm), acrossY, arm, thickness));
                    geometry.solid.addRect(
                        QRect(acrossX, runStart(cy, dy, gap + 1, arm), thickness, arm));
                }
            }
            // A bar thicker than the bracket stands from the middle reaches
            // past the aiming point and out the other side.
            geometry.halfExtent = qMax(corner, thickness - 1 - corner);
            break;
        }
    }

    return geometry;
}

} // namespace

namespace CrosshairRenderer {

const QVector<StyleInfo> &styles()
{
    //                                                              size  thick  gap   arm
    static const QVector<StyleInfo> allStyles = {
        { CrosshairStyle::Dot,                "dot",
          QCoreApplication::translate("CrosshairStyle", "Dot"),
          true,  false, false, false },
        { CrosshairStyle::Circle,             "circle",
          QCoreApplication::translate("CrosshairStyle", "Circle"),
          true,  true,  false, false },
        { CrosshairStyle::DotInCircle,        "dotInCircle",
          QCoreApplication::translate("CrosshairStyle", "Dot in circle"),
          true,  true,  false, false },
        { CrosshairStyle::Cross,              "cross",
          QCoreApplication::translate("CrosshairStyle", "Cross"),
          false, true,  false, true  },
        { CrosshairStyle::CrossWithGap,       "crossWithGap",
          QCoreApplication::translate("CrosshairStyle", "Cross with gap"),
          false, true,  true,  true  },
        { CrosshairStyle::CrossWithGapAndDot, "crossWithGapAndDot",
          QCoreApplication::translate("CrosshairStyle", "Cross with gap and dot"),
          true,  true,  true,  true  },
        { CrosshairStyle::TShape,             "tShape",
          QCoreApplication::translate("CrosshairStyle", "T-shape"),
          false, true,  true,  true  },
        { CrosshairStyle::XShape,             "xShape",
          QCoreApplication::translate("CrosshairStyle", "X-shape"),
          false, true,  true,  true  },
        { CrosshairStyle::Chevron,            "chevron",
          QCoreApplication::translate("CrosshairStyle", "Chevron"),
          false, true,  false, true  },
        { CrosshairStyle::Brackets,           "brackets",
          QCoreApplication::translate("CrosshairStyle", "Corner brackets"),
          false, true,  true,  true  }
    };

    return allStyles;
}

const StyleInfo &styleInfo(CrosshairStyle style)
{
    const QVector<StyleInfo> &allStyles = styles();

    for (const StyleInfo &info : allStyles) {
        if (info.style == style) {
            return info;
        }
    }

    return allStyles.first(); // the dot, used whenever a style is unknown
}

QString styleKey(CrosshairStyle style)
{
    return styleInfo(style).key;
}

CrosshairStyle styleFromKey(const QString &key, CrosshairStyle fallback)
{
    for (const StyleInfo &info : styles()) {
        if (info.key.compare(key, Qt::CaseInsensitive) == 0) {
            return info.style;
        }
    }

    return fallback;
}

int boundingSide(const SettingsData &settings)
{
    const Geometry geometry = buildGeometry(settings, 0, 0);

    // room for the contour plus one pixel for antialiasing on either side
    const int half = qCeil(geometry.halfExtent) + CONTOUR_WIDTH + 1;

    // an odd edge length puts the center of the crosshair on the middle pixel
    return 2 * half + 1;
}

void paint(QPainter &painter, const SettingsData &settings, const QPointF &center)
{
    // snap to the pixel the center falls into, keeps straight lines crisp
    const Geometry geometry = buildGeometry(settings, qFloor(center.x()), qFloor(center.y()));

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // The contour goes down first and is then covered by the fill. That hides
    // the seams where the shapes of a crosshair overlap each other.
    if (settings.contourColor.alpha() > 0) {
        const QPen contourPen(settings.contourColor, 2 * CONTOUR_WIDTH,
                              Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
        painter.strokePath(geometry.ring, contourPen);
        painter.strokePath(geometry.solid, contourPen);
    }

    painter.fillPath(geometry.ring, settings.fillColor);
    painter.fillPath(geometry.solid, settings.fillColor);

    painter.restore();
}

QPixmap renderPixmap(const SettingsData &settings, const QSize &size, qreal devicePixelRatio)
{
    QPixmap pixmap(size * devicePixelRatio);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    // the center of the middle pixel, same convention as the overlay window
    paint(painter, settings, QPointF(size.width() / 2, size.height() / 2) + QPointF(0.5, 0.5));

    return pixmap;
}

} // namespace CrosshairRenderer
