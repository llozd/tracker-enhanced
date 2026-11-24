// Master screen, Inserts page (openspec realtime-effects): the insert slots of every channel.

#include "masterParams/masterParams.h"

#if FEATURE_REALTIME_FX

#include <cr_section_macros.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "core/interfacePopups.h"
#include "fileManager.h"
#include "Platform.h"
#include "eqCurveControl.h"
#include "fx/insertChannels.h"
#include "fx/insertFx.h"
#include "mtAudioEngine.h"

namespace
{
constexpr int kParamsPerPage = 4;
constexpr uint8_t kPlaceChannel = 0, kPlaceSlot = 1, kPlaceEffect = 2, kPlacePage = 3,
                  kPlaceFirstParam = 4, kPlaces = 8;

__NOINIT(EXTERNAL_RAM) char labelText[kPlaces][24];
__NOINIT(EXTERNAL_RAM) char valueText[kPlaces][24];
__NOINIT(EXTERNAL_RAM) strEqCurve eqCurve;
uint32_t eqColors[4];

constexpr float kMeterFloorDb = -48.0f;
constexpr float kMaxReductionDb = 24.0f;
constexpr uint32_t kMeterIntervalMs = 40;
float inMeter = 0.0f, outMeter = 0.0f; // 0..1 of the bar
uint32_t lastMeterMs = 0;

int toBar(float peak)
{
    if (peak <= 0.0f)
        return 0;
    const float db = 20.0f * std::log10(peak);
    const float v = (db - kMeterFloorDb) / -kMeterFloorDb;
    return static_cast<int>(100.0f * (v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v));
}

// Effect types in picker order: grouped by family, then by ID.
insertfx::Type pickerOrder[insertfx::kTypeCount];

void buildPickerOrder()
{
    int n = 0;
    for (int f = 0; f < insertfx::kFamilyCount; f++)
    {
        for (int t = 0; t < insertfx::kTypeCount; t++)
        {
            if (insertfx::describe(static_cast<insertfx::Type>(t)).family == f)
                pickerOrder[n++] = static_cast<insertfx::Type>(t);
        }
    }
}

int pickerIndexOf(uint8_t type)
{
    for (int i = 0; i < insertfx::kTypeCount; i++)
    {
        if (pickerOrder[i] == type)
            return i;
    }
    return 0;
}

// Channel picker order: tracks, synth parts, buses, then the master (insert channel ids).
constexpr uint8_t kChannelOrder[kAllInsertChannels] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, insertfx::kBus0, insertfx::kBus0 + 1, insertfx::kBus0 + 2,
    insertfx::kBus0 + 3, insertfx::kMaster
};
static_assert(insertfx::kBusCount == 4 && kAllInsertChannels == insertfx::kChannelCount);

int channelOrderIndexOf(uint8_t ch)
{
    for (int i = 0; i < kAllInsertChannels; i++)
    {
        if (kChannelOrder[i] == ch)
            return i;
    }
    return 0;
}

bool isBus(uint8_t ch)
{
    return ch >= insertfx::kBus0;
}

bool isBusSource(uint8_t ch)
{
    return ch < kBusSources;
}

// bus-channels: after the effect slots, an "Out" slot holds the routing: a track or synth part's
// output, or a bus's volume. The master has none.
bool hasOutSlot(uint8_t ch)
{
    return isBusSource(ch) || isBus(ch);
}

int slotCount(uint8_t ch)
{
    return kInsertSlots + (hasOutSlot(ch) ? 1 : 0);
}

const char* channelName(uint8_t ch, char* buf, size_t size)
{
    if (ch < kSampleTracks)
        snprintf(buf, size, "Track %d", ch + 1);
    else if (ch < insertfx::kMaster)
        snprintf(buf, size, "Synth %d", ch - kSampleTracks + 1);
    else if (isBus(ch))
        snprintf(buf, size, "Bus %d", ch - insertfx::kBus0 + 1);
    else
        snprintf(buf, size, "Master");
    return buf;
}

const char* outputName(uint8_t output, char* buf, size_t size)
{
    if (output == kOutputMaster)
        snprintf(buf, size, "Master");
    else
        snprintf(buf, size, "Bus %d", output);
    return buf;
}

// As ampLogValues: 50 is 0 dB, 0.48 dB a step.
void formatBusVolume(uint8_t volume, char* buf, size_t size)
{
    if (volume == 0)
        snprintf(buf, size, "-inf dB");
    else
        snprintf(buf, size, "%+.1f dB", (volume - kBusVolumeDefault) * 0.48f);
}

