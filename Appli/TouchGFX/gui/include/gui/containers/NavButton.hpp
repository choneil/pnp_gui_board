#ifndef NAVBUTTON_HPP
#define NAVBUTTON_HPP

#include <gui_generated/containers/NavButtonBase.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/events/ClickEvent.hpp>

/**
 * Designer custom container button: white box + wildcard label, populated
 * from code with setLabel(). Fires a callback on release; dims while pressed.
 */
class NavButton : public NavButtonBase
{
public:
    NavButton();
    virtual ~NavButton() {}
    virtual void initialize();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);

    void setLabel(const char* text);
    void setClickAction(touchgfx::GenericCallback<const NavButton&>& callback) { action = &callback; }

protected:
    touchgfx::GenericCallback<const NavButton&>* action;
};

#endif // NAVBUTTON_HPP
