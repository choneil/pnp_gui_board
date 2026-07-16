#ifndef HOMEVIEW_HPP
#define HOMEVIEW_HPP

#include <gui_generated/home_screen/HomeViewBase.hpp>
#include <gui/home_screen/HomePresenter.hpp>
#include <touchgfx/Callback.hpp>

class HomeView : public HomeViewBase
{
public:
    HomeView();
    virtual ~HomeView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

protected:
    // 1. Add the ClickEvent to the Callback template
    touchgfx::Callback<HomeView, const NavCard&, const touchgfx::ClickEvent&> cardCallback;

    // 2. Add the ClickEvent argument to the function declaration
    void onCard(const NavCard& card, const touchgfx::ClickEvent& event);
};

#endif // HOMEVIEW_HPP