int busSourceCount(uint8_t ch)
{
    int n = 0;
    for (uint8_t output : mtProject.values.channelOutput)
    {
        if (output == ch - insertfx::kBus0 + 1)
            n++;
    }
    return n;
}

// The Out slot has no effect: it reads as an empty slot.
const strInsertSlot& slotAt(uint8_t ch, uint8_t slot)
{
    static const strInsertSlot kNoEffect{};
    return slot < kInsertSlots ? mtProject.values.insertSlotOf(ch, slot) : kNoEffect;
}

// The Out slot's controls: Output on a track or synth part; Volume and Inputs on a bus.
void outSlotText(uint8_t ch, int control, char* label, char* value, size_t size)
{
    snprintf(label, size, " "); // an empty place shows a space, like an unused parameter
    snprintf(value, size, " ");
    if (isBusSource(ch) && control == 0)
    {
        snprintf(label, size, "Output");
        outputName(mtProject.values.channelOutput[ch], value, size);
    }
    else if (isBus(ch) && control == 0)
    {
        snprintf(label, size, "Volume");
        formatBusVolume(mtProject.values.busVolume[ch - insertfx::kBus0], value, size);
    }
    else if (isBus(ch) && control == 1)
    {
        snprintf(label, size, "Inputs");
        snprintf(value, size, "%d", busSourceCount(ch));
    }
}

// Changes an Out slot control by value (encoder steps). Returns false when nothing changed.
bool changeOutSlot(uint8_t ch, int control, int16_t value)
{
    if (isBusSource(ch) && control == 0)
    {
        const int output = mtProject.values.channelOutput[ch] + (value > 0 ? 1 : -1);
        if (output < kOutputMaster || output > kBusCount)
            return false;
        engine.setChannelOutput(ch, static_cast<uint8_t>(output));
        return true;
    }
    if (isBus(ch) && control == 0)
    {
        const int bus = ch - insertfx::kBus0;
        const int volume =
          std::clamp(mtProject.values.busVolume[bus] + value, 0, int{ kBusVolumeMax });
        engine.setBusVolume(bus, static_cast<uint8_t>(volume));
        return true;
    }
    return false;
}

// Delete on an Out slot control: back to the master, or a bus back to 0 dB.
bool resetOutSlot(uint8_t ch, int control)
{
    if (isBusSource(ch) && control == 0)
    {
        engine.setChannelOutput(ch, kOutputMaster);
        return true;
    }
    if (isBus(ch) && control == 0)
    {
        engine.setBusVolume(ch - insertfx::kBus0, kBusVolumeDefault);
        return true;
    }
    return false;
}

int pageCount(const insertfx::Desc& d)
{
    return d.count == 0 ? 1 : (d.count + kParamsPerPage - 1) / kParamsPerPage;
}

void formatValue(const insertfx::ParamDesc& p, int16_t v, char* buf, size_t size)
{
    v = v < p.min ? p.min : v > p.max ? p.max : v;
    switch (p.format)
    {
        case insertfx::kFormatPercent: snprintf(buf, size, "%d%%", (v * 100 + 63) / 127); break;
        case insertfx::kFormatEnum: snprintf(buf, size, "%s", p.names ? p.names[v - p.min] : ""); break;
        default: snprintf(buf, size, "%d%s%s", v, p.unit ? " " : "", p.unit ? p.unit : ""); break;
    }
}

void setInsertsButtons();
} // namespace

strEqCurve* insertsEqCurveData()
{
    return &eqCurve;
}

void cMasterParams::switchToInserts()
{
    static bool pickerReady = false;
    if (!pickerReady)
    {
        buildPickerOrder();
        pickerReady = true;
    }

    keyboardManager.deactivateKeyboard();
    displayType = displayMode::inserts;
    isDelayScreen = false;
    isReverbScreen = false;
    isLimiterScreen = false;
    isEqualizerScreen = false;

    setInsertsButtons();
    showInsertsScreen();
}

void cMasterParams::showInsertsScreen()
{
    display.setControlHide(delayPingpongEnableList);
    display.setControlHide(delaySyncEnableList);
    display.setControlHide(delaySyncRateList);
    display.setControlHide(limiterSidechainList);
    display.setControlHide(saturationList);

    titleBar->show();
    titleBar->setTitle("Inserts");

    resizeToDefaultMaster();
    resizeBarControl(displayMode::masterValues);

    for (uint8_t i = 0; i < kPlaces; i++)
    {
        display.setControlValue(label[i], 1);
        display.setControlStyle2(label[i],
                                 controlStyleCenterX | controlStyleCenterY | controlStyleFont2);
        display.setControlShow(label[i]);
        display.setControlHide(trackNameLabel[i]);
        display.refreshControl(trackNameLabel[i]);
        display.setControlHide(trackVolumeBar[i]);
        display.refreshControl(trackVolumeBar[i]);
    }
    for (auto& cover : blankSpaceCover)
    {
        display.setControlHide(cover);
        display.refreshControl(cover);
    }
    display.setControlValue(bgLabel, 255);
    display.refreshControl(bgLabel);

    setFramePlaces(kPlaces, selectedPlaceInserts);

    showInsertsValues();
    display.synchronizeRefresh();
    lightUpPadBoard();
}

