#include <gui/common/FrontendApplication.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{

}

#include <touchgfx/transitions/SlideTransition.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/home_screen/HomeView.hpp>
#include <gui/home_screen/HomePresenter.hpp>
#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <gui/files_screen/FilesView.hpp>
#include <gui/files_screen/FilesPresenter.hpp>

void FrontendApplication::requestHome()
{
    navCallbackFA = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoHomeImpl);
    pendingScreenTransitionCallback = &navCallbackFA;
}
void FrontendApplication::gotoHomeImpl()
{
    touchgfx::makeTransition<HomeView, HomePresenter, touchgfx::SlideTransition<touchgfx::EAST>, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::requestMachine()
{
    navCallbackFA = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoMachineImpl);
    pendingScreenTransitionCallback = &navCallbackFA;
}
void FrontendApplication::gotoMachineImpl()
{
    touchgfx::makeTransition<MainView, MainPresenter, touchgfx::SlideTransition<touchgfx::WEST>, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::requestFiles()
{
    navCallbackFA = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoFilesImpl);
    pendingScreenTransitionCallback = &navCallbackFA;
}
void FrontendApplication::gotoFilesImpl()
{
    touchgfx::makeTransition<FilesView, FilesPresenter, touchgfx::SlideTransition<touchgfx::WEST>, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::requestCamera()
{
    navCallbackFA = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoCameraImpl);
    pendingScreenTransitionCallback = &navCallbackFA;
}

void FrontendApplication::gotoCameraImpl()
{
    touchgfx::makeTransition<CameraView, CameraPresenter, touchgfx::SlideTransition<touchgfx::WEST>, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}
