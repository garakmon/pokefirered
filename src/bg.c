#include "global.h"
#include "bg.h"
#include "dma3.h"
#include "gpu_regs.h"

#define DISPCNT_ALL_BG_AND_MODE_BITS    (DISPCNT_BG_ALL_ON | 0x7)

struct BgControl
{
    struct BgConfig {
        u16 visible:1;
        u16 unknown_1:1;
        u16 screenSize:2;
        u16 priority:2;
        u16 mosaic:1;
        u16 wraparound:1;

        u16 charBaseIndex:2;
        u16 mapBaseIndex:5;
        u16 paletteMode:1;

        u8 unknown_2;
        u8 unknown_3;
    } configs[4];

    u16 bgVisibilityAndMode;
};

struct BgConfig2
{
    u32 baseTile:10;
    u32 basePalette:4;
    u32 unk_3:18;

    void* tilemap;
    u32 bg_x;
    u32 bg_y;
};

static struct BgControl sGpuBgConfigs;
static struct BgConfig2 sGpuBgConfigs2[4];
static u32 sDmaBusyBitfield[4];
static u8 gpu_tile_allocation_map_bg[0x100];

bool32 gWindowTileAutoAllocEnabled;

static const struct BgConfig sZeroedBgControlStruct = { 0 };

void ResetBgs(void)
{
    ResetBgControlStructs();
    sGpuBgConfigs.bgVisibilityAndMode = 0;
    SetTextModeAndHideBgs();
}

void SetBgModeInternal(u8 bgMode)
{
    sGpuBgConfigs.bgVisibilityAndMode &= 0xFFF8;
    sGpuBgConfigs.bgVisibilityAndMode |= bgMode;
}

u8 GetBgMode(void)
{
    return sGpuBgConfigs.bgVisibilityAndMode & 0x7;
}

void ResetBgControlStructs(void)
{
    struct BgConfig* bgConfigs = &sGpuBgConfigs.configs[0];
    struct BgConfig zeroedConfig = sZeroedBgControlStruct;
    int i;

    for (i = 0; i < 4; i++)
    {
        bgConfigs[i] = zeroedConfig;
    }
}

void Unused_ResetBgControlStruct(u8 bg)
{
    if (IsInvalidBg(bg) == FALSE)
    {
        sGpuBgConfigs.configs[bg] = sZeroedBgControlStruct;
    }
}

void SetBgControlAttributes(u8 bg, u8 charBaseIndex, u8 mapBaseIndex, u8 screenSize, u8 paletteMode, u8 priority, u8 mosaic, u8 wraparound)
{
    if (IsInvalidBg(bg) == FALSE)
    {
        if (charBaseIndex != 0xFF)
        {
            sGpuBgConfigs.configs[bg].charBaseIndex = charBaseIndex & 0x3;
        }

        if (mapBaseIndex != 0xFF)
        {
            sGpuBgConfigs.configs[bg].mapBaseIndex = mapBaseIndex & 0x1F;
        }

        if (screenSize != 0xFF)
        {
            sGpuBgConfigs.configs[bg].screenSize = screenSize & 0x3;
        }

        if (paletteMode != 0xFF)
        {
            sGpuBgConfigs.configs[bg].paletteMode = paletteMode;
        }

        if (priority != 0xFF)
        {
            sGpuBgConfigs.configs[bg].priority = priority & 0x3;
        }

        if (mosaic != 0xFF)
        {
            sGpuBgConfigs.configs[bg].mosaic = mosaic & 0x1;
        }

        if (wraparound != 0xFF)
        {
            sGpuBgConfigs.configs[bg].wraparound = wraparound;
        }

        sGpuBgConfigs.configs[bg].unknown_2 = 0;
        sGpuBgConfigs.configs[bg].unknown_3 = 0;

        sGpuBgConfigs.configs[bg].visible = 1;
    }
}

u16 GetBgControlAttribute(u8 bg, u8 attributeId)
{
    if (IsInvalidBg(bg) == FALSE && sGpuBgConfigs.configs[bg].visible != FALSE)
    {
        switch (attributeId)
        {
            case BG_CTRL_ATTR_VISIBLE:
                return sGpuBgConfigs.configs[bg].visible;
            case BG_CTRL_ATTR_CHARBASEINDEX:
                return sGpuBgConfigs.configs[bg].charBaseIndex;
            case BG_CTRL_ATTR_MAPBASEINDEX:
                return sGpuBgConfigs.configs[bg].mapBaseIndex;
            case BG_CTRL_ATTR_SCREENSIZE:
                return sGpuBgConfigs.configs[bg].screenSize;
            case BG_CTRL_ATTR_PALETTEMODE:
                return sGpuBgConfigs.configs[bg].paletteMode;
            case BG_CTRL_ATTR_PRIORITY:
                return sGpuBgConfigs.configs[bg].priority;
            case BG_CTRL_ATTR_MOSAIC:
                return sGpuBgConfigs.configs[bg].mosaic;
            case BG_CTRL_ATTR_WRAPAROUND:
                return sGpuBgConfigs.configs[bg].wraparound;
        }
    }

    return 0xFF;
}

u8 LoadBgVram(u8 bg, const void *src, u16 size, u16 destOffset, u8 mode)
{
    u16 offset;
    s8 cursor;

    if (IsInvalidBg(bg) == FALSE && sGpuBgConfigs.configs[bg].visible != FALSE)
    {
        switch (mode)
        {
            case 0x1:
                offset = sGpuBgConfigs.configs[bg].charBaseIndex * BG_CHAR_SIZE;
                break;
            case 0x2:
                offset = sGpuBgConfigs.configs[bg].mapBaseIndex * BG_SCREEN_SIZE;
                break;
            default:
                cursor = -1;
                goto end;
        }

        offset = destOffset + offset;

        cursor = RequestDma3Copy(src, (void*)(offset + BG_VRAM), size, 0);

        if (cursor == -1)
        {
            return -1;
        }
    }
    else
    {
       return -1;
    }

end:
    return cursor;
}

void ShowBgInternal(u8 bg)
{
    u16 value;
    if (IsInvalidBg(bg) == FALSE && sGpuBgConfigs.configs[bg].visible != FALSE)
    {
        value = sGpuBgConfigs.configs[bg].priority |
                (sGpuBgConfigs.configs[bg].charBaseIndex << 2) |
                (sGpuBgConfigs.configs[bg].mosaic << 6) |
                (sGpuBgConfigs.configs[bg].paletteMode << 7) |
                (sGpuBgConfigs.configs[bg].mapBaseIndex << 8) |
                (sGpuBgConfigs.configs[bg].wraparound << 13) |
                (sGpuBgConfigs.configs[bg].screenSize << 14);

        SetGpuReg((bg << 1) + 0x8, value);

        sGpuBgConfigs.bgVisibilityAndMode |= 1 << (bg + 8);
        sGpuBgConfigs.bgVisibilityAndMode &= DISPCNT_ALL_BG_AND_MODE_BITS;
    }
}

