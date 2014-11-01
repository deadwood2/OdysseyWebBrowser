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
#include <Api/WebFrame.h>
#include <Api/WebView.h>
#include "Page.h"
#include "Frame.h"
#include <wtf/CurrentTime.h>

extern "C"
{
	#include <fontconfig/fontconfig.h>
	#include <cairo.h>
}

#if !OS(AROS)
#include <sys/signal.h>
#endif

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <clib/debug_protos.h>

#include "gui.h"
#include "utils.h"

#ifndef MUIM_Process_Launch

/* Methods */
#define MUIM_Process_Launch                 0x80425df7 /* V20 */
struct  MUIP_Process_Launch                 { ULONG MethodID; };

/* Attributes */
#define MUIA_Process_AutoLaunch             0x80428855 /* V20 i.. ULONG             */
#define MUIA_Process_Name                   0x8042732b /* V20 i.. ULONG             */
#define MUIA_Process_Priority               0x80422a54 /* V20 i.. ULONG             */
#define MUIA_Process_Task                   0x8042b123 /* V20 ..g ULONG             */

#endif

#define USE_SHARED_FONTCONFIG 0

using namespace WebCore;

struct Data
{
	Object *proc;
	Object *gauge;
	char infotext[256];
	struct Task *maintask;
	LONG StartupBit;
	ULONG running;
};

static ULONG indexation_needed = TRUE;
static Object *g_splash = NULL;

/* Special callback to report fontconfig cache creation progress */
extern void (*fontconfig_progress_callback)(int current, int total, char *currentfile);

void myfontconfig_progress_callback(int current, int total, char *currentfile)
{
	if(indexation_needed)
	{
		DoMethod(app, MUIM_Application_PushMethod, g_splash, 3, MUIM_Set, MUIA_Window_Open, TRUE);
		indexation_needed = FALSE;
	}

	DoMethod(app, MUIM_Application_PushMethod, g_splash, 4, MM_SplashWindow_Update, current+1, total, currentfile);
}

void fontconfig_testcache(void)
{
    FcObjectSet *os = 0;
    FcFontSet	*fs;
    FcPattern   *pat;

#if !OS(AROS)
    signal(SIGINT, SIG_IGN);
#endif

    //kprintf("[fontcache thread] doing stuff\n");

#if !USE_SHARED_FONTCONFIG
#if !OS(AROS)
   fontconfig_progress_callback = myfontconfig_progress_callback;
#else
   myfontconfig_progress_callback(1, 2, "FONTS:Loading");
#endif
#else
    FcExtInsertProgressCallback((FcExtProgressCallback)myfontconfig_progress_callback);
#endif

    if (!FcInit ())
    {
		return;
    }

    //kprintf("[fontcache thread] still doing stuff\n");

    pat = FcNameParse ((FcChar8 *) ":");
    os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, (char *) 0);
    fs = FcFontList (0, pat, os);
    FcObjectSetDestroy (os);
    if (pat)
		FcPatternDestroy (pat);

	if(!fs || !fs->nfont)
	{
		// We should fail here
	}

	//kprintf("[fontcache thread] always doing stuff\n");

    if (fs)
    {
		int	j;

		//kprintf("[fontcache thread] fs->nfont %d\n", fs->nfont);

		for (j = 0; j < fs->nfont; j++)
		{
			ULONG found = FALSE;
		    FcChar8 *file;
			if (FcPatternGetString (fs->fonts[j], FC_FAMILY, 0, &file) == FcResultMatch)
			{
				APTR n;

				//kprintf("[fontcache thread] font file <%s>\n", file);

				ITERATELIST(n, &family_list)
				{
					struct familynode *fn = (struct familynode *)n;
					if(!strcmp(fn->family, (char *)file))
					{
						found = TRUE;
						break;
					}
				}

				if(!found)
				{
					int size = sizeof(struct familynode) + strlen((char *) file) + 1;
					struct familynode *fn = (struct familynode *) malloc(size);

					if(n)
					{
						strcpy(fn->family, (char *) file);
						ADDTAIL(&family_list, (APTR) fn);
					}
				}
			}
		}
		FcFontSetDestroy (fs);
    }

#if USE_SHARED_FONTCONFIG
	FcExtRemoveProgressCallback((FcExtProgressCallback)myfontconfig_progress_callback);
#endif

	//kprintf("[fontcache thread] done\n");
}

