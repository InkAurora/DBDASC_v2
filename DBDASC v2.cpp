#include <string>
#include <fstream>
#include <math.h>

#include "ImageSearch.h"

using namespace std;



LARGE_INTEGER start, stop, freq;
double radOffset;
int widthHalf = GetSystemMetrics(SM_CXSCREEN) / 2;
int heightHalf = GetSystemMetrics(SM_CYSCREEN) / 2;
int radius = 0;
int xS, yS;
int bmpPadding = 5;
int bmpSize, bmpHalfSize;



typedef struct BMP_D {
  HDC hdc, hdcTemp;
  BITMAPINFO bitmapInfo;
  HBITMAP hBitmap;
  BYTE* bitPointer;
  int BIT_ALLOC;
  int MAP_SIZE = 0;

  void CreateBMP(int left = 0, int top = 0, int width = 0, int height = 0) {
    hdc = GetDC(NULL);
    if (!hdc) return;
    //GetWindowRect(FindWindowA(NULL, NULL), &rect);

    BIT_ALLOC = 4;

    MAP_SIZE = width * height * BIT_ALLOC;

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 8 * BIT_ALLOC;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage = MAP_SIZE;
    bitmapInfo.bmiHeader.biClrUsed = 0;
    bitmapInfo.bmiHeader.biClrImportant = 0;

    hBitmap = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, (void**)(&bitPointer), NULL, 0);

    if (!hBitmap) return;

    hdcTemp = CreateCompatibleDC(NULL);

    if (!hdcTemp) return;

    SelectObject(hdcTemp, hBitmap);

    BitBlt(hdcTemp, 0, 0, width, height, hdc, left - (width / 2), top - (height / 2), SRCCOPY);

    return;
  }

  void DeleteBMP() {
    DeleteObject(hBitmap);
    ReleaseDC(NULL, hdc);
    DeleteDC(hdc);
    DeleteDC(hdcTemp);

    return;
  }

  void UpdateBMP() {
    BitBlt(hdcTemp, 0, 0, bmpSize, bmpSize, hdc, xS - radius - bmpPadding, yS - radius - bmpPadding, SRCCOPY);

    return;
  }

  void svBMP() {
    BITMAPFILEHEADER bmfh;
    int nBitsOffset = sizeof(bmfh) + bitmapInfo.bmiHeader.biSize;
    LONG lImageSize = bitmapInfo.bmiHeader.biSizeImage;
    LONG lFileSize = nBitsOffset + lImageSize;
    bmfh.bfType = 'B' + ('M' << 8);
    bmfh.bfOffBits = nBitsOffset;
    bmfh.bfSize = lFileSize;
    bmfh.bfReserved1 = bmfh.bfReserved2 = 0;

    string szFilePath("C:\\Rules\\bmp.bmp");
    std::ofstream pFile(szFilePath, std::ios_base::binary);

    pFile.write((const char*)&bmfh, sizeof(BITMAPFILEHEADER));
    UINT nWrittenFileHeaderSize = pFile.tellp();

    pFile.write((const char*)&bitmapInfo.bmiHeader, sizeof(BITMAPINFOHEADER));
    UINT nWrittenInfoHeaderSize = pFile.tellp();

    const char* p = reinterpret_cast<const char*>(bitPointer);
    pFile.write(&p[0], MAP_SIZE);
    UINT nWrittenDIBDataSize = pFile.tellp();
    pFile.close();

    return;
  }
} BMP_D;

class Pixel {
public:
  int x;
  int y;
  int r;
  int g;
  int b;

  Pixel(double rad, BMP_D bmp) {
    GetCoords(rad);
    GetPixelData(bmp);

    return;
  }

private:
  void GetCoords(double rad) {
    x = round(cos(rad) * radius + bmpHalfSize );
    y = round(sin(rad) * radius + bmpHalfSize);

    return;
  }

  void GetPixelData(BMP_D bmp) {
    int i = bmp.bitmapInfo.bmiHeader.biWidth * y * 4 + x * 4;

    b = (int)bmp.bitPointer[i];
    g = (int)bmp.bitPointer[i + 1];
    r = (int)bmp.bitPointer[i + 2];

    return;
  }
};



double QPC(int mode) {
  if (mode) {
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
  } else if (!mode) {
    QueryPerformanceCounter(&stop);
    LONGLONG diff = stop.QuadPart - start.QuadPart;
    double duration = (double)diff * 1000 / (double)freq.QuadPart;

    return round(duration);
  }

  return 0;
}

void SendKey(UINT key) {
  INPUT input = { 0 };
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = key;
  input.ki.dwFlags = 0;
  SendInput(1, &input, sizeof(input));
  input.ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, &input, sizeof(input));
  return;
}

