#include <gui/camera_screen/CameraView.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/Application.hpp>

#ifndef SIMULATOR
extern "C" {
#include "app_camera.h"
}
#endif

CameraView::CameraView()
    : captureCallback(this, &CameraView::onCapture),
      backCallback(this, &CameraView::onBack),
      lastFrameId(0), saving(false), errorShown(false), noCardTicks(0)
{
}

void CameraView::setupScreen()
{
    CameraViewBase::setupScreen();

    navBtnCapture.setLabel("CAPTURE");
    navBtnCapture.setClickAction(captureCallback);
    navBtnBackCam.setLabel("BACK");
    navBtnBackCam.setClickAction(backCallback);

    /* The Designer Image is only a placeholder for the live feed; the preview
       widget draws the camera framebuffer directly. */
    cameraImage.setVisible(false);
    preview.setPosition(0, 0, 800, 480);
    add(preview);

    /* Keep the Designer buttons on top of the (full screen) preview. */
    remove(navBtnCapture);
    add(navBtnCapture);
    remove(navBtnBackCam);
    add(navBtnBackCam);

#ifndef SIMULATOR
    /* Wake the ThreadX camera task and start the hardware pipeline. */
    camera_request_start();
#endif
}

void CameraView::tearDownScreen()
{
    CameraViewBase::tearDownScreen();
}

void CameraView::handleTickEvent()
{
#ifndef SIMULATOR
    const camera_state_t state = camera_get_state();

    if (state == CAM_ERROR)
    {
        if (!errorShown)
        {
            navBtnCapture.setLabel("NO CAMERA");
            errorShown = true;
        }
        return;
    }

    if (state == CAM_RUNNING)
    {
        /* Repaint only when the DCMIPP has delivered a new frame. */
        const uint32_t id = camera_get_frame_id();
        if (id != lastFrameId)
        {
            lastFrameId = id;
            preview.invalidate();
        }
    }

    /* The storage thread clears the ready flag once the BMP is on the card. */
    if (saving && !camera_snapshot_ready())
    {
        saving = false;
        navBtnCapture.setLabel("CAPTURE");
    }
    else if (noCardTicks > 0 && --noCardTicks == 0)
    {
        navBtnCapture.setLabel("CAPTURE");
    }
#endif
}

void CameraView::onCapture(const NavButton&)
{
#ifndef SIMULATOR
    if (saving || camera_get_state() != CAM_RUNNING)
    {
        return;
    }
    /* Without a mounted card the staged BMP would never be written, leaving
       the button stuck on "SAVING..."; say so instead. */
    const int storage = presenter->getStorageState();
    if (storage != 1)
    {
        navBtnCapture.setLabel(storage == 3 ? "SD UNREADABLE" : "NO SD CARD");
        noCardTicks = NO_CARD_MESSAGE_TICKS;
        return;
    }
    camera_request_snapshot();
    saving = true;
    navBtnCapture.setLabel("SAVING...");
#endif
}

void CameraView::onBack(const NavButton&)
{
    static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->requestHome();
}
