#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include "app.h"
#include <vector>
#include "templates.h"
#include <Wingdi.h>
#include "GL/glew.h"



typedef struct {
	char *prefix;
	HWND window;
} window_with_prefix_data_t;
static bool captured = false;

CPtrArray m_apWindowHandles;


int SetDCPixelFormat(HDC hdc)
{
	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),	// Size of this structure
		1,				// Version number
		PFD_DRAW_TO_WINDOW |		// Flags
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,			// RGBA pixel values
		24,				// 24-bit color
		0, 0, 0, 0, 0, 0,			// Don't care about these
		0, 0,				// No alpha buffer
		0, 0, 0, 0, 0,			// No accumulation buffer
		32,				// 32-bit depth buffer
		0,				// No stencil buffer
		0,				// No auxiliary buffers
		PFD_MAIN_PLANE,			// Layer type
		0,				// Reserved (must be 0)
		0, 0, 0				// No layer masks
	};

	int nPixelFormat;

	nPixelFormat = ChoosePixelFormat(hdc, &pfd);
	if (SetPixelFormat(hdc, nPixelFormat, &pfd) == FALSE){
		// SetPixelFormat error
		return FALSE;
	}

	if (DescribePixelFormat(hdc, nPixelFormat,
		sizeof(PIXELFORMATDESCRIPTOR), &pfd) == 0) {
		// DescribePixelFormat error
		return FALSE;
	}

	if (pfd.dwFlags & PFD_NEED_PALETTE) {
		// Need palete !
	}
	return TRUE;
}

bool save_screenshot(HWND mhWnd)
{
	HDC mhDC = GetDC(mhWnd);


	// set the pixel format for the DC
	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;
	int format = ChoosePixelFormat(mhDC, &pfd);
	SetPixelFormat(mhDC, format, &pfd);

	// create the render context (RC)
	HGLRC mhRC = wglCreateContext(mhDC);

	// make it the current render context
	wglMakeCurrent(mhDC, mhRC);

	int h = 480;
	int w = 900;
	//This prevents the images getting padded 
	// when the width multiplied by 3 is not a multiple of 4
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	int nSize = w*h * 3;
	// First let's create our buffer, 3 channels per Pixel
	char* dataBuffer = (char*)malloc(nSize*sizeof(char));

	if (!dataBuffer) return false;

	// Let's fetch them from the backbuffer	
	// We request the pixels in GL_BGR format, thanks to Berzeger for the tip
	glReadPixels((GLint)0, (GLint)0,
		(GLint)w, (GLint)h,
		GL_BGR, GL_UNSIGNED_BYTE, dataBuffer);

	//Now the file creation
	FILE *filePtr = fopen("screenshot.tga", "wb");
	if (!filePtr) return false;


	unsigned char TGAheader[12] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned char header[6] = { w % 256, w / 256,
		h % 256, h / 256,
		24, 0 };
	// We write the headers
	fwrite(TGAheader, sizeof(unsigned char), 12, filePtr);
	fwrite(header, sizeof(unsigned char), 6, filePtr);
	// And finally our image data
	fwrite(dataBuffer, sizeof(GLubyte), nSize, filePtr);
	fclose(filePtr);

	free(dataBuffer);

	return true;
}

