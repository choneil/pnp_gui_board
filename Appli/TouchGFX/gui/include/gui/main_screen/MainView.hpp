#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <gui/containers/AxisCard.hpp>
#include <gui/containers/AxisJogPanel.hpp>
#include <gui/containers/Machine3DView.hpp>
#include <touchgfx/mixins/MoveAnimator.hpp>

class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /** Updates the X/Y/Z axis cards with the latest positions. */
    void updateAxisPositions(float x, float y, float z);

protected:
    AxisCard axisCardX;
    AxisCard axisCardY;
    AxisCard axisCardZ;

    // Slide-in jog panel over the card column.
    touchgfx::MoveAnimator<AxisJogPanel> jogPanel;
    int activeAxis;      // 0/1/2 = X/Y/Z while the panel is open, -1 = closed
    float lastPos[3];    // latest reported axis positions (mm)

    // Card click -> open panel; panel jog -> move request; DONE -> close.
    touchgfx::Callback<MainView, const AxisCard&> cardClickedCallback;
    touchgfx::Callback<MainView, float> jogCallback;
    touchgfx::Callback<MainView> jogDoneCallback;

    void onCardClicked(const AxisCard& card);
    void onJog(float delta);
    void onJogDone();

    // Machine visualizations: Blender-rendered sprite stacks (2D top/front)
    // and the front-right 3D view, all driven by the same widget.
    Machine3DView machine3D;
    Machine3DView viewTop;
    Machine3DView viewFront;
    bool show3D;

    // Designer NavButtons (navBtnView/navBtnTool/navBtnBack from the base).
    touchgfx::Callback<MainView, const NavButton&> navBtnCallback;
    void onNavButton(const NavButton& button);
    void applyViewMode();
};

#endif // MAINVIEW_HPP
