/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
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
#include "ContextMenu.h"

#include <exec/types.h>
#include <proto/intuition.h>
#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include <clib/debug_protos.h>

#include "include/macros/vapor.h"
#include "owb_cat.h"
#include "mui.h"

#include <cstdio>

struct IClass *getmenuclass(void);

namespace WebCore {

// TODO: ref-counting correctness checking.
// See http://bugs.webkit.org/show_bug.cgi?id=16115

ContextMenu::ContextMenu()
{
	m_platformDescription = (void *) NewObject(getmenuclass(), NULL, MUIA_Menu_Title, strdup(GSI(MSG_CONTEXTMENU_TITLE)), End;
}

ContextMenu::~ContextMenu()
{
}

void ContextMenu::appendItem(ContextMenuItem& item)
{
        m_items.append(item);
	BalMenuItem* platformItem = ContextMenuItem::createNativeMenuItem(item.releasePlatformDescription());


	if(platformItem && m_platformDescription)
	{
		DoMethod((Object *) m_platformDescription, OM_ADDMEMBER, platformItem);
	}
}

void ContextMenu::setPlatformDescription(PlatformMenuDescription menu)
{
    if (m_platformDescription)
	{
		MUI_DisposeObject((Object *) m_platformDescription);
		m_platformDescription = NULL;
	}

    m_platformDescription = menu;
}

PlatformMenuDescription ContextMenu::platformDescription() const
{
    return m_platformDescription;
}

PlatformMenuDescription ContextMenu::releasePlatformDescription()
{
    PlatformMenuDescription description = m_platformDescription;
    m_platformDescription = 0;

    return description;
}

unsigned ContextMenu::itemCount() const 
{ 
	return getv((Object*) m_platformDescription, MUIA_Family_ChildCount);
}

Vector<ContextMenuItem> contextMenuItemVector(const PlatformMenuDescription menu)
{
    UNUSED_PARAM(menu);
    Vector<ContextMenuItem> list;
    return list;
}

}