int CaptureAnImage(HWND hWnd)
{
	HDC hdcScreen;
	HDC hdcMemDC = NULL;
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;

	// Retrieve the handle to a display device context for the client 
	// area of the window. 
	hdcScreen = GetDC(NULL);
	//hdcWindow = GetDC(hWnd);

	//hdcScreen = GetDC(hWnd);
//	SetDCPixelFormat(hdcScreen);

//	HGLRC hRC = wglCreateContext(hdcScreen);
//	wglMakeCurrent(hdcScreen, hRC);

	
	
		// Get the client area for size calculation
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);


	// Create a compatible DC which is used in a BitBlt from the window DC
	hdcMemDC = CreateCompatibleDC(hdcScreen);

	if (!hdcMemDC)
	{
		MessageBox(hWnd, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
		goto done;
	}


	//This is the best stretch mode
	//SetStretchBltMode(hdcWindow, HALFTONE);

	//The source DC is the entire screen and the destination DC is the current window (HWND)
	/*if (!StretchBlt(hdcWindow,
		0, 0,
		rcClient.right, rcClient.bottom,
		hdcScreen,
		0, 0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		SRCCOPY))
	{
		MessageBox(hWnd, L"StretchBlt has failed", L"Failed", MB_OK);
		goto done;
	}*/
	
	// Create a compatible bitmap from the Window DC
	hbmScreen = CreateCompatibleBitmap(hdcScreen, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

	if (!hbmScreen)
	{
		MessageBox(hWnd, L"CreateCompatibleBitmap Failed", L"Failed", MB_OK);
		goto done;
	}

	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC, hbmScreen);

	// Bit block transfer into our compatible memory DC.
	if (!BitBlt(hdcMemDC,
		0, 0,
		rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
		hdcScreen,
		0, 0,
		SRCCOPY))
	{
		MessageBox(hWnd, L"BitBlt has failed", L"Failed", MB_OK);
		goto done;
	}

	// Get the BITMAP from the HBITMAP
	GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

	BITMAPFILEHEADER   bmfHeader;
	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmpScreen.bmWidth;
	bi.biHeight = bmpScreen.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	//DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
	DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 3 * bmpScreen.bmHeight;

	// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
	// call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
	// have greater overhead than HeapAlloc.
	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	unsigned char *lpbitmap = (unsigned char *)GlobalLock(hDIB);

	// Gets the "bits" from the bitmap and copies them into a buffer 
	// which is pointed to by lpbitmap.
	GetDIBits(hdcScreen, hbmScreen, 0,
		(UINT)bmpScreen.bmHeight,
		lpbitmap,
		(BITMAPINFO *)&bi, DIB_RGB_COLORS);

glReadBuffer(GL_BACK);
glReadPixels(0, 0, bmpScreen.bmWidth, bmpScreen.bmHeight, GL_RGB, GL_UNSIGNED_BYTE, lpbitmap);

//dwBmpSize = 3 * bmpScreen.bmWidth* bmpScreen.bmHeight;
	// A file is created, this is where we will save the screen capture.
	HANDLE hFile = CreateFile(L"captureqwsx.bmp",
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	//Offset to where the actual bitmap bits start.
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	//Size of the file
	bmfHeader.bfSize = dwSizeofDIB;

	//bfType must always be BM for Bitmaps
	bmfHeader.bfType = 0x4D42; //BM   

	DWORD dwBytesWritten = 0;
	WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);
	//WriteFile(hFile, (LPSTR)Pixels, dwBmpSize, &dwBytesWritten, NULL);

	//Unlock and Free the DIB from the heap
	GlobalUnlock(hDIB);
	GlobalFree(hDIB);

	//Close the handle for the file that was created
	CloseHandle(hFile);

	//Clean up
done:
	DeleteObject(hbmScreen);
	DeleteObject(hdcMemDC);
	ReleaseDC(NULL, hdcScreen);
//	ReleaseDC(hWnd, hdcWindow);

	return 0;
}
/*class CEnumWindows
{
public:
	CEnumWindows(HWND hWndParent);
	virtual ~CEnumWindows();

	HWND Window(int nIndex) const { return (HWND)m_apWindowHandles.GetAt(nIndex); }
	int Count() const { return m_apWindowHandles.GetSize(); }

private:
	BOOL CALLBACK TheEnumProc(HWND hWnd, LPARAM lParam);

	
};

BOOL CALLBACK CEnumWindows::TheEnumProc(HWND hWnd, LPARAM lParam)
{
	CEnumWindows *pEnumWindows = (CEnumWindows *)lParam;
	pEnumWindows->m_apWindowHandles.Add((void *)hWnd);
	return TRUE;
}

// in.cpp
CEnumWindows::CEnumWindows(HWND hWndParent)
{
	EnumChildWindows(hWndParent, TheEnumProc, (LPARAM)&this);
}

CEnumWindows::~CEnumWindows()
{
}
*/

BOOL CALLBACK andyGetChildwindow(HWND hwnd, LPARAM lParam)
{
	m_apWindowHandles.Add((void *)hwnd);

	return TRUE;
}


BOOL CALLBACK window_with_prefix_callback(HWND window, LPARAM lParam) {
	window_with_prefix_data_t *find = (window_with_prefix_data_t *)lParam;

	char title[80];
	GetWindowTextA(window, title, sizeof(title));

	if (!find->window && strncmp(find->prefix, title, strlen(find->prefix)) == 0) {
		find->window = window;
	}
	return TRUE;
}

