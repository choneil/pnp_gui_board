#include <gui/containers/topView.hpp>

namespace
{
// Direction of travel in screen space (top view). Machine X moves the beam
// assembly DOWN the side rails (home = top); machine Y moves the toolhead
// RIGHT along the beam (home = left). Flip a sign if a part moves the wrong
// way once you see it running.
const float DIR_X = 1.0f;   // machine X: +1 = downward on screen
const float DIR_Y = 1.0f;   // machine Y: +1 = rightward on screen

// The drawing's span between the part's home pose and far-endstop contact is
// shorter than the machine's real stroke, so map real mm onto the drawn span:
// full real travel lands the part exactly on the drawn endstop.
const float X_MM_TO_DRAWING = 320.0f / 380.0f;
const float Y_MM_TO_DRAWING = 370.0f / 500.0f;
}

topView::topView()
    : scaleFactor(1.0f), posX(0.0f), posY(0.0f)
{
    // topViewBase's constructor has already applied the design-time scale and
    // geometry to each SVG. Capture those as the stable reference for scaling
    // (so re-running scaleSvgsToContainer does not compound).
    svgBase[0] = { &svgImage1, svgImage1.getScaleX(), svgImage1.getScaleY(),
                   svgImage1.getX(), svgImage1.getY(), svgImage1.getWidth(), svgImage1.getHeight() };
    svgBase[1] = { &svgImage2, svgImage2.getScaleX(), svgImage2.getScaleY(),
                   svgImage2.getX(), svgImage2.getY(), svgImage2.getWidth(), svgImage2.getHeight() };
    svgBase[2] = { &svgImage3, svgImage3.getScaleX(), svgImage3.getScaleY(),
                   svgImage3.getX(), svgImage3.getY(), svgImage3.getWidth(), svgImage3.getHeight() };

    // Bounding box of the SVG layout at its authored (base) size.
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

void topView::initialize()
{
    topViewBase::initialize();
    scaleSvgsToContainer();
}

void topView::scaleSvgsToContainer()
{
    if (contentWidth <= 0 || contentHeight <= 0)
    {
        return;
    }

    // Uniform factor that fits the whole layout inside the current container.
    const float fx = static_cast<float>(getWidth())  / contentWidth;
    const float fy = static_cast<float>(getHeight()) / contentHeight;
    const float f = (fx < fy) ? fx : fy;
    scaleFactor = f;

    // Center the scaled layout within the container.
    const float offsetX = (static_cast<float>(getWidth())  - contentWidth  * f) * 0.5f;
    const float offsetY = (static_cast<float>(getHeight()) - contentHeight * f) * 0.5f;

    for (int i = 0; i < 3; i++)
    {
        SvgBase& b = svgBase[i];
        b.image->setScale(b.scaleX * f, b.scaleY * f);
        baseX[i] = static_cast<int16_t>(offsetX + (b.x - contentMinX) * f);
        baseY[i] = static_cast<int16_t>(offsetY + (b.y - contentMinY) * f);
        // Size every widget to the full fitted area so no layer's SVG is clipped.
        b.image->setPosition(baseX[i], baseY[i],
                             static_cast<int16_t>(contentWidth  * f),
                             static_cast<int16_t>(contentHeight * f));
        b.image->invalidate();
    }

    // Re-apply the current machine position on top of the fresh layout.
    applyAxisMotion();
    invalidate();
}

void topView::setAxisPositions(float x, float y)
{
    posX = x;
    posY = y;
    applyAxisMotion();
}

void topView::placePart(int i, float dxMm, float dyMm)
{
    SvgBase& b = svgBase[i];
    const int16_t x = static_cast<int16_t>(baseX[i] + dxMm * scaleFactor);
    const int16_t y = static_cast<int16_t>(baseY[i] + dyMm * scaleFactor);
    b.image->invalidate();  // clear old position
    b.image->setPosition(x, y,
                         static_cast<int16_t>(contentWidth  * scaleFactor),
                         static_cast<int16_t>(contentHeight * scaleFactor));
    b.image->invalidate();  // draw at new position
}

void topView::applyAxisMotion()
{
    // Beam assembly (Y axis) travels DOWN the side rails with machine X.
    placePart(GANTRY, 0.0f, posX * X_MM_TO_DRAWING * DIR_X);
    // Toolhead travels down with the beam (machine X) and RIGHT along the
    // beam with machine Y.
    placePart(TOOLHEAD, posY * Y_MM_TO_DRAWING * DIR_Y, posX * X_MM_TO_DRAWING * DIR_X);
    // Foundation is static (no call).
}
