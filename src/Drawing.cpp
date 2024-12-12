#include "Drawing.h"

void GetRoundRectPath(Gdiplus::GraphicsPath* pPath,Gdiplus::Rect& r, int dia)
{
    if (dia > r.Width)     dia = r.Width;
    if (dia > r.Height)    dia = r.Height;

    Gdiplus::Rect Corner(r.X, r.Y, dia, dia);

    pPath->Reset();

    pPath->AddArc(Corner, 180, 90);

    if (dia == 20)
    {
        Corner.Width += 1;
        Corner.Height += 1;
        r.Width -= 1; r.Height -= 1;
    }

    Corner.X += (r.Width - dia - 1);
    pPath->AddArc(Corner, 270, 90);

    Corner.Y += (r.Height - dia - 1);
    pPath->AddArc(Corner, 0, 90);

    Corner.X -= (r.Width - dia - 1);
    pPath->AddArc(Corner, 90, 90);

    pPath->CloseFigure();
}

void Drawing::FillRoundRect(Gdiplus::Graphics* pGraphics, Gdiplus::Rect& r,const Gdiplus::Color& color, int radius)
{
    Gdiplus::SolidBrush sbr(color);
    FillRoundRect(pGraphics, &sbr, r, color, radius);
}

void Drawing::FillRoundRect(Gdiplus::Graphics* pGraphics, Gdiplus::Brush* pBrush, Gdiplus::Rect& r,const Gdiplus::Color& border, int radius)
{
    int dia = 2 * radius;

    pGraphics->SetPageUnit(Gdiplus::UnitPixel);

    Gdiplus::Pen pen(border, 1);
    pen.SetAlignment(Gdiplus::PenAlignmentCenter);

    Gdiplus::GraphicsPath path;

    GetRoundRectPath(&path, r, dia);

    pGraphics->FillPath(pBrush, &path);

    pGraphics->DrawPath(&pen, &path);
}

void Drawing::DrawRoundRect(Gdiplus::Graphics* pGraphics,Gdiplus::Rect& r, const Gdiplus::Color& color, int radius, int width)
{
    int dia = 2 * radius;

    pGraphics->SetPageUnit(Gdiplus::UnitPixel);

    Gdiplus::Pen pen(color, 1);
    pen.SetAlignment(Gdiplus::PenAlignmentCenter);

    Gdiplus::GraphicsPath path;

    GetRoundRectPath(&path, r, dia);

    pGraphics->DrawPath(&pen, &path);

    for (int i = 1; i < width; i++)
    {
        r.Inflate(-1, 0);

        GetRoundRectPath(&path, r, dia);

        pGraphics->DrawPath(&pen, &path);

        r.Inflate(0, -1);

        GetRoundRectPath(&path, r, dia);

        pGraphics->DrawPath(&pen, &path);
    }
}