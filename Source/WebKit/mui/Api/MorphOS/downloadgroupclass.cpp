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

#include "Api/WebView.h"
#include "URL.h"
#include "FileIOLinux.h"
#include "WebDownload.h"
#include "WebDownloadPrivate.h"
#include <wtf/CurrentTime.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <clib/debug_protos.h>
#include <clib/macros.h>

#include "gui.h"
#include "utils.h"

using namespace WebCore;


enum { PAGE_IN_PROGRESS, PAGE_FINISHED, PAGE_FAILED };

struct Data
{
	Object *lv_downloads;
	Object *lv_finished;
	Object *lv_failed;
	Object *gr_pages;
	Object *gr_titles;
	Object *titles[3];

	ULONG showimg;
	UBYTE added;
	struct MUI_InputHandlerNode ihnode;
};

#define LABEL(x) (STRPTR)MSG_DOWNLOADGROUP_##x

STATIC STRPTR dltitles[] =
{
	LABEL(IN_PROGRESS),
	LABEL(FINISHED),
	LABEL(FAILED),
	NULL
};

static void cycles_init(void)
{
	APTR arrays[] = { (APTR) dltitles, NULL };


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

void restore_download_state(Object *obj, struct Data *data)
{
	Vector<String> downloads;
	OWBFile *downloadFile = new OWBFile("PROGDIR:Conf/downloads.prefs");

    if (!downloadFile)
		return;

	if (downloadFile->open('r') == -1)
	{
		delete downloadFile;
		return;
    }

	char *buffer = downloadFile->read(downloadFile->getSize());
	String fileBuffer = buffer;
	delete [] buffer;
    downloadFile->close();
	delete downloadFile;

	fileBuffer.split("\n", true, downloads);
	for(size_t i = 0; i < downloads.size(); i++)
	{
		Vector<String> downloadAttributes;
		downloads[i].split("\1", true, downloadAttributes);

		if(downloadAttributes.size() >= 5) // Now 6 for new format
		{
			struct downloadnode *dl = (struct downloadnode *) malloc(sizeof(*dl));

			if (dl)
			{
				memset(dl, 0, sizeof(*dl));

				dl->originurl = strdup(downloadAttributes.size() > 5 ? downloadAttributes[5].latin1().data() : "");
				dl->filename  = strdup(downloadAttributes[4].latin1().data());
				dl->path      = strdup(downloadAttributes[3].latin1().data());
				dl->url       = strdup(downloadAttributes[2].latin1().data());
				dl->size      = 0;
				dl->done      = 0;
				dl->speed     = 0;
				dl->prevdone  = 0;
				dl->starttime = dl->lastupdatetime = currentTime();
				dl->gaugeimg  = NULL;
				dl->gaugeobj  = GaugeObject, GaugeFrame, MUIA_FixWidth, 64, MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Current, 0, MUIA_Gauge_Max, 100, End;
				dl->iconimg   = NULL;
				dl->iconobj   = NULL;

				if(downloadAttributes[0] == "D")
				{
					dl->state = WEBKIT_WEB_DOWNLOAD_STATE_STARTED;
					stccpy(dl->status, GSI(MSG_DOWNLOADGROUP_INTERRUPTED), sizeof(dl->status)); // Translate
					DoMethod(data->lv_failed, MUIM_List_InsertSingle, dl, MUIV_List_Insert_Bottom);
				}
				else if(downloadAttributes[0] == "E")
				{
					dl->state = WEBKIT_WEB_DOWNLOAD_STATE_ERROR;
					stccpy(dl->status, downloadAttributes[1].latin1().data(), sizeof(dl->status));
					DoMethod(data->lv_failed, MUIM_List_InsertSingle, dl, MUIV_List_Insert_Bottom);
				}
				else if(downloadAttributes[0] == "C")
				{
					dl->state = WEBKIT_WEB_DOWNLOAD_STATE_CANCELLED;
					stccpy(dl->status, GSI(MSG_DOWNLOADGROUP_CANCELLED_BY_USER), sizeof(dl->status));
					DoMethod(data->lv_failed, MUIM_List_InsertSingle, dl, MUIV_List_Insert_Bottom);
				}
				else if(downloadAttributes[0] == "F")
				{
					dl->state = WEBKIT_WEB_DOWNLOAD_STATE_FINISHED;
					*dl->status = '\0';
					DoMethod(data->lv_finished, MUIM_List_InsertSingle, dl, MUIV_List_Insert_Bottom);
				}

				ADDTAIL(&download_list, dl);
			}
		}
	}
}

void save_download_state()
{
	APTR n;

	if(!getv(app, MA_OWBApp_DownloadSave))
		return;

	OWBFile *downloadFile = new OWBFile("PROGDIR:Conf/downloads.prefs");
	if(!downloadFile)
		return;

	if (downloadFile->open('w') == -1)
	{
		delete downloadFile;
		return;
    }

	ITERATELIST(n, &download_list)
	{
		struct downloadnode *dl = (struct downloadnode *) n;
		char *state;

		switch(dl->state)
		{
			case WEBKIT_WEB_DOWNLOAD_STATE_STARTED:
				state = "D";
				break;
			case WEBKIT_WEB_DOWNLOAD_STATE_ERROR:
				state = "E";
				break;
			case WEBKIT_WEB_DOWNLOAD_STATE_CANCELLED:
				state = "C";
				break;
			case WEBKIT_WEB_DOWNLOAD_STATE_FINISHED:
				state = "F";
				break;
			default:
				state = "E";
				break;
		}

		// Shouldn't be needed, but filter potentially corrupted url
		String url    = dl->url;
		String status = dl->status;
		if(url.startsWith("http") || url.startsWith("ftp"))
		{
			url.replace("\n", "\0");
			url.replace("\r", "\0");
			status.replace("\n", "\0");
			status.replace("\r", "\0");
			downloadFile->write(String::format("%s\1%s\1%s\1%s\1%s\1%s\n", state, status.latin1().data(), url.latin1().data(), dl->path, dl->filename, dl->originurl));
		}
	}

	downloadFile->close();
	delete downloadFile;
}

struct downloadnode* download_create(char *url, char *filename, char *path, char *originurl)
{
	struct downloadnode *dl = (struct downloadnode *) malloc(sizeof(*dl));

	if (dl)
	{
		memset(dl, 0, sizeof(*dl));

		dl->filename  = strdup(filename);
		dl->path      = strdup(path);
		dl->url       = strdup(url);
		dl->originurl = strdup(originurl);
		dl->mimetype  = NULL;
		dl->size      = 0;
		dl->done      = 0;
		dl->speed     = 0;
		dl->prevdone  = 0;
		dl->starttime = dl->lastupdatetime = currentTime();
		dl->status[0] = '\0';
		dl->state     = WEBKIT_WEB_DOWNLOAD_STATE_STARTED;
		dl->gaugeimg  = NULL;
		dl->gaugeobj  = GaugeObject, GaugeFrame, MUIA_FixWidth, 64, MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Current, 0, MUIA_Gauge_Max, 100, End;
		dl->iconimg   = NULL;
		dl->iconobj   = NULL;

		set(app, MA_OWBApp_DownloadsInProgress, getv(app, MA_OWBApp_DownloadsInProgress) + 1);

		ADDTAIL(&download_list, dl);

		save_download_state();
	}

	return dl;
}

void download_dispose_objects(struct Data *data, struct downloadnode *dl)
{
	if(dl->gaugeobj)
	{
		MUI_DisposeObject((Object *) dl->gaugeobj);
		dl->gaugeobj = NULL;
	}

	if(dl->iconobj)
	{
		MUI_DisposeObject((Object *) dl->iconobj);
		dl->iconobj = NULL;
	}
}

void download_clear_images(struct Data *data, struct downloadnode *dl)
{
	if(dl->gaugeimg)
	{
		DoMethod(data->lv_downloads, MUIM_List_DeleteImage, dl->gaugeimg);
		dl->gaugeimg = NULL;
	}

	if(dl->iconimg)
	{
		DoMethod(data->lv_downloads, MUIM_List_DeleteImage, dl->iconimg);
		dl->iconimg = NULL;
	}
}

void download_delete(struct Data *data, struct downloadnode *dl)
{
	REMOVE((APTR) dl);
	download_clear_images(data, dl);
	download_dispose_objects(data, dl);
	free(dl->path);
	free(dl->filename);
	free(dl->url);
	free(dl->originurl);
	free(dl->mimetype);
	free(dl);
}

DEFNEW
{
	Object *lv_downloads, *lv_finished, *lv_failed;
	Object *rem_finished, *rem_finished_all;
	Object *rem_failed, *rem_failed_all, *retry_download_finished, *retry_download_failed;
	Object *cancel, *cancel_all;
	Object *gr_pages, *gr_titles, *titles[3];

	cycles_init();

//	  kprintf("<%s> <%s> <%s>\n", dltitles[PAGE_IN_PROGRESS], dltitles[PAGE_FINISHED], dltitles[PAGE_FAILED]);

	obj = (Object *) DoSuperNew(cl, obj,
			Child, gr_pages = VGroup,
								MUIA_Background, MUII_RegisterBack,
								MUIA_Frame, MUIV_Frame_Register,
								MUIA_Group_PageMode, TRUE,
#if OS(AROS) // Needed as long as MUIV_Frame_Register is not implemented, see area.c/set_inner_sizes (right now random memory access)
                                InnerSpacing(0, 0),
#endif
				Child, gr_titles = MUI_NewObject(MUIC_Title,
										MUIA_CycleChain, 1,
										Child, titles[PAGE_IN_PROGRESS] = TextObject, MUIA_Text_Contents, dltitles[PAGE_IN_PROGRESS], End,
										Child, titles[PAGE_FINISHED]    = TextObject, MUIA_Text_Contents, dltitles[PAGE_FINISHED], End,
										Child, titles[PAGE_FAILED]      = TextObject, MUIA_Text_Contents, dltitles[PAGE_FAILED], End,
										TAG_DONE),

				Child, VGroup,
					Child, lv_downloads = (Object *) NewObject(getdownloadlistclass(), NULL,
													MUIA_List_Format, "BAR,MIW=-1 MAW=-2 BAR,MIW=-1 BAR, MIW=-1 BAR,BAR,BAR,BAR,BAR,",
													MA_DownloadList_Type, MV_DownloadList_Type_InProgress,
													TAG_DONE),
					Child, HGroup,
						Child, cancel = (Object *) MakeButton(GSI(MSG_DOWNLOADGROUP_ABORT)),
						Child, cancel_all = (Object *) MakeButton(GSI(MSG_DOWNLOADGROUP_ABORT_ALL)),
					End,
				End,

				Child, VGroup,
					Child, lv_finished = (Object *) NewObject(getdownloadlistclass(), NULL,
													MUIA_List_Format, "MIW=-1 MAW=-2 BAR,BAR,BAR,",
													MA_DownloadList_Type, MV_DownloadList_Type_Finished,
													TAG_DONE),
					Child, HGroup,
						Child, retry_download_finished = (Object *) MakeButton(GSI(MSG_DOWNLOADGROUP_RETRY)),
						Child, rem_finished = (Object *) MakeButton(GSI(MSG_DOWNLOADGROUP_REMOVE)),
						Child, rem_finished_all = (Object *) MakeButton(GSI(MSG_DOWNLOADGROUP_REMOVE_ALL)),
					End,
				End,

				Child, VGroup,
					Child, lv_failed = (Object *) NewObject(getdownloadlistclass(), NULL,
													MUIA_List_Format, "MIW=-1 MAW=-2 BAR,BAR,BAR,BAR,",
													MA_DownloadList_Type, MV_DownloadList_Type_Failed,
													TAG_DONE),

					Child, HGroup,
						Child, retry_download_failed = (Object *) MakeButton(GSI(MSG_DOWNLOADGROUP_RETRY)),
						Child, rem_failed = (Object *) MakeButton(GSI(MSG_DOWNLOADGROUP_REMOVE)),
						Child, rem_failed_all = (Object *) MakeButton(GSI(MSG_DOWNLOADGROUP_REMOVE_ALL)),
					End,
				End,
			End,
			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->lv_downloads = lv_downloads;
		data->lv_finished = lv_finished;
		data->lv_failed = lv_failed;
		data->gr_pages = gr_pages;
		data->gr_titles = gr_titles;
		data->titles[PAGE_IN_PROGRESS] = titles[PAGE_IN_PROGRESS];
		data->titles[PAGE_FINISHED]    = titles[PAGE_FINISHED];
		data->titles[PAGE_FAILED]      = titles[PAGE_FAILED];

		data->ihnode.ihn_Object = obj;
		data->ihnode.ihn_Flags = MUIIHNF_TIMER;
		data->ihnode.ihn_Millis = 1000;
		data->ihnode.ihn_Method = MM_Download_ComputeSpeed;

		DoMethod(rem_finished,     MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MM_Download_RemoveEntry, lv_finished, 0);
		DoMethod(rem_finished_all, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MM_Download_RemoveEntry, lv_finished, 1);
		DoMethod(rem_failed,       MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MM_Download_RemoveEntry, lv_failed, 0);
		DoMethod(rem_failed_all,   MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MM_Download_RemoveEntry, lv_failed, 1);
		DoMethod(retry_download_finished, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2, MM_Download_Retry, lv_finished);
		DoMethod(retry_download_failed,   MUIM_Notify, MUIA_Pressed, FALSE, obj, 2, MM_Download_Retry, lv_failed);
		DoMethod(cancel,           MUIM_Notify, MUIA_Pressed, FALSE, obj, 2, MM_Download_Cancel, 0);
		DoMethod(cancel_all,       MUIM_Notify, MUIA_Pressed, FALSE, obj, 2, MM_Download_Cancel, 1);
		
		DoMethod(gr_pages,         MUIM_Notify, MUIA_Group_ActivePage, MUIV_EveryTime, obj, 2, MM_Download_UnhilightPage, MUIV_TriggerValue);

		restore_download_state(obj, data);
	}

	return (IPTR)obj;
}

DEFMMETHOD(Show)
{
	ULONG rc = DOSUPER;

	if(rc)
	{
		GETDATA;

		if(!data->added)
		{
			DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR)&data->ihnode);
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
		DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->ihnode);
		data->added = FALSE;
	}

	return DOSUPER;
}

