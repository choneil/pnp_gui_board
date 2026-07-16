#include <gui/containers/AxisJogPanel.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Application.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/TypedText.hpp>

namespace
{
// Layout (panel is 305x480, matching the card column width).
const int16_t PANEL_W = 305;
const int16_t PANEL_H = 480;

const int16_t BTN_Y = 170;
const int16_t BTN_H = 100;
const int16_t MINUS_X = 24;
const int16_t PLUS_X = 161;
const int16_t BTN_W = 120;

const int16_t STEP_Y = 310;
const int16_t STEP_H = 54;
const int16_t STEP_W = 81;
const int16_t STEP_X[3] = { 24, 117, 210 };

const int16_t DONE_X = 205;
const int16_t DONE_Y = 16;
const int16_t DONE_W = 84;
const int16_t DONE_H = 44;

// Hold-to-repeat: first repeat after HOLD_DELAY ticks, then every HOLD_RATE.
const uint16_t HOLD_DELAY = 30;
const uint16_t HOLD_RATE = 8;

const float STEP_VALUES[3] = { 0.1f, 1.0f, 10.0f };

bool inside(const touchgfx::ClickEvent& e, int16_t x, int16_t y, int16_t w, int16_t h)
{
    return e.getX() >= x && e.getX() < x + w && e.getY() >= y && e.getY() < y + h;
}
}

AxisJogPanel::AxisJogPanel()
    : jogAction(0), doneAction(0), selectedStep(1), heldDirection(0), heldTicks(0)
{
    setWidth(PANEL_W);
    setHeight(PANEL_H);
    setTouchable(true);

    background.setPosition(0, 0, PANEL_W, PANEL_H);
    background.setColor(touchgfx::Color::getColorFromRGB(235, 238, 244));
    add(background);

    // Title: axis letter, small and muted like the card labels.
    titleBuffer[0] = 0;
    titleText.setPosition(24, 26, 140, 26);
    titleText.setColor(touchgfx::Color::getColorFromRGB(113, 132, 156));
    titleText.setTypedText(touchgfx::TypedText(T_AXISNAMELABEL));
    titleText.setWildcard(titleBuffer);
    add(titleText);

    // DONE button, top right.
    doneBox.setPosition(DONE_X, DONE_Y, DONE_W, DONE_H);
    doneBox.setColor(touchgfx::Color::getColorFromRGB(210, 216, 228));
    add(doneBox);
    doneText.setPosition(DONE_X, DONE_Y + 11, DONE_W, DONE_H - 11);
    doneText.setColor(touchgfx::Color::getColorFromRGB(0, 37, 128));
    doneText.setTypedText(touchgfx::TypedText(T_JOGDONE));
    add(doneText);

    // Position readout, big and deep blue like the card value.
    positionBuffer[0] = 0;
    positionText.setPosition(24, 74, 200, 70);
    positionText.setColor(touchgfx::Color::getColorFromRGB(0, 37, 128));
    positionText.setTypedText(touchgfx::TypedText(T___SINGLEUSE_K69Z));
    positionText.setWildcard(positionBuffer);
    add(positionText);

    unitText.setColor(touchgfx::Color::getColorFromRGB(0, 37, 128));
    unitText.setTypedText(touchgfx::TypedText(T_AXISUNIT));
    unitText.resizeToCurrentText();
    add(unitText);

    // Jog buttons.
    minusBox.setPosition(MINUS_X, BTN_Y, BTN_W, BTN_H);
    minusBox.setColor(touchgfx::Color::getColorFromRGB(210, 216, 228));
    add(minusBox);
    minusText.setPosition(MINUS_X, BTN_Y + 24, BTN_W, BTN_H - 24);
    minusText.setColor(touchgfx::Color::getColorFromRGB(0, 37, 128));
    minusText.setTypedText(touchgfx::TypedText(T_JOGMINUS));
    add(minusText);

    plusBox.setPosition(PLUS_X, BTN_Y, BTN_W, BTN_H);
    plusBox.setColor(touchgfx::Color::getColorFromRGB(210, 216, 228));
    add(plusBox);
    plusText.setPosition(PLUS_X, BTN_Y + 24, BTN_W, BTN_H - 24);
    plusText.setColor(touchgfx::Color::getColorFromRGB(0, 37, 128));
    plusText.setTypedText(touchgfx::TypedText(T_JOGPLUS));
    add(plusText);

    // Step selector.
    const TEXTS stepIds[3] = { T_JOGSTEP01, T_JOGSTEP1, T_JOGSTEP10 };
    for (int i = 0; i < 3; i++)
    {
        stepBox[i].setPosition(STEP_X[i], STEP_Y, STEP_W, STEP_H);
        add(stepBox[i]);
        stepText[i].setPosition(STEP_X[i], STEP_Y + 12, STEP_W, STEP_H - 12);
        stepText[i].setColor(touchgfx::Color::getColorFromRGB(0, 37, 128));
        stepText[i].setTypedText(touchgfx::TypedText(stepIds[i]));
        add(stepText[i]);
    }
    selectStep(1);  // default 1mm
}

