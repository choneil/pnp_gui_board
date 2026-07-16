#include <gui/containers/AxisCard.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Application.hpp>
#include <touchgfx/Color.hpp>

namespace
{
const float HALF_PI = 1.5707963f;
const uint16_t CLICK_FLIP_DURATION = 24;  // ticks for each half of the click flip
}

AxisCard::AxisCard()
    : clickAction(0), flippingIn(false), flippingOut(false), cycleBack(false),
      initDelay(0), rotationSpeed(0.0f)
{
    // Smoother background (matches the room-card look in MyApplication_4).
    textureMapper1.setRenderingAlgorithm(touchgfx::TextureMapper::BILINEAR_INTERPOLATION);
    // The mapper is only shown while flipping; the flat image is shown at rest.
    textureMapper1.setVisible(false);

    // Small, muted axis-name label near the top.
    axisName.setPosition(32, 30, 140, 22);
    axisName.setColor(touchgfx::Color::getColorFromRGB(113, 132, 156));
    axisName.setLinespacing(0);
    axisName.setTypedText(touchgfx::TypedText(T_AXISNAMELABEL));
    axisNameBuffer[0] = 0;
    axisName.setWildcard(axisNameBuffer);
    add(axisName);

    // Large, deep-blue position value below the label. Rebind the generated
    // field to our larger buffer so values wider than 4 chars fit.
    axisPositionBuffer[0] = 0;
    cardPosition.setWildcard(axisPositionBuffer);
    cardPosition.setXY(32, 50);
    cardPosition.setColor(touchgfx::Color::getColorFromRGB(0, 37, 128));

    // "mm" unit; repositioned next to the value in setPosition().
    axisUnit.setColor(touchgfx::Color::getColorFromRGB(0, 37, 128));
    axisUnit.setTypedText(touchgfx::TypedText(T_AXISUNIT));
    axisUnit.resizeToCurrentText();
    add(axisUnit);

    setAxisLabel("X");
    setPosition(0.0f);

    setTouchable(true);
}

void AxisCard::initialize()
{
    AxisCardBase::initialize();
    touchgfx::Application::getInstance()->registerTimerWidget(this);
}

void AxisCard::showFace()
{
    textureMapper1.setVisible(false);
    image1.setVisible(true);
    axisName.setVisible(true);
    cardPosition.setVisible(true);
    axisUnit.setVisible(true);
    invalidate();
}

void AxisCard::hideFace()
{
    axisName.setVisible(false);
    cardPosition.setVisible(false);
    axisUnit.setVisible(false);
    image1.setVisible(false);
    textureMapper1.setVisible(true);
    invalidate();
}

void AxisCard::handleTickEvent()
{
    if (!flippingIn && !flippingOut)
    {
        return;
    }
    if (initDelay > 0)
    {
        initDelay--;
        return;
    }

    const float xAngle = textureMapper1.getXAngle();
    if (flippingIn)
    {
        if (xAngle < 0.0f)
        {
            textureMapper1.invalidateContent();
            textureMapper1.setXAngle(xAngle + rotationSpeed);
            textureMapper1.invalidateContent();
        }
        else
        {
            flippingIn = false;
            showFace();
        }
    }
    else if (flippingOut)
    {
        if (xAngle > -HALF_PI)
        {
            textureMapper1.invalidateContent();
            textureMapper1.setXAngle(xAngle - rotationSpeed);
            textureMapper1.invalidateContent();
        }
        else
        {
            flippingOut = false;
            if (cycleBack)
            {
                cycleBack = false;
                flippingIn = true;
            }
        }
    }
}

void AxisCard::handleClickEvent(const touchgfx::ClickEvent& event)
{
    if (event.getType() == touchgfx::ClickEvent::RELEASED)
    {
        flipCycle(CLICK_FLIP_DURATION);
        if (clickAction && clickAction->isValid())
        {
            clickAction->execute(*this);
        }
    }
}

void AxisCard::startupAnimation(uint16_t duration, uint16_t sequenceNumber)
{
    initDelay = static_cast<uint16_t>(sequenceNumber * duration);
    rotationSpeed = HALF_PI / (duration / 2.0f);
    textureMapper1.setAngles(-HALF_PI, 0.0f, 0.0f);
    flippingIn = true;
    flippingOut = false;
    cycleBack = false;
    hideFace();
}

void AxisCard::flipCycle(uint16_t duration)
{
    if (flippingIn || flippingOut)
    {
        return;  // already animating
    }
    initDelay = 0;
    rotationSpeed = HALF_PI / (duration / 2.0f);
    textureMapper1.setAngles(0.0f, 0.0f, 0.0f);
    flippingOut = true;
    cycleBack = true;
    hideFace();
}

void AxisCard::setAxisLabel(const char* label)
{
    touchgfx::Unicode::strncpy(axisNameBuffer, label, AXIS_LABEL_SIZE);
    axisName.invalidate();
}

void AxisCard::setPosition(float value)
{
    touchgfx::Unicode::snprintfFloat(axisPositionBuffer, AXIS_POSITION_SIZE, "%.1f", value);
    cardPosition.resizeToCurrentText();
    cardPosition.invalidate();

    // Keep the "mm" unit glued just right of the (variable-width) value.
    axisUnit.invalidate();
    axisUnit.setXY(cardPosition.getX() + cardPosition.getWidth() + 6, 68);
    axisUnit.invalidate();
}
