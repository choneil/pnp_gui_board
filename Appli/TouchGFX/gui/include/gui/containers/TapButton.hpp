#ifndef TAPBUTTON_HPP
#define TAPBUTTON_HPP

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextArea.hpp>
#include <touchgfx/TypedText.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/Callback.hpp>

/**
 * Minimal code-only tap button: a rounded-ish flat Box with a centered text
 * label that fires a callback on release. Used for the 2D/3D view toggle and
 * the extruder/probe tool toggle (no Designer button bitmaps needed).
 */
class TapButton : public touchgfx::Container
{
public:
    TapButton();
    virtual ~TapButton() {}

    virtual void handleClickEvent(const touchgfx::ClickEvent& event);

    /** Sets size and installs the background + label. Call once after construction. */
    void setup(int16_t width, int16_t height);

    /** Sets the label to a typed text from the text database. */
    void setLabelText(const touchgfx::TypedText& text);

    /** Sets the idle and pressed background colors (RGB565-agnostic). */
    void setColors(touchgfx::colortype idle, touchgfx::colortype pressed);

    /** Registers a callback fired when the button is tapped (released). */
    void setAction(touchgfx::GenericCallback<const TapButton&>& callback) { action = &callback; }

protected:
    touchgfx::Box background;
    touchgfx::TextArea label;
    touchgfx::colortype idleColor;
    touchgfx::colortype pressedColor;
    touchgfx::GenericCallback<const TapButton&>* action;
};

#endif // TAPBUTTON_HPP
