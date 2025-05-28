#include "Gamepad.h"
#if ENABLE(GAMEPAD)

#define __NOLIBBASE__
#include <proto/sensors.h>
#undef __NOLIBBASE__
#include <proto/exec.h>
#include <proto/utility.h>
#include <libraries/sensors.h>
#include <libraries/sensors_hid.h>
#include <string.h>
#include <stddef.h>
#include <libraries/poseidon.h>
#include <proto/poseidon.h>

#define DEBUG

#ifdef DEBUG
#define kprintf dprintf
#define D(x) x
#else
#define D(x)
#endif

namespace WebKit {

typedef struct _gmlibGamepadDataInternal
{
	// Two analog joysticks - sensors (from childlist, don't free)
	APTR _leftStickSensor;
	APTR _rightStickSensor;
	// Two analog triggers - sensors (from childlist, don't free)
	APTR _leftTriggerSensor;
	APTR _rightTriggerSensor;
	// Buttons - notifies
	APTR _dpadSensor;
	APTR _backSensor;
	APTR _startSensor;
	APTR _leftStickButtonSensor;
	APTR _rightStickButtonSensor;
	APTR _xLeftSensor;
	APTR _yTopSensor;
	APTR _aBottomSensor;
	APTR _bRightSensor;
	APTR _shoulderLeftSensor;
	APTR _shoulderRightSensor;
	APTR _largeRumble;
	APTR _smallRumble;
	// Battery
	APTR _batterySensor;
} gmlibGamepadDataInternal;

struct internalID
{
	ULONG                    _vid, _pid;
	char                     _serial[65];
};

struct internalSlot
{
	gmlibGamepad             _pad;
	gmlibGamepadData         _data;
	APTR                     _notify;    // MUST be present if the gamepad is valid, removal notification
	APTR                     _childList;
	struct internalID        _id;
	union {
		gmlibButtons _bits;
		ULONG _all;
	} _buttons;
	gmlibGamepadDataInternal _internal;
};

#define GET_SLOT(_x_) ((_x_ >> 24) - 1)
#define GET_INDEX(_x_) (_x_ & 0xFF)
#define SET_SLOT(_x_, _y_) _x_ |= ((_y_ + 1) << 24)
 
struct internalHandle
{
	APTR _pool;
	struct Library *_sensorsBase;
	struct Library *_utilityBase;
	struct MsgPort *_port;
	APTR _classNotify;
	struct internalSlot _slots[gmlibSlotMax];
};

static BOOL gmlibSetupGamepad(struct internalHandle *ihandle, ULONG slotidx, APTR sensor);
static BOOL gmlibSetupHIDGamepad(struct internalHandle *ihandle, ULONG slotidx, APTR sensor);
static void gmlibReleaseAll(struct internalHandle *ihandle);
static void gmlibRealseSlot(struct internalHandle *ihandle, struct internalSlot *islot);
static BOOL gmlibGetID(struct internalHandle *ihandle, APTR sensor, struct internalID *outID);
static void gmlibListChanged(struct internalHandle *ihandle);

typedef enum {
	matchResult_Match        = 0,
	matchResult_NoMatch      = 1,
	matchResult_Undetermined = 2,
} matchResult;

static matchResult gmlibMatchID(struct internalHandle *ihandle, struct internalID *idA, struct internalID *idB);

#define GH(__x__) ((gmlibHandle *)__x__)
#define IH(__x__) ((struct internalHandle *)__x__)

static void gmlibLoadXboxClass(void)
{
	struct Library *PsdBase = OpenLibrary("poseidon.library", 1);
	if (PsdBase)
	{
		struct List *puclist;
		struct Node *puc;
		BOOL found = FALSE;

		Forbid();
		struct TagItem tags[] = { { PA_ClassList, (IPTR)&puclist }, { TAG_DONE, 0 }  };
		psdGetAttrsA(PGA_STACK, NULL, tags);
		puc = puclist->lh_Head;

		while(puc->ln_Succ)
		{
			if (strstr(puc->ln_Name, "xbox360.class"))
			{
				found = TRUE;
				break;
			}
			puc = puc->ln_Succ;
		}
		Permit();

		if (!found)
		{
			D(kprintf("%s: adding xbox360class...\n", __PRETTY_FUNCTION__));
			psdAddClass((STRPTR)"MOSSYS:Classes/USB/xbox360.class", 0);
			psdClassScan();
			D(kprintf("%s: xbox360class initialized\n", __PRETTY_FUNCTION__));
		}
		
		CloseLibrary(PsdBase);
	}
}

gmlibHandle *gmlibInitialize(const char *gameID, ULONG flags)
{
	(void)gameID;
	(void)flags;

	APTR pool = CreatePool(MEMF_ANY, 4096, 2048);
	if (pool)
	{
		struct internalHandle *handle = (struct internalHandle *)AllocPooled(pool, sizeof(struct internalHandle));
		
		if (handle)
		{
			memset(handle, 0, sizeof(struct internalHandle));

			handle->_pool = pool;
			handle->_sensorsBase = OpenLibrary("sensors.library", 53);
			handle->_utilityBase = OpenLibrary("utility.library", 0);
			handle->_port = CreateMsgPort();

			gmlibLoadXboxClass();
			
			if (handle->_sensorsBase && handle->_utilityBase && handle->_port)
			{
				D(kprintf("%s: initialized\n", __PRETTY_FUNCTION__));

				gmlibRenumerate(GH(handle));
				D(kprintf("%s: renumerated\n", __PRETTY_FUNCTION__));

				struct TagItem nottags[] =
				{
					{SENSORS_Notification_Destination, (ULONG)handle->_port},
					{SENSORS_Notification_ClassListChanged, TRUE},
					{SENSORS_Class, SensorClass_HID},
					{TAG_DONE, 0}
				};
				
				struct Library *SensorsBase = handle->_sensorsBase;
				handle->_classNotify = StartSensorNotify(NULL, nottags);
				D(kprintf("%s: classnotify %p\n", __PRETTY_FUNCTION__, handle->_classNotify));
				return GH(handle);
			}
			
			if (handle->_port)
				DeleteMsgPort(handle->_port);
			if (handle->_sensorsBase)
				CloseLibrary(handle->_sensorsBase);
			if (handle->_utilityBase)
				CloseLibrary(handle->_utilityBase);
		}
		
		DeletePool(pool);
	}		

	return NULL;	
}

void gmlibShutdown(gmlibHandle *handle)
{
	if (NULL != handle)
	{
		struct internalHandle *ihandle = IH(handle);
		struct SensorsNotificationMessage *s;
		struct Library *SensorsBase = ihandle->_sensorsBase;

		D(kprintf("%s: bye\n", __PRETTY_FUNCTION__));

		EndSensorNotify(ihandle->_classNotify, NULL);
		
		gmlibReleaseAll(ihandle);
		while ((s = (struct SensorsNotificationMessage *)GetMsg(ihandle->_port)))
		{
			ReplyMsg(&s->Msg);
		}

		DeleteMsgPort(ihandle->_port);
		if (ihandle->_sensorsBase)
			CloseLibrary(ihandle->_sensorsBase);
		if (ihandle->_utilityBase)
			CloseLibrary(ihandle->_utilityBase);

		DeletePool(ihandle->_pool);
	}		
}

static void gmlibHandleMessages(struct internalHandle *ihandle)
{
	struct Library *UtilityBase = ihandle->_utilityBase;
	struct SensorsNotificationMessage *s;

	// clear previous button states
	for (int i = 0; i < gmlibSlotMax; i++)
	{
		struct internalSlot *islot = &ihandle->_slots[i];
		islot->_data._buttons._all = islot->_buttons._all;
	}

	while ((s = (struct SensorsNotificationMessage *)GetMsg(ihandle->_port)))
	{
		if (s->UserData)
		{
			ULONG idx = GET_INDEX((ULONG)s->UserData);
			ULONG slot = GET_SLOT((ULONG)s->UserData);

			if (slot < gmlibSlotMax && idx < 32)
			{
				struct internalSlot *islot = &ihandle->_slots[slot];

				if (idx > 3)
				{
					IPTR valAddr = GetTagData(SENSORS_HIDInput_Value, 0, s->Notifications);
					DOUBLE *val = (DOUBLE *)valAddr;

					if (val != NULL)
					{
						// data state survives a frame, islot->_buttons holds current state
						if (*val >= 1.0)
						{
							islot->_data._buttons._all |= (1 << idx);
							islot->_buttons._all |= (1 << idx);
						}
						else
						{
							islot->_buttons._all &= ~(1 << idx);
						}
					}
				}
				else
				{
					struct TagItem *tag, *taglist = s->Notifications;
					IPTR valAddr;
					DOUBLE *val;

					while ((tag = NextTagItem(&taglist)))
					{
						switch (tag->ti_Tag)
						{
						case SENSORS_HIDInput_NS_Value:
							valAddr = tag->ti_Data;
							val = (DOUBLE *)valAddr;
							if (NULL != val)
							{
								if (*val <= -1.0)
								{
									islot->_data._buttons._bits._dpadUp = 1;
									islot->_buttons._bits._dpadUp = 1;
								}
								else if (*val >= 1.0)
								{
									islot->_data._buttons._bits._dpadDown = 1;
									islot->_buttons._bits._dpadDown = 1;
								}
								else
								{
									islot->_buttons._bits._dpadUp = 0;
									islot->_buttons._bits._dpadDown = 0;
								}
							}
							break;
						case SENSORS_HIDInput_EW_Value:
							valAddr = tag->ti_Data;
							val = (DOUBLE *)valAddr;
							if (NULL != val)
							{
								if (*val <= -1.0)
								{
									islot->_data._buttons._bits._dpadLeft = 1;
									islot->_buttons._bits._dpadLeft = 1;
								}
								else if (*val >= 1.0)
								{
									islot->_data._buttons._bits._dpadRight = 1;
									islot->_buttons._bits._dpadRight = 1;
								}
								else
								{
									islot->_buttons._bits._dpadLeft = 0;
									islot->_buttons._bits._dpadRight = 0;
								}
							}
							break;
						}
					}
				}

				D(kprintf("button idx %ld slot %ld - status %lx\n", idx, slot, islot->_data._buttons._all));
			}
			else if (slot < gmlibSlotMax && idx == 32)
			{
				IPTR valAddr = GetTagData(SENSORS_HIDInput_Value, 0, s->Notifications);
				DOUBLE *val = (DOUBLE *)valAddr;
				if (val)
				{
					struct internalSlot *islot = &ihandle->_slots[slot];
					islot->_data._battery = *val;
				}
			}
		}
		else
		{
			// There IS a point to doing 2x FindTagItem: we want to process removals first
			// before processing any pads being added. In any case, the list will be short and it's
			// not like we'd execute this on every frame

			if (FindTagItem(SENSORS_Notification_Removed, s->Notifications))
			{
				for (int i = 0; i < gmlibSlotMax; i++)
				{
					struct internalSlot *islot = &ihandle->_slots[i];
					if (islot->_notify == s->Sensor)
					{
						gmlibRealseSlot(ihandle, islot);
						break;
					}
				}
			}

			if (FindTagItem(SENSORS_Notification_ClassListChanged, s->Notifications))
			{
				gmlibListChanged(ihandle);
			}
		}
		
		ReplyMsg(&s->Msg);
	}
}

static void gmlibPoll(struct internalHandle *ihandle, struct internalSlot *islot)
{
	struct Library *SensorsBase = ihandle->_sensorsBase;

	if (islot->_notify)
	{
		if (islot->_internal._leftStickSensor)
		{
			struct TagItem pollTags[] = {
				{ SENSORS_HIDInput_NS_Value, (IPTR)&islot->_data._leftStick._northSouth },
				{ SENSORS_HIDInput_EW_Value, (IPTR)&islot->_data._leftStick._eastWest },
				{ TAG_DONE, 0 }
			};
			
			GetSensorAttr(islot->_internal._leftStickSensor, pollTags);
		}
		
		if (islot->_internal._rightStickSensor)
		{
			struct TagItem pollTags[] = {
				{ SENSORS_HIDInput_NS_Value, (IPTR)&islot->_data._rightStick._northSouth },
				{ SENSORS_HIDInput_EW_Value, (IPTR)&islot->_data._rightStick._eastWest },
				{ TAG_DONE, 0 }
			};
			
			GetSensorAttr(islot->_internal._rightStickSensor, pollTags);
		}
		
		if (islot->_internal._leftTriggerSensor)
		{
			struct TagItem pollTags[] = {
				{ SENSORS_HIDInput_Value, (IPTR)&islot->_data._leftTrigger },
				{ TAG_DONE, 0 }
			};
			
			GetSensorAttr(islot->_internal._leftTriggerSensor, pollTags);
		}

		if (islot->_internal._rightTriggerSensor)
		{
			struct TagItem pollTags[] = {
				{ SENSORS_HIDInput_Value, (IPTR)&islot->_data._rightTrigger },
				{ TAG_DONE, 0 }
			};
			
			GetSensorAttr(islot->_internal._rightTriggerSensor, pollTags);
		}
	}
}

void gmlibUpdate(gmlibHandle *handle)
{
	struct internalHandle *ihandle = IH(handle);

	if (ihandle)
	{
		gmlibHandleMessages(ihandle);
	
		// For analog inputs we want to get the current reading at time of gmlibUpdate
		for (int i = 0; i < gmlibSlotMax; i++)
		{
			struct internalSlot *islot = &ihandle->_slots[i];
			gmlibPoll(ihandle, islot);
		}
	}
}

BOOL gmlibGetGamepad(gmlibHandle *handle, ULONG slot, gmlibGamepad *outGamepad)
{
	if (handle && slot >= gmlibSlotMin && slot <= gmlibSlotMax)
	{
		struct internalHandle *ihandle = IH(handle);
		struct internalSlot *islot = &ihandle->_slots[slot - 1];
		if (islot->_notify)
		{
			if (outGamepad)
				memcpy(outGamepad, &islot->_pad, sizeof(*outGamepad));
			return TRUE;
		}
	}

	return FALSE;
}

static BOOL gmlibGetID(struct internalHandle *ihandle, APTR sensor, struct internalID *outID)
{
	struct Library *SensorsBase = ihandle->_sensorsBase;
	STRPTR serial = NULL;
	struct TagItem tags[] = {
		{SENSORS_HID_Product, (IPTR)&outID->_pid},
		{SENSORS_HID_Vendor, (IPTR)&outID->_vid},
		{SENSORS_HID_Serial, (IPTR)&serial},
		{TAG_DONE, 0},
	};

	outID->_pid = -1;
	outID->_vid = -1;
	outID->_serial[0] = 0;

	if (GetSensorAttr(sensor, tags) >= 2)
	{
		if (serial)
			stccpy(outID->_serial, serial, sizeof(outID->_serial));
		return TRUE;
	}
	
	return FALSE;
}

static matchResult gmlibMatchID(struct internalHandle *, struct internalID *idA, struct internalID *idB)
{
	if (idA && idB)
	{
		if (idA->_pid != idB->_pid || idA->_vid != idB->_vid)
			return matchResult_NoMatch;

		if (idA->_serial[0] && idB->_serial[0])
		{
			if (0 == strcmp(idA->_serial, idB->_serial))
				return matchResult_Match;
			return matchResult_NoMatch;
		}
		
		// if no serial, we must renumerate.
		// hid and xbox classes need to be fixed to try and provide / generate one in all cases
	}

	return matchResult_Undetermined;
}

static BOOL gmlibSetupGamepad(struct internalHandle *ihandle, ULONG slotidx, APTR parent)
{
	struct Library *SensorsBase = ihandle->_sensorsBase;
	struct internalSlot *islot = &ihandle->_slots[slotidx];

	CONST_STRPTR padName = NULL;
	struct TagItem nameTags[] = {
		{SENSORS_HID_Name, (IPTR)&padName},
		{ TAG_DONE, 0 }
	};

	GetSensorAttr(parent, nameTags);	
	
	if (!gmlibGetID(ihandle, parent, &islot->_id))
		return FALSE;

	D(kprintf("%s: @slot %ld, add gamepad pid %x vid %x serial '%s'\n", __PRETTY_FUNCTION__, slotidx, islot->_id._pid, islot->_id._vid, islot->_id._serial));
	
	struct TagItem tags[] = {
		{SENSORS_Parent, (IPTR)parent},
		{SENSORS_Class, SensorClass_HID},
		{ TAG_DONE, 0 },
	};

	APTR sensors = ObtainSensorsList(tags);

	if (sensors)
	{
		APTR sensor = NULL;

		while ((sensor = NextSensor(sensor, sensors, NULL)) != NULL)
		{
			ULONG type = 0, id = 0, limb = Sensor_HIDInput_Limb_Unknown;
			STRPTR name = NULL;

			struct TagItem qt[] = {
				{SENSORS_Type, (IPTR)&type},
				{SENSORS_HIDInput_Name, (IPTR)&name},
				{SENSORS_HIDInput_ID, (IPTR)&id},
				{SENSORS_HIDInput_Limb, (IPTR)&limb},
				{ TAG_DONE, 0 }
			};

			if (GetSensorAttr(sensor, qt) > 0)
			{
				D(kprintf("%s: child sensor type %lx\n", __PRETTY_FUNCTION__, type));

				switch (type)
				{
				case SensorType_HIDInput_Trigger:
					{
						struct TagItem tags[] = {
							{SENSORS_Notification_UserData, 0},
							{SENSORS_Notification_Destination, (IPTR)ihandle->_port},
							{SENSORS_Notification_SendInitialValue, TRUE},
							{SENSORS_HIDInput_Value, 1},
							{ TAG_DONE, 0 }
						};

						// need to map the buttons to their functions
						// not ideal but there's currently no other way to reliably do that
						if (0 == strcmp(name, "Shoulder Button Left"))
						{
							tags[0].ti_Data = 12; // bit number
							SET_SLOT(tags[0].ti_Data, slotidx);
							islot->_internal._shoulderLeftSensor = StartSensorNotify(sensor, tags);
						}
						else if (0 == strcmp(name, "Shoulder Button Right"))
						{
							tags[0].ti_Data = 13;
							SET_SLOT(tags[0].ti_Data, slotidx);
							islot->_internal._shoulderRightSensor = StartSensorNotify(sensor, tags);
						}
						else if ((0 == strcmp(name, "A Button")) || (0 == strcmp(name, "Cross Button")))
						{
							tags[0].ti_Data = 10;
							SET_SLOT(tags[0].ti_Data, slotidx);
							islot->_internal._aBottomSensor = StartSensorNotify(sensor, tags);
						}
						else if ((0 == strcmp(name, "B Button")) || (0 == strcmp(name, "Circle Button")))
						{
							tags[0].ti_Data = 11;
							SET_SLOT(tags[0].ti_Data, slotidx);
							islot->_internal._bRightSensor = StartSensorNotify(sensor, tags);
						}
						else if ((0 == strcmp(name, "X Button")) || (0 == strcmp(name, "Square Button")))
						{
							tags[0].ti_Data = 8;
							SET_SLOT(tags[0].ti_Data, slotidx);
							islot->_internal._xLeftSensor = StartSensorNotify(sensor, tags);
						}
						else if ((0 == strcmp(name, "Y Button")) || (0 == strcmp(name, "Triangle Button")))
						{
							tags[0].ti_Data = 9;
							SET_SLOT(tags[0].ti_Data, slotidx);
							islot->_internal._yTopSensor = StartSensorNotify(sensor, tags);
						}
						else if (0 == strcmp(name, "Left Analog Joystick Push Button"))
						{
							tags[0].ti_Data = 6;
							SET_SLOT(tags[0].ti_Data, slotidx);
							islot->_internal._leftStickButtonSensor = StartSensorNotify(sensor, tags);
						}
						else if (0 == strcmp(name, "Right Analog Joystick Push Button"))
						{
							tags[0].ti_Data = 7;
							SET_SLOT(tags[0].ti_Data, slotidx);
							islot->_internal._rightStickButtonSensor = StartSensorNotify(sensor, tags);
						}
						else if ((0 == strcmp(name, "Menu Button")) || (0 == strcmp(name, "Share Button")) || (0 == strcmp(name, "Start Button")))
						{
							tags[0].ti_Data = 5;
							SET_SLOT(tags[0].ti_Data, slotidx);
							islot->_internal._startSensor = StartSensorNotify(sensor, tags);
						}
						else if ((0 == strcmp(name, "View Button")) || (0 == strcmp(name, "Options Button")) || (0 == strcmp(name, "Back Button")))
						{
							tags[0].ti_Data = 4;
							SET_SLOT(tags[0].ti_Data, slotidx);
							islot->_internal._backSensor = StartSensorNotify(sensor, tags);
						}
					}
					break;
				case SensorType_HIDInput_Stick:
					{
						struct TagItem tags[] = {
							{SENSORS_Notification_UserData, 0},
							{SENSORS_Notification_Destination, (IPTR)ihandle->_port},
							{SENSORS_Notification_SendInitialValue, TRUE},
							{SENSORS_HIDInput_NS_Value, 1},
							{SENSORS_HIDInput_EW_Value, 1},
							{ TAG_DONE, 0 }
						};
						SET_SLOT(tags[0].ti_Data, slotidx);
						islot->_internal._dpadSensor = StartSensorNotify(sensor, tags);
					}
					break;
				case SensorType_HIDInput_Analog:
					if (limb == Sensor_HIDInput_Limb_LeftHand)
					{
						if (NULL == islot->_internal._leftTriggerSensor)
							islot->_internal._leftTriggerSensor = sensor;
					}
					else if (limb == Sensor_HIDInput_Limb_RightHand)
					{
						if (NULL == islot->_internal._rightTriggerSensor)
							islot->_internal._rightTriggerSensor = sensor;
					}
					break;
				case SensorType_HIDInput_AnalogStick:
					if (limb == Sensor_HIDInput_Limb_LeftHand)
					{
						if (NULL == islot->_internal._leftStickSensor)
							islot->_internal._leftStickSensor = sensor;
					}
					else if (limb == Sensor_HIDInput_Limb_RightHand)
					{
						if (NULL == islot->_internal._rightStickSensor)
							islot->_internal._rightStickSensor = sensor;
					}
					break;
				case SensorType_HIDInput_Rumble:
					if (0 == strncmp(name, "Large", 5))
						islot->_internal._largeRumble = sensor;
					else
						islot->_internal._smallRumble = sensor;
					break;
				case SensorType_HIDInput_Battery:
					{
						struct TagItem tags[] = {
							{SENSORS_Notification_UserData, 32},
							{SENSORS_Notification_Destination, (IPTR)ihandle->_port},
							{SENSORS_Notification_SendInitialValue, TRUE},
							{SENSORS_HIDInput_Value, 1},
							{ TAG_DONE, 0 }
						};
						SET_SLOT(tags[0].ti_Data, slotidx);
						islot->_internal._batterySensor = StartSensorNotify(sensor, tags);
					}
					break;
				}
			}
		}

		struct TagItem nt[] = 
		{
			{SENSORS_Notification_Destination, (IPTR)ihandle->_port},
			{SENSORS_Notification_Removed, TRUE},
			{ TAG_DONE, 0 }
		};

		islot->_childList = sensors;
		islot->_notify = StartSensorNotify(parent, nt);

		islot->_pad._pid = islot->_id._pid;
		islot->_pad._vid = islot->_id._vid;
		islot->_pad._hasRumble = TRUE;
		islot->_pad._hasBattery = islot->_internal._batterySensor != NULL;
		if (padName)
			stccpy(islot->_pad._name, padName, sizeof(islot->_pad._name));
		else
			strcpy(islot->_pad._name, "Unknown Gamepad");

		return TRUE;
	}
	
	return FALSE;
}

static BOOL gmlibSetupHIDGamepad(struct internalHandle *ihandle, ULONG slotidx, APTR parent)
{
	struct Library *SensorsBase = ihandle->_sensorsBase;
	struct internalSlot *islot = &ihandle->_slots[slotidx];
	struct TagItem tags[] = {
		{SENSORS_Parent, (IPTR)parent},
		{SENSORS_Class, SensorClass_HID},
		{ TAG_DONE, 0 },
	};
	static const char bits[] = { 8, 9, 10, 11, 4, 5, 6, 7 };
	static const int bitnos = sizeof(bits) / sizeof(char);
	static const size_t bitoffs[] = { 
		offsetof(struct _gmlibGamepadDataInternal, _xLeftSensor),
		offsetof(struct _gmlibGamepadDataInternal, _yTopSensor),
		offsetof(struct _gmlibGamepadDataInternal, _aBottomSensor),
		offsetof(struct _gmlibGamepadDataInternal, _bRightSensor),
		offsetof(struct _gmlibGamepadDataInternal, _backSensor),
		offsetof(struct _gmlibGamepadDataInternal, _startSensor),
		offsetof(struct _gmlibGamepadDataInternal, _leftStickButtonSensor),
		offsetof(struct _gmlibGamepadDataInternal, _rightStickButtonSensor)
		};

	CONST_STRPTR padName = NULL;
	struct TagItem nameTags[] = {
		{SENSORS_HID_Name, (IPTR)&padName},
		{ TAG_DONE, 0 }
	};

	GetSensorAttr(parent, nameTags);	

	if (!gmlibGetID(ihandle, parent, &islot->_id))
		return FALSE;
	
	D(kprintf("%s: @slot %ld, add gamepad pid %x vid %x serial '%s'\n", __PRETTY_FUNCTION__, slotidx, islot->_id._pid, islot->_id._vid, islot->_id._serial));

	APTR sensors = ObtainSensorsList(tags);

	if (sensors)
	{
		APTR sensor = NULL;
		ULONG buttonsFound = 0;

		while ((sensor = NextSensor(sensor, sensors, NULL)) != NULL)
		{
			ULONG type = 0, id = 0;
			STRPTR name = NULL;

			struct TagItem qt[] = {
				{SENSORS_Type, (IPTR)&type},
				{SENSORS_HIDInput_Name, (IPTR)&name},
				{SENSORS_HIDInput_ID, (IPTR)&id},
				{ TAG_DONE, 0 }
			};

			if (GetSensorAttr(sensor, qt) > 0)
			{

				switch (type)
				{
				case SensorType_HIDInput_Trigger:
					if (buttonsFound < bitnos)
					{
						struct TagItem tags[] = {
							{SENSORS_Notification_UserData, 0},
							{SENSORS_Notification_Destination, (IPTR)ihandle->_port},
							{SENSORS_Notification_SendInitialValue, TRUE},
							{SENSORS_HIDInput_Value, 1},
							{ TAG_DONE, 0 }
						};
						APTR *sensorAddr = (APTR *)(((UBYTE *)&islot->_internal) + bitoffs[buttonsFound]);
						tags[0].ti_Data = bits[buttonsFound]; // bit number
						SET_SLOT(tags[0].ti_Data, slotidx);
						*sensorAddr = StartSensorNotify(sensor, tags);
						buttonsFound ++;
					}
					break;
				case SensorType_HIDInput_Stick:
					if (NULL == islot->_internal._dpadSensor)
					{
						struct TagItem tags[] = {
							{SENSORS_Notification_UserData, 0},
							{SENSORS_Notification_Destination, (IPTR)ihandle->_port},
							{SENSORS_Notification_SendInitialValue, TRUE},
							{SENSORS_HIDInput_NS_Value, 1},
							{SENSORS_HIDInput_EW_Value, 1},
							{ TAG_DONE, 0 }
						};
						SET_SLOT(tags[0].ti_Data, slotidx);
						islot->_internal._dpadSensor = StartSensorNotify(sensor, tags);
					}
					break;
				case SensorType_HIDInput_Analog:
					if (NULL == islot->_internal._leftTriggerSensor)
						islot->_internal._leftTriggerSensor = sensor;
					else if (NULL == islot->_internal._rightTriggerSensor)
							islot->_internal._rightTriggerSensor = sensor;
					break;
				case SensorType_HIDInput_AnalogStick:
					if (NULL == islot->_internal._leftStickSensor)
						islot->_internal._leftStickSensor = sensor;
					else if (NULL == islot->_internal._rightStickSensor)
						islot->_internal._rightStickSensor = sensor;
					break;
				}
			}
		}

		struct TagItem nt[] = 
		{
			{SENSORS_Notification_Destination, (IPTR)ihandle->_port},
			{SENSORS_Notification_Removed, TRUE},
			{ TAG_DONE, 0 }
		};

		islot->_childList = sensors;
		islot->_notify = StartSensorNotify(parent, nt);
		islot->_pad._pid = islot->_id._pid;
		islot->_pad._vid = islot->_id._vid;
		islot->_pad._hasRumble = FALSE;
		islot->_pad._hasBattery = FALSE;
		if (padName)
			stccpy(islot->_pad._name, padName, sizeof(islot->_pad._name));
		else
			strcpy(islot->_pad._name, "Unknown Gamepad");
		return TRUE;
	}
	
	return FALSE;
}

static void gmlibRealseSlot(struct internalHandle *ihandle, struct internalSlot *islot)
{
	struct Library *SensorsBase = ihandle->_sensorsBase;

	D(kprintf("%s: slot %p\n", __PRETTY_FUNCTION__, islot));

	if (islot->_childList)
		ReleaseSensorsList(islot->_childList, NULL);
	
	if (islot->_notify)
		EndSensorNotify(islot->_notify, NULL);

	if (islot->_internal._dpadSensor)
		EndSensorNotify(islot->_internal._dpadSensor, NULL);
	
	if (islot->_internal._backSensor)
		EndSensorNotify(islot->_internal._backSensor, NULL);

	if (islot->_internal._startSensor)
		EndSensorNotify(islot->_internal._startSensor, NULL);

	if (islot->_internal._leftStickButtonSensor)
		EndSensorNotify(islot->_internal._leftStickButtonSensor, NULL);

	if (islot->_internal._rightStickButtonSensor)
		EndSensorNotify(islot->_internal._rightStickButtonSensor, NULL);

	if (islot->_internal._xLeftSensor)
		EndSensorNotify(islot->_internal._xLeftSensor, NULL);

	if (islot->_internal._yTopSensor)
		EndSensorNotify(islot->_internal._yTopSensor, NULL);

	if (islot->_internal._aBottomSensor)
		EndSensorNotify(islot->_internal._aBottomSensor, NULL);

	if (islot->_internal._bRightSensor)
		EndSensorNotify(islot->_internal._bRightSensor, NULL);

	if (islot->_internal._shoulderLeftSensor)
		EndSensorNotify(islot->_internal._shoulderLeftSensor, NULL);

	if (islot->_internal._shoulderRightSensor)
		EndSensorNotify(islot->_internal._shoulderRightSensor, NULL);

	if (islot->_internal._batterySensor)
		EndSensorNotify(islot->_internal._batterySensor, NULL);
	
	memset(islot, 0, sizeof(*islot));
}

static void gmlibReleaseAll(struct internalHandle *ihandle)
{
	for (int i = 0; i < gmlibSlotMax; i++)
	{
		struct internalSlot *islot = &ihandle->_slots[i];
		gmlibRealseSlot(ihandle, islot);
	}
}

void gmlibRenumerate(gmlibHandle *handle)
{
	if (handle)
	{
		struct internalHandle *ihandle = IH(handle);
		struct Library *SensorsBase = ihandle->_sensorsBase;
		APTR sensors;
		LONG slots = gmlibSlotMax;

		struct TagItem gamepadListTags[] = {
			{ SENSORS_Class, SensorClass_HID },
			{ SENSORS_Type, SensorType_HID_Gamepad },
			{ TAG_DONE, 0 }
		};

		struct TagItem hidListTags[] = {
			{ SENSORS_Class, SensorClass_HID },
			{ SENSORS_Type, SensorType_HID_Generic },
			{ TAG_DONE, 0 }
		};
		
		D(kprintf("%s: releasing gamepads...\n", __PRETTY_FUNCTION__));

		// release all gamepads...
		gmlibReleaseAll(ihandle);

		D(kprintf("%s: scanning x360 compatibles...\n", __PRETTY_FUNCTION__));

		// prefer actual gamepads to random hid devices
		if ((sensors = ObtainSensorsList(gamepadListTags)))
		{
			// setup the gamepad...
			APTR sensor = NULL;

			while (NULL != (sensor = NextSensor(sensor, sensors, NULL)) && (slots > 0))
			{
				if (gmlibSetupGamepad(ihandle, gmlibSlotMax - slots, sensor))
					slots --;
			}
			
			ReleaseSensorsList(sensors, NULL);
		}
		
		D(kprintf("%s: slots left %lu\n", __PRETTY_FUNCTION__, slots));
		
		if (slots > 0)
		{
			D(kprintf("%s: scanning hid compatibles...\n", __PRETTY_FUNCTION__));

			if ((sensors = ObtainSensorsList(hidListTags)))
			{
				APTR sensor = NULL;
				while ((sensor = NextSensor(sensor, sensors, NULL)) && slots > 0)
				{
					if (gmlibSetupHIDGamepad(ihandle, gmlibSlotMax - slots, sensor))
						slots --;
				}
				ReleaseSensorsList(sensors, NULL);
			}
		}
	}
}

static BOOL gmlibScanGamepads(struct internalHandle *ihandle, ULONG gclass)
{
	struct Library *SensorsBase = ihandle->_sensorsBase;
	APTR sensors;
	LONG slots = 0;
	BOOL needsRenumerate = FALSE;

	struct TagItem gamepadListTags[] = {
		{ SENSORS_Class, SensorClass_HID },
		{ SENSORS_Type, gclass },
		{ TAG_DONE, 0 }
	};

	// check how many slots are actually empty...
	for (int i = 0; i < gmlibSlotMax; i++)
	{
		if (NULL == ihandle->_slots[i]._notify)
			slots ++;
	}
	
	D(kprintf("%s: scanning class %ld compatibles...\n", __PRETTY_FUNCTION__, gclass));

	// prefer actual gamepads to random hid devices
	if ((sensors = ObtainSensorsList(gamepadListTags)))
	{
		// setup the gamepad...
		APTR sensor = NULL;

		while (NULL != (sensor = NextSensor(sensor, sensors, NULL)) && (slots > 0) && (!needsRenumerate))
		{
			ULONG freeID = gmlibSlotMax;
			BOOL addThisSensor = TRUE;

			struct internalID id;

			if (!gmlibGetID(ihandle, sensor, &id))
				addThisSensor = FALSE;
			
			if (addThisSensor)
			{
				for (int i = 0; i < gmlibSlotMax && addThisSensor && !needsRenumerate; i++)
				{
					struct internalSlot *islot = &ihandle->_slots[i];
					if (islot->_notify)
					{
						switch (gmlibMatchID(ihandle, &islot->_id, &id))
						{
						case matchResult_Match:
							// already added in some slot, skip it
							addThisSensor = FALSE;
							break;
						case matchResult_Undetermined:
							// we don't know how to match the sensors, force renumeration...
							needsRenumerate = TRUE;
							break;
						case matchResult_NoMatch:
							// continue checking remaining sensors for the possible match...
							break;
						}
					}					
					else if (gmlibSlotMax == freeID)
					{
						freeID = i;
					}
				}
			}
			
			if (addThisSensor && gmlibSetupGamepad(ihandle, freeID, sensor))
				slots --;
		}
		
		ReleaseSensorsList(sensors, NULL);
		
		if (needsRenumerate)
		{
			return FALSE;
		}
	}

	return TRUE;	
}

static void gmlibListChanged(struct internalHandle *ihandle)
{
	D(kprintf("%s: scanning for added gamepads...\n", __PRETTY_FUNCTION__));
	if (!gmlibScanGamepads(ihandle, SensorType_HID_Gamepad))
	{
		gmlibRenumerate(GH(ihandle));
		return;
	}

	if (!gmlibScanGamepads(ihandle, SensorType_HID_Generic))
	{
		gmlibRenumerate(GH(ihandle));
	}
}

void gmlibGetData(gmlibHandle *handle, ULONG slot, gmlibGamepadData *outData)
{
	if (handle && outData && slot >= gmlibSlotMin && slot <= gmlibSlotMax)
	{
		struct internalHandle *ihandle = IH(handle);		
		struct internalSlot *islot = &ihandle->_slots[slot - 1];
		memcpy(outData, &islot->_data, sizeof(*outData));
	}
}

void gmlibSetRumble(gmlibHandle *handle, ULONG slot, DOUBLE smallMotorPower, DOUBLE largeMotorPower, ULONG msDuration)
{
	if (handle && slot >= gmlibSlotMin && slot <= gmlibSlotMax)
	{
		struct internalHandle *ihandle = IH(handle);		
		struct Library *SensorsBase = ihandle->_sensorsBase;
		struct internalSlot *islot = &ihandle->_slots[slot - 1];
		if (islot->_notify)
		{
			struct TagItem large[] = 
			{
				{SENSORS_HIDInput_Rumble_Power, (IPTR)&largeMotorPower},
				{SENSORS_HIDInput_Rumble_Duration, msDuration},
				{ TAG_DONE, 0 }
			};

			struct TagItem small[] = 
			{
				{SENSORS_HIDInput_Rumble_Power, (IPTR)&smallMotorPower},
				{SENSORS_HIDInput_Rumble_Duration, msDuration},
				{ TAG_DONE, 0 }
			};
				
			if (islot->_internal._smallRumble)
				SetSensorAttr(islot->_internal._smallRumble, small);
			if (islot->_internal._largeRumble)
				SetSensorAttr(islot->_internal._largeRumble, large);
		}
	}
}

/*********************************************************************************************************************************/

class GamepadMorphOS : public WebCore::PlatformGamepad
{
public:
	GamepadMorphOS(ULONG slot) : m_slot(slot) { }
    const Vector<double>& axisValues() const override { return m_axisValues; }
    const Vector<double>& buttonValues() const override { return m_buttonValues; }

