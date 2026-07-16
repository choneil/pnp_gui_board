#include <gui/main_screen/MainView.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/Application.hpp>
#include <touchgfx/EasingEquations.hpp>
#include <BitmapDatabase.hpp>

namespace
{
const int16_t CARD_COLUMN_X = 495;   // left edge of the card column
const int16_t PANEL_HIDDEN_X = 800;  // parked off-screen right
const uint16_t PANEL_SLIDE_TICKS = 18;
}

MainView::MainView()
    : activeAxis(-1),
      cardClickedCallback(this, &MainView::onCardClicked),
      jogCallback(this, &MainView::onJog),
      jogDoneCallback(this, &MainView::onJogDone),
      show3D(false),
      navBtnCallback(this, &MainView::onNavButton)
{
    lastPos[0] = lastPos[1] = lastPos[2] = 0.0f;
}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    // The SVG containers are retired; the machine views are Blender-rendered
    // sprite stacks (renders/gui2d + gui3d, see the manifest json files).
    topView1.setVisible(false);
    sideView1.setVisible(false);
    axisCard1.setVisible(false);

    // Top view: machine X moves down the screen, Y moves right, Z invisible.
    const uint16_t topIds[6] = { BITMAP_M2T_STATIC_ID, BITMAP_M2T_GANTRY_ID,
                                 BITMAP_M2T_CARRIAGE_ID, BITMAP_M2T_TOOLHEAD_ID,
                                 BITMAP_M2T_EXTRUDER_ID, BITMAP_M2T_PROBE_ID };
    viewTop.setLayers(topIds);
    viewTop.setMotionVectors(0.0f, 0.613f, 0.613f, 0.0f, 0.0f, 0.0f);
    viewTop.init();
    add(viewTop);

    // Front view: Y moves right, Z moves down, X is along the view axis.
    const uint16_t frontIds[6] = { BITMAP_M2F_STATIC_ID, BITMAP_M2F_GANTRY_ID,
                                   BITMAP_M2F_CARRIAGE_ID, BITMAP_M2F_TOOLHEAD_ID,
                                   BITMAP_M2F_EXTRUDER_ID, BITMAP_M2F_PROBE_ID };
    viewFront.setLayers(frontIds);
    viewFront.setMotionVectors(0.0f, 0.0f, 0.613f, 0.0f, 0.0f, 0.613f);
    viewFront.init();
    add(viewFront);

    const int16_t viewX = static_cast<int16_t>((CARD_COLUMN_X - viewTop.getWidth()) / 2);
    viewTop.setXY(viewX, 0);
    viewFront.setXY(viewX, static_cast<int16_t>(viewTop.getHeight() + 2));

    // 3D view (front-right ortho sprites).
    machine3D.setXY(static_cast<int16_t>((CARD_COLUMN_X - Machine3DView::SPRITE_SIZE) / 2), 0);
    add(machine3D);
    machine3D.init();

    // X/Y/Z axis cards on the right.
    const int16_t pitch = 155;
    AxisCard* cards[3]   = { &axisCardX, &axisCardY, &axisCardZ };
    const char* labels[3] = { "X", "Y", "Z" };
    for (int i = 0; i < 3; i++)
    {
        cards[i]->initialize();
        cards[i]->setXY(CARD_COLUMN_X, static_cast<int16_t>(i * pitch));
        cards[i]->setAxisLabel(labels[i]);
        cards[i]->setPosition(0.0f);
        cards[i]->setClickAction(cardClickedCallback);
        add(*cards[i]);
        cards[i]->startupAnimation(20, static_cast<uint16_t>(i));
    }

    // Designer NavButtons: 2D/3D toggle, EXT/PRB tool toggle, BACK. Re-add on
    // top of the code-added widgets, then label them.
    NavButton* navs[3] = { &navBtnView, &navBtnTool, &navBtnBack };
    for (int i = 0; i < 3; i++)
    {
        remove(*navs[i]);
        add(*navs[i]);
        navs[i]->setClickAction(navBtnCallback);
    }
    applyViewMode();

    // Jog panel: parked off-screen right, slides in over the card column.
    jogPanel.init();
    jogPanel.setXY(PANEL_HIDDEN_X, 0);
    jogPanel.setJogAction(jogCallback);
    jogPanel.setDoneAction(jogDoneCallback);
    add(jogPanel);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::onNavButton(const NavButton& button)
{
    if (&button == &navBtnView)
    {
        show3D = !show3D;
        applyViewMode();
    }
    else if (&button == &navBtnTool)
    {
        const bool probe = !machine3D.isProbeSelected();
        machine3D.setProbeSelected(probe);
        viewTop.setProbeSelected(probe);
        viewFront.setProbeSelected(probe);
        applyViewMode();
    }
    else
    {
        static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->requestHome();
    }
}

void MainView::applyViewMode()
{
    viewTop.setVisible(!show3D);
    viewFront.setVisible(!show3D);
    machine3D.setVisible(show3D);

    // Buttons show what you switch TO next.
    navBtnView.setLabel(show3D ? "2D" : "3D");
    navBtnTool.setLabel(machine3D.isProbeSelected() ? "EXT" : "PRB");
    navBtnBack.setLabel("BACK");

    viewTop.invalidate();
    viewFront.invalidate();
    machine3D.invalidate();
}

void MainView::onCardClicked(const AxisCard& card)
{
    if (activeAxis >= 0)
    {
        return;  // panel already open
    }
    activeAxis = (&card == &axisCardX) ? 0 : (&card == &axisCardY) ? 1 : 2;

    const char* labels[3] = { "X", "Y", "Z" };
    jogPanel.setAxisLabel(labels[activeAxis]);
    jogPanel.setPosition(lastPos[activeAxis]);
    jogPanel.startMoveAnimation(CARD_COLUMN_X, 0, PANEL_SLIDE_TICKS,
                                touchgfx::EasingEquations::cubicEaseOut,
                                touchgfx::EasingEquations::cubicEaseOut);
}

void MainView::onJog(float delta)
{
    if (activeAxis < 0)
    {
        return;
    }
    presenter->requestAxisMove(activeAxis, lastPos[activeAxis] + delta);
}

void MainView::onJogDone()
{
    activeAxis = -1;
    jogPanel.startMoveAnimation(PANEL_HIDDEN_X, 0, PANEL_SLIDE_TICKS,
                                touchgfx::EasingEquations::cubicEaseIn,
                                touchgfx::EasingEquations::cubicEaseIn);
}

void MainView::updateAxisPositions(float x, float y, float z)
{
    lastPos[0] = x;
    lastPos[1] = y;
    lastPos[2] = z;

    axisCardX.setPosition(x);
    axisCardY.setPosition(y);
    axisCardZ.setPosition(z);

    if (activeAxis >= 0)
    {
        jogPanel.setPosition(lastPos[activeAxis]);
    }

    viewTop.setAxisPositions(x, y, z);
    viewFront.setAxisPositions(x, y, z);
    machine3D.setAxisPositions(x, y, z);
}
