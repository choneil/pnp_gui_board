#include <gui/files_screen/FilesView.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/Application.hpp>

FilesView::FilesView()
    : backCallback(this, &FilesView::onBack)
{
}

void FilesView::setupScreen()
{
    FilesViewBase::setupScreen();
    navBtnBackF.setLabel("BACK");
    navBtnBackF.setClickAction(backCallback);

    filePanel.init(800, 480);
    add(filePanel);
    refreshFileList();

    // Keep the Designer BACK button on top of the (full screen) list panel.
    remove(navBtnBackF);
    add(navBtnBackF);
}

void FilesView::tearDownScreen()
{
    FilesViewBase::tearDownScreen();
}

void FilesView::refreshFileList()
{
    const int state = presenter->getStorageState();
    if (state != 1)
    {
        filePanel.showStatus((state == 2) ? "CARD IN USE BY USB HOST" : "NO SD CARD");
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
