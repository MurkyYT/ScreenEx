#include "Bitmap.h"

Bitmap::Bitmap() : m_Bitmap(NULL)
{

}

Bitmap::~Bitmap()
{
    if (m_Bitmap)
        delete m_Bitmap;

    m_Bitmap = NULL;
}

HBITMAP Bitmap::GetHBITMAP()
{
    if (m_Bitmap) 
    {
        HBITMAP out = NULL;
        m_Bitmap->GetHBITMAP(Gdiplus::Color::White, &out);
        return out;
    }
    return NULL;
}

void Bitmap::Crop(RECT newImageRect)
{
    Gdiplus::Bitmap* target = new Gdiplus::Bitmap(newImageRect.right - newImageRect.left, newImageRect.bottom - newImageRect.top,m_Bitmap->GetPixelFormat());
    Gdiplus::Graphics g(target);
    Gdiplus::Rect rect{};
    rect.X = newImageRect.left;
    rect.Y = newImageRect.top;
    rect.Width = newImageRect.right - newImageRect.left;
    rect.Height = newImageRect.bottom - newImageRect.top;
    Gdiplus::Status stat = g.DrawImage(m_Bitmap,0,0,rect.X,rect.Y,rect.Width,rect.Height,Gdiplus::UnitPixel);

    delete m_Bitmap;

    m_Bitmap = target;
}

void Bitmap::Crop(const Gdiplus::Rect& newImageRect)
{
    Gdiplus::Bitmap* target = new Gdiplus::Bitmap(newImageRect.Width, newImageRect.Height, m_Bitmap->GetPixelFormat());
    Gdiplus::Graphics g(target);
    Gdiplus::Status stat = g.DrawImage(m_Bitmap, 0, 0, 
        newImageRect.X, 
        newImageRect.Y, 
        newImageRect.Width, 
        newImageRect.Height, Gdiplus::UnitPixel);

    delete m_Bitmap;

    m_Bitmap = target;
}

void Bitmap::Crop(const Gdiplus::RectF& newImageRect)
{
    Gdiplus::Bitmap* target = new Gdiplus::Bitmap((int)newImageRect.Width, (int)newImageRect.Height, m_Bitmap->GetPixelFormat());
    Gdiplus::Graphics g(target);
    Gdiplus::Status stat = g.DrawImage(m_Bitmap, 0, 0, 
        (int)newImageRect.X, 
        (int)newImageRect.Y, 
        (int)newImageRect.Width, 
        (int)newImageRect.Height, Gdiplus::UnitPixel);

    delete m_Bitmap;

    m_Bitmap = target;
}

INT Bitmap::GetBitCount()
{
    Gdiplus::PixelFormat format = m_Bitmap->GetPixelFormat();
    switch (format)
    {
    case PixelFormat1bppIndexed:
        return 1;
    case PixelFormat4bppIndexed:
        return 4;
    case PixelFormat8bppIndexed:
        return 8;
    case PixelFormat16bppGrayScale:
    case PixelFormat16bppRGB555:
    case PixelFormat16bppRGB565:
    case PixelFormat16bppARGB1555:
        return 16;
    case PixelFormat24bppRGB:
        return 24;
    case PixelFormat32bppRGB:
    case PixelFormat32bppARGB:
    case PixelFormat32bppPARGB:
    case PixelFormat32bppCMYK:
        return 32;
    case PixelFormat48bppRGB:
        return 48;
    case PixelFormat64bppARGB:
    case PixelFormat64bppPARGB:
        return 64;
    default:
        return -1;
    }
}

Gdiplus::Bitmap* Bitmap::GetGDIBitmap()
{
    return m_Bitmap;
}

Bitmap* Bitmap::Copy()
{
    Bitmap* bmp = new Bitmap();

    Gdiplus::Rect rect{};
    rect.X = 0;
    rect.Y = 0;
    rect.Height = GetHeight();
    rect.Width = GetWidth();

    bmp->m_Bitmap = m_Bitmap->Clone(rect, m_Bitmap->GetPixelFormat());

    return bmp;
}

UINT Bitmap::GetWidth()
{
    return m_Bitmap->GetWidth();
}

UINT Bitmap::GetHeight()
{
    return m_Bitmap->GetHeight();
}

