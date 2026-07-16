#include <gui/containers/frontView.hpp>

namespace
{
// The toolhead carriage travels horizontally with the Y axis; y=0 = left.
// Flip the sign if it moves the wrong way once you see it running.
const float DIR = 1.0f;

// The drawing's span between the carriage's home pose and far-endstop contact
// is shorter than the real 500mm stroke; map real mm onto the drawn span so
// full travel lands the carriage exactly on the drawn endstop. Must match
// Y_MM_TO_DRAWING in topView.cpp to keep the two views in lockstep.
const float Y_MM_TO_DRAWING = 370.0f / 500.0f;

// Z: the extruder body/toolhead slides vertically; +Z = downward, home = up.
// No Z endstop span has been measured yet, so the drawing is assumed 1:1 -
// adjust this ratio (real mm -> drawing mm) once the Z travel is calibrated.
const float DIR_Z = 1.0f;
const float Z_MM_TO_DRAWING = 1.0f;
}

frontView::frontView()
    : scaleFactor(1.0f), posX(0.0f), posY(0.0f), posZ(0.0f)
{
    // frontViewBase's constructor has already applied the design-time scale and
    // geometry to each SVG. Capture those as the stable reference for scaling.
    svgBase[0] = { &svgImage1, svgImage1.getScaleX(), svgImage1.getScaleY(),
                   svgImage1.getX(), svgImage1.getY(), svgImage1.getWidth(), svgImage1.getHeight() };
    svgBase[1] = { &svgImage2, svgImage2.getScaleX(), svgImage2.getScaleY(),
                   svgImage2.getX(), svgImage2.getY(), svgImage2.getWidth(), svgImage2.getHeight() };
    svgBase[2] = { &svgImage3, svgImage3.getScaleX(), svgImage3.getScaleY(),
                   svgImage3.getX(), svgImage3.getY(), svgImage3.getWidth(), svgImage3.getHeight() };

    int16_t minX = svgBase[0].x;
    int16_t minY = svgBase[0].y;
    int16_t maxX = svgBase[0].x + svgBase[0].width;
    int16_t maxY = svgBase[0].y + svgBase[0].height;
    for (int i = 1; i < 3; i++)
    {
        const SvgBase& b = svgBase[i];
        if (b.x < minX) { minX = b.x; }
        if (b.y < minY) { minY = b.y; }
        if (b.x + b.width  > maxX) { maxX = b.x + b.width; }
        if (b.y + b.height > maxY) { maxY = b.y + b.height; }
    }
    contentMinX = minX;
    contentMinY = minY;
    contentWidth  = maxX - minX;
    contentHeight = maxY - minY;

    for (int i = 0; i < 3; i++)
    {
        baseX[i] = 0;
        baseY[i] = 0;
    }
}

void frontView::initialize()
{
    frontViewBase::initialize();
    scaleSvgsToContainer();
}

void frontView::scaleSvgsToContainer()
{
    if (contentWidth <= 0 || contentHeight <= 0)
    {
        return;
    }

    const float fx = static_cast<float>(getWidth())  / contentWidth;
    const float fy = static_cast<float>(getHeight()) / contentHeight;
    const float f = (fx < fy) ? fx : fy;
    scaleFactor = f;

    const float offsetX = (static_cast<float>(getWidth())  - contentWidth  * f) * 0.5f;
    const float offsetY = (static_cast<float>(getHeight()) - contentHeight * f) * 0.5f;

    for (int i = 0; i < 3; i++)
    {
        SvgBase& b = svgBase[i];
        b.image->setScale(b.scaleX * f, b.scaleY * f);
        baseX[i] = static_cast<int16_t>(offsetX + (b.x - contentMinX) * f);
        baseY[i] = static_cast<int16_t>(offsetY + (b.y - contentMinY) * f);
        // Size every widget to the full fitted area so no layer's SVG is clipped
        // by a smaller authored widget rect (e.g. the 400-wide carriage widget).
        b.image->setPosition(baseX[i], baseY[i],
                             static_cast<int16_t>(contentWidth  * f),
                             static_cast<int16_t>(contentHeight * f));
        b.image->invalidate();
    }

    applyAxisMotion();
    invalidate();
}

void frontView::setAxisPositions(float x, float y, float z)
{
    posX = x;
    posY = y;
    posZ = z;
    applyAxisMotion();
}

void frontView::placePart(int i, float dxMm, float dyMm)
{
    SvgBase& b = svgBase[i];
    const int16_t x = static_cast<int16_t>(baseX[i] + dxMm * scaleFactor * DIR);
    const int16_t y = static_cast<int16_t>(baseY[i] + dyMm * scaleFactor);
    b.image->invalidate();
    b.image->setPosition(x, y,
                         static_cast<int16_t>(contentWidth  * scaleFactor),
                         static_cast<int16_t>(contentHeight * scaleFactor));
    b.image->invalidate();
}

void frontView::applyAxisMotion()
{
    // Carriage rides horizontally with the Y-axis travel.
    placePart(CARRIAGE, posY * Y_MM_TO_DRAWING, 0.0f);
    // Extruder body/toolhead rides with the carriage in Y and slides
    // vertically with Z.
    placePart(TOOLHEAD, posY * Y_MM_TO_DRAWING, posZ * Z_MM_TO_DRAWING * DIR_Z);
    // Foundation + Y-axis frame is static (no call).
}