DEFMMETHOD(Setup)
{
	ULONG rc;
	GETDATA;

	if((rc=DOSUPER))
	{
		data->showimg = TRUE;
	}

	return rc;
}

DEFMMETHOD(Cleanup)
{
	GETDATA;

	data->showimg = FALSE;

	return DOSUPER;
}

DEFDISP
{
	GETDATA;
	APTR n, m;

	ITERATELISTSAFE(n, m, &download_list)
	{
		download_delete(data, (struct downloadnode *) n);
	}

	return DOSUPER;
}

DEFMMETHOD(List_InsertSingle)
{
	GETDATA;

	DoMethod(obj, MM_Download_HilightPage, PAGE_IN_PROGRESS);

	return DoMethodA(data->lv_downloads, (_Msg_*)msg);
}

#define STUNTZI_BUG 0

DEFMMETHOD(List_Redraw)
{
	GETDATA;
	/*
	return DoMethod(app, MUIM_Application_PushMethod, data->lv_downloads, 3 | MUIV_PushMethod_Delay(50) | MUIF_PUSHMETHOD_SINGLE,
			 MUIM_List_Redraw, MUIV_List_Redraw_Entry, msg->entry);
	*/

	struct downloadnode *dl = (struct downloadnode *) msg->entry;
	if(dl && data->showimg)
	{
#if !STUNTZI_BUG
		if(dl->gaugeimg)
		{
			DoMethod(data->lv_downloads, MUIM_List_DeleteImage, dl->gaugeimg);
			dl->gaugeimg = NULL;
		}
#endif

		if(dl->gaugeobj)
		{
			nnset(dl->gaugeobj, MUIA_Gauge_Current, (ULONG)((float)dl->done / (float)dl->size * 100.f));
			if(!dl->gaugeimg)
				dl->gaugeimg = (Object *) DoMethod(data->lv_downloads, MUIM_List_CreateImage, (Object *) dl->gaugeobj, 0);
		}

		DoMethod(data->lv_downloads, MUIM_List_Redraw, MUIV_List_Redraw_Entry, msg->entry);
	}

	return 0;
}

