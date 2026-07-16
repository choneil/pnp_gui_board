#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }

    /**
     * Code-driven navigation (the screens' buttons are custom containers, not
     * Designer interactions, so these replace the generated goto methods).
     * Deferred via pendingScreenTransitionCallback like the generated pattern.
     */
    void requestHome();
    void requestMachine();
    void requestFiles();

private:
    touchgfx::Callback<FrontendApplication> navCallbackFA;
    void gotoHomeImpl();
    void gotoMachineImpl();
    void gotoFilesImpl();
};

#endif // FRONTENDAPPLICATION_HPP