void GetRadius(BMP_D bmp) {
  int r, g, b;

  auto Compare = [&]() {
    if (r > 141 and r < 151 and g > 152 and g < 162 and b > 166 and b < 176) return 1;
    return 0;
  };

  auto GetBmpData = [&](int x, int y) {
    int i = bmp.bitmapInfo.bmiHeader.biWidth * y * 4 + x * 4;
    b = (int)bmp.bitPointer[i];
    g = (int)bmp.bitPointer[i + 1];
    r = (int)bmp.bitPointer[i + 2];
    return;
  };

  int half = bmp.bitmapInfo.bmiHeader.biWidth / 2;

  for (int i = 40; i < 150; i++) {
    GetBmpData(half, half - i);
    if (Compare()) {
      radius = i;
      break;
    }

    GetBmpData(half + i, half);
    if (radius == 0 and Compare()) {
      radius = i;
      break;
    }

    GetBmpData(half, half + i);
    if (radius == 0 and Compare()) {
      radius = i;
      break;
    }

    GetBmpData(half - i, half);
    if (radius == 0 and Compare()) {
      radius = i;
      break;
    }
  }

  return;
}

int FindWhiteSpot(BMP_D bmp, double rad) {
  Pixel pixel(rad, bmp);
  if (pixel.r >= 220 and pixel.g >= 220 and pixel.b >= 220) {
    return 1;
  }
  
  return 0;
}

int FindRedSpot(BMP_D bmp,double rad) {
  Pixel pixel(rad, bmp);
  if (pixel.r >= 130 and pixel.g <= 30 and pixel.b <= 30) {
    return 1;
  }
  
  return 0;
}

int WhitePart(BMP_D bmp, double& rad) {
  QPC(1);

  cout << "Locating white spot" << endl;

  for (int j = 0; j < 5; j++) {
    rad = 0;

    bmp.UpdateBMP();

    for (int i = 0; i < 600; i++) {
      if (FindWhiteSpot(bmp, rad)) {
        cout << "White spot found at: " << rad << "  on attempt: " << j << ". Took: " << QPC(0) << " milliseconds" << endl;
        return 1;
      }

      rad += 0.01;
    }
  }

  cout << "White spot not found" << endl;

  return 0;
}

int MapNeedle(BMP_D bmp, double rad, double radRed) {
  cout << "Mapping red needle" << endl;

  while (1) {
    QPC(1);

    bmp.UpdateBMP();

    for (int i = 0; i < 100; i++) {
      if (FindRedSpot(bmp, radRed)) {
        if (radRed >= rad - radOffset) {
          cout << "Mapping successful" << endl;
          return 1;
        } else {
          break;
        }
      } else {
        if (i == 99) {
          cout << "Failed at mapping red needle" << endl;
          return 0;
        }
      }

      radRed += 0.01;
    }
  }
}

int RedPart(BMP_D bmp, double rad) {
  double radRed;

  cout << "Locating red needle" << endl;

  QPC(1);

  for (int j = 0; j < 3; j++) {
    radRed = -1;

    bmp.UpdateBMP();

    for (int i = 0; i < 300; i++) {
      if (FindRedSpot(bmp, radRed)) {
        cout << "Red needle found at: " << radRed << "  on attempt: " << j << ". Took: " << QPC(0) << " milliseconds" << endl;

        if (MapNeedle(bmp, rad, radRed)) {
          return 1;
        } else {
          return 0;
        }
      }

      radRed += 0.01;
    }
  }

  cout << "Red needle not found" << endl;
    
  return 0;
}


int main(int argc, char **argv) {
  BMP_D bmp;

  cout << "Dead by Daylight Auto Skill Check v1.1" << endl;
  cout << "https://github.com/InkAurora/DBDASC_v2" << endl;
  if (argc > 1) {
    radOffset = stod(string(argv[argc - 1]));
    cout << "Using custom offset: " << radOffset << endl;
  } else radOffset = 0.12;
  cout << "============================================================================" << endl;

  while (1) {
    string path(argv[0]);
    path = path.substr(0, path.find_last_of("\\/"));
    path += "\\m5btn.bmp";

    int success = ImageSearch(xS, yS, 200, 100, -200, -100, path, 20);
    xS += 15;
    yS += 15;
    if (success == 1) {
      cout << "Skillcheck found: " << xS << "x " << yS << "y" << endl;

      double rad;

      while (radius == 0) {
        BMP_D bmpTemp;
        bmpTemp.CreateBMP(xS, yS, 300, 300);
        GetRadius(bmpTemp);
        bmpTemp.DeleteBMP();
        bmpSize = radius * 2 + bmpPadding * 2;
        bmpHalfSize = bmpSize / 2;
        bmp.CreateBMP(xS - bmpPadding, yS - bmpPadding, bmpSize, bmpSize);
      }

      if (WhitePart(bmp, rad)) {
        if (RedPart(bmp, rad)) {
          cout << "Sending Mouse X2" << endl;
          SendKey(0x06);
          cout << "Delay: " << QPC(0) << " milliseconds" << endl;
        }
      }

      //bmp.svBMP();

      cout << "============================================================================" << endl;
      Sleep(1500);
    }
  }

  system("pause");

  return 0;
}
