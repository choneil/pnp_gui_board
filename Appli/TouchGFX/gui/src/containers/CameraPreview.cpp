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

    /* Clip the requested area to the camera frame we actually have. */
    const int16_t x0 = invalidatedArea.x;
    const int16_t y0 = invalidatedArea.y;
    const int16_t x1 = (invalidatedArea.right() > CAM_WIDTH) ? CAM_WIDTH : invalidatedArea.right();
    const int16_t y1 = (invalidatedArea.bottom() > CAM_HEIGHT) ? CAM_HEIGHT : invalidatedArea.bottom();
    if (x1 <= x0 || y1 <= y0)
    {
        return;
    }

    const touchgfx::Rect abs = getAbsoluteRect();
    touchgfx::HAL* hal = touchgfx::HAL::getInstance();
    uint16_t* fb = hal->lockFrameBuffer();
    const int32_t stride = touchgfx::HAL::FRAME_BUFFER_WIDTH;
    const size_t bytes = static_cast<size_t>(x1 - x0) * 2;

    /* FB_RAM is mapped Device-nGnRnE by MPU region 1, so a CPU copy here costs
       one un-gathered bus transaction per halfword -- far too slow for 768KB a
       frame. Hand full-width bands to the GPU2D block copy in one go instead;
       when the band spans whole rows at matching stride the region is
       contiguous in both buffers, so it is a single transfer. */
    const bool contiguous = (abs.x + x0 == 0) && (x0 == 0) &&
                            (x1 == CAM_WIDTH) && (stride == CAM_WIDTH);

    if (cam == 0)
    {
        /* No frame yet (starting up, or the sensor failed). getSolidRect()
           claims this widget is opaque, so it must paint something --
           leaving the framebuffer untouched would show stale garbage. */
        for (int16_t y = y0; y < y1; y++)
        {
            memset(&fb[(abs.y + y) * stride + (abs.x + x0)], 0, bytes);
        }
    }
    else if (contiguous)
    {
        hal->blockCopy(&fb[(abs.y + y0) * stride],
                       &cam[y0 * CAM_WIDTH],
                       static_cast<uint32_t>(y1 - y0) * CAM_WIDTH * 2);
    }
    else
    {
        for (int16_t y = y0; y < y1; y++)
        {
            hal->blockCopy(&fb[(abs.y + y) * stride + (abs.x + x0)],
                           &cam[y * CAM_WIDTH + x0],
                           static_cast<uint32_t>(bytes));
        }
    }

    hal->unlockFrameBuffer();
#else
    (void)invalidatedArea;
#endif
}
