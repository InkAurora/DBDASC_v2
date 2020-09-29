#include <iostream>
#include <thread>
#include <future>
#include <strsafe.h>
#include <windows.h>
#include <math.h>
#include <stdlib.h>

#define ASYNC 1

using namespace std;

typedef struct {
	COLORREF allStr;
	int r;
	int g;
	int b;
} PDATA;

typedef struct {
	int x;
	int y;
} ROUND;

typedef struct {
	HDC hdc, hdcTemp;
	BYTE * bitPointer;
	int MAX_WIDTH, MAX_HEIGHT;
	BITMAPINFO bitmap;
	HBITMAP hBitmap2;
	void doMagic() {
		hdc = GetDC(NULL);
		//GetWindowRect(FindWindowA(NULL, NULL), &rect);
		MAX_WIDTH = 1600;
		MAX_HEIGHT = 900;

		bitmap.bmiHeader.biSize = sizeof(bitmap.bmiHeader);
		bitmap.bmiHeader.biWidth = MAX_WIDTH;
		bitmap.bmiHeader.biHeight = MAX_HEIGHT;
		bitmap.bmiHeader.biPlanes = 1;
		bitmap.bmiHeader.biBitCount = 32;
		bitmap.bmiHeader.biCompression = BI_RGB;
		bitmap.bmiHeader.biSizeImage = MAX_WIDTH * 4 * MAX_HEIGHT;
		bitmap.bmiHeader.biClrUsed = 0;
		bitmap.bmiHeader.biClrImportant = 0;

		hBitmap2 = CreateDIBSection(hdcTemp, &bitmap, DIB_RGB_COLORS, (void**)(&bitPointer), NULL, 0);
		hdcTemp = CreateCompatibleDC(hdc);
		SelectObject(hdcTemp, hBitmap2);
		DeleteObject(hBitmap2);

		BitBlt(hdcTemp, 0, 0, MAX_WIDTH, MAX_HEIGHT, hdc, 0, 0, SRCCOPY);
		return;
	}
	void undoMagic() {
		DeleteObject(hdcTemp);
		ReleaseDC(NULL, hdcTemp);
		DeleteObject(hdc);
		ReleaseDC(NULL, hdc);
		DeleteObject(bitPointer);
		return;
	}
	void refreshMagic() {
		BitBlt(hdcTemp, 0, 0, MAX_WIDTH, MAX_HEIGHT, hdc, 0, 0, SRCCOPY);
		return;
	}
} BMP;

typedef struct tagTOOLINFO {
	UINT      cbSize;
	UINT      uFlags;
	HWND      hwnd;
	UINT_PTR  uId;
	RECT      rect;
	HINSTANCE hinst;
	LPTSTR    lpszText;
#if (_WIN32_IE >= 0x0300)
	LPARAM lParam;
#endif
} TOOLINFO, NEAR *PTOOLINFO, FAR *LPTOOLINFO;

LARGE_INTEGER start, stop, freq;
double radOffset = 0.15;

PDATA GetPixelData(HDC hdc, int x, int y) {
	PDATA pixelData;
	pixelData.allStr = GetPixel(hdc, x, y);
	pixelData.b = GetBValue(pixelData.allStr);
	pixelData.g = GetGValue(pixelData.allStr);
	pixelData.r = GetRValue(pixelData.allStr);
	return pixelData;
}

PDATA GetBmpData(BMP bmp, int x, int y) {
	int i = ((bmp.MAX_WIDTH * (bmp.MAX_HEIGHT - y) * 4) + (x * 4)) - 4;
	PDATA pixel;
	pixel.b = (int)bmp.bitPointer[i];
	pixel.g = (int)bmp.bitPointer[i + 1];
	pixel.r = (int)bmp.bitPointer[i + 2];
	return pixel;
}

void SendKey(UINT key) {
	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = key;
	input.ki.dwFlags = 0;
	SendInput(1, &input, sizeof(input));
	Sleep(20);
	input.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &input, sizeof(input));
	return;
}

ROUND calcCoords(double radian) {
	ROUND coords;
	coords.x = round(cos(radian) * 54 + 800);
	coords.y = round(sin(radian) * 54 + 450);
	return coords;
}

