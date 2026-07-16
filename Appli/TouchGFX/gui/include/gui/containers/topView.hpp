#ifndef TOPVIEW_HPP
#define TOPVIEW_HPP

#include <gui_generated/containers/topViewBase.hpp>

class topView : public topViewBase
{
public:
    topView();
    virtual ~topView() {}

    virtual void initialize();

    /**
     * Rescale and reposition the SVG images so the whole layout fits the
     * container uniformly, regardless of the container's current size. Call
     * again after changing the container size at runtime.
     */
    void scaleSvgsToContainer();

    /**
     * Moves the gantry (Y-axis assembly) and toolhead to reflect the machine
     * position, in millimetres. x is travel along the X rails, y is the toolhead
     * along the beam. (0,0) = home = top-left.
     */
    void setAxisPositions(float x, float y);

    /** @return Width of the SVG layout at its authored (base) size. */
    int16_t getContentWidth() const { return contentWidth; }

    /** @return Height of the SVG layout at its authored (base) size. */
    int16_t getContentHeight() const { return contentHeight; }

protected:
    // Reference (design-time) scale and geometry of each SVG, captured from the
    // values set by topViewBase so container-relative scaling has a stable
    // baseline that does not compound across calls.
    struct SvgBase
    {
        touchgfx::SVGImage* image;
        float scaleX;
        float scaleY;
        int16_t x;
        int16_t y;
        int16_t width;
        int16_t height;
    };
    SvgBase svgBase[3];

    // svgBase indices (match the constructor's capture order).
    static const int FOUNDATION = 0;
    static const int TOOLHEAD   = 1;
    static const int GANTRY     = 2;  // Y-axis assembly (svgImage3)

    // Bounding box of the SVG layout at base size (computed in the constructor).
    int16_t contentMinX;
    int16_t contentMinY;
    int16_t contentWidth;
    int16_t contentHeight;

    // Container-fit state (from scaleSvgsToContainer) used to place moving parts.
    float scaleFactor;   // mm -> pixels
    int16_t baseX[3];    // home (position-0) top-left of each part, in pixels
    int16_t baseY[3];

    // Current machine position, in mm.
    float posX;
    float posY;

    void applyAxisMotion();
    void placePart(int i, float dxMm, float dyMm);
};

#endif // TOPVIEW_HPP