static void HideBgInternal(u8 bg)
{
    if (IsInvalidBg(bg) == FALSE)
    {
        sGpuBgConfigs.bgVisibilityAndMode &= ~(1 << (bg + 8));
        sGpuBgConfigs.bgVisibilityAndMode &= DISPCNT_ALL_BG_AND_MODE_BITS;
    }
}

static void SyncBgVisibilityAndMode(void)
{
    SetGpuReg(0, (GetGpuReg(0) & ~DISPCNT_ALL_BG_AND_MODE_BITS) | sGpuBgConfigs.bgVisibilityAndMode);
}

void SetTextModeAndHideBgs(void)
{
    SetGpuReg(0, GetGpuReg(0) & ~DISPCNT_ALL_BG_AND_MODE_BITS);
}

static void SetBgAffineInternal(u8 bg, u32 srcCenterX, u32 srcCenterY, s16 dispCenterX, s16 dispCenterY, s16 scaleX, s16 scaleY, u16 rotationAngle)
{
    struct BgAffineSrcData src;
    struct BgAffineDstData dest;

    switch (sGpuBgConfigs.bgVisibilityAndMode & 0x7)
    {
        case 1:
            if (bg != 2)
                return;
            break;
        case 2:
            if (bg < 2 || bg > 3)
                return;
            break;
        case 0:
        default:
            return;
    }

    src.texX = srcCenterX;
    src.texY = srcCenterY;
    src.scrX = dispCenterX;
    src.scrY = dispCenterY;
    src.sx = scaleX;
    src.sy = scaleY;
    src.alpha = rotationAngle;

    BgAffineSet(&src, &dest, 1);

    SetGpuReg(REG_OFFSET_BG2PA, dest.pa);
    SetGpuReg(REG_OFFSET_BG2PB, dest.pb);
    SetGpuReg(REG_OFFSET_BG2PC, dest.pc);
    SetGpuReg(REG_OFFSET_BG2PD, dest.pd);
    SetGpuReg(REG_OFFSET_BG2PA, dest.pa);
    SetGpuReg(REG_OFFSET_BG2X_L, (s16)(dest.dx));
    SetGpuReg(REG_OFFSET_BG2X_H, (s16)(dest.dx >> 16));
    SetGpuReg(REG_OFFSET_BG2Y_L, (s16)(dest.dy));
    SetGpuReg(REG_OFFSET_BG2Y_H, (s16)(dest.dy >> 16));
}

bool8 IsInvalidBg(u8 bg)
{
    if (bg > 3)
        return TRUE;
    return FALSE;
}

int BgTileAllocOp(int bg, int offset, int count, int mode)
{
    int start, end;
    int blockSize;
    int blockStart;
    int i;

    switch (mode)
    {
    case BG_TILE_FIND_FREE_SPACE:
        start = GetBgControlAttribute(bg, BG_CTRL_ATTR_CHARBASEINDEX) * (BG_CHAR_SIZE / TILE_SIZE_4BPP);
        end = start + 0x400;
        if (end > 0x800)
            end = 0x800;
        blockSize = 0;
        blockStart = 0;
        for (i = start, offset = 0; i < end; i++, offset++)
        {
            if (!((gpu_tile_allocation_map_bg[i / 8] >> (i % 8)) & 1))
            {
                if (blockSize)
                {
                    blockSize++;
                    if (blockSize == count)
                        return blockStart;
                }
                else
                {
                    blockStart = offset;
                    blockSize = 1;
                }
            }
            else
            {
                blockSize = 0;
            }
        }
        return -1;
    case BG_TILE_ALLOC:
        start = GetBgControlAttribute(bg, BG_CTRL_ATTR_CHARBASEINDEX) * (BG_CHAR_SIZE / TILE_SIZE_4BPP) + offset;
        end = start + count;
        for (i = start; i < end; i++)
            gpu_tile_allocation_map_bg[i / 8] |= 1 << (i % 8);
        break;
    case BG_TILE_FREE:
        start = GetBgControlAttribute(bg, BG_CTRL_ATTR_CHARBASEINDEX) * (BG_CHAR_SIZE / TILE_SIZE_4BPP) + offset;
        end = start + count;
        for (i = start; i < end; i++)
            gpu_tile_allocation_map_bg[i / 8] &= ~(1 << (i % 8));
        break;
    }

    return 0;
}

void ResetBgsAndClearDma3BusyFlags(bool32 enableWindowTileAutoAlloc)
{
    int i;
    ResetBgs();

    for (i = 0; i < 4; i++)
    {
        sDmaBusyBitfield[i] = 0;
    }

    gWindowTileAutoAllocEnabled = enableWindowTileAutoAlloc;

    for (i = 0; i < 0x100; i++)
    {
        gpu_tile_allocation_map_bg[i] = 0;
    }
}

