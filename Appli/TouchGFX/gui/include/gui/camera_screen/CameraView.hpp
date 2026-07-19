#ifndef CAMERAVIEW_HPP
#define CAMERAVIEW_HPP

#include <gui_generated/camera_screen/CameraViewBase.hpp>
#include <gui/camera_screen/CameraPresenter.hpp>
#include <gui/containers/CameraPreview.hpp>

class CameraView : public CameraViewBase
{
public:
    CameraView();
    virtual ~CameraView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

protected:
    void onCapture(const NavButton&);
    void onBack(const NavButton&);

    /* Blits the camera framebuffer straight into the TouchGFX framebuffer;
       replaces the Designer's placeholder Image (which stays hidden). */
    CameraPreview preview;

    touchgfx::Callback<CameraView, const NavButton&> captureCallback;
    touchgfx::Callback<CameraView, const NavButton&> backCallback;

    uint32_t lastFrameId;   /* only repaint when a new frame has landed */
    bool saving;            /* a snapshot is staged/being written       */
    bool errorShown;        /* CAM_ERROR already reported on the button */
    int  noCardTicks;       /* countdown for the "NO SD CARD" message   */

    static const int NO_CARD_MESSAGE_TICKS = 120;  /* ~2s at 60Hz */
};

#endif // CAMERAVIEW_HPP