DEFSMETHOD(Download_Done)
{
	GETDATA;
	struct downloadnode *dl;
	ULONG i = 0;

	do
	{
		DoMethod(data->lv_downloads, MUIM_List_GetEntry, i, (struct downloadnode *) &dl);

		if ((APTR) dl == msg->entry)
		{
			DoMethod(data->lv_downloads, MUIM_List_Remove, i);

			download_clear_images(data, dl);
			download_dispose_objects(data, dl);

			DoMethod(data->lv_finished, MUIM_List_InsertSingle, msg->entry, MUIV_List_Insert_Bottom);
			DoMethod(obj, MM_Download_HilightPage, PAGE_FINISHED);

			set(app, MA_OWBApp_DownloadsInProgress, getv(app, MA_OWBApp_DownloadsInProgress) - 1);

			if(getv(app, MA_OWBApp_DownloadAutoClose) && getv(app, MA_OWBApp_DownloadsInProgress) == 0)
			{
				if(muiRenderInfo(obj))
				{
					DoMethod(app, MUIM_Application_PushMethod, _win(obj), 3, MUIM_Set, MUIA_Window_Open, FALSE);
				}
			}

			dl->state = WEBKIT_WEB_DOWNLOAD_STATE_FINISHED;

			// Notify
			struct external_notification notification = { "OWB.TRANSFERDONE", dl->filename };
			send_external_notification(&notification);

			break;
		}

		i++;
	}
	while (dl);

	save_download_state();

	return 0;
}

