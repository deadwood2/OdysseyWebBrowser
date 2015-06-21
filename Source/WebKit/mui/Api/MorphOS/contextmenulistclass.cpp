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
		MUIA_List_Format, "BAR, BAR, BAR H,",
		MUIA_List_Title, TRUE,
		TAG_MORE, INITTAGS
	);

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
	struct contextmenunode *cn = (struct contextmenunode *) msg->entry;

	if (cn)
	{
		char *category = NULL;
		char *commandType = NULL;

		switch(cn->category)
		{
			case MV_OWBApp_BuildUserMenu_Link:
				category = GSI(MSG_CONTEXTMENULIST_LINK);
				break;
			case MV_OWBApp_BuildUserMenu_Image:
				category = GSI(MSG_CONTEXTMENULIST_IMAGE);
				break;
			case MV_OWBApp_BuildUserMenu_Page:
				category = GSI(MSG_CONTEXTMENULIST_PAGE);
				break;
		}

		switch(cn->commandType)
		{
			case ACTION_AMIGADOS:
				commandType	= "AmigaDOS";
				break;
			case ACTION_REXX:
				commandType	= "ARexx";
				break;
			case ACTION_INTERNAL:
				commandType	= GSI(MSG_CONTEXTMENULIST_INTERNAL);
				break;
		}

		msg->array[0] = category;
		msg->array[1] = cn->label;
		msg->array[2] = commandType;
		msg->array[3] = cn->commandString;

		if( (ULONG)msg->array[-1] % 2 )
		{
#if !OS(AROS)
			/* This code overrides internal data structures and causes a crash on AROS */
			msg->array[-9] = (STRPTR) 10;
#endif
		}
	}
	else
	{
		msg->array[0] = GSI(MSG_CONTEXTMENULIST_CATEGORY);
		msg->array[1] = GSI(MSG_CONTEXTMENULIST_LABEL);
		msg->array[2] = GSI(MSG_CONTEXTMENULIST_TYPE);
		msg->array[3] = GSI(MSG_CONTEXTMENULIST_ACTION);
	}

	return TRUE;
}

BEGINMTABLE
DECNEW
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, contextmenulistclass)
