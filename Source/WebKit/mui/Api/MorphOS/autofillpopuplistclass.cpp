/*
 * Copyright 2010 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
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
		MUIA_CycleChain, TRUE,
		TAG_MORE, INITTAGS
	);

	return ((ULONG)obj);
}

DEFMMETHOD(List_Construct)
{
	String *entry = (String *) msg->entry;
	return (ULONG) (new String(*entry));
}

DEFMMETHOD(List_Destruct)
{
	String *entry = (String *) msg->entry;

	delete entry;

	return TRUE;
}

DEFMMETHOD(List_Display)
{
	String *entry = (String *) msg->entry;

	if (entry)
	{
		static char buffer[1024];

		stccpy(buffer, utf8_to_local(entry->utf8().data()), sizeof(buffer));
		msg->array[0] = buffer;
	}

	return TRUE;
}

BEGINMTABLE
DECNEW
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, autofillpopuplistclass)
