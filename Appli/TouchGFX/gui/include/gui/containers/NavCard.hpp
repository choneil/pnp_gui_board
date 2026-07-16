#ifndef NAVCARD_HPP
#define NAVCARD_HPP

#include <gui_generated/containers/NavCardBase.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/events/ClickEvent.hpp>

/**
 * Room-card style Designer button (HVAC motif): card image + wildcard label
 * populated from code. Fires a callback on release; dims while pressed.
 */
class NavCard : public NavCardBase
{
public:
    NavCard();
    virtual ~NavCard() {}
    virtual void initialize();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);

    void setLabel(const char* text);
    void setClickAction(touchgfx::GenericCallback<const NavCard&>& callback) { action = &callback; }

protected:
    touchgfx::GenericCallback<const NavCard&>* action;
};

#endif // NAVCARD_HPP
