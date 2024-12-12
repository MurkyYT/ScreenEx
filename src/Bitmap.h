#pragma once

#pragma comment(lib, "gdiplus.lib")
#include <windows.h>
#define GDIPVER 0x0110
#include <gdiplus.h>
#include <stdio.h>
#include <memory>

class Bitmap
{
public:
	~Bitmap();

	void Save(const wchar_t* path, const wchar_t* imageType);
	void Crop(const Gdiplus::Rect& newImageRect);
	void Crop(RECT newImageRect);
	void Crop(const Gdiplus::RectF& newImageRect);
	void TintArea(INT hue, INT amount, RECT* area);
	void ChangeContranst(INT contrast, RECT* area);
	void ChangeBrightness(INT brightness, RECT* area);
	void BlurImage(float radius, BOOL expandEdge, RECT* area);
	void ChangeHueSaturationLightness(INT hueLevel, INT saturationLevel, INT lightnessLevel, RECT* area);
	void ChangeLevels(INT highlight, INT midtone, INT shadow, RECT* area);
	void DarkenImage(INT darkenValue, RECT* area);

	UINT GetWidth();
	UINT GetHeight();
	HBITMAP GetHBITMAP();
	Gdiplus::Bitmap* GetGDIBitmap();
	INT GetBitCount();

	Bitmap* Copy();

	static Bitmap* CaptureScreen();
	static Bitmap* CaptureMonitor(HMONITOR monitor);

private:
	Bitmap();
	static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

	Gdiplus::Bitmap* m_Bitmap;
};

