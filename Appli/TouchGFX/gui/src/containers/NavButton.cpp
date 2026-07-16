#include <gui/containers/NavButton.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>

NavButton::NavButton() : action(0)
{
    setTouchable(true);
}

void NavButton::initialize()
{
    NavButtonBase::initialize();
}

void NavButton::setLabel(const char* text)
{
    touchgfx::Unicode::strncpy(btnLabelBuffer, text, BTNLABEL_SIZE - 1);
    btnLabelBuffer[BTNLABEL_SIZE - 1] = 0;
    btnLabel.invalidate();
}

void NavButton::handleClickEvent(const touchgfx::ClickEvent& event)
{
    if (event.getType() == touchgfx::ClickEvent::PRESSED)
    {
        btnBg.setColor(touchgfx::Color::getColorFromRGB(210, 219, 230));
        btnBg.invalidate();
    }
    else if (event.getType() == touchgfx::ClickEvent::RELEASED)
    {
        btnBg.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
        btnBg.invalidate();
        if (action && action->isValid())
        {
            action->execute(*this);
        }
    }
}
