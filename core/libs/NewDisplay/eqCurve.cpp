#include "FT812.h"
#include "display.h"
#include "eqCurveControl.h"

// colors: 0 curve, 1 grid, 2 background, 3 0 dB line
cEqCurve::cEqCurve(strControlProperties* properties)
  : cDisplayControl(controlIdEqCurve, 1)
{
    colorsCount = 4;
    if (properties == nullptr)
        return;
    colors = properties->colors;
    curve = static_cast<strEqCurve*>(properties->data);
    posX = properties->x;
    posY = properties->y;
    width = properties->w;
    height = properties->h;
    style = properties->style;
}

int16_t cEqCurve::yOf(float db) const
{
    const float half = height * 0.5f - 4.0f;
    float y = posY + height * 0.5f - db / strEqCurve::kRangeDb * half;
    if (y < posY + 1)
        y = posY + 1;
    if (y > posY + height - 2)
        y = posY + height - 2;
    return static_cast<int16_t>(y);
}

void cEqCurve::update()
{
    if (curve == nullptr || colors == nullptr)
        return;

    API_SAVE_CONTEXT();

    API_COLOR(colors[2]);
    API_BEGIN(RECTS);
    API_VERTEX2F(posX, posY);
    API_VERTEX2F(posX + width, posY + height);
    API_END();

    // Grid: +-6 and +-12 dB, then 0 dB brighter
    API_LINE_WIDTH(16);
    API_COLOR(colors[1]);
    API_BEGIN(LINES);
    for (int db : { -12, -6, 6, 12 })
    {
        API_VERTEX2F(posX, yOf(static_cast<float>(db)));
        API_VERTEX2F(posX + width, yOf(static_cast<float>(db)));
    }
    API_END();
    API_COLOR(colors[3]);
    API_BEGIN(LINES);
    API_VERTEX2F(posX, yOf(0.0f));
    API_VERTEX2F(posX + width, yOf(0.0f));
    API_END();

    if (curve->count >= 2)
    {
        API_COLOR(colors[0]);
        API_LINE_WIDTH(32);
        API_BEGIN(LINE_STRIP);
        for (uint8_t i = 0; i < curve->count; i++)
        {
            const int16_t x = posX + static_cast<int16_t>(static_cast<int32_t>(width - 1) * i / (curve->count - 1));
            API_VERTEX2F(x, yOf(curve->tenthsDb[i] * 0.1f));
        }
        API_END();
    }

    API_RESTORE_CONTEXT();
}
