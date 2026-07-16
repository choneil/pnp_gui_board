#ifndef FILELISTPANEL_HPP
#define FILELISTPANEL_HPP

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextArea.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/Unicode.hpp>

/**
 * Full-screen "Load File" panel: lists the files on the SD card (root
 * directory snapshot from the storage service; stubbed in the simulator) with
 * a BACK button. Code-only container, same pattern as the jog panel.
 */
class FileListPanel : public touchgfx::Container
{
public:
    static const int MAX_ROWS = 8;

    FileListPanel();
    virtual ~FileListPanel() {}

    /** Builds the child widgets; call once from the owning view's setup. */
    void init(int16_t width, int16_t height);

    /** Clears the list and shows a status line instead (e.g. "NO SD CARD"). */
    void showStatus(const char* message);

    /** Begins a list update; follow with addRow() calls. */
    void beginRows();

    /** Adds one file row (name + size). Rows beyond MAX_ROWS are ignored. */
    void addRow(const char* name, uint32_t sizeBytes);

    /** Finishes a list update (hides unused rows). */
    void endRows();


protected:
    static const uint16_t ROW_BUF = 56;
    static const int16_t ROW_HEIGHT = 40;

    touchgfx::Box background;   // unused (transparent panel)
    touchgfx::Box rowCards[MAX_ROWS];
    touchgfx::TextAreaWithOneWildcard rows[MAX_ROWS];
    touchgfx::Unicode::UnicodeChar rowBuffers[MAX_ROWS][ROW_BUF];
    int nextRow;
};

#endif // FILELISTPANEL_HPP
