#include "crosshairrenderer.h"

#include <QCoreApplication>
#include <QPainterPath>
#include <QPainterPathStroker>
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

// Horizontal bar of the given thickness, centered on axisY, spanning [xFrom, xTo).
QRect horizontalBar(int axisY, int thickness, int xFrom, int xTo)
{
    return QRect(xFrom, axisY - thickness / 2, xTo - xFrom, thickness);
}

// Vertical bar of the given thickness, centered on axisX, spanning [yFrom, yTo).
QRect verticalBar(int axisX, int thickness, int yFrom, int yTo)
{
    return QRect(axisX - thickness / 2, yFrom, thickness, yTo - yFrom);
}

// Adds a chain of line segments, drawn with the given thickness, to a path.
void addStrokedLines(QPainterPath &path, const QVector<QPointF> &points, int thickness)
{
    if (points.size() < 2) {
        return;
    }

    QPainterPath line;
    line.moveTo(points.first());
    for (int i = 1; i < points.size(); ++i) {
        line.lineTo(points.at(i));
    }

    QPainterPathStroker stroker;
    stroker.setWidth(thickness);
    stroker.setCapStyle(Qt::FlatCap);
    stroker.setJoinStyle(Qt::MiterJoin);
    path.addPath(stroker.createStroke(line));
}

// Adds a circle of the given diameter around the center to a path.
void addCircle(QPainterPath &path, const QPointF &center, qreal diameter)
{
    if (diameter <= 0.0) {
        return;
    }
    path.addEllipse(center, diameter / 2.0, diameter / 2.0);
}

// Adds the four arms of a cross, each one reaching from innerRadius to
// outerRadius. Directions that are switched off are skipped, that is how the
// T shape drops its upper arm.
void addCrossArms(QPainterPath &path, int cx, int cy, int thickness,
                  int innerRadius, int outerRadius, bool up, bool down, bool left, bool right)
{
    if (up) {
        path.addRect(verticalBar(cx, thickness, cy - outerRadius, cy - innerRadius));
    }
    if (down) {
        path.addRect(verticalBar(cx, thickness, cy + innerRadius, cy + outerRadius));
    }
    if (left) {
        path.addRect(horizontalBar(cy, thickness, cx - outerRadius, cx - innerRadius));
    }
    if (right) {
        path.addRect(horizontalBar(cy, thickness, cx + innerRadius, cx + outerRadius));
    }
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

    // the center of the (cx, cy) pixel, round shapes are drawn around it
    const QPointF center(cx + 0.5, cy + 0.5);

    Geometry geometry;
    geometry.ring.setFillRule(Qt::OddEvenFill);
    geometry.solid.setFillRule(Qt::WindingFill);

    // half the thickness of a bar, the amount an arm sticks out sideways
    const qreal halfThickness = thickness / 2.0;

    switch (settings.style) {
        case CrosshairStyle::Dot: {
            addCircle(geometry.solid, center, size);
            geometry.halfExtent = size / 2.0;
            break;
        }

        case CrosshairStyle::Circle: {
            addCircle(geometry.ring, center, size);
            addCircle(geometry.ring, center, size - 2.0 * thickness); // punches the hole
            geometry.halfExtent = size / 2.0;
            break;
        }

        case CrosshairStyle::DotInCircle: {
            addCircle(geometry.ring, center, size);
            const qreal holeDiameter = size - 2.0 * thickness;
            addCircle(geometry.ring, center, holeDiameter);

            // a dot that stays clear of the surrounding ring
            addCircle(geometry.solid, center, qMax(2.0, holeDiameter / 3.0));
            geometry.halfExtent = size / 2.0;
            break;
        }

        case CrosshairStyle::Cross: {
            addCrossArms(geometry.solid, cx, cy, thickness, 0, arm, true, true, true, true);
            geometry.halfExtent = qMax(qreal(arm), halfThickness);
            break;
        }

        case CrosshairStyle::CrossWithGap: {
            addCrossArms(geometry.solid, cx, cy, thickness, gap, gap + arm, true, true, true, true);
            geometry.halfExtent = qMax(qreal(gap + arm), halfThickness);
            break;
        }

        case CrosshairStyle::CrossWithGapAndDot: {
            addCrossArms(geometry.solid, cx, cy, thickness, gap, gap + arm, true, true, true, true);
            addCircle(geometry.solid, center, size);
            geometry.halfExtent = qMax(qreal(gap + arm), size / 2.0);
            break;
        }

        case CrosshairStyle::TShape: {
            // no upper arm, keeps the view above the aiming point clear
            addCrossArms(geometry.solid, cx, cy, thickness, gap, gap + arm, false, true, true, true);
            geometry.halfExtent = qMax(qreal(gap + arm), halfThickness);
            break;
        }

        case CrosshairStyle::XShape: {
            const qreal innerReach = gap * DIAGONAL_REACH;
            const qreal outerReach = (gap + arm) * DIAGONAL_REACH;

            for (int dx = -1; dx <= 1; dx += 2) {
                for (int dy = -1; dy <= 1; dy += 2) {
                    addStrokedLines(geometry.solid, {
                        center + QPointF(dx * innerReach, dy * innerReach),
                        center + QPointF(dx * outerReach, dy * outerReach)
                    }, thickness);
                }
            }
            geometry.halfExtent = outerReach + halfThickness;
            break;
        }

        case CrosshairStyle::Chevron: {
            // an arrow head with its tip on the aiming point, opening downwards
            const qreal reach = arm * DIAGONAL_REACH;
            addStrokedLines(geometry.solid, {
                center + QPointF(-reach, reach),
                center,
                center + QPointF(reach, reach)
            }, thickness);
            geometry.halfExtent = reach + halfThickness;
            break;
        }

        case CrosshairStyle::Brackets: {
            // four corner brackets, gap sets how wide they stand apart
            const int corner = gap + arm;

            for (int dx = -1; dx <= 1; dx += 2) {
                for (int dy = -1; dy <= 1; dy += 2) {
                    const int cornerX = cx + dx * corner;
                    const int cornerY = cy + dy * corner;

                    // the legs run from the corner towards the middle of the edges
                    const int xFrom = dx < 0 ? cornerX : cornerX - arm;
                    const int yFrom = dy < 0 ? cornerY : cornerY - arm;

                    geometry.solid.addRect(horizontalBar(cornerY, thickness, xFrom, xFrom + arm));
                    geometry.solid.addRect(verticalBar(cornerX, thickness, yFrom, yFrom + arm));
                }
            }
            geometry.halfExtent = corner + halfThickness;
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