void Bitmap::TintArea(INT hue, INT amount, RECT* area)
{
    Gdiplus::TintParams params{};
    params.hue = hue;
    params.amount = amount;

    Gdiplus::Tint effect;
    effect.SetParameters(&params);

    m_Bitmap->ApplyEffect(&effect, area);
}

void Bitmap::ChangeContranst(INT contrast, RECT* area)
{
    Gdiplus::BrightnessContrastParams  params{};
    params.contrastLevel = contrast;
    params.brightnessLevel = 0;

    Gdiplus::BrightnessContrast effect;
    effect.SetParameters(&params);

    m_Bitmap->ApplyEffect(&effect, area);
}

void Bitmap::ChangeBrightness(INT brightness, RECT* area)
{
    Gdiplus::BrightnessContrastParams  params{};
    params.contrastLevel = 0;
    params.brightnessLevel = brightness;

    Gdiplus::BrightnessContrast effect;
    effect.SetParameters(&params);

    m_Bitmap->ApplyEffect(&effect, area);
}

void Bitmap::BlurImage(float radius,BOOL expandEdge, RECT* area)
{
    Gdiplus::BlurParams  params{};
    params.radius = radius;
    params.expandEdge = expandEdge;

    Gdiplus::Blur effect;
    effect.SetParameters(&params);

    m_Bitmap->ApplyEffect(&effect, area);
}

void Bitmap::ChangeHueSaturationLightness(INT hueLevel, INT saturationLevel, INT lightnessLevel, RECT* area)
{
    Gdiplus::HueSaturationLightnessParams   params{};
    params.hueLevel = hueLevel;
    params.saturationLevel = saturationLevel;
    params.lightnessLevel = lightnessLevel;

    Gdiplus::HueSaturationLightness effect;
    effect.SetParameters(&params);

    m_Bitmap->ApplyEffect(&effect, area);
}

void Bitmap::ChangeLevels(INT highlight, INT midtone, INT shadow, RECT* area)
{
    Gdiplus::LevelsParams   params{};
    params.highlight = highlight;
    params.midtone = midtone;
    params.shadow = shadow;

    Gdiplus::Levels effect;
    effect.SetParameters(&params);

    m_Bitmap->ApplyEffect(&effect, area);
}

void Bitmap::DarkenImage(INT darkenValue, RECT* area)
{
    INT x, y, width, height;
    Gdiplus::Graphics g(m_Bitmap);
    Gdiplus::SolidBrush brush(Gdiplus::Color((BYTE)(255 * (darkenValue / 100.0f)), 0, 0, 0));
    if(area == NULL)
    {
        x = 0;
        y = 0;
        width = GetWidth();
        height = GetHeight();
    }
    else
    {
        x = area->left;
        y = area->top;
        width = area->right - area->left;
        height = area->bottom - area->top;
    }
    g.FillRectangle(&brush, x, y, width, height);
}

void Bitmap::Save(const wchar_t* path, const wchar_t* imageType)
{
    CLSID clsid;

    if (GetEncoderClsid(imageType, &clsid) == -1) return;

    if(m_Bitmap)
        m_Bitmap->Save(path, &clsid);
}

Bitmap* Bitmap::CaptureMonitor(HMONITOR monitor)
{
    Bitmap* obj = new Bitmap();

    MONITORINFOEX inf{};
    inf.cbSize = sizeof(MONITORINFOEX);
    if (!GetMonitorInfo(monitor,&inf)) return NULL;

    int x = inf.rcMonitor.left;
    int y = inf.rcMonitor.top;
    int w = inf.rcMonitor.right - inf.rcMonitor.left;
    int h = inf.rcMonitor.bottom - inf.rcMonitor.top;

    HDC     hScreen = GetDC(NULL);
    HDC     hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);

    SelectObject(hDC, hBitmap);
    BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY);

    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);

    obj->m_Bitmap = Gdiplus::Bitmap::FromHBITMAP(hBitmap, NULL);

    DeleteObject(hBitmap);

    return obj;
}

Bitmap* Bitmap::CaptureScreen()
{
    Bitmap* obj = new Bitmap();

    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    HDC     hScreen = GetDC(NULL);
    HDC     hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);

    SelectObject(hDC, hBitmap);
    BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY);

    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);

    obj->m_Bitmap = Gdiplus::Bitmap::FromHBITMAP(hBitmap, NULL);

    DeleteObject(hBitmap);

    return obj;
}

int Bitmap::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes

    Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;  // Failure

    pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
        return -1;  // Failure

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}