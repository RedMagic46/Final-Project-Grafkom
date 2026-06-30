#include "StartMenuApp.h"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    StartMenuApp app(hInstance);
    if (app.Initialize(nCmdShow)) {
        app.RunMessageLoop();
    }
    return 0;
}
