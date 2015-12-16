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
#if ENABLE(VIDEO)
#include "HTMLMediaElement.h"
#endif
#include "MediaPlayer.h"


#include <proto/intuition.h>
#include <proto/utility.h>

#include "gui.h"

#if ENABLE(VIDEO)
using namespace WebCore;
#endif

struct Data
{
	Object *bt_playpause;
	Object *sl_seek;
	Object *bt_fullscreen;
	Object *bt_mute;
	Object *sl_volume;

	Object *browser;

	ULONG update;
	ULONG added;
	struct MUI_InputHandlerNode ihnode;

	float duration;
	float currenttime;
	float volume;
	bool muted;
	bool paused;
};

static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		ULONG tdata = tag->ti_Data;

		switch (tag->ti_Tag)
		{
			case MA_MediaControlsGroup_Update:
			{
				data->update = tdata;
				break;
			}

			case MA_MediaControlsGroup_Browser:
			{
				data->browser = (Object *) tdata;
				break;
			}
		}
	}
}

DEFNEW
{
	Object *bt_playpause, *sl_seek, *bt_mute, *bt_fullscreen, *sl_volume;

	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_Group_Horiz, TRUE,
			InnerSpacing(2, 2),

			Child, bt_playpause = (Object *) NewObject(gettoolbuttonclass(), NULL,
								MA_ToolButton_Text,  "",
								MA_ToolButton_Image, "PROGDIR:resource/MediaThemeFullPage/Play.png",
								MA_ToolButton_Type,  MV_ToolButton_Type_Icon,
								MA_ToolButton_Frame, MV_ToolButton_Frame_None,
								MA_ToolButton_Background, MV_ToolButton_Background_Parent,
								MUIA_CycleChain, 1, TAG_DONE),

			Child, sl_seek  = (Object *) NewObject(getseeksliderclass(), NULL, TAG_DONE),

			Child, bt_fullscreen = (Object *) NewObject(gettoolbuttonclass(), NULL,
								MA_ToolButton_Text,  "",
								MA_ToolButton_Image, "PROGDIR:resource/MediaThemeFullPage/FullScreen.png",
								MA_ToolButton_Type,  MV_ToolButton_Type_Icon,
								MA_ToolButton_Frame, MV_ToolButton_Frame_None,
								MA_ToolButton_Background, MV_ToolButton_Background_Parent,
								MUIA_CycleChain, 1, TAG_DONE),

			Child, bt_mute = (Object *) NewObject(gettoolbuttonclass(), NULL,
								MA_ToolButton_Text,  "",
								MA_ToolButton_Image, "PROGDIR:resource/MediaThemeFullPage/SoundUnmute.png",
								MA_ToolButton_Type,  MV_ToolButton_Type_Icon,
								MA_ToolButton_Frame, MV_ToolButton_Frame_None,
								MA_ToolButton_Background, MV_ToolButton_Background_Parent,
								MUIA_CycleChain, 1, TAG_DONE),

			Child, sl_volume = (Object *) NewObject(getvolumesliderclass(), NULL, TAG_DONE),

			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->bt_playpause = bt_playpause;
		data->bt_mute = bt_mute;
		data->bt_fullscreen = bt_fullscreen;
		data->sl_seek = sl_seek;
		data->sl_volume = sl_volume;

		data->update = TRUE;
		data->browser = NULL;

		data->duration = 0;
		data->currenttime = 0;
		data->volume = 1.0f;
		data->muted = false;
		data->paused = true;

		data->ihnode.ihn_Object = obj;
		data->ihnode.ihn_Flags = MUIIHNF_TIMER;
		data->ihnode.ihn_Millis = 500;
		data->ihnode.ihn_Method = MM_MediaControlsGroup_Update;

		DoMethod(data->bt_playpause,  MUIM_Notify, MA_ToolButton_Pressed, FALSE, obj, 1, MM_MediaControlsGroup_PlayPause);
		DoMethod(data->bt_mute,       MUIM_Notify, MA_ToolButton_Pressed, FALSE, obj, 1, MM_MediaControlsGroup_Mute);
		DoMethod(data->bt_fullscreen, MUIM_Notify, MA_ToolButton_Pressed, FALSE, obj, 1, MM_MediaControlsGroup_FullScreen);
		DoMethod(data->sl_seek,   MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MUIM_Set, MA_MediaControlsGroup_Update, TRUE);
		DoMethod(data->sl_seek,   MUIM_Notify, MUIA_Pressed, TRUE, obj, 3, MUIM_Set, MA_MediaControlsGroup_Update, FALSE);
		DoMethod(data->sl_seek,   MUIM_Notify, MUIA_Slider_Level, MUIV_EveryTime, obj, 1, MM_MediaControlsGroup_Seek);
		DoMethod(data->sl_volume,   MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MUIM_Set, MA_MediaControlsGroup_Update, TRUE);
		DoMethod(data->sl_volume,   MUIM_Notify, MUIA_Pressed, TRUE, obj, 3, MUIM_Set, MA_MediaControlsGroup_Update, FALSE);
		DoMethod(data->sl_volume, MUIM_Notify, MUIA_Slider_Level, MUIV_EveryTime, obj, 1, MM_MediaControlsGroup_Volume);
	}

	return (ULONG)obj;
}