void cMasterParams::showInsertsValues()
{
    const bool outSlot = insertsSlot >= kInsertSlots;
    const strInsertSlot& slot = slotAt(insertsChannel, insertsSlot);
    const insertfx::Desc& d = insertfx::describe(static_cast<insertfx::Type>(slot.type));
    const int pages = pageCount(d);
    if (insertsPage >= pages)
        insertsPage = pages - 1;

    snprintf(labelText[kPlaceChannel], sizeof(labelText[0]), "Channel");
    channelName(insertsChannel, valueText[kPlaceChannel], sizeof(valueText[0]));
    snprintf(labelText[kPlaceSlot], sizeof(labelText[0]), "Slot");
    if (outSlot)
        snprintf(valueText[kPlaceSlot], sizeof(valueText[0]), "Out");
    else
        snprintf(valueText[kPlaceSlot], sizeof(valueText[0]), "%d", insertsSlot + 1);
    snprintf(labelText[kPlaceEffect], sizeof(labelText[0]), "Effect");
    snprintf(valueText[kPlaceEffect], sizeof(valueText[0]), "%s", outSlot ? "-" : d.name);
    snprintf(labelText[kPlacePage], sizeof(labelText[0]), "Page");
    snprintf(valueText[kPlacePage], sizeof(valueText[0]), "%d/%d", insertsPage + 1, pages);

    for (int i = 0; i < kParamsPerPage; i++)
    {
        const int param = insertsPage * kParamsPerPage + i;
        char* lt = labelText[kPlaceFirstParam + i];
        char* vt = valueText[kPlaceFirstParam + i];
        if (outSlot)
        {
            outSlotText(insertsChannel, i, lt, vt, sizeof(valueText[0]));
        }
        else if (param < d.count)
        {
            snprintf(lt, sizeof(labelText[0]), "%s", d.p[param].label);
            formatValue(d.p[param], slot.params[param], vt, sizeof(valueText[0]));
        }
        else
        {
            lt[0] = ' ';
            lt[1] = 0;
            vt[0] = ' ';
            vt[1] = 0;
        }
    }

    for (uint8_t i = 0; i < kPlaces; i++)
    {
        display.setControlText(label[i], labelText[i]);
        display.setControlText2(label[i], valueText[i]);
        display.refreshControl(label[i]);
    }

    // Bars: input and output level, gain reduction (dynamics), parameter positions; an EQ draws its curve.
    const uint32_t shadow = display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0;
    const uint32_t gradient = display.getTheme().barShadow > 0 ? controlStyleGradient : 0;
    const bool eq = insertfx::isEq(static_cast<insertfx::Type>(slot.type));
    const bool dynamics = insertfx::isDynamics(static_cast<insertfx::Type>(slot.type));
    for (uint8_t i = 0; i < kPlaces; i++)
    {
        bool show = i <= kPlaceSlot;
        if (i == kPlaceEffect)
            show = dynamics;
        else if (i >= kPlaceFirstParam)
            show = !eq && insertsPage * kParamsPerPage + (i - kPlaceFirstParam) < d.count;
        const bool volumeBar = outSlot && isBus(insertsChannel) && i == kPlaceFirstParam;
        show = show || volumeBar;
        display.setControlStyle(barControl[i],
                                (i == kPlaceEffect ? controlStyleReverse_0_100 : controlStyleValue_0_100) |
                                  controlStyleBackground | shadow | gradient);
        barData[i].value = i <= kPlaceSlot   ? display.getTheme().green
                           : i == kPlaceEffect ? display.getTheme().cyan
                                               : display.getTheme().white;
        if (volumeBar)
        {
            display.setControlValue(barControl[i],
                                    mtProject.values.busVolume[insertsChannel - insertfx::kBus0] *
                                      100 / kBusVolumeMax);
        }
        else if (i >= kPlaceFirstParam && show)
        {
            const int param = insertsPage * kParamsPerPage + (i - kPlaceFirstParam);
            const insertfx::ParamDesc& p = d.p[param];
            const int v = std::clamp<int>(slot.params[param], p.min, p.max);
            display.setControlValue(barControl[i], p.max > p.min ? (v - p.min) * 100 / (p.max - p.min) : 0);
        }
        if (show)
            display.setControlShow(barControl[i]);
        else
            display.setControlHide(barControl[i]);
        display.refreshControl(barControl[i]);
    }

    if (eq && eqCurveControl >= 0)
    {
        insertfx::Slot fx;
        fx.type = static_cast<insertfx::Type>(slot.type);
        memcpy(fx.params, slot.params, sizeof(fx.params));
        eqCurve.count = strEqCurve::kMaxPoints;
        for (int k = 0; k < eqCurve.count; k++)
        {
            const float hz = 20.0f * std::pow(1000.0f, static_cast<float>(k) / (eqCurve.count - 1));
            const float db = insertfx::responseDb(fx, hz);
            eqCurve.tenthsDb[k] = static_cast<int16_t>(std::clamp(db, -30.0f, 30.0f) * 10.0f);
        }
        const auto& theme = display.getTheme();
        eqColors[0] = theme.patternTrackerColors[2]; // the note colour
        eqColors[1] = theme.frameColors[1];
        eqColors[2] = theme.activeBarColors[4];
        eqColors[3] = theme.activeLabelsColors[0];
        display.setControlColors(eqCurveControl, eqColors);
        display.setControlShow(eqCurveControl);
        display.refreshControl(eqCurveControl);
    }
    else if (eqCurveControl >= 0)
    {
        display.setControlHide(eqCurveControl);
        display.refreshControl(eqCurveControl);
    }

    if (selectedPlaceInserts >= kPlaces)
        selectedPlaceInserts = kPlaceEffect;
    display.setControlValue(frameControl, selectedPlaceInserts);
    display.setControlShow(frameControl);
    display.refreshControl(frameControl);
}

