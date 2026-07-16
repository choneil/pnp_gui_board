#include <gui/containers/TapButton.hpp>
#include <touchgfx/Color.hpp>

TapButton::TapButton()
    : idleColor(touchgfx::Color::getColorFromRGB(255, 255, 255)),
      pressedColor(touchgfx::Color::getColorFromRGB(210, 219, 230)),
      action(0)
{
    setTouchable(true);
}

void TapButton::setup(int16_t width, int16_t height)
{
    setWidthHeight(width, height);

    background.setPosition(0, 0, width, height);
    background.setColor(idleColor);
    add(background);

    label.setPosition(0, 0, width, height);
    label.setColor(touchgfx::Color::getColorFromRGB(0, 37, 128));
    label.setLinespacing(0);
    add(label);
}

void TapButton::setColors(touchgfx::colortype idle, touchgfx::colortype pressed)
{
    idleColor = idle;
    pressedColor = pressed;
    background.setColor(idleColor);
    background.invalidate();
}

void TapButton::setLabelText(const touchgfx::TypedText& text)
{
    label.setTypedText(text);
    label.invalidate();
}

void TapButton::handleClickEvent(const touchgfx::ClickEvent& event)
{
    if (event.getType() == touchgfx::ClickEvent::PRESSED)
    {
        background.setColor(pressedColor);
        background.invalidate();
    }
    else if (event.getType() == touchgfx::ClickEvent::RELEASED)
    {
        background.setColor(idleColor);
        background.invalidate();
        if (action && action->isValid())
        {
            action->execute(*this);
        }
    }
}
