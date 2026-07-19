#include <gui/files_screen/FilesView.hpp>
#include <gui/files_screen/FilesPresenter.hpp>

FilesPresenter::FilesPresenter(FilesView& v)
    : view(v)
{
}

void FilesPresenter::activate() {}
void FilesPresenter::deactivate() {}

int FilesPresenter::getStorageState() const { return model->getStorageState(); }
int FilesPresenter::getFileCount() const    { return model->getFileCount(); }
const char* FilesPresenter::getFileName(int i) const { return model->getFileName(i); }
uint32_t FilesPresenter::getFileSize(int i) const    { return model->getFileSize(i); }
void FilesPresenter::requestRescan()                 { model->requestFileRescan(); }