void cMasterParams::insertsRefreshMeters()
{
    const uint32_t now = platform->getTimer()->millis();
    if (now - lastMeterMs < kMeterIntervalMs)
        return;
    lastMeterMs = now;

    float in = 0.0f, out = 0.0f;
    const auto id = static_cast<insertfx::ChannelId>(insertsChannel);
    insertfx::takeLevels(id, in, out);
    // peak hold with a quick release, like a VU's ballistics
    inMeter = std::max(static_cast<float>(toBar(in)), inMeter * 0.85f);
    outMeter = std::max(static_cast<float>(toBar(out)), outMeter * 0.85f);
    display.setControlValue(barControl[kPlaceChannel], static_cast<int>(inMeter));
    display.refreshControl(barControl[kPlaceChannel]);
    display.setControlValue(barControl[kPlaceSlot], static_cast<int>(outMeter));
    display.refreshControl(barControl[kPlaceSlot]);

    const strInsertSlot& slot = slotAt(insertsChannel, insertsSlot);
    if (insertfx::isDynamics(static_cast<insertfx::Type>(slot.type)))
    {
        const float gr = insertfx::gainReductionDb(id, insertsSlot);
        display.setControlValue(barControl[kPlaceEffect],
                                static_cast<int>(std::min(gr / kMaxReductionDb, 1.0f) * 100.0f));
        display.refreshControl(barControl[kPlaceEffect]);
    }
}

void cMasterParams::hideInsertsControls()
{
    if (eqCurveControl >= 0)
    {
        display.setControlHide(eqCurveControl);
        display.refreshControl(eqCurveControl);
    }
}

void cMasterParams::insertsSelect(uint8_t place)
{
    if (place >= kPlaces)
        return;
    // Only the frame moves; a full redraw here costs one screen swap per control
    selectedPlaceInserts = place;
    display.setControlValue(frameControl, selectedPlaceInserts);
    display.refreshControl(frameControl);
    display.synchronizeRefresh();
}

void cMasterParams::insertsMove(int8_t direction)
{
    const int place = static_cast<int>(selectedPlaceInserts) + direction;
    if (place >= 0 && place < kPlaces)
        insertsSelect(static_cast<uint8_t>(place));
}

