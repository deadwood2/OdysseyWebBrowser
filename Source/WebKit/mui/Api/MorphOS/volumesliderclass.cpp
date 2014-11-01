#include <clib/macros.h>
#include <proto/dos.h>

#include <stdio.h>
#include "gui.h"

/******************************************************************
 * volumesliderclass
 *****************************************************************/

struct Data
{
	char buffer[16];
};

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Numeric_Min,  0,
		MUIA_Numeric_Max,  100,
		MUIA_Slider_Min,   0,
		MUIA_Slider_Max,   100,
		MUIA_Slider_Level, 100,
		TAG_MORE, INITTAGS,
	End;

	return (ULONG)obj;
}

DEFMMETHOD(Numeric_Stringify)
{
	GETDATA;

	snprintf(data->buffer, sizeof(data->buffer), " ");

	return ((ULONG)data->buffer);
}

DEFMMETHOD(AskMinMax)
{
	ULONG maxwidth, defwidth, minwidth;

	DOSUPER;

	maxwidth = 50;
	defwidth = 50;
	minwidth = 50;

	msg->MinMaxInfo->MinWidth  = minwidth;
	msg->MinMaxInfo->DefWidth  = defwidth;
	msg->MinMaxInfo->MaxWidth  = maxwidth;

	return 0;
}

BEGINMTABLE
DECNEW
DECMMETHOD(AskMinMax)
DECMMETHOD(Numeric_Stringify)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Slider, volumesliderclass)

