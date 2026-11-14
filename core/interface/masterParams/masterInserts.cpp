// Master screen, Inserts page (openspec realtime-effects): the insert slots of every channel.

#include "masterParams/masterParams.h"

#if FEATURE_REALTIME_FX

#include <cr_section_macros.h>
#include <cstdio>

#include "core/interfacePopups.h"
#include "fileManager.h"
#include "fx/insertFx.h"
#include "mtAudioEngine.h"

namespace
{
constexpr int kParamsPerPage = 4;
constexpr uint8_t kPlaceChannel = 0, kPlaceSlot = 1, kPlaceEffect = 2, kPlacePage = 3,
                  kPlaceFirstParam = 4, kPlaces = 8;

__NOINIT(EXTERNAL_RAM) char labelText[kPlaces][24];
__NOINIT(EXTERNAL_RAM) char valueText[kPlaces][24];

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

const char* channelName(uint8_t ch, char* buf, size_t size)
{
    if (ch < kSampleTracks)
        snprintf(buf, size, "Track %d", ch + 1);
    else if (ch < kInsertChannels - 1)
        snprintf(buf, size, "Synth %d", ch - kSampleTracks + 1);
    else
        snprintf(buf, size, "Master");
    return buf;
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
        display.setControlHide(barControl[i]);
        display.refreshControl(barControl[i]);
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
    const strInsertSlot& slot = mtProject.values.insertSlot[insertsChannel][insertsSlot];
    const insertfx::Desc& d = insertfx::describe(static_cast<insertfx::Type>(slot.type));
    const int pages = pageCount(d);
    if (insertsPage >= pages)
        insertsPage = pages - 1;

    snprintf(labelText[kPlaceChannel], sizeof(labelText[0]), "Channel");
    channelName(insertsChannel, valueText[kPlaceChannel], sizeof(valueText[0]));
    snprintf(labelText[kPlaceSlot], sizeof(labelText[0]), "Slot");
    snprintf(valueText[kPlaceSlot], sizeof(valueText[0]), "%d", insertsSlot + 1);
    snprintf(labelText[kPlaceEffect], sizeof(labelText[0]), "Effect");
    snprintf(valueText[kPlaceEffect], sizeof(valueText[0]), "%s", d.name);
    snprintf(labelText[kPlacePage], sizeof(labelText[0]), "Page");
    snprintf(valueText[kPlacePage], sizeof(valueText[0]), "%d/%d", insertsPage + 1, pages);

    for (int i = 0; i < kParamsPerPage; i++)
    {
        const int param = insertsPage * kParamsPerPage + i;
        char* lt = labelText[kPlaceFirstParam + i];
        char* vt = valueText[kPlaceFirstParam + i];
        if (param < d.count)
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

    if (selectedPlaceInserts >= kPlaces)
        selectedPlaceInserts = kPlaceEffect;
    display.setControlValue(frameControl, selectedPlaceInserts);
    display.setControlShow(frameControl);
    display.refreshControl(frameControl);
}

void cMasterParams::insertsSelect(uint8_t place)
{
    if (place >= kPlaces)
        return;
    selectedPlaceInserts = place;
    showInsertsValues();
}

void cMasterParams::insertsMove(int8_t direction)
{
    const int place = static_cast<int>(selectedPlaceInserts) + direction;
    if (place >= 0 && place < kPlaces)
        insertsSelect(static_cast<uint8_t>(place));
}

void cMasterParams::insertsChange(int16_t value)
{
    strInsertSlot slot = mtProject.values.insertSlot[insertsChannel][insertsSlot];
    const insertfx::Desc& d = insertfx::describe(static_cast<insertfx::Type>(slot.type));

    switch (selectedPlaceInserts)
    {
        case kPlaceChannel:
        {
            const int ch = insertsChannel + (value > 0 ? 1 : -1);
            if (ch >= 0 && ch < kInsertChannels)
                insertsChannel = static_cast<uint8_t>(ch);
            insertsPage = 0;
            break;
        }
        case kPlaceSlot:
        {
            const int s = insertsSlot + (value > 0 ? 1 : -1);
            if (s >= 0 && s < kInsertSlots)
                insertsSlot = static_cast<uint8_t>(s);
            insertsPage = 0;
            break;
        }
        case kPlaceEffect:
        {
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

// Delete: on the effect, clears the slot; on a parameter, restores its default.
void cMasterParams::insertsClear()
{
    strInsertSlot slot = mtProject.values.insertSlot[insertsChannel][insertsSlot];
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
