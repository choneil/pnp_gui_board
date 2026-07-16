#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>

MainPresenter::MainPresenter(MainView& v)
    : view(v)
{

}

void MainPresenter::activate()
{

}

void MainPresenter::deactivate()
{

}

void MainPresenter::axisPositionsUpdated(float x, float y, float z)
{
    view.updateAxisPositions(x, y, z);
}

void MainPresenter::requestAxisMove(int axis, float target)
{
    model->requestAxisMove(axis, target);
}

int MainPresenter::getStorageState() const { return model->getStorageState(); }
int MainPresenter::getFileCount() const    { return model->getFileCount(); }
const char* MainPresenter::getFileName(int i) const { return model->getFileName(i); }
uint32_t MainPresenter::getFileSize(int i) const    { return model->getFileSize(i); }
