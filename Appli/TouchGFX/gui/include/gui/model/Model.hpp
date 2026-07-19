#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    /**
     * Sets the current axis positions and notifies the active screen. Values are
     * clamped to the endstop travel limits. Call this from wherever real machine
     * position feedback arrives.
     */
    void setAxisPositions(float x, float y, float z);

    /**
     * Requests moving one axis (0=X, 1=Y, 2=Z) to an absolute target in mm,
     * clamped to the travel limits. For now this drives the local state
     * directly (the digital twin follows instantly); replace the body with the
     * real machine move command when comms are wired up.
     */
    void requestAxisMove(int axis, float target);

    /**
     * SD-card file listing for the Load File screen. In the simulator this is
     * stubbed with sample jobs; the board build maps these to the FileX
     * storage service (storage_get_file_count/name/size in app_filex.h).
     * State: 0 = no card, 1 = mounted, 2 = handed to USB.
     */
    int getStorageState() const;
    int getFileCount() const;
    const char* getFileName(int i) const;
    uint32_t getFileSize(int i) const;

    /** Ask the storage service to re-read the card (no-op in the simulator). */
    void requestFileRescan();

    // Real axis travel limits, in mm (0 = home end).
    //   X: beam assembly along the side rails, 380mm
    //   Y: toolhead carriage along the beam, 500mm
    //   Z: extruder body/toolhead, 45mm downward from home (up)
    // (The X/Y drawings show a shorter span between home and far-endstop
    // contact; the views calibrate mm -> drawing units, see topView/frontView.)
    static const int16_t X_TRAVEL_MM = 380;
    static const int16_t Y_TRAVEL_MM = 500;
    static const int16_t Z_TRAVEL_MM = 45;

protected:
    ModelListener* modelListener;

    float posX;
    float posY;
    float posZ;
};

#endif // MODEL_HPP
