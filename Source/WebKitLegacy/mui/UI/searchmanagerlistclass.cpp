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
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include "Document.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>

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
        MUIA_List_Format, "BAR,BAR,",
        MUIA_List_Title, TRUE,
        TAG_MORE, INITTAGS
    );

    return ((IPTR)obj);
}

DEFDISP
{
    return DOSUPER;
}

DEFMMETHOD(List_Construct)
{
    // Built by caller
    return (IPTR)msg->entry;
}

DEFMMETHOD(List_Destruct)
{
    struct search_entry *entry = (struct search_entry *) msg->entry;

    if(entry)
    {
        free(entry->label);
        free(entry->request);
        free(entry->shortcut);
        free(entry);
    }

    return TRUE;
}

DEFMMETHOD(List_Display)
{
    struct search_entry *e = (struct search_entry *) msg->entry;

    if (e)
    {
        msg->array[0] = e->label;
        msg->array[1] = e->request;
        msg->array[2] = e->shortcut;
    }
    else
    {
        msg->array[0] = GSI(MSG_SEARCHMANAGERLIST_TITLE);
        msg->array[1] = GSI(MSG_SEARCHMANAGERLIST_LINK);
        msg->array[2] = GSI(MSG_SEARCHMANAGERLIST_SHORTCUT);
    }

    return TRUE;
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, searchmanagerlistclass)
