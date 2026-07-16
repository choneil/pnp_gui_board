#include <gui/containers/FileListPanel.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>

FileListPanel::FileListPanel()
    : nextRow(0)
{
}

void FileListPanel::init(int16_t width, int16_t height)
{
    setWidthHeight(width, height);

    // Transparent over the screen's light-theme background (HVAC motif); the
    // rows are white card strips with deep-blue text.
    (void)background;


    for (int i = 0; i < MAX_ROWS; i++)
    {
        rowBuffers[i][0] = 0;
        rowCards[i].setPosition(24, static_cast<int16_t>(66 + i * (ROW_HEIGHT + 6)),
                                static_cast<int16_t>(width - 48), ROW_HEIGHT);
        rowCards[i].setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
        rowCards[i].setVisible(false);
        add(rowCards[i]);
        rows[i].setTypedText(touchgfx::TypedText(T_FILEROW));
        rows[i].setWildcard(rowBuffers[i]);
        rows[i].setColor(touchgfx::Color::getColorFromRGB(0, 37, 128));
        rows[i].setPosition(40, static_cast<int16_t>(66 + i * (ROW_HEIGHT + 6) + 7),
                            static_cast<int16_t>(width - 80), ROW_HEIGHT);
        rows[i].setVisible(false);
        add(rows[i]);
    }

}

void FileListPanel::showStatus(const char* message)
{
    beginRows();
    addRow(message, 0);
    endRows();
}

void FileListPanel::beginRows()
{
    nextRow = 0;
}

void FileListPanel::addRow(const char* name, uint32_t sizeBytes)
{
    if (nextRow >= MAX_ROWS)
    {
        return;
    }
    touchgfx::Unicode::strncpy(rowBuffers[nextRow], name, ROW_BUF - 1);
    rowBuffers[nextRow][ROW_BUF - 1] = 0;
    if (sizeBytes > 0)
    {
        const uint16_t len = touchgfx::Unicode::strlen(rowBuffers[nextRow]);
        if (len + 12 < ROW_BUF)
        {
            if (sizeBytes >= 1024u)
            {
                touchgfx::Unicode::snprintf(&rowBuffers[nextRow][len], ROW_BUF - len,
                                            "   %d KB", static_cast<int>(sizeBytes / 1024u));
            }
            else
            {
                touchgfx::Unicode::snprintf(&rowBuffers[nextRow][len], ROW_BUF - len,
                                            "   %d B", static_cast<int>(sizeBytes));
            }
        }
    }
    rowCards[nextRow].setVisible(true);
    rowCards[nextRow].invalidate();
    rows[nextRow].setVisible(true);
    rows[nextRow].invalidate();
    nextRow++;
}

void FileListPanel::endRows()
{
    for (int i = nextRow; i < MAX_ROWS; i++)
    {
        rowCards[i].setVisible(false);
        rowCards[i].invalidate();
        rows[i].setVisible(false);
        rows[i].invalidate();
    }
    invalidate();
}
