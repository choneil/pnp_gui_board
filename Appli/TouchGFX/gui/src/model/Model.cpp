#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#ifndef SIMULATOR
extern "C" {
#include "app_filex.h"
}
#endif

namespace
{
float clampf(float value, float lo, float hi)
{
    if (value < lo) { return lo; }
    if (value > hi) { return hi; }
    return value;
}
}

Model::Model()
    : modelListener(0), posX(0.0f), posY(0.0f), posZ(0.0f)
{

}

void Model::setAxisPositions(float x, float y, float z)
{
    // Bound each axis to its travel limits.
    posX = clampf(x, 0.0f, static_cast<float>(X_TRAVEL_MM));
    posY = clampf(y, 0.0f, static_cast<float>(Y_TRAVEL_MM));
    posZ = clampf(z, 0.0f, static_cast<float>(Z_TRAVEL_MM));
    if (modelListener)
    {
        modelListener->axisPositionsUpdated(posX, posY, posZ);
    }
}

void Model::requestAxisMove(int axis, float target)
{
    // Placeholder: apply the move locally so the cards and machine views
    // follow. Send the command to the real machine here once comms exist.
    switch (axis)
    {
    case 0:
        setAxisPositions(target, posY, posZ);
        break;
    case 1:
        setAxisPositions(posX, target, posZ);
        break;
    case 2:
        setAxisPositions(posX, posY, target);
        break;
    default:
        break;
    }
}

namespace
{
// Simulator stand-in for the SD card contents (board: FileX storage service).
struct StubFile { const char* name; uint32_t size; };
const StubFile STUB_FILES[] =
{
    { "CAL_GRID.GCODE",   18432 },
    { "PASTE_TEST.GCODE",  7210 },
    { "PANEL_A_REV3.GCODE", 154880 },
};
const int STUB_COUNT = sizeof(STUB_FILES) / sizeof(STUB_FILES[0]);
}

int Model::getStorageState() const
{
#ifndef SIMULATOR
    return (int)storage_get_state();
#else
    return 1;
#endif
}

int Model::getFileCount() const
{
#ifndef SIMULATOR
    return (int)storage_get_file_count();
#else
    return STUB_COUNT;
#endif
}

const char* Model::getFileName(int i) const
{
#ifndef SIMULATOR
    return storage_get_file_name((UINT)i);
#else
    return (i >= 0 && i < STUB_COUNT) ? STUB_FILES[i].name : "";
#endif
}

uint32_t Model::getFileSize(int i) const
{
#ifndef SIMULATOR
    return (uint32_t)storage_get_file_size((UINT)i);
#else
    return (i >= 0 && i < STUB_COUNT) ? STUB_FILES[i].size : 0;
#endif
}

void Model::tick()
{
    // No periodic simulation. Call setAxisPositions(x, y, z) from your real
    // position source (serial/CAN/etc.) to update the axis cards; they stay at
    // their last set value (0.0 initially) until then.
}
