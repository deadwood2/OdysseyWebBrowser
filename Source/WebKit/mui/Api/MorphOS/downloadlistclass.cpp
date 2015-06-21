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

#include "WebDownload.h"
#include <wtf/CurrentTime.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <clib/macros.h>
#include <proto/wb.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <devices/timer.h>

#include "gui.h"
#include "utils.h"

using namespace WebCore;

enum {
	POPMENU_OPEN_FILE = 1,
	POPMENU_OPEN_DIRECTORY,
	POPMENU_OPEN_PAGE
};

struct Data
{
	ULONG type;
	Object *cmenu;
};

static void doset(Object *obj, struct Data *data, struct TagItem *tags)
{
	struct downloadnode *dl = NULL;
	FORTAG(tags)
	{
		case MUIA_List_DoubleClick:
			DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &dl);
			if (dl)
			{
				char fullname[1024];
				stccpy(fullname, dl->path, sizeof(fullname));
				AddPart(fullname, dl->filename, sizeof(fullname));
				OpenWorkbenchObjectA(fullname, NULL);
			}
			break;
		case MA_DownloadList_Type:
			data->type = tag->ti_Data;
			break;
	}
	NEXTTAG
}

// Helper to retrieve default icon for a downloaded file.
static void create_icon_if_needed(Object *obj, struct downloadnode *dl, ULONG type)
{
	if(!dl->iconobj)
	{
		String command;
		String iconPath = "defaultIcon"; // Default icon

		if(type == MV_DownloadList_Type_InProgress) // Check against given mimetype
		{
			if(dl->mimetype)
			{
				String mimetype = dl->mimetype;

				command = "GetDeficonPath MIMETYPE=\"" + mimetype + "\"";
				if(rexx_send((char *) "AMBIENT", (char *) command.utf8().data()))
				{
					iconPath = String(rexx_result());
				}
				else // Try again without "x-" prefix, ambient is not always compliant with naming
				{
					mimetype.replace("x-", "");
					command = "GetDeficonPath MIMETYPE=\"" + mimetype + "\"";
					if(rexx_send((char *) "AMBIENT", (char *) command.utf8().data()))
				    {
						iconPath = String(rexx_result());
					}
				}
			}
		}
		else // Check the file mimetype
		{
			int len = strlen(dl->filename) + strlen(dl->path) + 2;
			char *path = (char *) malloc(len);

			if(path)
			{
				stccpy(path, dl->path, len);
				AddPart(path, dl->filename, len);

				if(path)
				{
					// Prevent DOS requester if file doesn't exist
					struct Process *process = (struct Process *)FindTask(NULL);
					APTR oldwinptr = process->pr_WindowPtr;
					process->pr_WindowPtr = (APTR) -1;

					BPTR lock = Lock(path, ACCESS_READ);

					if(lock)
					{
						UnLock(lock);
						command = "GetDeficonPath PATH=\"" + String(path) + "\"";
						if(rexx_send((char *) "AMBIENT", (char *) command.utf8().data()))
					    {
							iconPath = String(rexx_result());
						}
					}

					process->pr_WindowPtr = oldwinptr;
				}

				free(path);
			}
		}

		if((type == MV_DownloadList_Type_InProgress && dl->mimetype) || type != MV_DownloadList_Type_InProgress)
		{
		
			dl->iconobj  = NewObject(geticonclass(), NULL,
								    MA_Icon_Path, iconPath.utf8().data(),
								    MUIA_Weight, 0,
								    TAG_DONE);

			if(dl->iconobj)
			{
				dl->iconimg	= (APTR) DoMethod(obj, MUIM_List_CreateImage, dl->iconobj, 0);
			}
		}
	}
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		InputListFrame,
        MUIA_CycleChain, 1,
		MUIA_List_MinLineHeight, 24,
		MUIA_List_Title, TRUE,
		MUIA_ContextMenu, TRUE,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;
		data->cmenu=NULL;
		doset(obj, data, msg->ops_AttrList);
	}
	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;
	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
	}
	return DOSUPER;
}

DEFMMETHOD(Setup)
{
	ULONG rc;

	if((rc=DOSUPER))
	{
		struct downloadnode *dl;
		ULONG i = 0;

		do
		{
			DoMethod(obj, MUIM_List_GetEntry, i, (struct downloadnode *) &dl);

			if (dl)
			{
				if(!dl->gaugeimg)
					dl->gaugeimg = (Object *) DoMethod(obj, MUIM_List_CreateImage, (Object *) dl->gaugeobj, 0);

				if(!dl->iconimg && dl->iconobj)
					dl->iconimg = (Object *) DoMethod(obj, MUIM_List_CreateImage, (Object *) dl->iconobj, 0);
			}

			i++;
		}
		while (dl);
	}

	return rc;
}