void cMasterParams::insertsChange(int16_t value)
{
    const bool outSlot = insertsSlot >= kInsertSlots;
    strInsertSlot slot = slotAt(insertsChannel, insertsSlot);
    const insertfx::Desc& d = insertfx::describe(static_cast<insertfx::Type>(slot.type));

    switch (selectedPlaceInserts)
    {
        case kPlaceChannel:
        {
            const int idx = channelOrderIndexOf(insertsChannel) + (value > 0 ? 1 : -1);
            if (idx >= 0 && idx < kAllInsertChannels)
                insertsChannel = kChannelOrder[idx];
            if (insertsSlot >= slotCount(insertsChannel))
                insertsSlot = kInsertSlots - 1; // the master has no Out slot
            insertsPage = 0;
            break;
        }
        case kPlaceSlot:
        {
            const int s = insertsSlot + (value > 0 ? 1 : -1);
            if (s >= 0 && s < slotCount(insertsChannel))
                insertsSlot = static_cast<uint8_t>(s);
            insertsPage = 0;
            break;
        }
        case kPlaceEffect:
        {
            if (outSlot)
                return;
            int idx = pickerIndexOf(slot.type) + (value > 0 ? 1 : -1);
            if (idx < 0 || idx >= insertfx::kTypeCount)
                break;
            insertfx::Slot defaults;
            insertfx::setDefaults(defaults, pickerOrder[idx]);
            slot.type = defaults.type;
            memcpy(slot.params, defaults.params, sizeof(slot.params));
            if (!engine.setInsertSlot(insertsChannel, insertsSlot, slot))
            {
                char text[40];
                snprintf(text, sizeof(text), "%d reverbs at most", insertfx::kReverbPool);
                mtPopups.show(0, "No free reverb", text);
                return;
            }
            insertsPage = 0;
            newFileManager.setProjectStructChanged();
            break;
        }
        case kPlacePage:
        {
            const int p = insertsPage + (value > 0 ? 1 : -1);
            if (p >= 0 && p < pageCount(d))
                insertsPage = static_cast<uint8_t>(p);
            break;
        }
        default:
        {
            if (outSlot)
            {
                if (!changeOutSlot(insertsChannel, selectedPlaceInserts - kPlaceFirstParam, value))
                    return;
                newFileManager.setProjectStructChanged();
                break;
            }
            const int param = insertsPage * kParamsPerPage + (selectedPlaceInserts - kPlaceFirstParam);
            if (param >= d.count)
                return;
            const insertfx::ParamDesc& p = d.p[param];
            // Wide ranges (0..127) follow the encoder's speed; short ones and lists move by one.
            const int step = p.max - p.min > 40 ? value : (value > 0 ? 1 : -1);
            int v = slot.params[param] + step;
            v = v < p.min ? p.min : v > p.max ? p.max : v;
            slot.params[param] = static_cast<int16_t>(v);
            engine.setInsertSlot(insertsChannel, insertsSlot, slot);
            newFileManager.setProjectStructChanged();
            break;
        }
    }
    showInsertsValues();
    display.synchronizeRefresh();
}

// Delete: on the effect, clears the slot; on a parameter, restores its default; on an Out slot
// control, routes back to the master or sets the bus to 0 dB.
void cMasterParams::insertsClear()
{
    if (insertsSlot >= kInsertSlots)
    {
        if (selectedPlaceInserts < kPlaceFirstParam ||
            !resetOutSlot(insertsChannel, selectedPlaceInserts - kPlaceFirstParam))
            return;
        newFileManager.setProjectStructChanged();
        showInsertsValues();
        display.synchronizeRefresh();
        return;
    }
    strInsertSlot slot = mtProject.values.insertSlotOf(insertsChannel, insertsSlot);
    if (selectedPlaceInserts == kPlaceEffect)
    {
        slot = strInsertSlot{};
    }
    else if (selectedPlaceInserts >= kPlaceFirstParam)
    {
        const insertfx::Desc& d = insertfx::describe(static_cast<insertfx::Type>(slot.type));
        const int param = insertsPage * kParamsPerPage + (selectedPlaceInserts - kPlaceFirstParam);
        if (param >= d.count)
            return;
        slot.params[param] = d.p[param].def;
    }
    else
    {
        return;
    }
    engine.setInsertSlot(insertsChannel, insertsSlot, slot);
    newFileManager.setProjectStructChanged();
    showInsertsValues();
    display.synchronizeRefresh();
}

namespace
{
void functInsertsButton(uint8_t button)
{
    masterParams.insertsSelect(button - interfaceButton0);
}

void setInsertsButtons()
{
    masterParams.ip->clearButtonsRange(interfaceButton0, interfaceButton7);
    masterParams.ip->clearButton(interfaceButtonShift);
    for (uint8_t b = interfaceButton0; b <= interfaceButton7; b++)
    {
        masterParams.ip->setButtonObj(b, buttonPress, functInsertsButton);
    }
}
} // namespace

#endif // FEATURE_REALTIME_FX