void AxisJogPanel::init()
{
    touchgfx::Application::getInstance()->registerTimerWidget(this);
}

float AxisJogPanel::stepValue() const
{
    return STEP_VALUES[selectedStep];
}

void AxisJogPanel::selectStep(int index)
{
    selectedStep = index;
    for (int i = 0; i < 3; i++)
    {
        // Selected step gets a stronger fill.
        stepBox[i].setColor(i == selectedStep
                            ? touchgfx::Color::getColorFromRGB(170, 182, 205)
                            : touchgfx::Color::getColorFromRGB(210, 216, 228));
        stepBox[i].invalidate();
    }
}

void AxisJogPanel::jog(int direction)
{
    if (jogAction && jogAction->isValid())
    {
        jogAction->execute(direction * stepValue());
    }
}

void AxisJogPanel::handleTickEvent()
{
    if (heldDirection == 0 || !isVisible())
    {
        return;
    }
    heldTicks++;
    if (heldTicks >= HOLD_DELAY && ((heldTicks - HOLD_DELAY) % HOLD_RATE) == 0)
    {
        jog(heldDirection);
    }
}

void AxisJogPanel::handleClickEvent(const touchgfx::ClickEvent& event)
{
    if (event.getType() == touchgfx::ClickEvent::PRESSED)
    {
        if (inside(event, MINUS_X, BTN_Y, BTN_W, BTN_H))
        {
            heldDirection = -1;
            heldTicks = 0;
            jog(-1);
        }
        else if (inside(event, PLUS_X, BTN_Y, BTN_W, BTN_H))
        {
            heldDirection = 1;
            heldTicks = 0;
            jog(1);
        }
    }
    else if (event.getType() == touchgfx::ClickEvent::RELEASED)
    {
        heldDirection = 0;
        for (int i = 0; i < 3; i++)
        {
            if (inside(event, STEP_X[i], STEP_Y, STEP_W, STEP_H))
            {
                selectStep(i);
                return;
            }
        }
        if (inside(event, DONE_X, DONE_Y, DONE_W, DONE_H))
        {
            if (doneAction && doneAction->isValid())
            {
                doneAction->execute();
            }
        }
    }
    else // CANCEL
    {
        heldDirection = 0;
    }
}

void AxisJogPanel::setAxisLabel(const char* label)
{
    touchgfx::Unicode::strncpy(titleBuffer, label, LABEL_SIZE);
    titleText.invalidate();
}

void AxisJogPanel::setPosition(float value)
{
    touchgfx::Unicode::snprintfFloat(positionBuffer, POSITION_SIZE, "%.1f", value);
    positionText.resizeToCurrentText();
    positionText.invalidate();

    unitText.invalidate();
    unitText.setXY(positionText.getX() + positionText.getWidth() + 6, 92);
    unitText.invalidate();
}
