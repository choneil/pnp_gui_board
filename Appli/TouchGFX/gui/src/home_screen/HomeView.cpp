#include <gui/home_screen/HomeView.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/Application.hpp>

HomeView::HomeView()
    : cardCallback(this, &HomeView::onCard)
{
}

void HomeView::setupScreen()
{
    HomeViewBase::setupScreen();
    navCardMachine.setLabel("MACHINE");
    navCardMachine.setClickAction(cardCallback);
    navCardLoadFile.setLabel("LOAD FILE");
    navCardLoadFile.setClickAction(cardCallback);
    navCardCamera.setLabel("CAMERA");
    navCardCamera.setClickAction(cardCallback);
}

void HomeView::tearDownScreen()
{
    HomeViewBase::tearDownScreen();
}

// 3. Update the signature to accept the event
void HomeView::onCard(const NavCard& card, const touchgfx::ClickEvent& event)
{
    // 4. Ensure the action only fires when you release the button
    if (event.getType() == touchgfx::ClickEvent::RELEASED)
    {
        FrontendApplication* app = static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
        if (&card == &navCardMachine)
        {
            app->requestMachine();
        }
        else if(&card == &navCardLoadFile)
        {
            app->requestFiles();
        }
        else
        {
            app->requestCamera();
        }
    }
}