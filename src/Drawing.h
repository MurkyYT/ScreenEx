#pragma once
#include <Windows.h>
#include <gdiplus.h>
class Drawing
{
public:
	static void DrawRoundRect(Gdiplus::Graphics* pGraphics,Gdiplus::Rect& r, const Gdiplus::Color& color, int radius, int width);
	static void FillRoundRect(Gdiplus::Graphics* pGraphics, Gdiplus::Brush* pBrush, Gdiplus::Rect& r, const Gdiplus::Color& border, int radius);
	static void FillRoundRect(Gdiplus::Graphics* pGraphics, Gdiplus::Rect& r, const Gdiplus::Color& color, int radius);
};

