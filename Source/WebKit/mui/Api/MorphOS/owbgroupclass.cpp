/*
 * Copyright 2009 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gui.h"

/******************************************************************
 * owbgroupclass
 *****************************************************************/

struct Data
{
	Object *browser;
	Object *mediacontrolsgroup;
	Object *inspectorgroup;
};

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		InnerSpacing(0, 0),
        GroupSpacing(0),
		TAG_MORE, INITTAGS,
		TAG_DONE
	);

	return (ULONG)obj;
}

static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_OWBGroup_Browser:
		{
			data->browser = (Object *) tag->ti_Data;
		}
		break;

		case MA_OWBGroup_MediaControlsGroup:
		{
			data->mediacontrolsgroup = (Object *) tag->ti_Data;
		}
		break;

		case MA_OWBGroup_InspectorGroup:
		{
			data->inspectorgroup = (Object *) tag->ti_Data;
		}
		break;
	}
	NEXTTAG
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return DOSUPER;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_OWBGroup_Browser:
		{
			*msg->opg_Storage = (ULONG) data->browser;
		}
		return TRUE; 

		case MA_OWBGroup_MediaControlsGroup:
		{
			*msg->opg_Storage = (ULONG) data->mediacontrolsgroup;
		}
		return TRUE;

		case MA_OWBGroup_InspectorGroup:
		{
			*msg->opg_Storage = (ULONG) data->inspectorgroup;
		}
		return TRUE;
	}

	return DOSUPER;
}

BEGINMTABLE
DECNEW
DECSET
DECGET
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, owbgroupclass)