DEFMMETHOD(Cleanup)
{
	struct downloadnode *dl;
	ULONG i = 0;

	do
	{
		DoMethod(obj, MUIM_List_GetEntry, i, (struct downloadnode *) &dl);

		if (dl)
		{
			if(dl->gaugeimg)
			{
				DoMethod(obj, MUIM_List_DeleteImage, dl->gaugeimg);
				dl->gaugeimg = NULL;
			}

			if(dl->iconimg)
			{
				DoMethod(obj, MUIM_List_DeleteImage, dl->iconimg);
				dl->iconimg = NULL;
			}
		}

		i++;
	}
	while (dl);

	return DOSUPER;
}

DEFSET
{
	GETDATA;
	doset(obj, data, msg->ops_AttrList);
	return DOSUPER;
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
	GETDATA;
	struct downloadnode *dl = (struct downloadnode *) msg->entry;

	if (dl)
	{
		STATIC char buf0[512];

		if( (ULONG)msg->array[-1] % 2 )
		{
#if !OS(AROS)
			/* This code overrides internal data structures and causes a crash on AROS */
			msg->array[-9] = (STRPTR) 10;
#endif
		}

		create_icon_if_needed(obj, dl, data->type);

		if(!dl->iconimg)
			snprintf(buf0, sizeof(buf0), "%s", dl->filename);
		else
			snprintf(buf0, sizeof(buf0), "\033O[%08lx] %s", (unsigned long) dl->iconimg, dl->filename);

		switch(data->type)
		{
			case MV_DownloadList_Type_InProgress:
			{
				STATIC char buf1[32];
				STATIC char buf2[64];
				STATIC char buf3[64];
				STATIC char buf4[32];
				STATIC char buf5[128];
				char progresstext[32];
				char remainingtext[64];
				char sizetext[32];
				char speedtext[32];

				if(dl->size)
				{
					format_size(sizetext, sizeof(sizetext), dl->size);
					snprintf(buf1, sizeof(buf1), "\033r%s", sizetext);
				}
				else
				{
					stccpy(buf1, GSI(MSG_DOWNLOADLIST_SIZE_UNKNOWN), sizeof(buf1));
				}

				format_size(progresstext, sizeof(progresstext), dl->done);
				if(dl->size)
				{
					snprintf(buf2, sizeof(buf2), "%s (%ld%%)", progresstext, (unsigned long)((float)dl->done / (float)dl->size * 100.f));
				}
				else
				{
					snprintf(buf2, sizeof(buf2), "%s", progresstext);
				}

				if(dl->speed)
				{
					format_size(speedtext, sizeof(speedtext), dl->speed);
					snprintf(buf3, sizeof(buf3), "\033r%s/s", speedtext);
				}
				else
				{
					buf3[0] = 0;
				}

				if(dl->size && dl->done)
				{
					dl->remainingtime = (currentTime() - dl->starttime) * (dl->size - dl->done) / dl->done;
					format_time_compact(remainingtext, sizeof(remainingtext), (ULONG) dl->remainingtime);
					snprintf(buf5, sizeof(buf5), "\033r%s", remainingtext);

					/*
					struct timeval timeval;
					GetSysTime(&timeval);
					timeval.tv_secs += (ULONG) ((currentTime() - dl->starttime) * dl->size / dl->done);
					Amiga2Date(timeval.tv_secs, &dl->eta);
					snprintf(buf5, sizeof(buf5), "%.2d:%.2d:%.2d", dl->eta.hour, dl->eta.min, dl->eta.sec);
					*/
				}
				else
				{
					buf5[0] = 0;
				}

				if(dl->size)
				{
					snprintf(buf4, sizeof(buf4), "\33O[%08lx]", (long unsigned int) dl->gaugeimg);
				}
				else
				{
					buf4[0] = 0;
				}

				*msg->array++  = buf0;
				*msg->array++  = buf1;
				*msg->array++  = buf4;
				*msg->array++  = buf2;
				*msg->array++  = buf3;
				*msg->array++  = buf5;
				*msg->array++  = dl->path;
				*msg->array++  = dl->url;
				*msg->array++  = dl->originurl;
			}
			break;

			case MV_DownloadList_Type_Finished:
			{
				*msg->array++ = buf0;
				*msg->array++ = dl->path;
				*msg->array++ = dl->url;
				*msg->array++ = dl->originurl;
			}
			break;

			case MV_DownloadList_Type_Failed:
			{
				*msg->array++ = buf0;
				*msg->array++ = dl->status;
				*msg->array++ = dl->path;
				*msg->array++ = dl->url;
				*msg->array++ = dl->originurl;
			}
			break;
		}
	}
	else
	{
		switch(data->type)
		{
			case MV_DownloadList_Type_InProgress:
			{
				*msg->array++  = GSI(MSG_DOWNLOADLIST_NAME);
				*msg->array++  = GSI(MSG_DOWNLOADLIST_SIZE);
				*msg->array++  = GSI(MSG_DOWNLOADLIST_PROGRESS);
				*msg->array++  = GSI(MSG_DOWNLOADLIST_DETAIL);
				*msg->array++  = GSI(MSG_DOWNLOADLIST_SPEED);
				*msg->array++  = GSI(MSG_DOWNLOADLIST_ETA);
				*msg->array++  = GSI(MSG_DOWNLOADLIST_PATH);
				*msg->array++  = GSI(MSG_DOWNLOADLIST_URL);
				*msg->array++  = GSI(MSG_DOWNLOADLIST_DOWNLOADPAGE);
			}
			break;

			case MV_DownloadList_Type_Finished:
			{
				*msg->array++ = GSI(MSG_DOWNLOADLIST_NAME);
				*msg->array++ = GSI(MSG_DOWNLOADLIST_PATH);
				*msg->array++ = GSI(MSG_DOWNLOADLIST_URL);
				*msg->array++ = GSI(MSG_DOWNLOADLIST_DOWNLOADPAGE);
			}
			break;

			case MV_DownloadList_Type_Failed:
			{
				*msg->array++ = GSI(MSG_DOWNLOADLIST_NAME);
				*msg->array++ = GSI(MSG_DOWNLOADLIST_STATUS);
				*msg->array++ = GSI(MSG_DOWNLOADLIST_PATH);
				*msg->array++ = GSI(MSG_DOWNLOADLIST_URL);
				*msg->array++ = GSI(MSG_DOWNLOADLIST_DOWNLOADPAGE);
			}
			break;
		}
	}
	return TRUE;
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	struct MUI_List_TestPos_Result res;
	struct downloadnode *dl;

	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
		data->cmenu = NULL;
	}

	if (DoMethod(obj, MUIM_List_TestPos, msg->mx, msg->my, &res) && (res.entry != -1))
	{
		DoMethod(obj, MUIM_List_GetEntry, res.entry, (ULONG *)&dl);

		if(dl)
		{
			data->cmenu = MenustripObject,
					MUIA_Family_Child, MenuObjectT(dl->filename),
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_DOWNLOADLIST_OPEN_FILE),
						MUIA_UserData, POPMENU_OPEN_FILE,
	                    End,
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_DOWNLOADLIST_OPEN_DIRECTORY),
						MUIA_UserData, POPMENU_OPEN_DIRECTORY,
						End,
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_DOWNLOADLIST_OPEN_DOWNLOADPAGE),
						MUIA_UserData, POPMENU_OPEN_PAGE,
						End,
	                End,
	            End;
		}
	}
	return (ULONG)data->cmenu;
}

DEFMMETHOD(ContextMenuChoice)
{
	struct downloadnode *dl;

	DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&dl);

	if(dl)
	{
		ULONG udata = muiUserData(msg->item);

		switch(udata)
		{
			case POPMENU_OPEN_FILE:
			{
				char fullname[1024];
				stccpy(fullname, dl->path, sizeof(fullname));
				AddPart(fullname, dl->filename, sizeof(fullname));
				OpenWorkbenchObjectA(fullname, NULL);
			}
			break;

			case POPMENU_OPEN_DIRECTORY:
			{
				struct Screen *ambientScreen;
				char fullname[1024];
				stccpy(fullname, dl->path, sizeof(fullname));
				OpenWorkbenchObjectA(fullname, NULL);
				ambientScreen = LockPubScreen("Workbench");
				ScreenToFront(ambientScreen);
				UnlockPubScreen(NULL, ambientScreen);
			}
			break;

			case POPMENU_OPEN_PAGE:
			{
				DoMethod(app, MM_OWBApp_AddBrowser, NULL, dl->originurl, FALSE, NULL, FALSE, FALSE, TRUE);
			}
			break;
		}		 
	}
	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, downloadlistclass)
