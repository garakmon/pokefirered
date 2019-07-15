#include "global.h"

#include "field_player_avatar.h"

#define EVENT_OBJ_ID_PLAYER 0xFF



// 
extern void strange_npc_table_clear(void);
extern u8 GetFieldObjectIdByLocalId(u8);
extern u8 GetFieldObjectIdByLocalIdAndMapInternal(u8, u8, u8);

//
void sub_805DE8C(void);

// put in header
extern const struct SpriteTemplate *const gUnknown_83A0010[];



// static void ClearEventObject
void npc_clear_ids_and_state(struct MapObject *eventObject)
{
    *eventObject = (struct MapObject){};
    eventObject->localId = 0xFF;
    eventObject->mapNum = 0xFF;
    eventObject->mapGroup = 0xFF;
    eventObject->movementActionId = 0xFF;
}

// static void ClearAllEventObjects
// gMapObjects -> gEventObjects
void npcs_clear_ids_and_state(void)
{
    u8 i;

    for (i = 0; i < EVENT_OBJECTS_COUNT; i++)
        npc_clear_ids_and_state(&gMapObjects[i]);
}

// void ResetEventObjects
void sub_805DE74(void)
{
    strange_npc_table_clear();
    npcs_clear_ids_and_state();
    ClearPlayerAvatarInfo();
    sub_805DE8C();
}

// static void CreateReflectionEffectSprites
// gUnknown_83A0010 -> gFieldEffectObjectTemplatePointers
void sub_805DE8C(void)
{
    u8 spriteId = CreateSpriteAtEnd(gUnknown_83A0010[21], 0, 0, 31);
    gSprites[spriteId].oam.affineMode = 1;
    InitSpriteAffineAnim(&gSprites[spriteId]);
    StartSpriteAffineAnim(&gSprites[spriteId], 0);
    gSprites[spriteId].invisible = TRUE;

    spriteId = CreateSpriteAtEnd(gUnknown_83A0010[21], 0, 0, 31);
    gSprites[spriteId].oam.affineMode = 1;
    InitSpriteAffineAnim(&gSprites[spriteId]);
    StartSpriteAffineAnim(&gSprites[spriteId], 1);
    gSprites[spriteId].invisible = TRUE;
}

// GetFirstInactiveEventObjectId
u8 sub_805DF30(void)
{
    u8 i;
    for (i = 0; i < EVENT_OBJECTS_COUNT; i++)
    {
        if (!gMapObjects[i].active)
            break;
    }

    return i;
}

// GetEventObjectIdByLocalIdAndMap
u8 GetFieldObjectIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroupId)
{
    if (localId < EVENT_OBJ_ID_PLAYER)
    {
        return GetFieldObjectIdByLocalIdAndMapInternal(localId, mapNum, mapGroupId);
    }
    return GetFieldObjectIdByLocalId(localId);
}

// TryGetEventObjectIdByLocalIdAndMap
bool8 TryGetFieldObjectIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroupId, u8 *eventObjectId)
{
    *eventObjectId = GetFieldObjectIdByLocalIdAndMap(localId, mapNum, mapGroupId);
    if (*eventObjectId == EVENT_OBJECTS_COUNT)
        return TRUE;
    else
        return FALSE;
}

// GetEventObjectIdByXY
u8 GetFieldObjectIdByXY(s16 x, s16 y)
{
    u8 i;
    for (i = 0; i < EVENT_OBJECTS_COUNT; i++)
    {
        if (gMapObjects[i].active && gMapObjects[i].currentCoords.x == x && gMapObjects[i].currentCoords.y == y)
            break;
    }

    return i;
}

// GetEventObjectIdByLocalIdAndMapInternal
u8 GetFieldObjectIdByLocalIdAndMapInternal(u8 localId, u8 mapNum, u8 mapGroupId)
{
    u8 i;
    for (i = 0; i < EVENT_OBJECTS_COUNT; i++)
    {
        if (gMapObjects[i].active && gMapObjects[i].localId == localId
         && gMapObjects[i].mapNum == mapNum && gMapObjects[i].mapGroup == mapGroupId)
            return i;
    }

    return EVENT_OBJECTS_COUNT;
}

// GetEventObjectIdByLocalId
u8 GetFieldObjectIdByLocalId(u8 localId)
{
    u8 i;
    for (i = 0; i < EVENT_OBJECTS_COUNT; i++)
    {
        if (gMapObjects[i].active && gMapObjects[i].localId == localId)
            return i;
    }

    return EVENT_OBJECTS_COUNT;
}

// InitEventObjectStateFromTemplate
// get_mapheader_by_bank_and_number -> 
u8 sub_805E080(struct MapObjectTemplate *template, u8 mapNum, u8 mapGroup)
{
    //
    struct MapObject *eventObject;
    u8 eventObjectId;
    s16 x;
    s16 y;

    eventObject->spriteId = 0;
    eventObject-localId = 0;
}
/*
{
    struct MapObject *eventObject;
    u8 eventObjectId;
    s16 x;
    s16 y;

    // mapNum and mapGroup are in the wrong registers (r7/r6 instead of r6/r7)
    if (GetAvailableEventObjectId(template->localId, mapNum, mapGroup, &eventObjectId))
    {
        return EVENT_OBJECTS_COUNT;
    }
    eventObject = &gEventObjects[eventObjectId];
    ClearEventObject(eventObject);
    x = template->x + 7;
    y = template->y + 7;
    eventObject->active = TRUE;
    eventObject->triggerGroundEffectsOnMove = TRUE;
    eventObject->graphicsId = template->graphicsId;
    eventObject->movementType = template->movementType;
    eventObject->localId = template->localId;
    eventObject->mapNum = mapNum;
    eventObject->mapGroup = mapGroup;
    eventObject->initialCoords.x = x;
    eventObject->initialCoords.y = y;
    eventObject->currentCoords.x = x;
    eventObject->currentCoords.y = y;
    eventObject->previousCoords.x = x;
    eventObject->previousCoords.y = y;
    eventObject->currentElevation = template->elevation;
    eventObject->previousElevation = template->elevation;
    // For some reason, 0x0F is placed in r9, to be used later
    eventObject->range.as_nybbles.x = template->movementRangeX;
    eventObject->range.as_nybbles.y = template->movementRangeY;
    eventObject->trainerType = template->trainerType;
    eventObject->trainerRange_berryTreeId = template->trainerRange_berryTreeId;
    eventObject->previousMovementDirection = gInitialMovementTypeFacingDirections[template->movementType];
    SetEventObjectDirection(eventObject, eventObject->previousMovementDirection);
    SetEventObjectDynamicGraphicsId(eventObject);

    if (gRangedMovementTypes[eventObject->movementType])
    {
        if ((eventObject->range.as_nybbles.x) == 0)
        {
            // r9 is invoked here
            eventObject->range.as_nybbles.x++;
        }
        if ((eventObject->range.as_nybbles.y) == 0)
        {
            eventObject->range.as_nybbles.y++;
        }
    }
    return eventObjectId;
}
*/


