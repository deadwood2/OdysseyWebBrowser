#include <clib/macros.h>
#include <proto/dos.h>

#include <stdio.h>
#include "gui.h"

/******************************************************************
 * seeksliderclass
 *****************************************************************/

struct Data
{
	char buffer[64];
	struct MUI_EventHandlerNode ehnode;
};

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
//		  MUIA_Numeric_Min,  0,
//		  MUIA_Numeric_Max,  100,
		MUIA_Slider_Min,   0,
		MUIA_Slider_Max,   100,
		MUIA_Slider_Level, 0,
		TAG_MORE, INITTAGS,
	End;

	return (ULONG)obj;
}

DEFMMETHOD(Setup)
{
	GETDATA;

	if (!DOSUPER)
	{
		return (0);
	}

	data->ehnode.ehn_Object = obj;
	data->ehnode.ehn_Class = cl;
	data->ehnode.ehn_Events =  IDCMP_RAWKEY;
	data->ehnode.ehn_Priority = 3;
	data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;
	DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)&data->ehnode);

	return TRUE;
}

DEFMMETHOD(Cleanup)
{
	GETDATA;

	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);

	return DOSUPER;
}

DEFMMETHOD(Numeric_Stringify)
{
	GETDATA;

	int	h=0, m=0, s=0;
	int val = msg->value;

	h = val / 3600;
	m = val / 60 % 60;
	s = val % 60;

	snprintf(data->buffer, sizeof(data->buffer), "%02d:%02d:%02d", h, m, s);

	return ((ULONG)data->buffer);
}

DEFMMETHOD(AskMinMax)
{
	ULONG maxwidth, defwidth, minwidth;

	DOSUPER;

	maxwidth = 0;
	defwidth = 0;
	minwidth = 100;

	msg->MinMaxInfo->MinWidth  += minwidth;
	msg->MinMaxInfo->DefWidth  += defwidth;
	msg->MinMaxInfo->MaxWidth  += maxwidth;

	return 0;
}

DEFMMETHOD(HandleEvent)
{
	if (msg->imsg)
	{
		ULONG Class;
		UWORD Code;
		int MouseX, MouseY;

		Class     = msg->imsg->Class;
		Code      = msg->imsg->Code;
		MouseX    = msg->imsg->MouseX;
		MouseY    = msg->imsg->MouseY;

		if(_isinobject(obj, MouseX, MouseY))
		{
			switch( Class )
			{
				case IDCMP_RAWKEY:
					 switch ( Code )
					 {
						 case NM_WHEEL_UP:
							//
	                        return MUI_EventHandlerRC_Eat;
						 case NM_WHEEL_DOWN:
							//
	                        return MUI_EventHandlerRC_Eat;
					}
		    }
		}
	}

	return DOSUPER;
}

BEGINMTABLE
DECNEW
DECMMETHOD(AskMinMax)
DECMMETHOD(Numeric_Stringify)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Slider, seeksliderclass)

