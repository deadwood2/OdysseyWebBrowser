/*
 * Copyright 2011 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
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
#include "ResourceHandle.h"
#include "ResourceRequest.h"
#include "ResourceHandleClient.h"
#include "SuggestEntry.h"

#include <expat.h>
#include <devices/rawkeycodes.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "gui.h"
#include "utils.h"

using namespace WebCore;

class SuggestClient;

// Should be in SuggestEntry.cpp
bool operator<(const SuggestEntry &e1, const SuggestEntry &e2)
{
	if(e1.occurrences() < e2.occurrences())
	{
		return true;
	}
	else if(e1.occurrences() > e2.occurrences())
	{
		return false;
	}
	else
	{
		return WTF::codePointCompare(e1.suggestion(), e2.suggestion()) < 0;
	}
}

struct Data
{
	Object *str;
	Object *pop;
	Object *lv_entries;

    RefPtr<ResourceHandle> resource_handle;
	SuggestClient *suggest_client;

	ULONG added;
	struct MUI_EventHandlerNode ehnode;
};

#if !OS(AROS)
static void XMLCALL startElement(void *userData, const char *name, const char **atts);
static void XMLCALL endElement(void *userData, const char *name);
#endif

class SuggestClient : public ResourceHandleClient
{
WTF_MAKE_NONCOPYABLE(SuggestClient);
public:
	SuggestClient(Object *obj)
	 : m_obj(obj)
	{
#if !OS(AROS)
		m_parser = XML_ParserCreate(NULL);	  
		if(m_parser)
		{
			XML_SetUserData(m_parser, this);
			XML_SetElementHandler(m_parser, startElement, endElement);
		}
#else
		m_parser = NULL;
#endif
	}

	~SuggestClient()
	{
#if !OS(AROS)
		if(m_parser)
		{
			XML_ParserFree(m_parser);
		}
#endif
	}

	virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
	{
	}

	virtual void didReceiveData(ResourceHandle*, const char* data, unsigned length, int lengthReceived)
	{
#if !OS(AROS)
		if(m_parser)
		{
			XML_Parse(m_parser, data, length, 0);
		}
#endif
	}

	virtual void didFinishLoading(ResourceHandle*, double)
	{
#if !OS(AROS)
		if(m_parser)
		{
			XML_Parse(m_parser, NULL, 0, 1);
		}
#endif
	}

	virtual void didFail(ResourceHandle*, const ResourceError& error)
	{
#if !OS(AROS)
		if(m_parser)
		{
			XML_Parse(m_parser, NULL, 0, 1);
		}
#endif
	}

	void addEntry(SuggestEntry *entry)
	{
		if(m_obj)
		{
			DoMethod(m_obj, MM_SuggestPopString_Insert, entry);
		}
	}

	void setCurrentEntry(SuggestEntry *entry) { m_currentEntry = entry; }
	SuggestEntry* currentEntry() { return m_currentEntry; }

private:
	Object *m_obj;
	XML_Parser m_parser;
	SuggestEntry *m_currentEntry;
};

#if !OS(AROS)
static void XMLCALL startElement(void *userData, const char *name, const char **atts)
{
	SuggestClient *suggest_client = (SuggestClient *) userData;

	if(suggest_client)
	{
		if(strcmp(name, "CompleteSuggestion") == 0)
		{
			suggest_client->setCurrentEntry(new SuggestEntry());
		}
		else if(strcmp(name, "suggestion") == 0)
		{
			if(suggest_client->currentEntry() && atts[1]) suggest_client->currentEntry()->setSuggestion(String::fromUTF8(atts[1]));
		}
		else if(strcmp(name, "num_queries") == 0)
		{
			if(suggest_client->currentEntry() && atts[1]) suggest_client->currentEntry()->setOccurrences(atoi(atts[1]));
		}
	}
}

static void XMLCALL endElement(void *userData, const char *name)
{
	if(strcmp(name, "CompleteSuggestion") == 0)
	{
		SuggestClient *suggest_client = (SuggestClient *) userData;

		if(suggest_client)
		{
			suggest_client->addEntry(suggest_client->currentEntry());
			suggest_client->setCurrentEntry(0); // Reset
		}
	}
}
#endif

MUI_HOOK(suggest_popclose, APTR list, APTR str)
{
	SuggestEntry *s = NULL;

	DoMethod((Object *) list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &s);

	if(s)
	{
		char *suggestion = (char *) utf8_to_local(s->suggestion().utf8().data());

		if(suggestion)
		{
			nnset((Object *) str, MUIA_String_Contents, suggestion);
			set(_win((Object *) str), MUIA_Window_ActiveObject, (Object *) str);
			free(suggestion);
		}
	}

	return 0;
}

MUI_HOOK(suggest_popopen, APTR pop, APTR win)
{
	Object *list = (Object *)getv((Object *)pop, MUIA_Listview_List);

	SetAttrs((Object *) win, MUIA_Window_DefaultObject, list, MUIA_Window_ActiveObject, list, TAG_DONE);
	set(list, MUIA_List_Active, MUIV_List_Active_Off/*0*/);

	return (TRUE);
}

