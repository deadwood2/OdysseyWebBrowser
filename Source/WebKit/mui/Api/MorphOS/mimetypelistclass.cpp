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

#include "config.h"
#include "GraphicsContext.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <clib/macros.h>
#include <proto/dos.h>

#include "gui.h"
#include "utils.h"


using namespace WebCore;

struct Data
{
};

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_Title, TRUE,
		MUIA_List_Format, "BAR, BAR, BAR,",
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
	}
	return ((ULONG)obj);
}

DEFMMETHOD(List_Construct)
{
	return (ULONG)msg->entry;
}

DEFMMETHOD(List_Destruct)
{
	return TRUE;
}

DEFMMETHOD(List_Display)
{
	struct mimetypenode *mimetype = (struct mimetypenode *) msg->entry;

	if (mimetype)
	{
		char *action = NULL;
		STATIC char buf[512];

		switch(mimetype->action)
		{
			case MIMETYPE_ACTION_INTERNAL:
				action = GSI(MSG_MIMETYPELIST_INTERNAL_VIEWER);
				break;
			case MIMETYPE_ACTION_EXTERNAL:
				action = GSI(MSG_MIMETYPELIST_EXTERNAL_VIEWER);
				break;
			case MIMETYPE_ACTION_DOWNLOAD:
				action = GSI(MSG_MIMETYPELIST_SAVE_TO_DISK);
				break;
			case MIMETYPE_ACTION_ASK:
				action = GSI(MSG_MIMETYPELIST_ASK);
				break;
			case MIMETYPE_ACTION_STREAM:
				action = GSI(MSG_MIMETYPELIST_STREAM);
				break;
			case MIMETYPE_ACTION_PIPE:
				action = GSI(MSG_MIMETYPELIST_PIPE);
				break;
			case MIMETYPE_ACTION_IGNORE:
				action = GSI(MSG_MIMETYPELIST_IGNORE);
				break;
			case MIMETYPE_ACTION_PLUGIN:
				action = GSI(MSG_MIMETYPELIST_PLUGIN);
				break;
		}

		snprintf(buf, sizeof(buf), "%s %s", mimetype->viewer, mimetype->parameters);

		msg->array[0] = mimetype->mimetype;
		msg->array[1] = mimetype->extensions;
		msg->array[2] = action;
		msg->array[3] = buf;

		if( (ULONG)msg->array[-1] % 2 )
		{
			msg->array[-9] = (STRPTR) 10;
		}
	}
	else
	{
		msg->array[0] = GSI(MSG_MIMETYPELIST_MIMETYPE);
		msg->array[1] = GSI(MSG_MIMETYPELIST_EXTENSION);
		msg->array[2] = GSI(MSG_MIMETYPELIST_ACTION);
		msg->array[3] = GSI(MSG_MIMETYPELIST_VIEWER);
	}

	return TRUE;
}

DEFMMETHOD(List_Compare)
{
	struct mimetypenode *m1 = (struct mimetypenode *) msg->entry1;
	struct mimetypenode *m2 = (struct mimetypenode *) msg->entry2;

	return stricmp(m1->mimetype, m2->mimetype);
}

BEGINMTABLE
DECNEW
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECMMETHOD(List_Compare)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, mimetypelistclass)
