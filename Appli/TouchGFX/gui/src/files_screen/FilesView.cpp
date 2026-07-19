#include <gui/files_screen/FilesView.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/Application.hpp>

FilesView::FilesView()
    : backCallback(this, &FilesView::onBack), lastState(-1), lastCount(-1)
{
}

void FilesView::setupScreen()
{
    FilesViewBase::setupScreen();
    navBtnBackF.setLabel("BACK");
    navBtnBackF.setClickAction(backCallback);

    filePanel.init(800, 480);
    add(filePanel);
    presenter->requestRescan();
    refreshFileList();

    // Keep the Designer BACK button on top of the (full screen) list panel.
    remove(navBtnBackF);
    add(navBtnBackF);
}

void FilesView::tearDownScreen()
{
    FilesViewBase::tearDownScreen();
}

void FilesView::handleTickEvent()
{
    // The FileX thread scans the card asynchronously, so poll for the result
    // instead of assuming the listing is ready when the screen opens.
    const int state = presenter->getStorageState();
    const int count = presenter->getFileCount();
    if (state != lastState || count != lastCount)
    {
        refreshFileList();
    }
}

void FilesView::refreshFileList()
{
    const int state = presenter->getStorageState();
    lastState = state;
    lastCount = presenter->getFileCount();
    if (state != 1)
    {
        const char* msg = "NO SD CARD";
        if (state == 2)      { msg = "CARD IN USE BY USB HOST"; }
        else if (state == 3) { msg = "SD CARD UNREADABLE"; }
        filePanel.showStatus(msg);
        return;
    }
    const int count = presenter->getFileCount();
    filePanel.beginRows();
    if (count == 0)
    {
        filePanel.addRow("NO FILES ON CARD", 0);
    }
    for (int i = 0; i < count && i < FileListPanel::MAX_ROWS; i++)
    {
        filePanel.addRow(presenter->getFileName(i), presenter->getFileSize(i));
    }
    filePanel.endRows();
}

void FilesView::onBack(const NavButton&)
{
    static_cast<FrontendApplication*>(touchgfx::Application::getInstance())->requestHome();
}
