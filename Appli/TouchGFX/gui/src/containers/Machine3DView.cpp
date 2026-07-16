#include <gui/containers/Machine3DView.hpp>
#include <BitmapDatabase.hpp>
#include <touchgfx/Application.hpp>

namespace
{
// Pixel shift on screen per mm of axis travel, from renders/gui3d/manifest.json
// (front-right orthographic camera). Verified against the running sim: machine
// X travels along world Y, machine Y along world X, and Z points down as it
// increases (extruder lowers). Carriage (Y) direction is negated to match the
// real machine.
const float VX_X = -0.332f, VX_Y =  0.188f;   // gantry (machine X) -> world Y
const float VY_X =  0.461f, VY_Y =  0.135f;   // carriage (machine Y) -> -world X
const float VZ_X =  0.000f, VZ_Y =  0.519f;   // tool down (machine Z), +Z lowers

// Per-axis visual gain: each axis contacts its endstop before the full GUI
// stroke in the model, so compress the command onto the drawn span (mirrors the
// 2D views' X_MM_TO_DRAWING / Y_MM_TO_DRAWING). Clamps stay at 380/500/45.
const float X_GAIN = 290.0f / 380.0f;
const float Y_GAIN = 370.0f / 500.0f;
const float Z_GAIN = 1.0f;

// Fraction of the remaining distance covered each tick (ease-out glide) and the
// mm threshold below which the target counts as reached.
const float EASE = 0.30f;
const float SNAP_MM = 0.02f;
}

Machine3DView::Machine3DView()
    : probeSelected(false),
      tgtX(0.0f), tgtY(0.0f), tgtZ(0.0f),
      curX(0.0f), curY(0.0f), curZ(0.0f),
      animating(false)
{
    // Default motion vectors: the front-right 3D view.
    vX[0] = VX_X; vX[1] = VX_Y;
    vY[0] = VY_X; vY[1] = VY_Y;
    vZ[0] = VZ_X; vZ[1] = VZ_Y;

    setWidthHeight(SPRITE_SIZE, SPRITE_SIZE);

    // Add in draw order (back to front). The static frame never moves, so it
    // stays a cheap plain Image; the rest are TextureMappers for sub-pixel
    // translation.
    imgStatic.setBitmap(touchgfx::Bitmap(BITMAP_M3D_STATIC_ID));
    imgStatic.setPosition(0, 0, SPRITE_SIZE, SPRITE_SIZE);
    add(imgStatic);

    setupLayer(mtGantry, BITMAP_M3D_GANTRY_ID);
    setupLayer(mtCarriage, BITMAP_M3D_CARRIAGE_ID);
    setupLayer(mtToolhead, BITMAP_M3D_TOOLHEAD_ID);
    setupLayer(mtExtruder, BITMAP_M3D_EXTRUDER_ID);
    setupLayer(mtProbe, BITMAP_M3D_PROBE_ID);

    // Foreground occluders, drawn over the movers: the near rail + leadscrew
    // mask anything dipping behind them, and the near bracket + nut (riding
    // the gantry, with a baked-in leadscrew stub) draws the through-hole.
    imgStaticFg.setBitmap(touchgfx::Bitmap(BITMAP_M3D_STATIC_FG_ID));
    imgStaticFg.setPosition(0, 0, SPRITE_SIZE, SPRITE_SIZE);
    add(imgStaticFg);
    setupLayer(mtGantryFg, BITMAP_M3D_GANTRY_FG_ID);

    setProbeSelected(false);
    applyAxisMotion();
}

void Machine3DView::setupLayer(touchgfx::TextureMapper& tm, uint16_t bitmapId)
{
    tm.setBitmap(touchgfx::Bitmap(bitmapId));
    tm.setPosition(0, 0, SPRITE_SIZE, SPRITE_SIZE);
    // Identity transform (no rotation/scale); we only translate the bitmap, at
    // sub-pixel precision, with bilinear sampling.
    tm.setBitmapPosition(0.0f, 0.0f);
    tm.setRenderingAlgorithm(touchgfx::TextureMapper::BILINEAR_INTERPOLATION);
    add(tm);
}

