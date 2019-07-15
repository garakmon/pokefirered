#include "global.h"
#include "bike.h"
#include "script.h"
#include "task.h"
#include "field_player_avatar.h"
#include "field_map_obj.h"
#include "field_map_obj_helpers.h"
#include "field_effect.h"
#include "field_effect_helpers.h"
#include "quest_log.h"

void sub_81504A8(void);
void sub_81504E8(void);
void sub_8150530(void);
void sub_81505C4(u8 taskId);
void sub_8150708(void);
void sub_815077C(void);
void sub_8150794(u8 taskId);
void sub_81507CC(void);
void sub_81507D8(void);
void sub_81507E4(void);
void sub_81507F0(void);
void sub_81507BC(struct MapObject *a0, u8 a1);

void (*const gUnknown_8471EDC[])(void) = {
    sub_81504A8,
    sub_81504E8,
    sub_8150530,
    sub_8150708,
    sub_81507CC,
    sub_81507D8,
    sub_81507E4,
    sub_81507F0,
    sub_815077C
};

void sub_8150454(u8 a0)
{
    if (a0 < ARRAY_COUNT(gUnknown_8471EDC))
        gUnknown_8471EDC[a0]();
}

bool8 sub_8150474(u8 a0)
{
    if (gUnknown_3005E88 == 2)
    {
        sub_8112720(a0);
        return TRUE;
    }
    return FALSE;
}

void sub_8150498(u8 a0)
{
    return sub_8150454(a0);
}

void sub_81504A8(void)
{
    struct MapObject *mapObject = &gMapObjects[gPlayerAvatar.mapObjectId];
    sub_81507BC(mapObject, sub_805C808(0));
    FieldObjectTurn(mapObject, mapObject->placeholder18);
    SetPlayerAvatarStateMask(0x01);
}

void sub_81504E8(void)
{
    struct MapObject *mapObject = &gMapObjects[gPlayerAvatar.mapObjectId];
    sub_81507BC(mapObject, sub_805C808(1));
    FieldObjectTurn(mapObject, mapObject->placeholder18);
    SetPlayerAvatarStateMask(0x02);
    sub_80BD620(0, 0);
}

void sub_8150530(void)
{
    struct MapObject *mapObject = &gMapObjects[gPlayerAvatar.mapObjectId];
    struct Sprite *sprite = &gSprites[mapObject->spriteId];

    if (gUnknown_3005E88 == 1 || gUnknown_3005E88 == 3)
    {
        u8 taskId;
        ScriptContext2_Enable();
        gPlayerAvatar.unk6 = 1;
        taskId = CreateTask(sub_81505C4, 0xFF);
        gTasks[taskId].data[0] = 0;
    }
    else
    {
        sub_81507BC(mapObject, sub_805C808(4));
        StartSpriteAnim(sprite, sub_80634F0(mapObject->mapobj_unk_18));
    }
}

void sub_81505C4(u8 taskId)
{
    struct MapObject *mapObject = &gMapObjects[gPlayerAvatar.mapObjectId];
    struct Sprite *sprite = &gSprites[mapObject->spriteId];

    switch (gTasks[taskId].data[0])
    {
        case 0:
            FieldObjectClearAnimIfSpecialAnimActive(mapObject);
            mapObject->mapobj_bit_11 = TRUE;
            sub_81507BC(mapObject, sub_805C808(4));
            StartSpriteAnim(sprite, sub_80634F0(mapObject->mapobj_unk_18));
            gTasks[taskId].data[0]++;
            gTasks[taskId].data[1] = 0;
            break;
        case 1:
            sub_805D9C4(sprite);
            if (gTasks[taskId].data[1] < 60)
                gTasks[taskId].data[1]++;
            else
                gTasks[taskId].data[0]++;
            break;
        case 2:
            StartSpriteAnim(sprite, sub_8063500(GetPlayerFacingDirection()));
            gTasks[taskId].data[0]++;
            break;
        case 3:
            sub_805D9C4(sprite);
            if (sprite->animEnded)
            {
                if (!(gPlayerAvatar.flags & 8))
                    sub_81507BC(mapObject, sub_805C808(0));
                else
                    sub_81507BC(mapObject, sub_805C808(2));
                FieldObjectTurn(mapObject, mapObject->placeholder18);
                sprite->pos2.x = 0;
                sprite->pos2.y = 0;
                ScriptContext2_Disable();
                DestroyTask(taskId);
            }
            break;
    }
}

void sub_8150708(void)
{
    struct MapObject *mapObject = &gMapObjects[gPlayerAvatar.mapObjectId];
    u8 fieldEffectId;

    if (!(gPlayerAvatar.flags & 0x08))
    {
        sub_81507BC(mapObject, sub_805C808(2));
        FieldObjectTurn(mapObject, mapObject->placeholder18);
        SetPlayerAvatarStateMask(0x08);
        gFieldEffectArguments[0] = mapObject->currentCoords.x;
        gFieldEffectArguments[1] = mapObject->currentCoords.y;
        gFieldEffectArguments[2] = gPlayerAvatar.mapObjectId;
        fieldEffectId = FieldEffectStart(0x08);
        mapObject->mapobj_unk_1A = fieldEffectId;
        sub_80DC44C(fieldEffectId, 1);
    }
}

void sub_815077C(void)
{
    FieldEffectStart(0x41);
    CreateTask(sub_8150794, 0x00);
}

void sub_8150794(u8 taskId)
{
    if (!FieldEffectActiveListContains(0x41))
    {
        UnfreezeMapObjects();
        ScriptContext2_Disable();
        DestroyTask(taskId);
    }
}

void sub_81507BC(struct MapObject * a0, u8 a1)
{
    EventObjectSetGraphicsId(a0, a1);
}

void sub_81507CC(void)
{
    sub_805D154(1);
}

void sub_81507D8(void)
{
    sub_805D154(2);
}

void sub_81507E4(void)
{
    sub_805D154(3);
}

void sub_81507F0(void)
{
    sub_805D154(4);
}
