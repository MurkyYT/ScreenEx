#pragma once
#include <windows.h>
#include <gdiplus.h>

namespace Intersection
{
	BOOL IsPointInsde(const RECT& rect,const POINT& point)
	{
		return point.x >= rect.left
			&& point.x <= rect.right
			&& point.y >= rect.top
			&& point.y <= rect.bottom;
	}

	BOOL IsPointInsde(const Gdiplus::Rect& rect, const POINT& point)
	{
		RECT win32Rect = { rect.X,rect.Y,rect.X + rect.Width,rect.Y + rect.Height };
		return IsPointInsde(win32Rect, point);
	}
}