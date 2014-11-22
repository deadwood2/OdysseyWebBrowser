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

#include <string.h>
#include <stdio.h>
#include <devices/rawkeycodes.h>
#include <clib/debug_protos.h>

#include "gui.h"

Object * create_toolbutton(CONST_STRPTR text, CONST_STRPTR imagepath, ULONG type)
{
	return (Object *) NewObject(gettoolbuttonclass(), NULL,
								MA_ToolButton_Text,  text,
								MA_ToolButton_Image, imagepath,
								MA_ToolButton_Type,  type,
								MA_ToolButton_Frame, getv(app, MA_OWBApp_ShowButtonFrame) ? MV_ToolButton_Frame_Button : MV_ToolButton_Frame_None,
								MA_ToolButton_Background, getv(app, MA_OWBApp_ShowButtonFrame) ? MV_ToolButton_Background_Button :MV_ToolButton_Background_Parent,
								MUIA_CycleChain, 1, TAG_DONE);
}

struct Data
{
	ULONG type;
	Object *group;
	Object *txtobj;
	Object *imageobj;
	char buffer[256];
	char text[256];
	char path[256];
	ULONG disabled;
	ULONG pressed;
	ULONG frame;
	ULONG background;
};

static void doset(Object * obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_ToolButton_Text:
		{
			if(tag->ti_Data)
			{
				stccpy(data->text, (char *) tag->ti_Data, sizeof(data->text));
				set(obj, MUIA_ShortHelp, data->text);

				if(data->txtobj)
				{
					if(data->type == MV_ToolButton_Type_IconAndText)
					{
						snprintf(data->buffer, sizeof(data->buffer), "%s ", data->text);
					}
					else
					{
						snprintf(data->buffer, sizeof(data->buffer), "%s", data->text);
					}
					set(data->txtobj, MUIA_Text_Contents, data->buffer);
				}
			}
		}
		break;

		case MA_ToolButton_Image:
		{
			if(tag->ti_Data)
			{
				stccpy(data->path, (char *) tag->ti_Data, sizeof(data->path));

				if(data->imageobj)
				{
	                DoMethod(obj, MUIM_Group_InitChange);

					DoMethod(obj, MUIM_Group_Remove, data->imageobj);
					MUI_DisposeObject(data->imageobj);
					data->imageobj = (Object *) MUI_NewObject(MUIC_Dtpic,
									               MUIA_Dtpic_Name, data->path,
												   MUIA_Dtpic_Alpha, data->disabled ? 0x60 : 0xFF,
											 	   MUIA_Dtpic_DarkenSelState, TRUE,
												   //MUIA_Dtpic_LightenOnMouse, !data->disabled,
									          	   TAG_DONE);
					if(data->imageobj)
					{
						DoMethod(obj, MUIM_Group_AddHead, data->imageobj);
					}

					DoMethod(obj, MUIM_Group_ExitChange);
				}

			}
		}
		break;

		case MA_ToolButton_Type:
		{
			data->type = tag->ti_Data;

			switch(data->type)
			{
				default:
				case MV_ToolButton_Type_Icon:
				{
					DoMethod(obj, MUIM_Group_InitChange);

					if(data->txtobj)
					{
						DoMethod(obj, MUIM_Group_Remove, data->txtobj);
						MUI_DisposeObject(data->txtobj);
						data->txtobj = NULL;
					}

					if(!data->imageobj)
					{
						data->imageobj = MUI_NewObject(MUIC_Dtpic,
										 MUIA_Dtpic_Name, data->path,
										 MUIA_Dtpic_Alpha, 0xFF,
										 MUIA_Dtpic_DarkenSelState, TRUE,
										 //MUIA_Dtpic_LightenOnMouse, TRUE,
										 TAG_DONE);
						DoMethod(obj, MUIM_Group_AddTail, data->imageobj);
					}
					else
					{
						set(data->imageobj, MUIA_Dtpic_Name, data->path);
					}

					DoMethod(obj, MUIM_Group_ExitChange);

					break;
				}

				case MV_ToolButton_Type_Text:
				{
					DoMethod(obj, MUIM_Group_InitChange);

					if(data->imageobj)
					{
						DoMethod(obj, MUIM_Group_Remove, data->imageobj);
						MUI_DisposeObject(data->imageobj);
						data->imageobj = NULL;
					}

					if(!data->txtobj)
					{
						data->txtobj = MUI_NewObject(MUIC_Text,
											   MUIA_Font, MUIV_Font_Button,
											   MUIA_Text_Contents, data->text,
											   MUIA_Text_PreParse, "\33c",
											   TAG_DONE);
						DoMethod(obj, MUIM_Group_AddTail, data->txtobj);
					}
					else
					{
						set(data->txtobj, MUIA_Text_Contents, data->text);
					}

					DoMethod(obj, MUIM_Group_ExitChange);

					break;
				}

				case MV_ToolButton_Type_IconAndText:
				{
					DoMethod(obj, MUIM_Group_InitChange);

					if(data->txtobj)
					{
						DoMethod(obj, MUIM_Group_Remove, data->txtobj);
						MUI_DisposeObject(data->txtobj);
						data->txtobj = NULL;
					}

					if(data->imageobj)
					{
						DoMethod(obj, MUIM_Group_Remove, data->imageobj);
						MUI_DisposeObject(data->imageobj);
						data->imageobj = NULL;
					}

					data->imageobj = MUI_NewObject(MUIC_Dtpic,
									 MUIA_Dtpic_Name, data->path,
                                     MUIA_Dtpic_Alpha, 0xFF,
									 MUIA_Dtpic_DarkenSelState, TRUE,
									 //MUIA_Dtpic_LightenOnMouse, TRUE,
									 TAG_DONE);

					if(data->imageobj)
					{
						DoMethod(obj, MUIM_Group_AddTail, data->imageobj);
					}

					snprintf(data->buffer, sizeof(data->buffer), "%s ", data->text);

					data->txtobj = MUI_NewObject(MUIC_Text,
										   MUIA_Font, MUIV_Font_Button,
										   MUIA_Text_Contents, data->buffer,
										   MUIA_Text_PreParse, "\33c",
										   TAG_DONE);

					if(data->txtobj)
					{
						DoMethod(obj, MUIM_Group_AddTail, data->txtobj);
					}

					DoMethod(obj, MUIM_Group_ExitChange);

					break;
				}
			}
		}
		break;

		case MA_ToolButton_Pressed:
		{
			data->pressed = (ULONG) tag->ti_Data;
		}
		break;

		case MA_ToolButton_Disabled:
		{
			ULONG oldState = data->disabled;
			data->disabled = tag->ti_Data;

			if(data->disabled != oldState)
			{
				data->pressed = FALSE;
				set(obj, MUIA_Selected, FALSE);

				if(data->frame == MV_ToolButton_Frame_None)
				{
					set(obj, MUIA_FrameVisible, FALSE);
					set(obj, MUIA_FrameDynamic, !data->disabled);
				}
				else
				{
					set(obj, MUIA_FrameVisible, TRUE);
					set(obj, MUIA_FrameDynamic, FALSE);
				}

				if(data->txtobj)
				{
					set(data->txtobj, MUIA_Text_PreParse, data->disabled ? /*"\33c\33P[80------]"*/ "\33c\33g" : "\33c");
				}

				if(data->imageobj)
				{
	                DoMethod(obj, MUIM_Group_InitChange);

					DoMethod(obj, MUIM_Group_Remove, data->imageobj);
					MUI_DisposeObject(data->imageobj);
					data->imageobj = (Object *) MUI_NewObject(MUIC_Dtpic,
									               MUIA_Dtpic_Name, data->path,
												   MUIA_Dtpic_Alpha, data->disabled ? 0x60 : 0xFF,
											 	   MUIA_Dtpic_DarkenSelState, TRUE,
												   //MUIA_Dtpic_LightenOnMouse, !data->disabled,
									          	   TAG_DONE);
					if(data->imageobj)
					{
						DoMethod(obj, MUIM_Group_AddHead, data->imageobj);
					}

					DoMethod(obj, MUIM_Group_ExitChange);
				}

				set(obj, MUIA_ShowSelState, !data->disabled);
			}
		}
		break;
	}
	NEXTTAG
}