	bool update(gmlibHandle *handle)
	{
		return false;
	}

protected:
	Vector<double> m_axisValues;
	Vector<double> m_buttonValues;
	ULONG          m_slot;
};

GamepadProviderMorphOS& GamepadProviderMorphOS::singleton()
{
    static NeverDestroyed<GamepadProviderMorphOS> sharedProvider;
    return sharedProvider;
}

GamepadProviderMorphOS::GamepadProviderMorphOS() = default;

GamepadProviderMorphOS::~GamepadProviderMorphOS()
{
	gmlibShutdown(m_handle);
}

void GamepadProviderMorphOS::startMonitoringGamepads(WebCore::GamepadProviderClient& client)
{
    ASSERT(!m_clients.contains(&client));
    m_clients.add(&client);
	D(dprintf("%s: clients %d\n", __PRETTY_FUNCTION__, m_clients.size()));
	
	if (1 == m_clients.size())
	{
		m_handle = gmlibInitialize("WebKitty", 0);
	}
}

void GamepadProviderMorphOS::stopMonitoringGamepads(WebCore::GamepadProviderClient& client)
{
    ASSERT(m_clients.contains(&client));
    m_clients.remove(&client);
	D(dprintf("%s: clients %d\n", __PRETTY_FUNCTION__, m_clients.size()));

	if (0 == m_clients.size())
	{
	
	}
}

}

#endif

