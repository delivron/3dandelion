#include "window.h"

int main()
{
    ddn::Window window(L"3Dandelion", 800, 600);
    window.Show();

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
