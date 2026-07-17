#ifndef CAMERAPREVIEW_HPP
#define CAMERAPREVIEW_HPP

#include <touchgfx/widgets/Widget.hpp>

/**
 * Live camera preview widget. On the board it copies the DCMIPP/ISP output
 * framebuffer (RGB565) into the TouchGFX framebuffer during draw(); in the
 * simulator it draws nothing (transparent) so the screen's placeholder shows.
 *
 * The owning view invalidates this widget when a new camera frame arrives,
 * which triggers the per-frame blit.
 */
class CameraPreview : public touchgfx::Widget
{
public:
    CameraPreview();
    virtual ~CameraPreview() {}

    virtual void draw(const touchgfx::Rect& invalidatedArea) const;
    virtual touchgfx::Rect getSolidRect() const;
};

#endif // CAMERAPREVIEW_HPP
