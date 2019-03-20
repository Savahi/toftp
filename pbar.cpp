#include <windows.h>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")

HWND pbarCreate(HINSTANCE hInstance, int stepsNumber)
{
	HWND hwndPB = NULL;    // Handle of progress bar.

	InitCommonControls();

	int cyHScroll = GetSystemMetrics(SM_CYHSCROLL);
	int cxFullScreen = GetSystemMetrics(SM_CXFULLSCREEN);
	int cyFullScreen = GetSystemMetrics(SM_CYFULLSCREEN);

	int hm = 40;
	int left = cxFullScreen * hm / 100;
	int width = cxFullScreen * (100 - 2 * hm) / 100;
	int vm = 40;
	int top = cyFullScreen * vm / 100;

	hwndPB = CreateWindowEx(0, PROGRESS_CLASS, (LPTSTR)"SPIDER PROJECT", WS_VISIBLE,
		left, top, width, SM_CYSMCAPTION + cyHScroll, NULL, (HMENU)0, hInstance, NULL);

	SendMessage(hwndPB, PBM_SETRANGE, 0, MAKELPARAM(0, stepsNumber));

	SendMessage(hwndPB, PBM_SETSTEP, (WPARAM)1, 0);

	return hwndPB;
}

void pbarStep(HWND hwndPB) {
	SendMessage(hwndPB, PBM_STEPIT, 0, 0);
}

void pbarDestroy(HWND hwndPB) {
	DestroyWindow(hwndPB);
}