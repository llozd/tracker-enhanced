#ifndef LIBS_NEWDISPLAY_EQCURVECONTROL_H_
#define LIBS_NEWDISPLAY_EQCURVECONTROL_H_

// An EQ's frequency response (openspec realtime-effects): a curve over a log frequency axis,
// with grid lines at 0 and +-6 / +-12 dB.

#include "displayControls.h"

struct strEqCurve
{
    static constexpr uint8_t kMaxPoints = 128;
    static constexpr float kRangeDb = 18.0f; // top and bottom of the graph
    uint8_t count = 0;
    int16_t tenthsDb[kMaxPoints] = {}; // the response at evenly spaced log frequencies, in 0.1 dB
};

class cEqCurve : public cDisplayControl
{
  public:
    cEqCurve(strControlProperties* properties = nullptr);
    virtual ~cEqCurve() {}

    virtual void update();
    virtual void setStyle(uint32_t style) { this->style = style; }
    virtual void setText(const char* text) { this->text = text; }
    virtual void setValue(int value) { this->value = value; }
    virtual void setColors(const uint32_t* colors) { this->colors = colors; }
    virtual void setData(void* data) { curve = static_cast<strEqCurve*>(data); }

  private:
    strEqCurve* curve = nullptr;
    int16_t yOf(float db) const;
};

#endif // LIBS_NEWDISPLAY_EQCURVECONTROL_H_
