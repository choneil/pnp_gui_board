#ifndef AXISCARD_HPP
#define AXISCARD_HPP

#include <gui_generated/containers/AxisCardBase.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/Callback.hpp>

class AxisCard : public AxisCardBase
{
public:
    AxisCard();
    virtual ~AxisCard() {}

    virtual void initialize();
    virtual void handleTickEvent();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);

    /** Sets the axis name shown on the card, e.g. "X", "Y" or "Z". */
    void setAxisLabel(const char* label);

    /** Sets the position value shown on the card (one decimal place). */
    void setPosition(float value);

    /**
     * Plays the App4-style 3D flip-in when the screen appears. Cards flip in
     * one after another: card N starts after N*duration ticks and the flip
     * itself takes duration/2 ticks.
     */
    void startupAnimation(uint16_t duration, uint16_t sequenceNumber);

    /** Flip the card out and back in (used as the click transition). */
    void flipCycle(uint16_t duration);

    /** Registers a callback fired when the card is clicked (after the flip starts). */
    void setClickAction(touchgfx::GenericCallback<const AxisCard&>& callback)
    {
        clickAction = &callback;
    }

protected:
    static const uint16_t AXIS_LABEL_SIZE = 4;
    static const uint16_t AXIS_POSITION_SIZE = 10;

    // Axis-name label (X/Y/Z), added here because the generated card only has a
    // value field. Uses the same typography as the value.
    touchgfx::TextAreaWithOneWildcard axisName;
    touchgfx::Unicode::UnicodeChar axisNameBuffer[AXIS_LABEL_SIZE];

    // "mm" unit shown just right of the value.
    touchgfx::TextArea axisUnit;

    // Larger value buffer than the generated one (which holds only 4 chars).
    touchgfx::Unicode::UnicodeChar axisPositionBuffer[AXIS_POSITION_SIZE];

    touchgfx::GenericCallback<const AxisCard&>* clickAction;

    // Flip animation state (ported from MyApplication_4's RoomCard).
    bool flippingIn;      // rotating -90deg -> 0 (face coming up)
    bool flippingOut;     // rotating 0 -> -90deg (face going down)
    bool cycleBack;       // after a flip-out, flip back in (click transition)
    uint16_t initDelay;   // ticks to wait before animating
    float rotationSpeed;  // radians per tick

    void showFace();
    void hideFace();
};

#endif // AXISCARD_HPP
