#ifndef FRONTVIEW_HPP
#define FRONTVIEW_HPP

#include <gui_generated/containers/frontViewBase.hpp>

class frontView : public frontViewBase
{
public:
    frontView();
    virtual ~frontView() {}

    virtual void initialize();

    /**
     * Rescale and reposition the SVG images so the whole layout fits the
     * container uniformly, regardless of the container's current size.
     */
    void scaleSvgsToContainer();

    /**
     * Moves the toolhead carriage to reflect the machine position, in mm.
     * The carriage rides horizontally with the Y-axis travel (y=0 = home =
     * left); the extruder body/toolhead additionally slides vertically with Z
     * (z=0 = home = up).
     */
    void setAxisPositions(float x, float y, float z);

    /** @return Width of the SVG layout at its authored (base) size. */
    int16_t getContentWidth() const { return contentWidth; }

    /** @return Height of the SVG layout at its authored (base) size. */
    int16_t getContentHeight() const { return contentHeight; }

protected:
    // Reference (design-time) scale and geometry of each SVG, captured from the
    // values set by frontViewBase so container-relative scaling has a stable
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
    static const int TOOLHEAD   = 0;
    static const int CARRIAGE   = 1;
    static const int FOUNDATION = 2;  // foundation + Y-axis (svgImage3, static)

    // Bounding box of the SVG layout at base size (computed in the constructor).
    int16_t contentMinX;
    int16_t contentMinY;
    int16_t contentWidth;
    int16_t contentHeight;

    // Container-fit state used to place the moving parts.
    float scaleFactor;   // mm -> pixels
    int16_t baseX[3];
    int16_t baseY[3];

    // Current machine position, in mm.
    float posX;
    float posY;
    float posZ;

    void applyAxisMotion();
    void placePart(int i, float dxMm, float dyMm);
};

#endif // FRONTVIEW_HPP