void Machine3DView::init()
{
    touchgfx::Application::getInstance()->registerTimerWidget(this);
}

void Machine3DView::setLayers(const uint16_t bitmapIds[6])
{
    const touchgfx::Bitmap staticBmp(bitmapIds[0]);
    const int16_t w = staticBmp.getWidth();
    const int16_t h = staticBmp.getHeight();
    setWidthHeight(w, h);

    imgStatic.setBitmap(staticBmp);
    imgStatic.setPosition(0, 0, w, h);
    imgStaticFg.setVisible(false);   // fg occluders belong to the 3D set only
    mtGantryFg.setVisible(false);

    touchgfx::TextureMapper* layers[5] = { &mtGantry, &mtCarriage, &mtToolhead, &mtExtruder, &mtProbe };
    for (int i = 0; i < 5; i++)
    {
        layers[i]->setBitmap(touchgfx::Bitmap(bitmapIds[i + 1]));
        layers[i]->setPosition(0, 0, w, h);
        layers[i]->setBitmapPosition(0.0f, 0.0f);
    }
    setProbeSelected(probeSelected);
    applyAxisMotion();
}

void Machine3DView::setMotionVectors(float vxX, float vxY, float vyX, float vyY, float vzX, float vzY)
{
    vX[0] = vxX; vX[1] = vxY;
    vY[0] = vyX; vY[1] = vyY;
    vZ[0] = vzX; vZ[1] = vzY;
    applyAxisMotion();
}

void Machine3DView::setAxisPositions(float x, float y, float z)
{
    tgtX = x;
    tgtY = y;
    tgtZ = z;
    animating = true;
}

void Machine3DView::handleTickEvent()
{
    if (!animating)
    {
        return;
    }

    curX += (tgtX - curX) * EASE;
    curY += (tgtY - curY) * EASE;
    curZ += (tgtZ - curZ) * EASE;

    if ((tgtX - curX) * (tgtX - curX) + (tgtY - curY) * (tgtY - curY) +
        (tgtZ - curZ) * (tgtZ - curZ) < SNAP_MM * SNAP_MM)
    {
        curX = tgtX;
        curY = tgtY;
        curZ = tgtZ;
        animating = false;
    }

    applyAxisMotion();
}

void Machine3DView::setProbeSelected(bool probe)
{
    probeSelected = probe;
    mtExtruder.setVisible(!probe);
    mtProbe.setVisible(probe);
    mtExtruder.invalidate();
    mtProbe.invalidate();
}

void Machine3DView::applyAxisMotion()
{
    // Cumulative offsets: each stage rides the one before it. Kept as floats so
    // the TextureMappers translate at sub-pixel precision.
    const float ax = curX * X_GAIN;
    const float ay = curY * Y_GAIN;
    const float az = curZ * Z_GAIN;
    // Always sub-pixel: the layers ride at fractional offsets (bilinear
    // sampling - hardware on the NeoChrom), so small jog steps translate
    // smoothly instead of quantizing to whole-pixel hops. The worst case is a
    // half-pixel softening of a moving layer, which also applies to its baked
    // junction pixels and stays visually consistent with the statics beneath.
    const float gx = ax * vX[0];
    const float gy = ax * vX[1];
    const float cx = gx + ay * vY[0];
    const float cy = gy + ay * vY[1];
    const float tx = cx + az * vZ[0];
    const float ty = cy + az * vZ[1];

    mtGantry.invalidate();
    mtGantry.setBitmapPosition(gx, gy);
    mtGantry.invalidate();

    mtGantryFg.invalidate();
    mtGantryFg.setBitmapPosition(gx, gy);
    mtGantryFg.invalidate();

    mtCarriage.invalidate();
    mtCarriage.setBitmapPosition(cx, cy);
    mtCarriage.invalidate();

    touchgfx::TextureMapper* tool[3] = { &mtToolhead, &mtExtruder, &mtProbe };
    for (int i = 0; i < 3; i++)
    {
        tool[i]->invalidate();
        tool[i]->setBitmapPosition(tx, ty);
        tool[i]->invalidate();
    }
}