DEFNEW
{
	ULONG frame      = GetTagData(MA_ToolButton_Frame, MV_ToolButton_Frame_Button, msg->ops_AttrList);
	ULONG background = GetTagData(MA_ToolButton_Background, MV_ToolButton_Background_Button, msg->ops_AttrList);
	ULONG muibackground;

	switch(background)
	{
		case MV_ToolButton_Background_Parent:
			muibackground = (ULONG) "";
			break;

		default:
		case MV_ToolButton_Background_Button:
			muibackground = MUII_ButtonBack;
			break;
	}

	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_Group_Horiz, TRUE,
			MUIA_HorizWeight, 0,
			MUIA_FrameDynamic, frame == MV_ToolButton_Frame_Button ? FALSE : TRUE,
			MUIA_FrameVisible, frame == MV_ToolButton_Frame_Button ? TRUE : FALSE,
			MUIA_Frame, MUIV_Frame_Button,
			MUIA_Background, muibackground,
			MUIA_ShowSelState, TRUE,
			//MUIA_InputMode, MUIV_InputMode_RelVerify,
		    TAG_MORE, INITTAGS);

	if (obj)
	{
		GETDATA;

		data->frame      = frame;
		data->background = background;

		doset(obj, data, INITTAGS);

		return (ULONG)obj;
	}

	return(0);
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_ToolButton_Disabled:
		{
			*msg->opg_Storage = data->disabled;
		}
		return TRUE;

		case MA_ToolButton_Pressed:
		{
			*msg->opg_Storage = data->pressed;
		}
		return TRUE;
	}

	return DOSUPER;
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);
	return DOSUPER;
}

