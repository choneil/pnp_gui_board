#ifndef MACHINE3DVIEW_HPP
#define MACHINE3DVIEW_HPP

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/widgets/TextureMapper.hpp>

/**
 * Pre-rendered 3D machine view built from layered sprites (one transparent PNG
 * per movable assembly, all from the same front-right orthographic camera in
 * Blender). The GUI shifts each layer by the axis position to make a live
 * digital twin. The extruder and probe occupy the same toolhead slot and are
 * shown one at a time (tool selection).
 *
 * Layer stack (back -> front): static, gantry (X), carriage (X+Y),
 * toolhead Z-drive (X+Y+Z), tool (X+Y+Z), then two foreground masks that fake
 * depth occlusion: static_fg (the near-side rail + leadscrew, drawn over the
 * movers so they tuck behind it) and gantry_fg (the near Y bracket + orange
 * nut, WITH a short stub of the leadscrew baked into the sprite). The stub is
 * rendered together with the nut in Blender, so the through-hole occlusion is
 * pixel-exact; because the nut travels exactly along the rod's screen
 * direction, the stub stays seamlessly aligned with the static rod beneath it
 * at every position (the stub's edge is feathered into an identical render).
 *
 * The moving layers are TextureMappers so they can be offset by a *sub-pixel*
 * amount (setBitmapPosition + bilinear sampling). On the STM32N6 the NeoChrom
 * GPU does this in hardware, which removes the whole-pixel staircase you get
 * when translating a plain Image along a shallow diagonal.
 *
 * Pixel shift per mm of travel is measured from the render (renders/gui3d/
 * manifest.json). Sprites are authored at 480x480; place the container at that
 * size for a 1:1 mapping.
 */
class Machine3DView : public touchgfx::Container
{
public:
    Machine3DView();
    virtual ~Machine3DView() {}

    /** Native (authored) sprite size of the default 3D view, in pixels. */
    static const int16_t SPRITE_SIZE = 480;

    /**
     * Rebinds all six layers to another sprite set (order: static, gantry,
     * carriage, toolhead, extruder, probe) and resizes the container to the
     * static sprite's dimensions. Used to reuse this widget for the Blender-
     * rendered 2D top/front views.
     */
    void setLayers(const uint16_t bitmapIds[6]);

    /**
     * Sets the on-screen pixel shift per mm of travel for each machine axis.
     * Axes that don't move on screen in a given view (Z in the top view, X in
     * the front view) use (0, 0).
     */
    void setMotionVectors(float vxX, float vxY, float vyX, float vyY, float vzX, float vzY);

    /** Registers the smoothing tick; call once from the owning view's setup. */
    void init();

    virtual void handleTickEvent();

    /**
     * Commands the target machine pose, in millimetres. x = gantry along the X
     * rails, y = carriage along the beam, z = tool down. The view eases toward
     * this target over a few ticks so diagonal motion glides instead of hopping.
     */
    void setAxisPositions(float x, float y, float z);

    /** Selects the active tool: true = touch probe, false = paste extruder. */
    void setProbeSelected(bool probe);

    /** @return true if the touch probe (not the extruder) is selected. */
    bool isProbeSelected() const { return probeSelected; }

protected:
    touchgfx::Image imgStatic;              // never moves -> plain blit
    touchgfx::TextureMapper mtGantry;       // moving layers -> sub-pixel offset
    touchgfx::TextureMapper mtCarriage;
    touchgfx::TextureMapper mtToolhead;
    touchgfx::TextureMapper mtExtruder;
    touchgfx::TextureMapper mtProbe;
    touchgfx::Image imgStaticFg;            // near rail + leadscrew occluder
    touchgfx::TextureMapper mtGantryFg;     // near Y bracket + nut (rides X)

    bool probeSelected;
    float vX[2];  // px shift per mm of machine X (gantry)
    float vY[2];  // px shift per mm of machine Y (carriage)
    float vZ[2];  // px shift per mm of machine Z (tool)
    float tgtX;   // commanded target position (mm)
    float tgtY;
    float tgtZ;
    float curX;   // eased, currently-displayed position (mm)
    float curY;
    float curZ;
    bool animating;

    void setupLayer(touchgfx::TextureMapper& tm, uint16_t bitmapId);
    void applyAxisMotion();
};

#endif // MACHINE3DVIEW_HPP