double QPC(int mode) {
	if (mode) {
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&start);
	}
	else if (!mode) {
		QueryPerformanceCounter(&stop);
		LONGLONG diff = stop.QuadPart - start.QuadPart;
		double duration = (double)diff * 1000 / (double)freq.QuadPart;

		return duration;
	}
	return 0;
}

int findWhiteSpot(BMP bmp, double radian) {
	ROUND coords = calcCoords(radian);
	PDATA pixel = GetBmpData(bmp, coords.x, coords.y);
	if (pixel.r >= 200 and pixel.g >= 200 and pixel.b >= 200) {
		return 1;
	}
	else {
		return 0;
	}
}

int findRedSpot(BMP bmp,double radian) {
	ROUND coords = calcCoords(radian);
	PDATA pixel = GetBmpData(bmp, coords.x, coords.y);
	if (pixel.r >= 130 and pixel.g <= 30 and pixel.b <= 30) {
		return 1;
	}
	else {
		return 0;
	}
}

int whitePart(BMP bmp, double * rad) {
	future<int> fReturn[1000];
	double fRad[1000];
	QPC(1);

	cout << "finding white" << endl;

	for (int j = 0; j < 5; j++) {
		*rad = -0.5;

		bmp.refreshMagic();

		for (int i = 0; i < 500; i++) {
			fReturn[i] = async(launch::async, findWhiteSpot, bmp, *rad);
			fRad[i] = *rad;
			*rad += 0.01;
		}

		for (int i = 0; i < 500; i++) {
			if (fReturn[i].get()) {
				*rad = fRad[i];
				cout  << j << "White spot found at: " << *rad << endl;
				cout << QPC(0) << "ms" << endl;
				return 1;
			}
		}
	}

	return 0;
}

int mapNeedle(BMP bmp, double rad, double radRed) {
	future<int> fReturn[1000];
	double fRadRed[1000];

	cout << "mapping needle" << endl;

	while (1) {
		radRed -= 0.5;

		bmp.refreshMagic();

		QPC(1);

		for (int i = 0; i < 100; i++) {
			fReturn[i] = async(launch::async, findRedSpot, bmp, radRed);
			fRadRed[i] = radRed;
			radRed += 0.01;
		}

		for (int i = 0; i < 100; i++) {
			if (fReturn[i].get()) {
				radRed = fRadRed[i];
				if (abs(radRed - (rad - 0.14)) < radOffset) {
					return 1;
				}
				else { break; }
			}
			else { 
				if (i == 99) {
					cout << "needle fail" << endl;
					return 0;
				} 
			}
		}
	}
}

int redPart(BMP bmp, double rad) {
	future<int> fReturn[1000];
	double fRadRed[1000];
	double radRed;

	cout << "finding red" << endl;

	for (int j = 0; j < 3; j++) {
		radRed = -0.5;

		bmp.refreshMagic();

		QPC(1);

		for (int i = 0; i < 150; i++) {
			fReturn[i] = async(launch::async, findRedSpot, bmp, radRed);
			fRadRed[i] = radRed;
			radRed += 0.01;
		}

		for (int i = 0; i < 150; i++) {
			if (fReturn[i].get()) {
				radRed = fRadRed[i];
				cout << j << "red done " << QPC(0) << "ms" << endl;
				if (mapNeedle(bmp, rad, radRed)) {
					return 1;
				}
				else { return  0; }
			}
		}
	}
	
	return 0;
}

ROUND circle;
HHOOK hHook;

int main() {
	HDC hdc = GetDC(NULL);
	PDATA pixel;
	BMP bmp;

	bmp.doMagic();
	QPC(1);

	while (1) {
		pixel = GetPixelData(hdc, 800, 440);
		if (pixel.r == 0 and pixel.g == 0 and pixel.b == 0) {
			cout << "Skillcheck found:" << pixel.r << " " << pixel.g << " " << pixel.b << endl;
			cout << QPC(0) << "ms" << endl;

			double rad;

			if (whitePart(bmp, &rad)) {
				if (redPart(bmp, rad)) {
					QPC(1);
					SendKey(0x06);
					cout << "Sending Mouse X2" << endl;
					cout << QPC(0) << "ms" << endl;
				}
			}

			Sleep(1500);
		}
	}

	system("pause");

	return 0;
}