STATIC VOID doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		switch (tag->ti_Tag)
		{
			case MA_PopString_ActivateString:
			{
				if(tag->ti_Data)
				{
					set((Object *) _win(obj), MUIA_Window_ActiveObject, data->str);
				}
				break;
			}
		}
	}
}

DEFNEW
{
	Object *pop, *str, *lv_entries;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		Child, pop = PopobjectObject,
			MUIA_Popstring_String, str = (Object *) StringObject, StringFrame, MUIA_CycleChain, 1, MUIA_String_MaxLen, 16384,  End,
			MUIA_Popstring_Button, RectangleObject, MUIA_Weight, 0, MUIA_ShowMe, FALSE, End,
			MUIA_Popobject_Object, lv_entries =  (Object *) NewObject(getsuggestlistclass(), NULL, TAG_DONE),
			MUIA_Popobject_ObjStrHook, &suggest_popclose_hook,
			MUIA_Popobject_WindowHook, &suggest_popopen_hook,
		End,
		TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->added = FALSE;

		data->str = str;
		data->pop = pop;
		data->lv_entries = lv_entries;

		data->ehnode.ehn_Object   = obj;
		data->ehnode.ehn_Class    = cl;
		data->ehnode.ehn_Events   = IDCMP_RAWKEY;
		data->ehnode.ehn_Priority = 5;
		data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;

		set(lv_entries, MUIA_Popstring_String, str);
		set(lv_entries, MUIA_Popobject_Object, obj);

		DoMethod(str, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_SuggestPopString_Initiate);
		DoMethod(lv_entries, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, data->lv_entries, 1, MM_SuggestList_SelectChange);
		DoMethod(lv_entries, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, obj, 2, MUIM_Popstring_Close, TRUE);
	}

	return (IPTR)obj;
}

DEFDISP
{
	GETDATA;

	if(data->resource_handle)
	{
		data->resource_handle->clearClient();
		data->resource_handle->cancel();
		data->resource_handle.release();
		data->resource_handle = nullptr;
	}

	if(data->suggest_client)
	{
		delete data->suggest_client;
		data->suggest_client = 0;
	}

	return DOSUPER;
}

DEFSET
{
	GETDATA;
	doset(obj, data, msg->ops_AttrList);
	return DOSUPER;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_OWB_ObjectType:
			*msg->opg_Storage = MV_OWB_ObjectType_URL;
			return TRUE;

		case MUIA_Popobject_Object:
            *msg->opg_Storage = (ULONG) data->lv_entries;
            return TRUE;

		case MUIA_Popstring_String:
			*msg->opg_Storage = (ULONG) data->str;
			return TRUE;

		case MA_OWB_URL:
		case MUIA_String_Contents:
			return GetAttr(MUIA_String_Contents, data->str, (ULONGPTR)msg->opg_Storage);
	}

	return DOSUPER;
}

DEFMMETHOD(Show)
{
	ULONG rc;
	GETDATA;

	if ((rc = DOSUPER))
	{
		if(!data->added)
		{
			DoMethod( _win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
			data->added = TRUE;
		}
	}

	return rc;
}

DEFMMETHOD(Hide)
{
	GETDATA;

	if(data->added)
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);
		data->added = FALSE;
	}
	return DOSUPER;
}

DEFMMETHOD(Setup)
{
	return DOSUPER;
}

DEFMMETHOD(Cleanup)
{
	return DOSUPER;
}

