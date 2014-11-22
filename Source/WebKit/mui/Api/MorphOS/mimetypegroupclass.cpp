/*
 * Copyright 2009 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
 * Copyright 2009 Ilkka Lehtoranta <ilkleht@isoveli.org>
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
#include <wtf/text/CString.h>
#include "MIMETypeRegistry.h"
#include "PluginDatabase.h"
#include "PluginPackage.h"
#include "PluginData.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "gui.h"
#include "utils.h"

using namespace WebCore;

struct mimetypenode* mimetype_create(char *mimetype, char *extensions, mimetype_action_t action, char *viewer, char *parameters, int builtin, char *description)
{
	APTR n;
	bool found = false;
	struct mimetypenode *mn = NULL;

	ITERATELIST(n, &mimetype_list)
	{
		struct mimetypenode *mn = (struct mimetypenode *) n;

		if(!stricmp(mn->mimetype, mimetype))
		{
			free(mn->extensions);
			mn->extensions = strdup(extensions ? extensions : "");
			mn->action = action;
			free(mn->viewer);
			mn->viewer = strdup(viewer ? viewer : "");
			free(mn->parameters);
			mn->parameters = strdup(parameters ? parameters : "");
			//free(mn->description);
			//mn->description = strdup(description ? description : "");

			found = true;
			break;
		}
	}
	
	if(!found)
	{
		mn = (struct mimetypenode *) malloc(sizeof(*mn));

		if(mn)
		{
			mn->mimetype = strdup(mimetype ? mimetype : "");
			mn->extensions = strdup(extensions ? extensions : "");
			mn->action = action;
			mn->viewer = strdup(viewer ? viewer : "");
			mn->parameters = strdup(parameters ? parameters : "");
			mn->description = strdup(description ? description : "");
			mn->builtin = builtin;

			ADDTAIL(&mimetype_list, mn);
		}
	}

	return mn;
}

void mimetype_delete(struct mimetypenode *mn)
{
	REMOVE(mn);
	free(mn->mimetype);
	free(mn->extensions);
	free(mn->viewer);
	free(mn->parameters);
	free(mn->description);
	free(mn);
}

#define LABEL(x) (STRPTR)MSG_MIMETYPEGROUP_##x

STATIC CONST CONST_STRPTR actiontypes[] =
{
	LABEL(INTERNAL_VIEWER),
	LABEL(EXTERNAL_VIEWER),
	LABEL(SAVE_TO_DISK),
	LABEL(ASK),
	LABEL(IGNORE),
//	"Stream",
//	"Pipe",
//  "Plugin",
	NULL
};

STATIC CONST CONST_STRPTR placeholders[] = {"%l", "%f", "%p", NULL};
STATIC CONST CONST_STRPTR placeholders_desc[] = {"%l - link URL", "%f - generated local path", "%p - OWB REXX port", NULL};

static void cycles_init(void)
{
	APTR arrays[] = { (APTR) actiontypes, NULL };


	APTR *ptr = arrays;

	while(*ptr)
	{
		STRPTR *current = (STRPTR *)*ptr;
		while(*current)
		{
			*current = (STRPTR)GSI((ULONG)*current);
			current++;
		}
		ptr++;
	}
}

struct Data
{
	Object *lv_mimetypes;
	Object *bt_add;
	Object *bt_remove;
	Object *st_mimetype;
	Object *st_mimefamily;
	Object *st_extension;
	Object *cy_action;
	Object *st_viewer;
	Object *st_parameters;
	Object *pop_parameters;
	Object *lv_parameters;
	Object *actiongroup;
	Object *la_plugindescription;
};

DEFNEW
{
	Object *lv_mimetypes, *bt_add, *bt_remove, *st_mimetype, *st_mimefamily, *st_extension,
		   *cy_action, *st_viewer, *st_parameters, *pop_parameters, *lv_parameters,
		   *actiongroup, *la_plugindescription;

	cycles_init();

	obj = (Object *) DoSuperNew(cl, obj,
				MUIA_ObjectID, MAKE_ID('S','M','0','1'),
				Child, lv_mimetypes = (Object *) NewObject(getmimetypelistclass(), NULL, TAG_DONE),
				Child, HGroup,
						Child, bt_add = (Object *) MakeButton(GSI(MSG_MIMETYPEGROUP_ADD)),
						Child, bt_remove = (Object *) MakeButton(GSI(MSG_MIMETYPEGROUP_REMOVE)),
						End,
				Child, ColGroup(2),
					Child, MakeLabel(GSI(MSG_MIMETYPEGROUP_MIMETYPE)),
					Child, HGroup,
							Child, st_mimefamily = (Object *) MakeString("", FALSE),
							Child, MakeLabel("/"),
							Child, st_mimetype = (Object *) MakeString("", FALSE),
							End,
					
					Child, MakeLabel(GSI(MSG_MIMETYPEGROUP_EXTENSION)),
					Child, st_extension = (Object *) MakeString("", FALSE),
					
					Child, MakeLabel(GSI(MSG_MIMETYPEGROUP_ACTION)),

					Child, actiongroup = HGroup,
						MUIA_Group_PageMode, TRUE,
						Child, HGroup,
								Child, cy_action = (Object *) MakeCycle(GSI(MSG_MIMETYPEGROUP_ACTION), actiontypes),
								Child, MakeLabel(GSI(MSG_MIMETYPEGROUP_VIEWER)),
								Child, st_viewer = (Object *) MakeFileString(GSI(MSG_MIMETYPEGROUP_VIEWER), "", 0),
								Child, pop_parameters = PopobjectObject,
										MUIA_Popstring_String, st_parameters = StringObject,
											StringFrame,
											MUIA_CycleChain, 1,
											MUIA_String_MaxLen, 512,
											End,
										MUIA_Popstring_Button, PopButton(MUII_PopUp),
											MUIA_Popobject_Object, lv_parameters = ListObject,
											InputListFrame,
											MUIA_List_SourceArray, placeholders_desc,
											End,
										End,
								End,

						Child, HGroup,
								Child, la_plugindescription = TextObject,
																MUIA_Text_SetMax, FALSE,
																MUIA_Text_SetMin, FALSE,
																End,
								End,
						End,
					End,
			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->bt_remove = bt_remove;
		data->bt_add = bt_add;
		data->lv_mimetypes = lv_mimetypes;
		data->st_mimetype =  st_mimetype;
		data->st_mimefamily = st_mimefamily;
		data->st_extension = st_extension;
		data->cy_action = cy_action;
		data->st_viewer = st_viewer;
		data->st_parameters = st_parameters;
		data->pop_parameters = pop_parameters;
		data->lv_parameters = lv_parameters;
		data->actiongroup = actiongroup;
		data->la_plugindescription = la_plugindescription;

		set(bt_remove, MUIA_Disabled, TRUE);
		set(st_mimetype, MUIA_Disabled, TRUE);
		set(st_mimefamily, MUIA_Disabled, TRUE);
		set(st_extension, MUIA_Disabled, TRUE);
		set(cy_action, MUIA_Disabled, TRUE);
		set(st_viewer, MUIA_Disabled, TRUE);
		set(pop_parameters, MUIA_Disabled, TRUE);

		DoMethod(bt_add,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_MimeTypeGroup_Add);
		DoMethod(bt_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_MimeTypeGroup_Remove);

		DoMethod(lv_mimetypes, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MM_MimeTypeGroup_Refresh);

		DoMethod(st_mimetype, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_MimeTypeGroup_Change);
		DoMethod(st_mimefamily, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_MimeTypeGroup_Change);
		DoMethod(st_extension, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_MimeTypeGroup_Change);
		DoMethod(cy_action, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, obj, 1, MM_MimeTypeGroup_Change);
		DoMethod(st_viewer, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_MimeTypeGroup_Change);
		DoMethod(st_parameters, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_MimeTypeGroup_Change);

		DoMethod(lv_parameters, MUIM_Notify, MUIA_List_DoubleClick, TRUE, pop_parameters, 2, MUIM_Popstring_Close, TRUE);
		DoMethod(lv_parameters, MUIM_Notify, MUIA_List_DoubleClick, TRUE, obj, 1, MM_MimeTypeGroup_AddPlaceholder);
	}

	return (IPTR)obj;
}

DEFDISP
{
	GETDATA;
	APTR n, m;

	DoMethod(data->lv_mimetypes, MUIM_List_Clear);

	ITERATELISTSAFE(n, m, &mimetype_list)
	{
		mimetype_delete((mimetypenode *) n);
	}

	return DOSUPER;
}

DEFTMETHOD(MimeTypeGroup_AddPlaceholder)
{
    GETDATA;

    STRPTR oldpat;
	char newpat[512];
    ULONG clicked, pos;

	DoMethod(data->pop_parameters, MUIM_Popstring_Close, TRUE);

	oldpat  = (STRPTR) getv(data->st_parameters, MUIA_String_Contents);
	pos     = getv(data->st_parameters, MUIA_String_BufferPos);
	clicked = getv(data->lv_parameters, MUIA_List_Active);

	if (clicked < sizeof(placeholders))
	{
		newpat[pos] = '\0';
		strncpy(newpat, oldpat, pos);
		strcat(newpat, placeholders[clicked]);
		strcat(newpat, oldpat + pos);
		set(data->st_parameters, MUIA_String_Contents, newpat);
	}

	return (0);
}

DEFTMETHOD(MimeTypeGroup_Refresh)
{
	GETDATA;
	struct mimetypenode *mn;

	DoMethod(data->lv_mimetypes, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct mimetypenode *) &mn);

	if(mn)
	{
		Vector<String> types;
		String mimetypestr = mn->mimetype;
		mimetypestr.split("/", true, types);

		set(data->bt_remove, MUIA_Disabled, mn->builtin);
		set(data->st_mimefamily, MUIA_Disabled, mn->builtin);
		set(data->st_mimetype, MUIA_Disabled, mn->builtin);
		set(data->st_extension, MUIA_Disabled, FALSE);
		set(data->cy_action, MUIA_Disabled, FALSE);
		set(data->st_viewer, MUIA_Disabled, FALSE);
		set(data->pop_parameters, MUIA_Disabled, FALSE);

		nnset(data->st_mimefamily, MUIA_String_Contents, types[0].latin1().data());
		nnset(data->st_mimetype, MUIA_String_Contents, types[1].latin1().data());
		nnset(data->st_extension, MUIA_String_Contents, mn->extensions);

		if(mn->action == MIMETYPE_ACTION_PLUGIN)
		{
			set(data->cy_action, MUIA_Disabled, TRUE);
			nnset(data->la_plugindescription, MUIA_Text_Contents, mn->description);
			nnset(data->actiongroup, MUIA_Group_ActivePage, 1);
		}
		else
		{
			int cy_idx = 0;
			switch(mn->action)
			{
				case MIMETYPE_ACTION_INTERNAL:
					cy_idx = 0;
					break;
				case MIMETYPE_ACTION_EXTERNAL:
					cy_idx = 1;
					break;
				case MIMETYPE_ACTION_DOWNLOAD:
					cy_idx = 2;
					break;
				case MIMETYPE_ACTION_ASK:
					cy_idx = 3;
					break;
				case MIMETYPE_ACTION_IGNORE:
					cy_idx = 4;
					break;
				case MIMETYPE_ACTION_STREAM:
				case MIMETYPE_ACTION_PIPE:
				case MIMETYPE_ACTION_PLUGIN:
					cy_idx = 0; // Not supported yet
					break;
			}
			nnset(data->cy_action, MUIA_Cycle_Active, cy_idx);
			nnset(data->st_viewer, MUIA_String_Contents, mn->viewer);
			nnset(data->st_parameters, MUIA_String_Contents, mn->parameters);

			nnset(data->actiongroup, MUIA_Group_ActivePage, 0);
		}
	}
	else
	{
		set(data->bt_remove, MUIA_Disabled, TRUE);
		set(data->st_mimefamily, MUIA_Disabled, TRUE);
		set(data->st_mimetype, MUIA_Disabled, TRUE);
		set(data->st_extension, MUIA_Disabled, TRUE);
		set(data->cy_action, MUIA_Disabled, TRUE);
		set(data->st_viewer, MUIA_Disabled, TRUE);
		set(data->pop_parameters, MUIA_Disabled, TRUE);
	}

	return 0;
}

DEFTMETHOD(MimeTypeGroup_Change)
{
	GETDATA;
	struct mimetypenode *mn;

	DoMethod(data->lv_mimetypes, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct mimetypenode *) &mn);

	if(mn)
	{
		String mimetype;

		free(mn->mimetype);
		mimetype = (char *) getv(data->st_mimefamily, MUIA_String_Contents);
		mimetype.append("/");
		mimetype.append((char *) getv(data->st_mimetype, MUIA_String_Contents));
		mn->mimetype = strdup(mimetype.latin1().data());

		free(mn->extensions);
		mn->extensions = strdup((char *) getv(data->st_extension, MUIA_String_Contents));

		switch(getv(data->cy_action, MUIA_Cycle_Active))
		{
			case 0:
				mn->action = (mimetype_action_t) MIMETYPE_ACTION_INTERNAL;
				break;
			case 1:
				mn->action = (mimetype_action_t) MIMETYPE_ACTION_EXTERNAL;
				break;
			case 2:
				mn->action = (mimetype_action_t) MIMETYPE_ACTION_DOWNLOAD;
				break;
			case 3:
				mn->action = (mimetype_action_t) MIMETYPE_ACTION_ASK;
				break;
			case 4:
				mn->action = (mimetype_action_t) MIMETYPE_ACTION_IGNORE;
				break;
			case 5:
				mn->action = (mimetype_action_t) MIMETYPE_ACTION_STREAM;
				break;
			case 6:
				mn->action = (mimetype_action_t) MIMETYPE_ACTION_PIPE;
				break;
			case 7: // Shouldn't exist
				mn->action = (mimetype_action_t) MIMETYPE_ACTION_PLUGIN;
				break;
		}

		free(mn->viewer);
		mn->viewer = strdup((char *) getv(data->st_viewer, MUIA_String_Contents));

		free(mn->parameters);
		mn->parameters = strdup((char *) getv(data->st_parameters, MUIA_String_Contents));

		DoMethod(data->lv_mimetypes,  MUIM_List_Redraw, MUIV_List_Redraw_Entry, mn);
	}

	return 0;
}

DEFTMETHOD(MimeTypeGroup_Add)
{
	GETDATA;
	struct mimetypenode *mn;

	mn = mimetype_create("/*", "", MIMETYPE_ACTION_EXTERNAL, "", "%l", FALSE, NULL);

	if(mn)
	{
		DoMethod(data->lv_mimetypes, MUIM_List_InsertSingle, mn, MUIV_List_Insert_Bottom);
		set(data->lv_mimetypes, MUIA_List_Active, MUIV_List_Active_Bottom);
		set(_win(obj), MUIA_Window_ActiveObject, data->st_mimefamily);
	}

	return 0;
}

DEFTMETHOD(MimeTypeGroup_Remove)
{
	GETDATA;
	struct mimetypenode *mn;
	DoMethod(data->lv_mimetypes, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct mimetypenode *) &mn);

	if(mn)
	{
		DoMethod(data->lv_mimetypes, MUIM_List_Remove, MUIV_List_Remove_Active);
		mimetype_delete(mn);
	}

	return 0;
}

DEFMMETHOD(Import)
{
	GETDATA;
	ULONG id;
	char *val;
	APTR n, m;

	HashSet<String>* supportedCategories[] =
	{
		&(MIMETypeRegistry::getSupportedImageMIMETypes()),
		&(MIMETypeRegistry::getSupportedNonImageMIMETypes()),
		&(MIMETypeRegistry::getSupportedMediaMIMETypes()),
		NULL,
	};

	HashSet<String>**supportedCategoriesIterator = supportedCategories;

	DoMethod(data->lv_mimetypes, MUIM_List_Clear);

	ITERATELISTSAFE(n, m, &mimetype_list)
	{
		mimetype_delete((mimetypenode *) n);
	}

	NEWLIST(&mimetype_list);

	/* Fill builtin types */
	while(*supportedCategoriesIterator)
	{
		HashSet<String> *supportedMimeTypes = *supportedCategoriesIterator;

		for(HashSet<String>::iterator it = supportedMimeTypes->begin(); it != supportedMimeTypes->end(); ++it)
		{
			String mimetype = *it;
			Vector<String> extensions = MIMETypeRegistry::getExtensionsForMIMEType(mimetype);
			String extension = "";

			for(unsigned int i = 0; i < extensions.size(); i++)
			{
			        extension.append(extensions[i]);
				if(i < extensions.size() - 1)
				{
				    extension.append(" ");
				}
			}

			mimetype_create((char *) mimetype.latin1().data(), (char *) extension.latin1().data(), MIMETYPE_ACTION_INTERNAL, "", "", TRUE, NULL);
		}

		supportedCategoriesIterator++;
	}

	/* Make sure octet-stream and "force-download" is added if user didn't add it */
	mimetype_create("application/octet-stream", NULL, MIMETYPE_ACTION_DOWNLOAD, NULL, NULL, FALSE, NULL);
	mimetype_create("application/force-download", NULL, MIMETYPE_ACTION_DOWNLOAD, NULL, NULL, FALSE, NULL);

	/* Add some video/audio types that aren't added by default (because the formats aren't enabled by default) */
	mimetype_create("audio/webm",         "webm",    MIMETYPE_ACTION_INTERNAL, "", "", TRUE, NULL);
	mimetype_create("video/webm",         "webm",    MIMETYPE_ACTION_INTERNAL, "", "", TRUE, NULL);
	mimetype_create("video/x-flv",        "flv",     MIMETYPE_ACTION_INTERNAL, "", "", TRUE, NULL);
        mimetype_create("video/flv",          "flv",     MIMETYPE_ACTION_INTERNAL, "", "", TRUE, NULL);
	mimetype_create("audio/ogg",          "ogg oga", MIMETYPE_ACTION_INTERNAL, "", "", TRUE, NULL);
	mimetype_create("video/ogg",          "ogv",     MIMETYPE_ACTION_INTERNAL, "", "", TRUE, NULL);
	mimetype_create("video/x-theora+ogg", "ogv",     MIMETYPE_ACTION_INTERNAL, "", "", TRUE, NULL);

	/* Loaded Plugins */
	PluginDatabase *pluginDatabase = PluginDatabase::installedPlugins();
	for(unsigned i = 0; i < pluginDatabase->plugins().size(); i++)
	{
		PluginPackage *package = pluginDatabase->plugins()[i];

		if(package)
		{
		    const MIMEToDescriptionsMap& mimeToDescriptions = package->mimeToDescriptions();
		    MIMEToDescriptionsMap::const_iterator end = mimeToDescriptions.end();
			for (MIMEToDescriptionsMap::const_iterator it = mimeToDescriptions.begin(); it != end; ++it)
			{
				String mimetype    = it->key;
				String description = it->value;
				
				// build the suffixes
				String suffixes    = "";
				Vector<String> extensions = package->mimeToExtensions().get(mimetype);

				for (unsigned j = 0; j < extensions.size(); j++) {
					if (j > 0)
					    suffixes.append(" ");

					suffixes.append(extensions[j]);
		        }

                mimetype_create((char *) mimetype.latin1().data(), (char *) suffixes.latin1().data(), MIMETYPE_ACTION_PLUGIN, "", "", TRUE, (char *) description.latin1().data());
		    }
		}
	}

	/* Fill user types */
	if((id=(muiNotifyData(obj)->mnd_ObjectID)))
	{
		if ((val = (char *)DoMethod(msg->dataspace, MUIM_Dataspace_Find, id)))
		{
			ULONG i;
			String input = val;
			Vector<String> mimetypes;
			input.split("\n", true, mimetypes);
			for(i = 0; i < mimetypes.size(); i++)
			{
				Vector<String> mimetypeAttributes;
				mimetypes[i].split("\1", true, mimetypeAttributes);

				if(mimetypeAttributes.size() == 6)
				{
					mimetype_create((char *) mimetypeAttributes[0].latin1().data(),
											(char *) mimetypeAttributes[1].latin1().data(),
											(mimetype_action_t) mimetypeAttributes[2].toInt(),
											(char *) mimetypeAttributes[3].latin1().data(),
											(char *) mimetypeAttributes[4].latin1().data(),
											mimetypeAttributes[5].toInt(),
											NULL);
				}
			}
		}
	}

	ITERATELIST(n, &mimetype_list)
	{
		DoMethod(data->lv_mimetypes, MUIM_List_InsertSingle, n, MUIV_List_Insert_Sorted);
	}

	return 0;
}

DEFMMETHOD(Export)
{
	ULONG id;

	if((id=(muiNotifyData(obj)->mnd_ObjectID)))
	{
		APTR n;
		String output = "";

		ITERATELIST(n, &mimetype_list)
		{
			struct mimetypenode *mn = (struct mimetypenode *) n;

			//if(!mn->builtin)
			{
			  output.append(String::format("%s\1%s\1%d\1%s\1%s\1%d\n", mn->mimetype, mn->extensions, mn->action, mn->viewer, mn->parameters, mn->builtin));
			}
		}

		DoMethod(msg->dataspace, MUIM_Dataspace_Add, output.latin1().data(), output.length()+1, id);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(Import)
DECMMETHOD(Export)
DECTMETHOD(MimeTypeGroup_Add)
DECTMETHOD(MimeTypeGroup_Remove)
DECTMETHOD(MimeTypeGroup_Change)
DECTMETHOD(MimeTypeGroup_Refresh)
DECTMETHOD(MimeTypeGroup_AddPlaceholder)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, mimetypegroupclass)
