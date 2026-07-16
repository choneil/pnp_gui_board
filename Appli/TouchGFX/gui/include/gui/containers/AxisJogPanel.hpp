#ifndef AXISJOGPANEL_HPP
#define AXISJOGPANEL_HPP

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextArea.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/Unicode.hpp>

/**
 * Slide-in jog panel for one axis: current position readout, +/- jog buttons
 * with hold-to-repeat, and a 0.1/1/10 mm step selector. Built in code (no
 * Designer container). The panel emits jog deltas through a callback; the
 * owning view routes them to the model.
 */
class AxisJogPanel : public touchgfx::Container
{
public:
    AxisJogPanel();
    virtual ~AxisJogPanel() {}

    /** Registers the tick timer; call once from the owning view's setup. */
    void init();

    virtual void handleTickEvent();
    virtual void handleClickEvent(const touchgfx::ClickEvent& event);

    /** Sets the axis letter shown in the title ("X"/"Y"/"Z"). */
    void setAxisLabel(const char* label);

    /** Updates the position readout (mm). */
    void setPosition(float value);

    /** Called with a signed delta (mm) when the user jogs. */
    void setJogAction(touchgfx::GenericCallback<float>& callback) { jogAction = &callback; }

    /** Called when the user taps DONE. */
    void setDoneAction(touchgfx::GenericCallback<>& callback) { doneAction = &callback; }

protected:
    static const uint16_t LABEL_SIZE = 4;
    static const uint16_t POSITION_SIZE = 10;

    touchgfx::Box background;
    touchgfx::Box doneBox;
    touchgfx::TextArea doneText;
    touchgfx::TextAreaWithOneWildcard titleText;
    touchgfx::Unicode::UnicodeChar titleBuffer[LABEL_SIZE];
    touchgfx::TextAreaWithOneWildcard positionText;
    touchgfx::Unicode::UnicodeChar positionBuffer[POSITION_SIZE];
    touchgfx::TextArea unitText;
    touchgfx::Box minusBox;
    touchgfx::TextArea minusText;
    touchgfx::Box plusBox;
    touchgfx::TextArea plusText;
    touchgfx::Box stepBox[3];
    touchgfx::TextArea stepText[3];

    touchgfx::GenericCallback<float>* jogAction;
    touchgfx::GenericCallback<>* doneAction;

    int selectedStep;   // 0/1/2 -> 0.1/1/10 mm
    int heldDirection;  // -1, 0, +1 while a jog button is held
    uint16_t heldTicks;

    float stepValue() const;
    void selectStep(int index);
    void jog(int direction);
};

#endif // AXISJOGPANEL_HPP
