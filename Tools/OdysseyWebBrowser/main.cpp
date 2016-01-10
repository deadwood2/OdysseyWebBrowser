/*
 * Copyright (C) 2009 Fabien Coeurjoly
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


#include <cairo.h>

#include <proto/timer.h>
#include <dos/dos.h>
#include <dos/notify.h>
#include <proto/dos.h>

#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/icon.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if !defined(__AROS__)
#include <sys/signal.h>
#endif
#include <locale.h>
#include <setjmp.h>

#include <clib/debug_protos.h>

#include "gui.h"
#include "alocale.h"

#undef accept

#define D(x)

void close_dictionary();
void destroy_application(void);
void close_libs(void);

extern "C"
{
	int raise(int sig)
	{
		kprintf("raise(%d) was called, dumping stackframe...\n", sig);

#if !defined(__AROS__)
		DumpTaskState(FindTask(NULL));
#endif

		return(0);
	}
	/*
	void abort(void)
	{
		kprintf("abort was called, dumping stackframe...\n");

        DumpTaskState(FindTask(NULL));

		Wait(0);
	} */
}

/* Globals */
unsigned long __stack = 2*1024*1024;
jmp_buf bailout_env;

struct Library * IconBase = NULL;
struct Library * MUIMasterBase = NULL;
struct Library * KeymapBase = NULL;
struct Library * CharsetsBase = NULL;
struct Library * SpellCheckerBase = NULL;
struct Library * ExpatBase = NULL;

static ULONG __morphos2 = FALSE;

/* Dos notify stuff */

ULONG dosnotifysig=0;
LONG dosnotifybit=-1;
struct NotifyRequest nr;

LONG dosnotify_init(void)
{
	dosnotifybit = AllocSignal(-1);
	if(dosnotifybit!=-1)
	{
		dosnotifysig = 1L << dosnotifybit;
		return (TRUE);
	}
	return (FALSE);
}

void dosnotify_cleanup(void)
{
	if(dosnotifybit)
	{
		FreeSignal(dosnotifybit);
	}
}

struct NotifyRequest *dosnotify_start(CONST_STRPTR name)
{
	struct NotifyRequest *nr;
	ULONG namelen;

	namelen = strlen(name);
	if((namelen) && (dosnotifysig))
	{
		if( (nr = (struct NotifyRequest *)malloc(sizeof(*nr) + namelen + 1)) )
		{
			nr->nr_Name= (char *) (nr + 1);
			memcpy(nr->nr_Name, name, namelen + 1);
			nr->nr_stuff.nr_Signal.nr_Task   = FindTask(NULL);
			nr->nr_stuff.nr_Signal.nr_SignalNum = dosnotifysig;
			nr->nr_Flags           = NRF_SEND_SIGNAL;
			if(StartNotify(nr))
			{
				return (nr);
			}
			free(nr);
		}
	}
	return (NULL);
}


void dosnotify_stop(struct NotifyRequest *nr)
{
	if(nr)
	{
		EndNotify(nr);
		free(nr);
	}
}

ULONG is_morphos2(void)
{
	return __morphos2;
}

ULONG open_libs(void)
{
	if(!(KeymapBase = OpenLibrary("keymap.library", 39)))
	{
		fprintf(stderr, "Failed to open keymap.library.\n");
		return FALSE;
	}

	if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
	{
		fprintf(stderr, "Failed to open " MUIMASTER_NAME ".\n");
		return FALSE;
	}

	if(!(IconBase = OpenLibrary("icon.library", 39)))
	{
		fprintf(stderr, "Failed to open icon.library.\n");
		return FALSE;
	}

	if(!(ExpatBase = OpenLibrary("expat.library", 0)))
	{
		fprintf(stderr, "Failed to open expat.library.\n");
		return FALSE;
	}

	struct Library * ibase = (struct Library *) IntuitionBase;
	if(ibase)
	{
		if(ibase->lib_Version > 50 || (ibase->lib_Version == 50 && ibase->lib_Revision >= 61))
		{
			if(KeymapBase->lib_Version > 50) // Henes 1.5 check
			{
				__morphos2 = TRUE;
			}
		}
	}

	extern void init_useragent();
	init_useragent();

	// Use charsets.library if available
	CharsetsBase = OpenLibrary("charsets.library", 51);

	// Use spellchecker if available
	SpellCheckerBase = OpenLibrary("spellchecker.library", 0);

	// BsdSocket check. Making it later is too risky.
	struct Library *mainSocketBase = OpenLibrary("bsdsocket.library", 4);

	if(!mainSocketBase)
	{
		fprintf(stderr, "Failed to open bsdsocket.library. OWB requires a running TCP/IP stack.\n");
		return FALSE;
	}
	else
	{
		CloseLibrary(mainSocketBase);
		mainSocketBase = NULL;
	}

	// Hack to make sure cairo mutex are initialized
	{
		cairo_surface_t *dummysurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 4, 4);
		cairo_surface_destroy(dummysurface);
	}

	return TRUE;
}

