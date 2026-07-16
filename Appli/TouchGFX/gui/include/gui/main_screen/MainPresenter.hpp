#ifndef MAINPRESENTER_HPP
#define MAINPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MainView;

class MainPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MainPresenter(MainView& v);

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

    virtual void axisPositionsUpdated(float x, float y, float z);

    /** Forwards a jog/move request from the view to the model. */
    void requestAxisMove(int axis, float target);

    /** SD file list passthroughs for the Load File screen. */
    int getStorageState() const;
    int getFileCount() const;
    const char* getFileName(int i) const;
    uint32_t getFileSize(int i) const;

    virtual ~MainPresenter() {}

private:
    MainPresenter();

    MainView& view;
};

#endif // MAINPRESENTER_HPP
