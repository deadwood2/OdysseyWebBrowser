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

#include "WebView.h"
#include "WebDownload.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <clib/macros.h>

#include "gui.h"
#include "utils.h"

using namespace WebCore;

struct Data
{
    Object *group;
};

DEFNEW
{
    Object *group;

    obj = (Object *) DoSuperNew(cl, obj,
            MUIA_Window_ID, MAKE_ID('W','C','O','O'),
            MUIA_Window_Title, GSI(MSG_COOKIEMANAGERWINDOW_COOKIES),
            MUIA_Window_NoMenus, TRUE,
            WindowContents, group = (Object *) NewObject(getcookiemanagergroupclass(), NULL, TAG_DONE),
            TAG_MORE, msg->ops_AttrList);

    if (obj)
    {
        GETDATA;

        data->group = group;

        DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);
    }

    return (IPTR)obj;
}

DEFGET
{
    switch (msg->opg_AttrID)
    {
        case MA_OWB_WindowType:
        {
            *msg->opg_Storage = (IPTR) MV_OWB_Window_CookieManager;
        }
        return TRUE;
    }

    return DOSUPER;
}

DEFSMETHOD(CookieManagerGroup_DidInsert)
{
    GETDATA;
    return DoMethodA(data->group, (_Msg_*)msg);
}

DEFSMETHOD(CookieManagerGroup_DidRemove)
{
    GETDATA;
    return DoMethodA(data->group, (_Msg_*)msg);
}

BEGINMTABLE
DECNEW
DECGET
DECSMETHOD(CookieManagerGroup_DidInsert)
DECSMETHOD(CookieManagerGroup_DidRemove)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, cookiemanagerwindowclass)