HWND window_with_prefix(char *title_prefix) {
	window_with_prefix_data_t find = { title_prefix, NULL };
	EnumWindows(window_with_prefix_callback, (LPARAM)&find);

	char title[80];
	GetWindowTextA(find.window, title, sizeof(title));
	if (strncmp("Andy 46.14.616", title, strlen(find.prefix)) == 0)
	{ //this window is andy
		HWND main_andy_window = find.window;
		EnumChildWindows(main_andy_window, andyGetChildwindow, (LPARAM)&find);
		
		for (int nWindow = 0; nWindow < m_apWindowHandles.GetSize(); nWindow++)
		{
			HWND hWndWindow = (HWND)m_apWindowHandles.GetAt(nWindow); 
			char title[80];
			GetWindowTextA(hWndWindow, title, sizeof(title));
			//::MessageBoxA(NULL, title, "", MB_OK);
			if (strncmp("sub", title, strlen("sub")) == 0)
			{
				//find.window = hWndWindow;
				//fishfish
				
			}
			// do whatever
		}
	}
	//CaptureAnImage(find.window);
	//save_screenshot(find.window);
	return find.window;
}

void exit_usage(char *self_name) {
	printf(
		"Usage: %s [options] <window name>\n\n"

		"Options:\n"
		"	-b bitrate in kilobit/s (default: estimated by output size)\n"
		"	-s output size as WxH. E.g: -s 640x480 (default: same as window size)\n"
		"	-f target framerate (default: 60)\n"
		"	-p port (default: 8080)\n\n"

		"Use \"desktop\" as the window name to capture the whole Desktop. Use \"cursor\"\n"
		"to capture the window at the current cursor position.\n\n"

		"To enable mouse lock in the browser (useful for games that require relative\n"
		"mouse movements, not absolute ones), append \"?mouselock\" at the target URL.\n"
		"i.e: http://<server-ip>:8080/?mouselock\n\n",		
		self_name
	);
	exit(0);
}

int main(int argc, char* argv[]) {
	if( argc < 2 ) {
		exit_usage(argv[0]);
	}

	int bit_rate = 0,
		fps = 60,
		port = 8080,
		width = 0,
		height = 0;

	// Parse command line options
	for( int i = 1; i < argc-1; i+=2 ) {
		if( strlen(argv[i]) < 2 || i >= argc-2 || argv[i][0] != '-' ) {
			exit_usage(argv[0]);
		}

		switch( argv[i][1] ) {
			case 'b': bit_rate = atoi(argv[i+1]) * 1000; break;
			case 'p': port = atoi(argv[i+1]); break;
			case 's': sscanf(argv[i+1], "%dx%d", &width, &height); break;
			case 'f': fps = atoi(argv[i+1]); break;
			default: exit_usage(argv[0]);
		}
	}

	// Find target window
	char *window_title = argv[argc-1];
	HWND window = NULL;
	if( strcmp(window_title, "desktop") == 0 ) {
		window = GetDesktopWindow();
	}
	else if( strcmp(window_title, "cursor") == 0 ) {
		POINT cursor;
		GetCursorPos(&cursor);
		window = WindowFromPoint(cursor);
	}
	else {
		window = window_with_prefix(window_title);
	}

	if( !window ) {
		printf("No window with title starting with \"%s\"\n", window_title);
		return 0;
	}

	// Start the app
	app_t *app = app_create(window, port, bit_rate, width, height);

	if( !app ) {
		return 1;
	}

	char real_window_title[56];
	GetWindowTextA(window, real_window_title, sizeof(real_window_title));
	printf(
		"Window 0x%08x: \"%s\"\n"
		"Window size: %dx%d, output size: %dx%d, bit rate: %d kb/s\n\n"
		"Server started on: http://%s:%d/\n\n",
		window, real_window_title,
		app->grabber->width, app->grabber->height,
		app->encoder->out_width, app->encoder->out_height,
		app->encoder->context->bit_rate / 1000,
		server_get_host_address(app->server), app->server->port
	);

	app_run(app, fps);

	app_destroy(app);	

	return 0;
}

