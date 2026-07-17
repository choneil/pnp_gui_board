#include <gui/containers/CameraPreview.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <string.h>

#ifndef SIMULATOR
extern "C" {
#include "app_camera.h"
}
#endif

CameraPreview::CameraPreview()
{
}

touchgfx::Rect CameraPreview::getSolidRect() const
{
#ifndef SIMULATOR
    /* The camera frame fully covers the widget (opaque). */
    return touchgfx::Rect(0, 0, getWidth(), getHeight());
#else
    /* Transparent in the simulator so the placeholder background shows. */
    return touchgfx::Rect(0, 0, 0, 0);
#endif
}

void CameraPreview::draw(const touchgfx::Rect& invalidatedArea) const
{
#ifndef SIMULATOR
    const uint16_t* cam = camera_get_framebuffer();
    if (cam == 0)
    {
        return;
    }

    const touchgfx::Rect abs = getAbsoluteRect();
    uint16_t* fb = touchgfx::HAL::getInstance()->lockFrameBuffer();
    const int32_t stride = touchgfx::HAL::DISPLAY_WIDTH;

    for (int16_t y = invalidatedArea.y; y < invalidatedArea.bottom(); y++)
    {
        const int32_t fbRow = (abs.y + y) * stride + (abs.x + invalidatedArea.x);
        const int32_t camRow = y * CAM_WIDTH + invalidatedArea.x;
        memcpy(&fb[fbRow], &cam[camRow], static_cast<size_t>(invalidatedArea.width) * 2);
    }

    touchgfx::HAL::getInstance()->unlockFrameBuffer();
#else
    (void)invalidatedArea;
#endif
}