DEFSMETHOD(Download_Cancelled)
{
	GETDATA;
	struct downloadnode *dl;
	ULONG i = 0;

	do
	{
		DoMethod(data->lv_downloads, MUIM_List_GetEntry, i, (struct downloadnode *) &dl);

		if ((APTR) dl == msg->entry)
		{
			DoMethod(data->lv_downloads, MUIM_List_Remove, i);
			set(app, MA_OWBApp_DownloadsInProgress, getv(app, MA_OWBApp_DownloadsInProgress) - 1);

			// Notify
			struct external_notification notification = { "OWB.TRANSFERCANCELLED", dl->filename };
			send_external_notification(&notification);

			download_delete(data, dl); // This one can be freed at that point
			break;
		}

		i++;
	}
	while (dl);

	return 0;
}

DEFSMETHOD(Download_Error)
{
	GETDATA;

	if(msg->entry)
	{
		struct downloadnode *dl;
		ULONG i = 0;

		if(msg->error)
		{
			dl = (struct downloadnode *) msg->entry;
			stccpy(dl->status, msg->error, sizeof(dl->status));
			dl->state = WEBKIT_WEB_DOWNLOAD_STATE_ERROR;
		}

		do
		{
			DoMethod(data->lv_downloads, MUIM_List_GetEntry, i, (struct downloadnode *) &dl);

			if ((APTR) dl == msg->entry)
			{
				DoMethod(data->lv_downloads, MUIM_List_Remove, i);

				download_clear_images(data, dl);
				download_dispose_objects(data, dl);

				DoMethod(data->lv_failed, MUIM_List_InsertSingle, msg->entry, MUIV_List_Insert_Bottom);
				DoMethod(obj, MM_Download_HilightPage, PAGE_FAILED);

				set(app, MA_OWBApp_DownloadsInProgress, getv(app, MA_OWBApp_DownloadsInProgress) - 1);

				// Notify
				struct external_notification notification = { "OWB.TRANSFERFAILED", dl->filename };
				send_external_notification(&notification);

				break;
			}

			i++;
		}
		while (dl);

		save_download_state();
	}

	return 0;
}

