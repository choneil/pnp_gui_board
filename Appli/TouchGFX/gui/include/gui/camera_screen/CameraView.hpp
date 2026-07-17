#ifndef CAMERAVIEW_HPP
#define CAMERAVIEW_HPP

#include <gui_generated/camera_screen/CameraViewBase.hpp>
#include <gui/camera_screen/CameraPresenter.hpp>

class CameraView : public CameraViewBase
{
public:
    CameraView();
    virtual ~CameraView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // CAMERAVIEW_HPP