DEFMMETHOD(HandleEvent)
{
	GETDATA;
	ULONG rc = 0;
	struct IntuiMessage *imsg;

	if(msg->muikey > MUIKEY_NONE)
	{
		switch(msg->muikey)
		{
			case MUIKEY_WINDOW_CLOSE:
			{
				Object *active = (Object *) getv(_win(obj), MUIA_Window_ActiveObject);

				if(active == data->str)
				{
					DoMethod(obj, MUIM_Popstring_Close, FALSE);
					rc = MUI_EventHandlerRC_Eat;
				}
				break;
			}
		}
	}
	else if((imsg = msg->imsg))
	{
		if(imsg->Class == IDCMP_RAWKEY)
		{
			Object *active = (Object *) getv(_win(obj), MUIA_Window_ActiveObject);

			if(active == data->str && !(imsg->Code & IECODE_UP_PREFIX))
			{
				switch(imsg->Code & ~IECODE_UP_PREFIX)
				{
					case RAWKEY_ESCAPE:
						DoMethod(obj, MUIM_Popstring_Close, FALSE);
	                    rc = MUI_EventHandlerRC_Eat;
						break;

					case RAWKEY_RETURN:
						DoMethod(obj, MUIM_Popstring_Close, TRUE);
	                    rc = MUI_EventHandlerRC_Eat;
						break;

					case RAWKEY_UP:
					//case RAWKEY_NM_WHEEL_UP:
						set(data->lv_entries, MUIA_List_Active, MUIV_List_Active_Up);
						rc = MUI_EventHandlerRC_Eat;
						break;

					case RAWKEY_DOWN:
					//case RAWKEY_NM_WHEEL_DOWN:
						DoMethod(obj, MUIM_Popstring_Open);
						set(_win(obj), MUIA_Window_Activate, TRUE);
						set(_win(obj), MUIA_Window_ActiveObject, data->str);
						set(data->lv_entries, MUIA_List_Active, MUIV_List_Active_Down);
	                    rc = MUI_EventHandlerRC_Eat;
						break;
				}
			}
		}
	}

	return rc;

}

DEFSMETHOD(SuggestPopString_Insert)
{
	GETDATA;

	/* Open history listview and focus to string again */
	DoMethod(obj, MUIM_Popstring_Open);
	set(_win(obj), MUIA_Window_Activate, TRUE);
	set(_win(obj), MUIA_Window_ActiveObject, data->str);

	DoMethod(data->lv_entries, MUIM_List_InsertSingle, msg->item, MUIV_List_Insert_Sorted);

	return 0;
}

DEFMMETHOD(Popstring_Open)
{
	GETDATA;

	// Avoid closing popup when it's told to be opened again. :)
	if(!getv(data->lv_entries, MA_SuggestList_Opened))
	{
		return DOSUPER;
	}
	else
	{	
		return 0;
	}
}

DEFMMETHOD(Popstring_Close)
{
	return DOSUPER;
}

DEFTMETHOD(SuggestPopString_Abort)
{
	GETDATA;

	if(data->resource_handle)
	{
		data->resource_handle->clearClient();
		data->resource_handle->cancel();
		data->resource_handle.release();
		data->resource_handle = nullptr;
	}

	if(data->suggest_client)
	{
		delete data->suggest_client;
		data->suggest_client = 0;
	}

	DoMethod(obj, MUIM_Popstring_Close, FALSE);

	return 0;
}

DEFTMETHOD(SuggestPopString_Initiate)
{
	GETDATA;

    STRPTR converted = local_to_utf8((char *) getv(obj, MUIA_String_Contents));
	if(converted)
	{
		String words = String::fromUTF8(converted);
		free(converted);

		if(words.length() > 2)
		{
			String encoded = encodeWithURLEscapeSequences(words);
			encoded.replace("+", "%2B");
		    encoded.replace("%20", "+");
			encoded.replace("&", "%26");

			DoMethod(data->lv_entries, MUIM_List_Clear);

			NetworkingContext* context = 0;
			ResourceRequest request(URL(URL(), "http://google.com/complete/search?output=toolbar&ie=UTF-8&oe=UTF-8&q=" + encoded));

			if(data->resource_handle)
			{
				data->resource_handle->clearClient();
				data->resource_handle->cancel();
				data->resource_handle.release();
				data->resource_handle = nullptr;
			}

			if(data->suggest_client)
			{
				delete data->suggest_client;
				data->suggest_client = 0;
			}

			data->suggest_client = new SuggestClient(obj);
			data->resource_handle = ResourceHandle::create(context, request, data->suggest_client, false, false);
		}
		else if(words.length() == 0)
		{
			DoMethod(obj, MUIM_Popstring_Close);
		}
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSET
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
DECSMETHOD(SuggestPopString_Insert)
DECMMETHOD(Popstring_Open)
DECMMETHOD(Popstring_Close)
DECTMETHOD(SuggestPopString_Initiate)
DECTMETHOD(SuggestPopString_Abort)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, suggestpopstringclass)
