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
#include "ContextMenuItem.h"
#include <wtf/text/CString.h>
#include "Logging.h"

#include <proto/intuition.h>
#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include <libraries/gadtools.h>
#include <aros-overrides.h>

#include <cstdio>

#undef String

struct IClass *getmenuitemclass(void);
ULONG getv(APTR obj, ULONG attr);

namespace WebCore {

// Extract the ActionType from the menu item
ContextMenuItem::ContextMenuItem(BalMenuItem* item)
    : m_platformDescription()
{
	PlatformMenuItemDescription *description = NULL;

	GetAttr(MUIA_UserData, (Object *)item, (ULONGPTR)&description);

	if(description)
	{
	        m_platformDescription.title   = (char *) getv(item, MUIA_Menuitem_Title);
		m_platformDescription.type    = description->type;
		m_platformDescription.checked = description->checked;
		m_platformDescription.action  = description->action;
		m_platformDescription.subMenu = description->subMenu;
		m_platformDescription.enabled = description->enabled;
	}
}

ContextMenuItem::ContextMenuItem(ContextMenu*)
{
}

ContextMenuItem::ContextMenuItem()
{
}

ContextMenuItem::ContextMenuItem(ContextMenuItemType type, ContextMenuAction action, const String& title, ContextMenu* subMenu)
{
    m_platformDescription.type   = type;
    m_platformDescription.action = action;
    m_platformDescription.title  = title;

    setSubMenu(subMenu);
}

ContextMenuItem::~ContextMenuItem()
{
}

BalMenuItem* ContextMenuItem::createNativeMenuItem(const PlatformMenuItemDescription& menu)
{
	Object *item = NULL;

	PlatformMenuItemDescription *description = static_cast<PlatformMenuItemDescription*>(malloc(sizeof(PlatformMenuItemDescription)));
	
	if(description)
	{
		description->type    = menu.type;
		description->checked = menu.checked;
		description->action  = menu.action;
		description->subMenu = menu.subMenu;
		description->enabled = menu.enabled;

		CString label = menu.title.latin1();
		const char *title = label.data() ? strdup(label.data()) : NULL;

		// title and userdata are freed in menu/menuitem class dispose method

		if(menu.subMenu)
		{
			SetAttrs((Object *) menu.subMenu,
						MUIA_Menu_Enabled, menu.enabled,
						MUIA_UserData, (ULONG) description,
						MUIA_Menu_Title, (menu.type == SeparatorType || title == NULL) ? NM_BARLABEL : title,
						TAG_DONE);

			item = (Object *) menu.subMenu;
		}
		else
		{
			item = (Object *) NewObject(getmenuitemclass(), NULL,
						MUIA_UserData, (ULONG) description,
						MUIA_Menuitem_Title, (menu.type == SeparatorType || title == NULL) ? NM_BARLABEL : title,
						MUIA_Menuitem_Enabled, (ULONG) menu.enabled,
						MUIA_Menuitem_Checkit, menu.type == CheckableActionType,
						menu.type == CheckableActionType ? MUIA_Menuitem_Checked : TAG_IGNORE, (ULONG) menu.checked,
					 End;
		}

	}

	return item;
}

PlatformMenuItemDescription ContextMenuItem::releasePlatformDescription()
{
    PlatformMenuItemDescription description = m_platformDescription;
    m_platformDescription = PlatformMenuItemDescription();
    return description;
}

ContextMenuItemType ContextMenuItem::type() const
{
    return m_platformDescription.type;
}

void ContextMenuItem::setType(ContextMenuItemType type)
{
    m_platformDescription.type = type;
}

ContextMenuAction ContextMenuItem::action() const
{
    return m_platformDescription.action;
}

void ContextMenuItem::setAction(ContextMenuAction action)
{
    m_platformDescription.action = action;
}

String ContextMenuItem::title() const
{
	return m_platformDescription.title;
}

void ContextMenuItem::setTitle(const String& title)
{
	m_platformDescription.title = title;
}

PlatformMenuDescription ContextMenuItem::platformSubMenu() const
{
    return m_platformDescription.subMenu;
}

void ContextMenuItem::setSubMenu(ContextMenu* menu)
{
    if (m_platformDescription.subMenu)
	{
		MUI_DisposeObject((Object *) m_platformDescription.subMenu);
		m_platformDescription.subMenu = NULL;
	}

    if (!menu)
        return;

    m_platformDescription.subMenu = menu->releasePlatformDescription();
    m_platformDescription.type    = SubmenuType;
}

void ContextMenuItem::setChecked(bool shouldCheck)
{
    m_platformDescription.checked = shouldCheck;
}

void ContextMenuItem::setEnabled(bool shouldEnable)
{
    m_platformDescription.enabled = shouldEnable;
}

}
