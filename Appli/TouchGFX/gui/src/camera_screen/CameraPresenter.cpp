#include <gui/camera_screen/CameraView.hpp>
#include <gui/camera_screen/CameraPresenter.hpp>

CameraPresenter::CameraPresenter(CameraView& v)
    : view(v)
{

}

void CameraPresenter::activate()
{

}

void CameraPresenter::deactivate()
{

}

int CameraPresenter::getStorageState() const
{
    return model->getStorageState();
}