#ifdef NONMATCHING
void InitBgsFromTemplates(u8 bgMode, const struct BgTemplate *templates, u8 numTemplates)
{
    int i;
    u8 bg;

    SetBgModeInternal(bgMode);
    ResetBgControlStructs();

    for (i = 0; i < numTemplates; i++)
    {
        bg = templates[i].bg;
        if (bg < 4) {
            SetBgControlAttributes(bg,
                                   templates[i].charBaseIndex,
                                   templates[i].mapBaseIndex,
                                   templates[i].screenSize,
                                   templates[i].paletteMode,
                                   templates[i].priority,
                                   0,
                                   0);

            sGpuBgConfigs2[bg].baseTile = templates[i].baseTile;
            sGpuBgConfigs2[bg].basePalette = 0;
            sGpuBgConfigs2[bg].unk_3 = 0;

            sGpuBgConfigs2[bg].tilemap = NULL;
            sGpuBgConfigs2[bg].bg_x = 0;
            sGpuBgConfigs2[bg].bg_y = 0;

            gpu_tile_allocation_map_bg[(templates[i].charBaseIndex * (BG_CHAR_SIZE / TILE_SIZE_4BPP)) / 8] = 1;
        }
    }
}
#else
NAKED
void InitBgsFromTemplates(u8 bgMode, const struct BgTemplate *templates, u8 numTemplates)
{
    asm(".syntax unified\n\
    push {r4-r7,lr}\n\
    mov r7, r10\n\
    mov r6, r9\n\
    mov r5, r8\n\
    push {r5-r7}\n\
    sub sp, 0x10\n\
    adds r5, r1, 0\n\
    lsls r0, 24\n\
    lsrs r0, 24\n\
    lsls r2, 24\n\
    lsrs r4, r2, 24\n\
    bl SetBgModeInternal\n\
    bl ResetBgControlStructs\n\
    cmp r4, 0\n\
    beq _08001712\n\
    movs r7, 0\n\
    ldr r0, _08001724 @ =sGpuBgConfigs2\n\
    mov r9, r0\n\
    adds r6, r5, 0\n\
    ldr r2, _08001728 @ =gpu_tile_allocation_map_bg\n\
    mov r10, r2\n\
    mov r8, r4\n\
_08001688:\n\
    ldr r4, [r6]\n\
    lsls r0, r4, 30\n\
    lsrs r5, r0, 30\n\
    cmp r5, 0x3\n\
    bhi _08001704\n\
    lsls r1, r4, 28\n\
    lsrs r1, 30\n\
    lsls r2, r4, 23\n\
    lsrs r2, 27\n\
    lsls r3, r4, 21\n\
    lsrs r3, 30\n\
    lsls r0, r4, 20\n\
    lsrs r0, 31\n\
    str r0, [sp]\n\
    lsls r0, r4, 18\n\
    lsrs r0, 30\n\
    str r0, [sp, 0x4]\n\
    str r7, [sp, 0x8]\n\
    str r7, [sp, 0xC]\n\
    adds r0, r5, 0\n\
    bl SetBgControlAttributes\n\
    lsls r4, r5, 4\n\
    mov r5, r9\n\
    adds r3, r4, r5\n\
    ldr r2, [r6]\n\
    lsls r2, 8\n\
    lsrs r2, 22\n\
    ldrh r0, [r3]\n\
    ldr r5, _0800172C @ =0xfffffc00\n\
    adds r1, r5, 0\n\
    ands r0, r1\n\
    orrs r0, r2\n\
    strh r0, [r3]\n\
    ldrb r0, [r3, 0x1]\n\
    movs r2, 0x3D\n\
    negs r2, r2\n\
    adds r1, r2, 0\n\
    ands r0, r1\n\
    strb r0, [r3, 0x1]\n\
    ldr r0, [r3]\n\
    ldr r1, _08001730 @ =0x00003fff\n\
    ands r0, r1\n\
    str r0, [r3]\n\
    mov r0, r9\n\
    adds r0, 0x4\n\
    adds r0, r4, r0\n\
    str r7, [r0]\n\
    mov r0, r9\n\
    adds r0, 0x8\n\
    adds r0, r4, r0\n\
    str r7, [r0]\n\
    ldr r5, _08001734 @ =sGpuBgConfigs2 + 0xC\n\
    adds r4, r5\n\
    str r7, [r4]\n\
    ldr r0, [r6]\n\
    lsls r0, 28\n\
    lsrs r0, 30\n\
    lsls r0, 6\n\
    add r0, r10\n\
    movs r1, 0x1\n\
    strb r1, [r0]\n\
_08001704:\n\
    adds r6, 0x4\n\
    movs r0, 0x1\n\
    negs r0, r0\n\
    add r8, r0\n\
    mov r2, r8\n\
    cmp r2, 0\n\
    bne _08001688\n\
_08001712:\n\
    add sp, 0x10\n\
    pop {r3-r5}\n\
    mov r8, r3\n\
    mov r9, r4\n\
    mov r10, r5\n\
    pop {r4-r7}\n\
    pop {r0}\n\
    bx r0\n\
    .align 2, 0\n\
_08001724: .4byte sGpuBgConfigs2\n\
_08001728: .4byte gpu_tile_allocation_map_bg\n\
_0800172C: .4byte 0xfffffc00\n\
_08001730: .4byte 0x00003fff\n\
_08001734: .4byte sGpuBgConfigs2 + 0xC\n\
.syntax divided");
}
#endif // NONMATCHING

void InitBgFromTemplate(const struct BgTemplate *template)
{
    u8 bg = template->bg;

    if (bg < 4)
    {
        SetBgControlAttributes(bg,
                               template->charBaseIndex,
                               template->mapBaseIndex,
                               template->screenSize,
                               template->paletteMode,
                               template->priority,
                               0,
                               0);

        sGpuBgConfigs2[bg].baseTile = template->baseTile;
        sGpuBgConfigs2[bg].basePalette = 0;
        sGpuBgConfigs2[bg].unk_3 = 0;

        sGpuBgConfigs2[bg].tilemap = NULL;
        sGpuBgConfigs2[bg].bg_x = 0;
        sGpuBgConfigs2[bg].bg_y = 0;

        gpu_tile_allocation_map_bg[(template->charBaseIndex * (BG_CHAR_SIZE / TILE_SIZE_4BPP)) / 8] = 1;
    }
}

u16 LoadBgTiles(u8 bg, const void* src, u16 size, u16 destOffset)
{
    u16 tileOffset;
    u8 cursor;

    if (GetBgControlAttribute(bg, BG_CTRL_ATTR_PALETTEMODE) == 0)
    {
        tileOffset = (sGpuBgConfigs2[bg].baseTile + destOffset) * 0x20;
    }
    else
    {
        tileOffset = (sGpuBgConfigs2[bg].baseTile + destOffset) * 0x40;
    }

    cursor = LoadBgVram(bg, src, size, tileOffset, DISPCNT_MODE_1);

    if (cursor == 0xFF)
    {
        return -1;
    }

    sDmaBusyBitfield[cursor / 0x20] |= (1 << (cursor % 0x20));

    if (gWindowTileAutoAllocEnabled == TRUE)
    {
        BgTileAllocOp(bg, tileOffset / 0x20, size / 0x20, BG_TILE_ALLOC);
    }

    return cursor;
}

u16 LoadBgTilemap(u8 bg, const void *src, u16 size, u16 destOffset)
{
    u8 cursor;

    cursor = LoadBgVram(bg, src, size, destOffset * 32, DISPCNT_MODE_2);

    if (cursor == 0xFF)
    {
        return -1;
    }

    sDmaBusyBitfield[cursor / 0x20] |= (1 << (cursor % 0x20));

    return cursor;
}

u16 Unused_LoadBgPalette(u8 bg, const void *src, u16 size, u16 destOffset)
{
    u16 paletteOffset;
    s8 cursor;

    if (IsInvalidBg32(bg) == FALSE)
    {
        paletteOffset = (sGpuBgConfigs2[bg].basePalette * 0x20) + (destOffset * 2);
        cursor = RequestDma3Copy(src, (void*)(paletteOffset + BG_PLTT), size, 0);

        if (cursor == -1)
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }

    sDmaBusyBitfield[cursor / 0x20] |= (1 << (cursor % 0x20));

    return (u8)cursor;
}