DEFSMETHOD(Download_RemoveEntry)
{
	GETDATA;

	set((Object *) msg->listview, MUIA_List_Quiet, TRUE);

	if (msg->all)
	{
		struct downloadnode *dl;

		do
		{
			DoMethod((Object *) msg->listview, MUIM_List_GetEntry, 0, (struct downloadnode *) &dl);

			if (dl)
			{
				DoMethod((Object *) msg->listview, MUIM_List_Remove, MUIV_List_Remove_First);				 

				download_delete(data, dl);
			}
		}
		while (dl);
	}
	else
	{
		struct downloadnode *dl;
		DoMethod((Object *) msg->listview, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct downloadnode *) &dl);
		
		if(dl)
		{
			DoMethod((Object *) msg->listview, MUIM_List_Remove, MUIV_List_Remove_Active);

			download_delete(data, dl);
		}
	}

	set((Object *) msg->listview, MUIA_List_Quiet, FALSE);
	
	save_download_state();

	return 0;
}

DEFSMETHOD(Download_Cancel)
{
	GETDATA;

	if (msg->all)
	{
		struct downloadnode *dl;

		do
		{
			DoMethod(data->lv_downloads, MUIM_List_GetEntry, 0, (struct downloadnode *) &dl);

			if (dl)
			{
                WebDownload *webdownload = (WebDownload *)dl->webdownload;
				webdownload->cancel();
				dl->webdownload = NULL; // just in case
				dl->state = WEBKIT_WEB_DOWNLOAD_STATE_CANCELLED;

				stccpy(dl->status, GSI(MSG_DOWNLOADGROUP_CANCELLED_BY_USER), sizeof(dl->status));

				DoMethod(data->lv_downloads, MUIM_List_Remove, MUIV_List_Remove_First);

				download_clear_images(data, dl);
				download_dispose_objects(data, dl);

				DoMethod(data->lv_failed, MUIM_List_InsertSingle, dl, MUIV_List_Insert_Bottom);
			}
		}
		while (dl);

		set(app, MA_OWBApp_DownloadsInProgress, 0);
	}
	else
	{
		struct downloadnode *dl;
		DoMethod(data->lv_downloads, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct downloadnode *) &dl);

		if(dl)
		{
			WebDownload *webdownload = (WebDownload *)dl->webdownload;
			webdownload->cancel();
			dl->webdownload = NULL; // just in case
			dl->state = WEBKIT_WEB_DOWNLOAD_STATE_CANCELLED;

			// Notify
			struct external_notification notification = { "OWB.TRANSFERCANCELLED", dl->filename };
			send_external_notification(&notification);

			stccpy(dl->status, GSI(MSG_DOWNLOADGROUP_CANCELLED_BY_USER), sizeof(dl->status));

			DoMethod(data->lv_downloads, MUIM_List_Remove, MUIV_List_Remove_Active);

			download_clear_images(data, dl);
			download_dispose_objects(data, dl);

			DoMethod(data->lv_failed, MUIM_List_InsertSingle, dl, MUIV_List_Insert_Bottom);

			set(app, MA_OWBApp_DownloadsInProgress, getv(app, MA_OWBApp_DownloadsInProgress) - 1);
		}
	}

	save_download_state();

	return 0;
}