DEFSET
{
	GETDATA;
	doset(obj, data, msg->ops_AttrList);
	return DOSUPER;
}

DEFTMETHOD(MediaControlsGroup_PlayPause)
{
	GETDATA;

	if(data->browser)
	{
#if ENABLE(VIDEO)
		HTMLMediaElement *element = (HTMLMediaElement *) getv(data->browser, MA_OWBBrowser_VideoElement);

		if(element)
		{
			if(element->paused())
			{
				element->play();
			}
			else
			{
				element->pause();
			}

			set(data->bt_playpause, MA_ToolButton_Image, element->paused() ?  "PROGDIR:resource/MediaThemeFullPage/Play.png" : "PROGDIR:resource/MediaThemeFullPage/Pause.png");
		}
#endif
	}

	return 0;
}

DEFTMETHOD(MediaControlsGroup_Mute)
{
	GETDATA;

	if(data->browser)
	{
#if ENABLE(VIDEO)
		HTMLMediaElement *element = (HTMLMediaElement *) getv(data->browser, MA_OWBBrowser_VideoElement);

		if(element)
		{
			element->setMuted(element->muted() ? false : true);
			set(data->bt_mute, MA_ToolButton_Image, element->muted() ? "PROGDIR:resource/MediaThemeFullPage/SoundMute.png" : "PROGDIR:resource/MediaThemeFullPage/SoundUnmute.png");
		}
#endif
	}

	return 0;
}

DEFTMETHOD(MediaControlsGroup_FullScreen)
{
	GETDATA;

	if(data->browser)
	{
#if ENABLE(VIDEO)
		HTMLMediaElement *element = (HTMLMediaElement *) getv(data->browser, MA_OWBBrowser_VideoElement);

		if(element)
		{
			element->exitFullscreen();
		}
#endif
	}

	return 0;
}

DEFTMETHOD(MediaControlsGroup_Seek)
{
	GETDATA;

	if(data->browser)
	{
#if ENABLE(VIDEO)
		HTMLMediaElement *element = (HTMLMediaElement *) getv(data->browser, MA_OWBBrowser_VideoElement);

		if(element)
		{
			element->setCurrentTime((float) getv(data->sl_seek, MUIA_Slider_Level));
		}
#endif
	}

	return 0;
}

DEFTMETHOD(MediaControlsGroup_Volume)
{
	GETDATA;

	if(data->browser)
	{
#if ENABLE(VIDEO)
		HTMLMediaElement *element = (HTMLMediaElement *) getv(data->browser, MA_OWBBrowser_VideoElement);

		if(element)
		{
			ExceptionCode e;
			element->setVolume(((float) getv(data->sl_volume, MUIA_Slider_Level)) / 100.f, e);
		}
#endif
	}

	return 0;
}

DEFTMETHOD(MediaControlsGroup_Update)
{
	GETDATA;

	if(data->update)
	{
		if(data->browser)
		{
#if ENABLE(VIDEO)
			HTMLMediaElement *element = (HTMLMediaElement *) getv(data->browser, MA_OWBBrowser_VideoElement);

			if(element)
			{
				float duration = element->duration();
				if(data->duration != duration)
				{
					data->duration = element->duration();
					nnset(data->sl_seek, MUIA_Slider_Max, (LONG) data->duration);
					nnset(data->sl_seek, MUIA_Numeric_Max, (LONG) data->duration);
				}

				float currenttime = element->currentTime();
				if(data->currenttime != currenttime)
				{
					data->currenttime = currenttime;
					nnset(data->sl_seek, MUIA_Slider_Level, (LONG) data->currenttime);
				}

				float volume = element->volume();
				if(data->volume != volume)
				{
					data->volume = volume;
					nnset(data->sl_volume, MUIA_Slider_Level, (LONG) ((float)data->volume*100.f));
				}

				bool paused = element->paused();
				if(data->paused != paused)
				{
					data->paused = paused;
					set(data->bt_playpause, MA_ToolButton_Image, paused ?  "PROGDIR:resource/MediaThemeFullPage/Play.png" : "PROGDIR:resource/MediaThemeFullPage/Pause.png");
				}

				bool muted = element->muted();
				if(data->muted != muted)
				{
					data->muted = muted;
					set(data->bt_mute, MA_ToolButton_Image, muted ? "PROGDIR:resource/MediaThemeFullPage/SoundMute.png" : "PROGDIR:resource/MediaThemeFullPage/SoundUnmute.png");
				}
			}
#endif
		}
	}

	return 0;
}

DEFMMETHOD(Show)
{
	BOOL rc = DOSUPER;

	if (rc)
	{
		GETDATA;

		if (!data->added)
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

	if (data->added)
	{
		DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->ihnode);
		data->added = FALSE;
	}

	return DOSUPER;
}

BEGINMTABLE
DECNEW
DECSET
DECMMETHOD(Hide)
DECMMETHOD(Show)
DECTMETHOD(MediaControlsGroup_PlayPause)
DECTMETHOD(MediaControlsGroup_Mute)
DECTMETHOD(MediaControlsGroup_FullScreen)
DECTMETHOD(MediaControlsGroup_Seek)
DECTMETHOD(MediaControlsGroup_Volume)
DECTMETHOD(MediaControlsGroup_Update)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, mediacontrolsgroupclass)