DEFNEW
{
	Object *gauge;

	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_Window_ID, MAKE_ID('W','S','P','L'),
			MUIA_Window_NoMenus, TRUE,
		    MUIA_Window_Title,       NULL,
		    MUIA_Window_DepthGadget, FALSE,
		    MUIA_Window_DragBar,     FALSE,
		    MUIA_Window_CloseGadget, FALSE,
		    MUIA_Window_SizeGadget,  FALSE,
		    MUIA_Window_ID,          0,

			WindowContents, VGroup,
					Child, HGroup,
						Child, HSpace(50),
						Child, MUI_NewObject(MUIC_Dtpic, MUIA_Dtpic_Name, "PROGDIR:resource/about.png"),
						Child, HSpace(50),
						End,

					Child, MUI_MakeObject(MUIO_BarTitle, GSI(MSG_SPLASHWINDOW_INITIALIZING)),

					Child, gauge = GaugeObject,
									GaugeFrame,
									MUIA_Gauge_Max, 100,
									MUIA_Gauge_Horiz, TRUE,
									MUIA_FixHeightTxt, "/",
									MUIA_Gauge_InfoText, GSI(MSG_SPLASHWINDOW_LOADING),
									End,
					End,
			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		//kprintf("[splashwindow] OM_NEW\n");

		g_splash = obj;
		data->maintask = FindTask(NULL);
		data->gauge = gauge;
		data->proc = MUI_NewObject(MUIC_Process,
					MUIA_Process_SourceClass , cl,
					MUIA_Process_SourceObject, obj,
					MUIA_Process_Name        , "[OWB] FontConfig Cache",
					MUIA_Process_Priority    , 0,
					MUIA_Process_AutoLaunch  , FALSE,
					TAG_DONE);

		//kprintf("[splashwindow] proc = %p\n", data->proc);
	}

	return (IPTR)obj;
}

DEFDISP
{
	GETDATA;

	MUI_DisposeObject(data->proc);

	return DOSUPER;
}

DEFTMETHOD(SplashWindow_Open)
{
	GETDATA;
	ULONG sigs;
	
	data->running = FALSE;

	if(data->proc == NULL)
	{
		return 0;
	}

	//kprintf("[splashwindow] running fontcache thread %p\n", data->proc);

	if((data->StartupBit = AllocSignal(-1)) != -1)
	{
		if(DoMethod(data->proc, MUIM_Process_Launch))
		{
			Wait(1L<<data->StartupBit);

			//kprintf("[splashwindow] splashwindow loop\n");

			for(;data->running;)
			{
				DoMethod(app, MUIM_Application_NewInput, &sigs);

				if(sigs)
				{
					sigs = Wait(sigs|SIGBREAKF_CTRL_C|SIGBREAKF_CTRL_D);
					if (sigs & SIGBREAKF_CTRL_C)
					{
						//DoMethod(data->proc, MUIM_Process_Signal, SIGBREAKF_CTRL_C);
					}
					if (sigs & SIGBREAKF_CTRL_D)
					{
						break;
					}
				}
			}
		}

		FreeSignal(data->StartupBit);
	}

	set(obj, MUIA_Window_Open, FALSE);

	//kprintf("[splashwindow] quitting loop\n");

	return 0;
}

DEFSMETHOD(SplashWindow_Update)
{
	GETDATA;

	stccpy(data->infotext, (char *) FilePart((STRPTR)msg->file), sizeof(data->infotext));

	set(data->gauge, MUIA_Gauge_Max, msg->total);
	set(data->gauge, MUIA_Gauge_Current, msg->current);
	set(data->gauge, MUIA_Gauge_InfoText, data->infotext);

	return 0;
}

DEFMMETHOD(Process_Process)
{
	GETDATA;

	//kprintf("[fontcache thread] running indexation\n");

	struct Process *myproc = (struct Process *)FindTask(NULL);
	APTR oldwindowptr = myproc->pr_WindowPtr;

	myproc->pr_WindowPtr = (APTR)-1;

	data->running = TRUE;
	Signal(data->maintask, 1L<<data->StartupBit);

	/* XXX: Seriously, it sucks, but needed with dynamicache, else thread finishes too early,
	 * and it somehow causes a race in main thread, even though i don't see how.
	 */
	Delay(5);

	fontconfig_testcache();

	//kprintf("[fontcache thread] signaling thread end to main task\n");

	myproc->pr_WindowPtr = oldwindowptr;

	data->running = FALSE;
	Signal(data->maintask, SIGBREAKF_CTRL_D);

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECTMETHOD(SplashWindow_Open)
DECSMETHOD(SplashWindow_Update)
DECMMETHOD(Process_Process)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, splashwindowclass)
