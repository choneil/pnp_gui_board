#ifndef FILESVIEW_HPP
#define FILESVIEW_HPP

#include <gui_generated/files_screen/FilesViewBase.hpp>
#include <gui/files_screen/FilesPresenter.hpp>
#include <gui/containers/FileListPanel.hpp>
#include <touchgfx/Callback.hpp>

class FilesView : public FilesViewBase
{
public:
    FilesView();
    virtual ~FilesView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

protected:
    FileListPanel filePanel;
    touchgfx::Callback<FilesView, const NavButton&> backCallback;
    void onBack(const NavButton& button);
    void refreshFileList();
};

#endif // FILESVIEW_HPP
