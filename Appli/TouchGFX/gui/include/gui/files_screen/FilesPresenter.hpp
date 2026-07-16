#ifndef FILESPRESENTER_HPP
#define FILESPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class FilesView;

class FilesPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    FilesPresenter(FilesView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~FilesPresenter() {}

    /** SD file list passthroughs. */
    int getStorageState() const;
    int getFileCount() const;
    const char* getFileName(int i) const;
    uint32_t getFileSize(int i) const;

private:
    FilesPresenter();
    FilesView& view;
};

#endif // FILESPRESENTER_HPP