void close_libs(void)
{
	if(MUIMasterBase)
	{
		CloseLibrary(MUIMasterBase);
		MUIMasterBase = NULL;
	}

	if(ExpatBase)
	{
		CloseLibrary(ExpatBase);
		ExpatBase = NULL;
	}

	if(IconBase)
	{
		CloseLibrary(IconBase);
		IconBase = NULL;
	}

	if(KeymapBase)
	{
		CloseLibrary(KeymapBase);
		KeymapBase = NULL;
	}

	if(CharsetsBase)
	{
		CloseLibrary(CharsetsBase);
		CharsetsBase = NULL;
	}

	if(SpellCheckerBase)
	{
		close_dictionary();
		CloseLibrary(SpellCheckerBase);
		SpellCheckerBase = NULL;
	}
}

void destroy_application(void)
{
	if(app)
	{
		MUI_DisposeObject(app);
		methodstack_cleanup();
		classes_cleanup();
		app = NULL;
	}

	locale_cleanup();
}

// Safety checks
extern ULONG icu_check(void);

ULONG safety_check(void)
{
	ULONG passed = TRUE;

	passed &= icu_check();

	return passed;
}

Object *create_application(char *url)
{
	Object *obj = NULL;

	locale_init();

	if(classes_init())
	{
	    methodstack_init();
		obj = (Object *) NewObject(getowbappclass(), NULL, MA_OWBBrowser_URL, (ULONG) url, TAG_DONE);
	}
	else
	{
		fprintf(stderr, "Failed to create internal classes.\n");
	}

	return obj;
}

#if defined(__AROS__)
extern "C" {
void aros_register_trap_handler();
int aros_timer_open();
void aros_timer_close();
}
#endif

void main_loop(void)
{
	ULONG running = TRUE;
	ULONG signals;

	setIsSafeToQuit(TRUE);
	
	if(setjmp(bailout_env) == -1)
	{
	    running = FALSE;
	}

#if defined(__AROS__)
    if (running)
        aros_register_trap_handler();
#endif

	while (running)
	{
		ULONG ret = DoMethod(app,MUIM_Application_NewInput,&signals);

		switch(ret)
		{
			case MUIV_Application_ReturnID_Quit:
			{
				setIsQuitting(TRUE);
				if(isSafeToQuit())
					running = FALSE;
				break;
			}
		}

		methodstack_check();

		/* Refresh each active browser if needed */
		DoMethod(app, MM_OWBApp_Expose);

		if(running && signals) signals = Wait(signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F/* | dosnotifysig */);

		if((signals & SIGBREAKF_CTRL_C) || isQuitting())
		{
			setIsQuitting(TRUE);
			if(isSafeToQuit())
				running = FALSE;
		}
		
		if(signals & SIGBREAKF_CTRL_E)
		{
			/* Run webkit events for each active browser */
			DoMethod(app, MM_OWBApp_WebKitEvents);
		}
		
		/*
		if(signals & dosnotifysig )
		{
			//kprintf("DosNotify Trig\n");
			//DoMethod(getv(app, MA_OWBApp_BookmarkGroup), MM_Bookmarkgroup_DosNotify);
		}
		*/

		methodstack_check();
	}
}

int main (int argc, char* argv[])
{
#if !defined(__AROS__)
	signal(SIGINT, SIG_IGN);
#endif
	atexit(close_libs);
	atexit(destroy_application);
#if defined(__AROS__)
	atexit(aros_timer_close);
#endif

	setlocale(LC_TIME, "C");
	setlocale(LC_NUMERIC, "C");

#if defined(__AROS__)
   SetVar("FONTCONFIG_PATH", "PROGDIR:Conf/font", -1, LV_VAR | GVF_LOCAL_ONLY);
   aros_timer_open();
#endif

	if (!argc)
	{
		freopen("NIL:", "w", stderr);
		freopen("NIL:", "w", stdout);
	}

	//dosnotify_init();

	if(open_libs())
	{
		if(safety_check())
		{
			app = create_application(argc > 1 ? (char*)argv[1] : (char *)"");

			if(app)
			{
				main_loop();
			}

			destroy_application();
		}
	}
	close_libs();

	//dosnotify_cleanup();

    return 0;
}