DEFSMETHOD(Download_Retry)
{
	GETDATA;
	struct downloadnode *dl;

	DoMethod((Object *) msg->listview, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct downloadnode *) &dl);

	if(dl)
	{
		DoMethod((Object *) msg->listview, MUIM_List_Remove, MUIV_List_Remove_Active);

		Object *activebrowser = (Object *) getv(app, MA_OWBApp_ActiveBrowser);

		if(activebrowser)
		{
			BalWidget *widget = (BalWidget *) getv(activebrowser, MA_OWBBrowser_Widget);

			if(widget)
			{
				TransferSharedPtr<WebDownloadDelegate> downloadDelegate = widget->webView->downloadDelegate();

			    if(downloadDelegate)
			    {
					URL url(ParsedURLString, dl->url);
					WebDownload* download = WebDownload::createInstance(url, downloadDelegate);
			        download->start();
			    }
			}
		}

		download_delete(data, dl);
	}

	save_download_state();

	return 0;
}

DEFTMETHOD(Download_ComputeSpeed)
{
	GETDATA;
	struct downloadnode *dl;
	ULONG i = 0;

	for(;;)
	{
		DoMethod((Object *) data->lv_downloads, MUIM_List_GetEntry, i, (struct downloadnode *) &dl);

		if (dl)
		{
			dl->speed = (ULONG) (dl->done - dl->prevdone);
			dl->prevdone = dl->done;
		}
		else
		{
			break;
		}

		i++;
	}

	return 0;
}

DEFSMETHOD(Download_HilightPage)
{
	GETDATA;

	if(getv(data->gr_pages, MUIA_Group_ActivePage) != msg->pagenum)
	{
		char hilighted[sizeof("\033b") + strlen(dltitles[msg->pagenum]) + 1];
		snprintf(hilighted, sizeof(hilighted), "\033b%s", dltitles[msg->pagenum]);
		set(data->titles[msg->pagenum], MUIA_Text_Contents, hilighted);
	}

	return 0;
}

DEFSMETHOD(Download_UnhilightPage)
{
	GETDATA;
	set(data->titles[msg->pagenum], MUIA_Text_Contents, dltitles[msg->pagenum]);

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECSMETHOD(Download_Done)
DECSMETHOD(Download_Error)
DECSMETHOD(Download_Cancelled)
DECSMETHOD(Download_RemoveEntry)
DECSMETHOD(Download_Cancel)
DECSMETHOD(Download_Retry)
DECMMETHOD(List_InsertSingle)
DECMMETHOD(List_Redraw)
DECTMETHOD(Download_ComputeSpeed)
DECSMETHOD(Download_HilightPage)
DECSMETHOD(Download_UnhilightPage)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, downloadgroupclass)
