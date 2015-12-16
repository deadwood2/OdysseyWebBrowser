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

#include <libraries/asl.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "config.h"
#include "gui.h"

APTR MakeCheck(CONST_STRPTR str, ULONG checked)
{
	APTR obj;

	obj = MUI_MakeObject(MUIO_Checkmark, (IPTR) str);

	if (obj)
		SetAttrs(obj,
			MUIA_CycleChain	, TRUE,
			MUIA_Selected	, checked,
			TAG_DONE);

	return (obj);
}

APTR MakeRect(void)
{
	return RectangleObject, End;
}

APTR MakeButton(CONST_STRPTR msg)
{
	APTR obj;

	if ((obj = MUI_MakeObject(MUIO_Button, (IPTR) msg)))
		SetAttrs(obj, MUIA_CycleChain, TRUE, TAG_DONE);

	return obj;
}

APTR MakeVBar(void)
{
	return RectangleObject, MUIA_Rectangle_VBar, TRUE, MUIA_Weight, 0, End;
}

APTR MakeHBar(void)
{
	return RectangleObject, MUIA_Rectangle_HBar, TRUE, MUIA_Weight, 0, End;
}

APTR MakeLabel(CONST_STRPTR msg)
{
	return MUI_MakeObject(MUIO_Label, (IPTR) msg, 0);
}

APTR MakeNewString(CONST_STRPTR str, ULONG maxlen)
{
	APTR	obj;

	if ((obj = MUI_MakeObject(MUIO_String, (IPTR) str, maxlen)))
		SetAttrs(obj, MUIA_CycleChain, 1, MUIA_String_AdvanceOnCR, TRUE, TAG_DONE);

	return obj;
}

APTR MakeString(CONST_STRPTR def, ULONG secret)
{
	return StringObject, StringFrame, MUIA_CycleChain, 1, MUIA_String_AdvanceOnCR, TRUE, MUIA_String_Contents, def, MUIA_String_Secret, secret, MUIA_String_MaxLen, 1024, TAG_DONE);
}

APTR MakePrefsString(CONST_STRPTR str, CONST_STRPTR def, ULONG maxlen, ULONG id)
{
	APTR	obj;

	if ((obj = MUI_MakeObject(MUIO_String, (IPTR)str, maxlen)))
		SetAttrs(obj, MUIA_CycleChain, 1, MUIA_String_AdvanceOnCR, TRUE, MUIA_ObjectID, id, MUIA_String_Contents, def, TAG_DONE);

	return obj;
}

APTR MakePrefsStringWithWeight(CONST_STRPTR str, CONST_STRPTR def, ULONG maxlen, ULONG weight, ULONG id)
{
	APTR	obj;

	if ((obj = MUI_MakeObject(MUIO_String, (IPTR)str, maxlen)))
		SetAttrs(obj, MUIA_CycleChain, 1, MUIA_String_AdvanceOnCR, TRUE, MUIA_ObjectID, id, MUIA_String_Contents, def, MUIA_Weight, weight, TAG_DONE);

	return obj;
}

APTR MakeDirString(CONST_STRPTR str, CONST_STRPTR def, ULONG id)
{
	APTR obj, pop;

	pop = PopButton(MUII_PopDrawer);

	obj = PopaslObject,
		ASLFR_DrawersOnly, TRUE,
		ASLFR_InitialShowVolumes, TRUE,
		MUIA_Popstring_Button, (ULONG)pop,
		MUIA_Popstring_String, (ULONG)MakePrefsString(str, def, 1024, id),
		MUIA_Popasl_Type, ASL_FileRequest,
		TAG_DONE);

	if (obj)
		SetAttrs(pop, MUIA_CycleChain, 1, TAG_DONE);

	return obj;
}

APTR MakeFileString(CONST_STRPTR str, CONST_STRPTR def, ULONG id)
{
	APTR obj, pop;

	pop = PopButton(MUII_PopFile);

	obj = PopaslObject,
		ASLFR_InitialFile, def,
		MUIA_Popstring_Button, (ULONG)pop,
		MUIA_Popstring_String, (ULONG)MakePrefsString(str, def, 1024, id),
		MUIA_Popasl_Type, ASL_FileRequest,
		TAG_DONE);

	if (obj)
		SetAttrs(pop, MUIA_CycleChain, 1, TAG_DONE);

	return obj;
}

APTR MakeCycle(CONST_STRPTR label, const CONST_STRPTR *entries)
{
    APTR obj = MUI_MakeObject(MUIO_Cycle, (IPTR)label, (IPTR)entries);

	if(obj)
		SetAttrs(obj, MUIA_CycleChain, 1, MUIA_Weight, 0, TAG_DONE);

    return obj;
}

APTR MakePrefsCycle(CONST_STRPTR label, const CONST_STRPTR *entries, ULONG id)
{
    APTR obj = MUI_MakeObject(MUIO_Cycle, (IPTR)label, (IPTR)entries);

    if (obj)
        SetAttrs(obj, MUIA_CycleChain, 1, MUIA_ObjectID, id, TAG_DONE);

    return obj;
}

APTR MakeSlider(CONST_STRPTR label, LONG min, LONG max, LONG def)
{
	APTR obj = MUI_MakeObject(MUIO_Slider, label, min, max);

    if (obj)
		SetAttrs(obj, MUIA_CycleChain, 1, MUIA_Slider_Level, def, TAG_DONE);

    return obj;
}

APTR MakePrefsSlider(CONST_STRPTR label, LONG min, LONG max, LONG def, ULONG id)
{
	APTR obj = MUI_MakeObject(MUIO_Slider, label, min, max);

    if (obj)
		SetAttrs(obj, MUIA_CycleChain, 1, MUIA_ObjectID, id, MUIA_Slider_Level, def, TAG_DONE);

    return obj;
}

APTR MakePrefsCheck(CONST_STRPTR str, ULONG def, ULONG id)
{
	APTR obj;

	obj = MUI_MakeObject(MUIO_Checkmark, (IPTR) str);

	if (obj)
		SetAttrs(obj,
			MUIA_CycleChain, TRUE,
			MUIA_Selected, def,
			MUIA_ObjectID, id,
			TAG_DONE);

	return (obj);

}

APTR MakePrefsPopString(CONST_STRPTR label, CONST_STRPTR def, ULONG id)
{
	APTR obj = NULL;
	/*
	obj = NewObject(getfontfamilypopstringclass(), NULL, TAG_DONE);

	if(obj)
		SetAttrs(obj,
			MUIA_CycleChain, TRUE,
			MUIA_String_Contents, def,
			MUIA_Text_Contents, def,
			MUIA_ObjectID, id,
			TAG_DONE);
	*/
	return (obj);
}

#if OS(MORPHOS)
ULONG getv(APTR obj, ULONG attr)
{
	ULONG val;
	GetAttr(attr, obj, &val);
	return val;
}
#endif
#if OS(AROS)
IPTR getv(APTR obj, ULONG attr)
{
    IPTR val;
    GetAttr(attr, (Object *)obj, &val);
    return val;
}
#endif
