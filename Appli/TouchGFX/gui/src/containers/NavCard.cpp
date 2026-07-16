#include <gui/containers/NavCard.hpp>
#include <touchgfx/Unicode.hpp>

NavCard::NavCard() : action(0)
{
    setTouchable(true);
}

void NavCard::initialize()
{
    NavCardBase::initialize();
}

void NavCard::setLabel(const char* text)
{
    touchgfx::Unicode::strncpy(cardLabelBuffer, text, CARDLABEL_SIZE - 1);
    cardLabelBuffer[CARDLABEL_SIZE - 1] = 0;
    cardLabel.invalidate();
}

void NavCard::handleClickEvent(const touchgfx::ClickEvent& event)
{
    if (event.getType() == touchgfx::ClickEvent::PRESSED)
    {
        // Fade out on press
        cardBg.setAlpha(200);
        cardBg.invalidate();
    }
    else if (event.getType() == touchgfx::ClickEvent::RELEASED ||
             event.getType() == touchgfx::ClickEvent::CANCEL)
    {
        // Reset to full opacity on release or if the user drags off the button
        cardBg.setAlpha(255);
        cardBg.invalidate();

        // ADD THIS: Fire the callback only on a true release, not a cancel
        if (event.getType() == touchgfx::ClickEvent::RELEASED)
        {
            if (action && action->isValid())
            {
                action->execute(*this);
            }
        }
    }
}