DEFMMETHOD(Setup)
{
	if (!DOSUPER)
	{
		return (0);
	}

	MUI_RequestIDCMP(obj, IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE|IDCMP_RAWKEY);

	return TRUE;
}

DEFMMETHOD(Cleanup)
{
	MUI_RejectIDCMP(obj, IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE|IDCMP_RAWKEY);

	return DOSUPER;
}

DEFMMETHOD(HandleInput)
{
	GETDATA;

	if(data->disabled)
		return 0;

	if (msg->imsg)
	{
	    switch (msg->imsg->Class)
	    {
			case IDCMP_MOUSEMOVE:
			{
				if(data->pressed)
				{
					ULONG selected = getv(obj, MUIA_Selected);
					ULONG inobject = _isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY);

					if(selected != inobject)
					{
						set(obj, MUIA_Selected, inobject);
					}
				}
				break;
			}
			case IDCMP_MOUSEBUTTONS:
			{
				if (msg->imsg->Code == SELECTDOWN)
			    {
					if (_isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY))
					{
						set(obj, MUIA_Selected, TRUE);
						set(obj, MA_ToolButton_Pressed, TRUE);
			        }
				}
				else if(msg->imsg->Code == SELECTUP)
			    {
					if(data->pressed)
					{
						if(_isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY))
						{
							set(obj, MA_ToolButton_Pressed, FALSE);
						}
						else
						{
							nnset(obj, MA_ToolButton_Pressed, FALSE);
						}
						set(obj, MUIA_Selected, FALSE);
					}
			    }
			    break;
			}

			case IDCMP_RAWKEY:
			{
				if((msg->imsg->Code & 0x7F) == RAWKEY_RETURN)
				{
					if((Object *) getv(_win(obj), MUIA_Window_ActiveObject) == obj)
					{
						if(msg->imsg->Code & IECODE_UP_PREFIX)
						{
							if(data->frame == MV_ToolButton_Frame_None)
							{
								set(obj, MUIA_FrameVisible, FALSE);
							}
							set(obj, MA_ToolButton_Pressed, FALSE);
							set(obj, MUIA_Selected, FALSE);
						}
						else
						{
							if(data->frame == MV_ToolButton_Frame_None)
							{
								set(obj, MUIA_FrameVisible, TRUE);
							}
							set(obj, MA_ToolButton_Pressed, TRUE);
							set(obj, MUIA_Selected, TRUE);
						}
					}
				}
			    break;
			}

	    }
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECSET
DECGET
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleInput)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, toolbuttonclass)