bool8 IsDma3ManagerBusyWithBgCopy(void)
{
    int i;

    for (i = 0; i < 0x80; i++)
    {
        u8 div = i / 0x20;
        u8 mod = i % 0x20;

        if ((sDmaBusyBitfield[div] & (1 << mod)))
        {
            s8 reqSpace = CheckForSpaceForDma3Request(i);
            if (reqSpace == -1)
                return TRUE;
            sDmaBusyBitfield[div] &= ~(1 << mod);
        }
    }
    return FALSE;
}

void ShowBg(u8 bg)
{
    ShowBgInternal(bg);
    SyncBgVisibilityAndMode();
}

void HideBg(u8 bg)
{
    HideBgInternal(bg);
    SyncBgVisibilityAndMode();
}

void SetBgAttribute(u8 bg, u8 attributeId, u8 value)
{
    switch (attributeId)
    {
        case 1:
            SetBgControlAttributes(bg, value, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
            break;
        case 2:
            SetBgControlAttributes(bg, 0xFF, value, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
            break;
        case 3:
            SetBgControlAttributes(bg, 0xFF, 0xFF, value, 0xFF, 0xFF, 0xFF, 0xFF);
            break;
        case 4:
            SetBgControlAttributes(bg, 0xFF, 0xFF, 0xFF, value, 0xFF, 0xFF, 0xFF);
            break;
        case 7:
            SetBgControlAttributes(bg, 0xFF, 0xFF, 0xFF, 0xFF, value, 0xFF, 0xFF);
            break;
        case 5:
            SetBgControlAttributes(bg, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, value, 0xFF);
            break;
        case 6:
            SetBgControlAttributes(bg, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, value);
            break;
    }
}

u16 GetBgAttribute(u8 bg, u8 attributeId)
{
    switch (attributeId)
    {
        case 1:
            return GetBgControlAttribute(bg, BG_CTRL_ATTR_CHARBASEINDEX);
        case 2:
            return GetBgControlAttribute(bg, BG_CTRL_ATTR_MAPBASEINDEX);
        case 3:
            return GetBgControlAttribute(bg, BG_CTRL_ATTR_SCREENSIZE);
        case 4:
            return GetBgControlAttribute(bg, BG_CTRL_ATTR_PALETTEMODE);
        case 7:
            return GetBgControlAttribute(bg, BG_CTRL_ATTR_PRIORITY);
        case 5:
            return GetBgControlAttribute(bg, BG_CTRL_ATTR_MOSAIC);
        case 6:
            return GetBgControlAttribute(bg, BG_CTRL_ATTR_WRAPAROUND);
        case 8:
            switch (GetBgType(bg))
            {
                case 0:
                    return GetBgMetricTextMode(bg, 0) * 0x800;
                case 1:
                    return GetBgMetricAffineMode(bg, 0) * 0x100;
                default:
                    return 0;
            }
        case 9:
            return GetBgType(bg);
        case 10:
            return sGpuBgConfigs2[bg].baseTile;
        default:
            return -1;
    }
}

u32 ChangeBgX(u8 bg, u32 value, u8 op)
{
    u8 mode;
    u16 temp1;
    u16 temp2;

    if (IsInvalidBg32(bg) != FALSE || GetBgControlAttribute(bg, BG_CTRL_ATTR_VISIBLE) == 0)
    {
        return -1;
    }

    switch (op)
    {
        case 0:
        default:
            sGpuBgConfigs2[bg].bg_x = value;
            break;
        case 1:
            sGpuBgConfigs2[bg].bg_x += value;
            break;
        case 2:
            sGpuBgConfigs2[bg].bg_x -= value;
            break;
    }

    mode = GetBgMode();

    switch (bg)
    {
        case 0:
            temp1 = sGpuBgConfigs2[0].bg_x >> 0x8;
            SetGpuReg(REG_OFFSET_BG0HOFS, temp1);
            break;
        case 1:
            temp1 = sGpuBgConfigs2[1].bg_x >> 0x8;
            SetGpuReg(REG_OFFSET_BG1HOFS, temp1);
            break;
        case 2:
            if (mode == 0)
            {
                temp1 = sGpuBgConfigs2[2].bg_x >> 0x8;
                SetGpuReg(REG_OFFSET_BG2HOFS, temp1);
            }
            else
            {
                temp1 = sGpuBgConfigs2[2].bg_x >> 0x10;
                temp2 = sGpuBgConfigs2[2].bg_x & 0xFFFF;
                SetGpuReg(REG_OFFSET_BG2X_H, temp1);
                SetGpuReg(REG_OFFSET_BG2X_L, temp2);
            }
            break;
        case 3:
            if (mode == 0)
            {
                temp1 = sGpuBgConfigs2[3].bg_x >> 0x8;
                SetGpuReg(REG_OFFSET_BG3HOFS, temp1);
            }
            else if (mode == 2)
            {
                temp1 = sGpuBgConfigs2[3].bg_x >> 0x10;
                temp2 = sGpuBgConfigs2[3].bg_x & 0xFFFF;
                SetGpuReg(REG_OFFSET_BG3X_H, temp1);
                SetGpuReg(REG_OFFSET_BG3X_L, temp2);
            }
            break;
    }

    return sGpuBgConfigs2[bg].bg_x;
}

u32 GetBgX(u8 bg)
{
    if (IsInvalidBg32(bg) != FALSE)
        return -1;
    if (GetBgControlAttribute(bg, BG_CTRL_ATTR_VISIBLE) == 0)
        return -1;
    return sGpuBgConfigs2[bg].bg_x;
}

u32 ChangeBgY(u8 bg, u32 value, u8 op)
{
    u8 mode;
    u16 temp1;
    u16 temp2;

    if (IsInvalidBg32(bg) != FALSE || GetBgControlAttribute(bg, BG_CTRL_ATTR_VISIBLE) == 0)
    {
        return -1;
    }

    switch (op)
    {
        case 0:
        default:
            sGpuBgConfigs2[bg].bg_y = value;
            break;
        case 1:
            sGpuBgConfigs2[bg].bg_y += value;
            break;
        case 2:
            sGpuBgConfigs2[bg].bg_y -= value;
            break;
    }

    mode = GetBgMode();

    switch (bg)
    {
        case 0:
            temp1 = sGpuBgConfigs2[0].bg_y >> 0x8;
            SetGpuReg(REG_OFFSET_BG0VOFS, temp1);
            break;
        case 1:
            temp1 = sGpuBgConfigs2[1].bg_y >> 0x8;
            SetGpuReg(REG_OFFSET_BG1VOFS, temp1);
            break;
        case 2:
            if (mode == 0)
            {
                temp1 = sGpuBgConfigs2[2].bg_y >> 0x8;
                SetGpuReg(REG_OFFSET_BG2VOFS, temp1);
            }
            else
            {
                temp1 = sGpuBgConfigs2[2].bg_y >> 0x10;
                temp2 = sGpuBgConfigs2[2].bg_y & 0xFFFF;
                SetGpuReg(REG_OFFSET_BG2Y_H, temp1);
                SetGpuReg(REG_OFFSET_BG2Y_L, temp2);
            }
            break;
        case 3:
            if (mode == 0)
            {
                temp1 = sGpuBgConfigs2[3].bg_y >> 0x8;
                SetGpuReg(REG_OFFSET_BG3VOFS, temp1);
            }
            else if (mode == 2)
            {
                temp1 = sGpuBgConfigs2[3].bg_y >> 0x10;
                temp2 = sGpuBgConfigs2[3].bg_y & 0xFFFF;
                SetGpuReg(REG_OFFSET_BG3Y_H, temp1);
                SetGpuReg(REG_OFFSET_BG3Y_L, temp2);
            }
            break;
    }

    return sGpuBgConfigs2[bg].bg_y;
}

u32 GetBgY(u8 bg)
{
    if (IsInvalidBg32(bg) != FALSE)
        return -1;
    if (GetBgControlAttribute(bg, BG_CTRL_ATTR_VISIBLE) == 0)
        return -1;
    return sGpuBgConfigs2[bg].bg_y;
}

void SetBgAffine(u8 bg, u32 srcCenterX, u32 srcCenterY, s16 dispCenterX, s16 dispCenterY, s16 scaleX, s16 scaleY, u16 rotationAngle)
{
    SetBgAffineInternal(bg, srcCenterX, srcCenterY, dispCenterX, dispCenterY, scaleX, scaleY, rotationAngle);
}

u8 AdjustBgMosaic(u8 value, u8 mode)
{
    u16 mosaicSize;
    s16 bgMosaicH;
    s16 bgMosaicV;
    mosaicSize = GetGpuReg(REG_OFFSET_MOSAIC);
    bgMosaicH = mosaicSize & 0xF;
    bgMosaicV = (mosaicSize >> 4) & 0xF;
    mosaicSize &= 0xFF00;

    switch (mode)
    {
    case BG_MOSAIC_SET:
    default:
        bgMosaicH = value & 0xF;
        bgMosaicV = value >> 0x4;
        break;
    case BG_MOSAIC_SET_H:
        bgMosaicH = value & 0xF;
        break;
    case BG_MOSAIC_INC_H:
        if ((bgMosaicH + value) > 0xF)
            bgMosaicH = 0xF;
        else
            bgMosaicH += value;
        break;
    case BG_MOSAIC_DEC_H:
        if ((bgMosaicH - value) < 0)
            bgMosaicH = 0x0;
        else
            bgMosaicH -= value;
        break;
    case BG_MOSAIC_SET_V:
        bgMosaicV = value & 0xF;
        break;
    case BG_MOSAIC_INC_V:
        if ((bgMosaicV + value) > 0xF)
            bgMosaicV = 0xF;
        else
            bgMosaicV += value;
        break;
    case BG_MOSAIC_DEC_V:
        if ((bgMosaicV - value) < 0)
            bgMosaicV = 0x0;
        else
            bgMosaicV -= value;
        break;
    }
    mosaicSize |= ((bgMosaicV << 0x4) & 0xF0);
    mosaicSize |= (bgMosaicH & 0xF);
    SetGpuReg(REG_OFFSET_MOSAIC, mosaicSize);
    return mosaicSize;
}

void SetBgTilemapBuffer(u8 bg, void *tilemap)
{
    if (IsInvalidBg32(bg) == FALSE && GetBgControlAttribute(bg, BG_CTRL_ATTR_VISIBLE) != 0x0)
    {
        sGpuBgConfigs2[bg].tilemap = tilemap;
    }
}

void UnsetBgTilemapBuffer(u8 bg)
{
    if (IsInvalidBg32(bg) == FALSE && GetBgControlAttribute(bg, BG_CTRL_ATTR_VISIBLE) != 0x0)
    {
        sGpuBgConfigs2[bg].tilemap = NULL;
    }
}

void* GetBgTilemapBuffer(u8 bg)
{
    if (IsInvalidBg32(bg) != FALSE)
        return NULL;
    if (GetBgControlAttribute(bg, BG_CTRL_ATTR_VISIBLE) == 0)
        return NULL;
    return sGpuBgConfigs2[bg].tilemap;
}

void CopyToBgTilemapBuffer(u8 bg, const void *src, u16 mode, u16 destOffset)
{
    if (IsInvalidBg32(bg) == FALSE && IsTileMapOutsideWram(bg) == FALSE)
    {
        if (mode != 0)
        {
            CpuCopy16(src, (void *)(sGpuBgConfigs2[bg].tilemap + (destOffset * 32)), mode);
        }
        else
        {
            LZ77UnCompWram(src, (void *)(sGpuBgConfigs2[bg].tilemap + (destOffset * 32)));
        }
    }
}

void CopyBgTilemapBufferToVram(u8 bg)
{
    u16 sizeToLoad;

    if (IsInvalidBg32(bg) == FALSE && IsTileMapOutsideWram(bg) == FALSE)
    {
        switch (GetBgType(bg))
        {
            case 0:
                sizeToLoad = GetBgMetricTextMode(bg, 0) * 0x800;
                break;
            case 1:
                sizeToLoad = GetBgMetricAffineMode(bg, 0) * 0x100;
                break;
            default:
                sizeToLoad = 0;
                break;
        }
        LoadBgVram(bg, sGpuBgConfigs2[bg].tilemap, sizeToLoad, 0, 2);
    }
}

void CopyToBgTilemapBufferRect(u8 bg, const void* src, u8 destX, u8 destY, u8 width, u8 height)
{
    u16 destX16;
    u16 destY16;
    u16 mode;

    if (IsInvalidBg32(bg) == FALSE && IsTileMapOutsideWram(bg) == FALSE)
    {
        switch (GetBgType(bg))
        {
            case 0:
            {
                const u16 * srcCopy = src;
                for (destY16 = destY; destY16 < (destY + height); destY16++)
                {
                    for (destX16 = destX; destX16 < (destX + width); destX16++)
                    {
                        ((u16*)sGpuBgConfigs2[bg].tilemap)[((destY16 * 0x20) + destX16)] = *(srcCopy)++;
                    }
                }
                break;
            }
            case 1:
            {
                const u8 * srcCopy = src;
                mode = GetBgMetricAffineMode(bg, 0x1);
                for (destY16 = destY; destY16 < (destY + height); destY16++)
                {
                    for (destX16 = destX; destX16 < (destX + width); destX16++)
                    {
                        ((u8*)sGpuBgConfigs2[bg].tilemap)[((destY16 * mode) + destX16)] = *(srcCopy)++;
                    }
                }
                break;
            }
        }
    }
}

void CopyToBgTilemapBufferRect_ChangePalette(u8 bg, const void *src, u8 destX, u8 destY, u8 rectWidth, u8 rectHeight, u8 palette)
{
    CopyRectToBgTilemapBufferRect(bg, src, 0, 0, rectWidth, rectHeight, destX, destY, rectWidth, rectHeight, palette, 0, 0);
}
// Skipping for now, it probably uses structs passed by value
/*
void CopyRectToBgTilemapBufferRect(u8 bg, void* src, u8 srcX, u8 srcY, u8 srcWidth, u8 srcHeight, u8 destX, u8 destY, u8 rectWidth, u8 rectHeight, u8 palette1, u16 tileOffset, u16 palette2)
{
    u16 attribute;
    u16 mode;
    u16 mode2;

    void* srcCopy;
    u16 destX16;
    u16 destY16;

    if (IsInvalidBg32(bg) == FALSE && IsTileMapOutsideWram(bg) == FALSE)
    {
        attribute = GetBgControlAttribute(bg, BG_CTRL_ATTR_SCREENSIZE);
        mode = GetBgMetricTextMode(bg, 0x1) * 0x20;
        mode2 = GetBgMetricTextMode(bg, 0x2) * 0x20;
        switch (GetBgType(bg))
        {
            case 0:
                srcCopy = src;
                for (destY16 = destY; destY16 < (destY + rectHeight); destY16++)
                {
                    for (destX16 = destX; destX16 < (destX + rectWidth); destX16++)
                    {
                        CopyTileMapEntry(&((u16*)srcCopy)[(srcY * rectWidth) + srcX], &((u16*)sGpuBgConfigs2[bg].tilemap)[GetTileMapIndexFromCoords(destX16, destY16, attribute, mode, mode2)], palette1, tileOffset, palette2);
                    }
                }
                break;
            case 1:
                srcCopy = src;
                mode = GetBgMetricAffineMode(bg, 0x1);
                for (destY16 = destY; destY16 < (destY + rectHeight); destY16++)
                {
                    for (destX16 = destX; destX16 < (destX + rectWidth); destX16++)
                    {
                        CopyTileMapEntry(&((u16*)srcCopy)[(srcY * rectWidth) + srcX], &((u16*)sGpuBgConfigs2[bg].tilemap)[GetTileMapIndexFromCoords(destX16, destY16, attribute, mode, mode2)], palette1, tileOffset, palette2);
                    }
                }
                break;
        }
    }
}*/
NAKED
void CopyRectToBgTilemapBufferRect(u8 bg, const void* src, u8 srcX, u8 srcY, u8 srcWidth, u8 srcHeight, u8 destX, u8 destY, u8 rectWidth, u8 rectHeight, u8 palette1, u16 tileOffset, u16 palette2)
{
    asm("push {r4-r7,lr}\n\
    mov r7, r10\n\
    mov r6, r9\n\
    mov r5, r8\n\
    push {r5-r7}\n\
    sub sp, #0x40\n\
    str r1, [sp, #0x8]\n\
    ldr r1, [sp, #0x60]\n\
    ldr r4, [sp, #0x68]\n\
    ldr r5, [sp, #0x6C]\n\
    ldr r6, [sp, #0x70]\n\
    ldr r7, [sp, #0x74]\n\
    mov r8, r7\n\
    ldr r7, [sp, #0x78]\n\
    mov r9, r7\n\
    ldr r7, [sp, #0x7C]\n\
    mov r10, r7\n\
    ldr r7, [sp, #0x80]\n\
    mov r12, r7\n\
    lsl r0, #24\n\
    lsr r0, #24\n\
    str r0, [sp, #0x4]\n\
    lsl r2, #24\n\
    lsr r2, #24\n\
    str r2, [sp, #0xC]\n\
    lsl r3, #24\n\
    lsr r3, #24\n\
    str r3, [sp, #0x10]\n\
    lsl r1, #24\n\
    lsr r7, r1, #24\n\
    lsl r4, #24\n\
    lsr r4, #24\n\
    str r4, [sp, #0x14]\n\
    lsl r5, #24\n\
    lsr r5, #24\n\
    lsl r6, #24\n\
    lsr r6, #24\n\
    str r6, [sp, #0x18]\n\
    mov r0, r8\n\
    lsl r0, #24\n\
    lsr r4, r0, #24\n\
    mov r1, r9\n\
    lsl r1, #24\n\
    lsr r1, #24\n\
    str r1, [sp, #0x1C]\n\
    mov r2, r10\n\
    lsl r2, #16\n\
    lsr r2, #16\n\
    str r2, [sp, #0x20]\n\
    mov r0, r12\n\
    lsl r0, #16\n\
    lsr r0, #16\n\
    str r0, [sp, #0x24]\n\
    ldr r0, [sp, #0x4]\n\
    bl IsInvalidBg32\n\
    cmp r0, #0\n\
    beq _08002592\n\
    b _080026EE\n\
_08002592:\n\
    ldr r0, [sp, #0x4]\n\
    bl IsTileMapOutsideWram\n\
    cmp r0, #0\n\
    beq _0800259E\n\
    b _080026EE\n\
_0800259E:\n\
    ldr r0, [sp, #0x4]\n\
    mov r1, #0x4\n\
    bl GetBgControlAttribute\n\
    lsl r0, #16\n\
    lsr r0, #16\n\
    str r0, [sp, #0x30]\n\
    ldr r0, [sp, #0x4]\n\
    mov r1, #0x1\n\
    bl GetBgMetricTextMode\n\
    lsl r0, #21\n\
    lsr r0, #16\n\
    str r0, [sp, #0x28]\n\
    ldr r0, [sp, #0x4]\n\
    mov r1, #0x2\n\
    bl GetBgMetricTextMode\n\
    lsl r0, #21\n\
    lsr r0, #16\n\
    str r0, [sp, #0x2C]\n\
    ldr r0, [sp, #0x4]\n\
    bl GetBgType\n\
    cmp r0, #0\n\
    beq _080025D8\n\
    cmp r0, #0x1\n\
    beq _08002674\n\
    b _080026EE\n\
_080025D8:\n\
    ldr r1, [sp, #0x10]\n\
    add r0, r1, #0\n\
    mul r0, r7\n\
    ldr r2, [sp, #0xC]\n\
    add r0, r2\n\
    lsl r0, #1\n\
    ldr r1, [sp, #0x8]\n\
    add r6, r1, r0\n\
    add r0, r5, r4\n\
    cmp r5, r0\n\
    blt _080025F0\n\
    b _080026EE\n\
_080025F0:\n\
    ldr r2, [sp, #0x18]\n\
    sub r2, r7, r2\n\
    str r2, [sp, #0x34]\n\
    str r0, [sp, #0x38]\n\
_080025F8:\n\
    ldr r4, [sp, #0x14]\n\
    ldr r7, [sp, #0x18]\n\
    add r0, r4, r7\n\
    add r1, r5, #0x1\n\
    str r1, [sp, #0x3C]\n\
    cmp r4, r0\n\
    bge _0800265A\n\
    ldr r2, [sp, #0x4]\n\
    lsl r0, r2, #4\n\
    ldr r1, =sGpuBgConfigs2+4\n\
    add r0, r1\n\
    mov r10, r0\n\
    ldr r7, [sp, #0x20]\n\
    lsl r7, #16\n\
    mov r9, r7\n\
    ldr r1, [sp, #0x24]\n\
    lsl r0, r1, #16\n\
    asr r0, #16\n\
    mov r8, r0\n\
_0800261E:\n\
    ldr r2, [sp, #0x2C]\n\
    str r2, [sp]\n\
    add r0, r4, #0\n\
    add r1, r5, #0\n\
    ldr r2, [sp, #0x30]\n\
    ldr r3, [sp, #0x28]\n\
    bl GetTileMapIndexFromCoords\n\
    lsl r0, #16\n\
    lsr r0, #15\n\
    mov r7, r10\n\
    ldr r1, [r7]\n\
    add r1, r0\n\
    mov r0, r8\n\
    str r0, [sp]\n\
    add r0, r6, #0\n\
    ldr r2, [sp, #0x1C]\n\
    mov r7, r9\n\
    asr r3, r7, #16\n\
    bl CopyTileMapEntry\n\
    add r6, #0x2\n\
    add r0, r4, #0x1\n\
    lsl r0, #16\n\
    lsr r4, r0, #16\n\
    ldr r1, [sp, #0x14]\n\
    ldr r2, [sp, #0x18]\n\
    add r0, r1, r2\n\
    cmp r4, r0\n\
    blt _0800261E\n\
_0800265A:\n\
    ldr r5, [sp, #0x34]\n\
    lsl r0, r5, #1\n\
    add r6, r0\n\
    ldr r7, [sp, #0x3C]\n\
    lsl r0, r7, #16\n\
    lsr r5, r0, #16\n\
    ldr r0, [sp, #0x38]\n\
    cmp r5, r0\n\
    blt _080025F8\n\
    b _080026EE\n\
    .pool\n\
_08002674:\n\
    ldr r1, [sp, #0x10]\n\
    add r0, r1, #0\n\
    mul r0, r7\n\
    ldr r2, [sp, #0xC]\n\
    add r0, r2\n\
    ldr r1, [sp, #0x8]\n\
    add r6, r1, r0\n\
    ldr r0, [sp, #0x4]\n\
    mov r1, #0x1\n\
    bl GetBgMetricAffineMode\n\
    lsl r0, #16\n\
    lsr r0, #16\n\
    mov r9, r0\n\
    add r0, r5, r4\n\
    cmp r5, r0\n\
    bge _080026EE\n\
    ldr r2, [sp, #0x18]\n\
    sub r2, r7, r2\n\
    str r2, [sp, #0x34]\n\
    str r0, [sp, #0x38]\n\
    ldr r7, =sGpuBgConfigs2+4\n\
    mov r10, r7\n\
    ldr r0, [sp, #0x4]\n\
    lsl r0, #4\n\
    mov r8, r0\n\
_080026A8:\n\
    ldr r4, [sp, #0x14]\n\
    ldr r1, [sp, #0x18]\n\
    add r0, r4, r1\n\
    add r2, r5, #0x1\n\
    str r2, [sp, #0x3C]\n\
    cmp r4, r0\n\
    bge _080026DE\n\
    mov r3, r8\n\
    add r3, r10\n\
    mov r7, r9\n\
    mul r7, r5\n\
    mov r12, r7\n\
    add r2, r0, #0\n\
_080026C2:\n\
    ldr r1, [r3]\n\
    mov r5, r12\n\
    add r0, r5, r4\n\
    add r1, r0\n\
    ldrb r0, [r6]\n\
    ldr r7, [sp, #0x20]\n\
    add r0, r7\n\
    strb r0, [r1]\n\
    add r6, #0x1\n\
    add r0, r4, #0x1\n\
    lsl r0, #16\n\
    lsr r4, r0, #16\n\
    cmp r4, r2\n\
    blt _080026C2\n\
_080026DE:\n\
    ldr r0, [sp, #0x34]\n\
    add r6, r0\n\
    ldr r1, [sp, #0x3C]\n\
    lsl r0, r1, #16\n\
    lsr r5, r0, #16\n\
    ldr r2, [sp, #0x38]\n\
    cmp r5, r2\n\
    blt _080026A8\n\
_080026EE:\n\
    add sp, #0x40\n\
    pop {r3-r5}\n\
    mov r8, r3\n\
    mov r9, r4\n\
    mov r10, r5\n\
    pop {r4-r7}\n\
    pop {r0}\n\
    bx r0\n\
    .pool\n");
}

void FillBgTilemapBufferRect_Palette0(u8 bg, u16 tileNum, u8 x, u8 y, u8 width, u8 height)
{
    u16 x16;
    u16 y16;
    u16 mode;

    if (IsInvalidBg32(bg) == FALSE && IsTileMapOutsideWram(bg) == FALSE)
    {
        switch (GetBgType(bg))
        {
            case 0:
                for (y16 = y; y16 < (y + height); y16++)
                {
                    for (x16 = x; x16 < (x + width); x16++)
                    {
                        ((u16*)sGpuBgConfigs2[bg].tilemap)[((y16 * 0x20) + x16)] = tileNum;
                    }
                }
                break;
            case 1:
                mode = GetBgMetricAffineMode(bg, 0x1);
                for (y16 = y; y16 < (y + height); y16++)
                {
                    for (x16 = x; x16 < (x + width); x16++)
                    {
                        ((u8*)sGpuBgConfigs2[bg].tilemap)[((y16 * mode) + x16)] = tileNum;
                    }
                }
                break;
        }
    }
}

void FillBgTilemapBufferRect(u8 bg, u16 tileNum, u8 x, u8 y, u8 width, u8 height, u8 palette)
{
    WriteSequenceToBgTilemapBuffer(bg, tileNum, x, y, width, height, palette, 0);
}

void WriteSequenceToBgTilemapBuffer(u8 bg, u16 firstTileNum, u8 x, u8 y, u8 width, u8 height, u8 paletteSlot, s16 tileNumDelta)
{
    u16 mode;
    u16 mode2;
    u16 attribute;
    u16 mode3;

    u16 x16;
    u16 y16;

    if (IsInvalidBg32(bg) == FALSE && IsTileMapOutsideWram(bg) == FALSE)
    {
        attribute = GetBgControlAttribute(bg, BG_CTRL_ATTR_SCREENSIZE);
        mode = GetBgMetricTextMode(bg, 0x1) * 0x20;
        mode2 = GetBgMetricTextMode(bg, 0x2) * 0x20;
        switch (GetBgType(bg))
        {
            case 0:
                for (y16 = y; y16 < (y + height); y16++)
                {
                    for (x16 = x; x16 < (x + width); x16++)
                    {
                        CopyTileMapEntry(&firstTileNum, &((u16*)sGpuBgConfigs2[bg].tilemap)[(u16)GetTileMapIndexFromCoords(x16, y16, attribute, mode, mode2)], paletteSlot, 0, 0);
                        firstTileNum = (firstTileNum & 0xFC00) + ((firstTileNum + tileNumDelta) & 0x3FF);
                    }
                }
                break;
            case 1:
                mode3 = GetBgMetricAffineMode(bg, 0x1);
                for (y16 = y; y16 < (y + height); y16++)
                {
                    for (x16 = x; x16 < (x + width); x16++)
                    {
                        ((u8*)sGpuBgConfigs2[bg].tilemap)[(y16 * mode3) + x16] = firstTileNum;
                        firstTileNum = (firstTileNum & 0xFC00) + ((firstTileNum + tileNumDelta) & 0x3FF);
                    }
                }
                break;
        }
    }
}

u16 GetBgMetricTextMode(u8 bg, u8 whichMetric)
{
    u8 attribute;

    attribute = GetBgControlAttribute(bg, BG_CTRL_ATTR_SCREENSIZE);

    switch (whichMetric)
    {
        case 0:
            switch (attribute)
            {
                case 0:
                    return 1;
                case 1:
                case 2:
                    return 2;
                case 3:
                    return 4;
            }
            break;
        case 1:
            switch (attribute)
            {
                case 0:
                    return 1;
                case 1:
                    return 2;
                case 2:
                    return 1;
                case 3:
                    return 2;
            }
            break;
        case 2:
            switch (attribute)
            {
                case 0:
                case 1:
                    return 1;
                case 2:
                case 3:
                    return 2;
            }
            break;
    }
    return 0;
}

u32 GetBgMetricAffineMode(u8 bg, u8 whichMetric)
{
    u8 attribute;

    attribute = GetBgControlAttribute(bg, BG_CTRL_ATTR_SCREENSIZE);

    switch (whichMetric)
    {
        case 0:
            switch (attribute)
            {
                case 0:
                    return 0x1;
                case 1:
                    return 0x4;
                case 2:
                    return 0x10;
                case 3:
                    return 0x40;
            }
            break;
        case 1:
        case 2:
            return 0x10 << attribute;
    }
    return 0;
}

u32 GetTileMapIndexFromCoords(s32 x, s32 y, s32 screenSize, u32 screenWidth, u32 screenHeight)
{
    x = x & (screenWidth - 1);
    y = y & (screenHeight - 1);

    switch (screenSize)
    {
        case 0:
        case 2:
            break;
        case 3:
            if (y >= 0x20)
                y += 0x20;
        case 1:
            if (x >= 0x20)
            {
                x -= 0x20;
                y += 0x20;
            }
    }
    return (y * 0x20) + x;
}

#ifdef NONMATCHING // This one has some weird switch statement cases that refuse to cooperate
void CopyTileMapEntry(u16 *src, u16 *dest, s32 palette1, u32 tileOffset, u32 palette2)
{
    u16 test;
    switch (palette1)
    {
        default:
            if (palette1 > 0x10 || palette1 < 0)
                test = *src + tileOffset + (palette2 << 12);
            else
                test = ((*src + tileOffset) & 0xFFF) + ((palette1 + palette2) << 12);
            break;
        case 0x10:
            test = ((*dest & 0xFC00) + (palette2 << 12)) | ((*src + tileOffset) & 0x3FF);
            break;
    }

    *dest = test;
}
#else
NAKED
void CopyTileMapEntry(u16 *src, u16 *dest, s32 palette1, u32 tileOffset, u32 palette2)
{
    asm("push {r4-r6,lr}\n\
    add r4, r0, #0\n\
    add r6, r1, #0\n\
    ldr r5, [sp, #0x10]\n\
    cmp r2, #0x10\n\
    beq _08002B14\n\
    cmp r2, #0x10\n\
    bgt _08002B34\n\
    cmp r2, #0\n\
    blt _08002B34\n\
    ldrh r0, [r4]\n\
    add r0, r3\n\
    ldr r3, =0x00000fff\n\
    add r1, r3, #0\n\
    and r0, r1\n\
    add r1, r2, r5\n\
    lsl r1, #12\n\
    b _08002B3A\n\
    .pool\n\
_08002B14:\n\
    ldrh r1, [r6]\n\
    mov r0, #0xFC\n\
    lsl r0, #8\n\
    and r1, r0\n\
    lsl r2, r5, #12\n\
    add r2, r1, r2\n\
    ldrh r0, [r4]\n\
    add r0, r3\n\
    ldr r3, =0x000003ff\n\
    add r1, r3, #0\n\
    and r0, r1\n\
    orr r0, r2\n\
    b _08002B3C\n\
    .pool\n\
_08002B34:\n\
    ldrh r0, [r4]\n\
    add r0, r3\n\
    lsl r1, r5, #12\n\
_08002B3A:\n\
    add r0, r1\n\
_08002B3C:\n\
    lsl r0, #16\n\
    lsr r1, r0, #16\n\
    strh r1, [r6]\n\
    pop {r4-r6}\n\
    pop {r0}\n\
    bx r0\n");
}
#endif // NONMATCHING

u32 GetBgType(u8 bg)
{
    u8 mode;

    mode = GetBgMode();


    switch (bg)
    {
        case 0:
        case 1:
            switch (mode)
            {
                case 0:
                case 1:
                    return 0;
            }
            break;
        case 2:
            switch (mode)
            {
                case 0:
                    return 0;
                case 1:
                case 2:
                    return 1;
            }
            break;
        case 3:
            switch (mode)
            {
                case 0:
                    return 0;
                case 2:
                    return 1;
            }
            break;
    }

    return 0xFFFF;
}

bool32 IsInvalidBg32(u8 bg)
{
    if (bg > 3)
        return TRUE;
    return FALSE;
}

bool32 IsTileMapOutsideWram(u8 bg)
{
    if (sGpuBgConfigs2[bg].tilemap > (void*)IWRAM_END)
        return TRUE;
    if (sGpuBgConfigs2[bg].tilemap == 0x0)
        return TRUE;
    return FALSE;
}
