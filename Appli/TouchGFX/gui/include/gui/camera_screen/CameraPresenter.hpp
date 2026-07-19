#ifndef CAMERAPRESENTER_HPP
#define CAMERAPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class CameraView;

class CameraPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    CameraPresenter(CameraView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~CameraPresenter() {}

    /** 0 = no card, 1 = mounted, 2 = handed to USB. */
    int getStorageState() const;

private:
    CameraPresenter();

    CameraView& view;
};

#endif // CAMERAPRESENTER_HPP
