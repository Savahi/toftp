#include <windows.h>

#ifndef __PBAR_H
#define __PBAR_H

HWND pbarCreate(HINSTANCE hInstance, int stepsNumber, HWND parent=0);

void pbarSetStepsNumber( HWND hwndPB, int stepsNumber );

void pbarStep(HWND hwndPB);

void pbarDestroy(HWND hwndPB);

#endif

/*
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* cmdLine, int nCmdShow)
{
	HWND h = pbarCreate( hInstance, 20 );

	MessageBox( NULL, "Ok", "Ok", MB_OK );
	for( int i = 0 ; i < 20 ; i++ ) {
		pbarStep(h);
		MessageBox( NULL, "Ok", "Ok", MB_OK );
	}

	pbarDestroy(h);
}
*/