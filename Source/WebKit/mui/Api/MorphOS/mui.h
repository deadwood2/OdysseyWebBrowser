#ifndef __MUI_H__
#define __MUI_H__

/* "private" methods */
#undef MIN
#undef MAX
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#ifndef MUIM_Application_KillPushMethod
#define MUIM_Application_KillPushMethod     0x80429954 /* private */ /* V15 */
struct  MUIP_Application_KillPushMethod     { ULONG MethodID; Object *o; ULONG id; ULONG method; };
#endif

#ifndef MUIF_PUSHMETHOD_SINGLE
#define MUIF_PUSHMETHOD_SINGLE       (1<<28UL)
#endif

#ifndef MUIM_Group_GetPageChild
#define MUIM_Group_GetPageChild             0x80422697 /* private */ /* V20 */
struct  MUIP_Group_GetPageChild             { ULONG MethodID; LONG nr; }; /* private */
#endif

#ifndef MUIM_Group_Insert
#define MUIM_Group_Insert                   0x80424d34 /* private */ /* V20 */
struct  MUIP_Group_Insert                   { ULONG MethodID; Object *obj; Object *pred; }; /* private */
#endif

#ifndef MUIC_Title
#define MUIC_Title "Title.mui"
#endif

#ifndef MUIA_Title_Closable
#define MUIA_Title_Closable                 0x80420402 /* V20 isg BOOL */
#endif

#ifndef MUII_Close
#define MUII_Close             54
#endif

#ifndef MUIM_Title_Close
#define MUIM_Title_Close                    0x8042303a /* private */ /* V20 */
struct MUIP_Title_Close { ULONG MethodID; APTR tito; };
#endif

#ifndef MUIA_Title_Newable
#define MUIA_Title_Newable                  0x80424145 /* V20 isg BOOL              */
//#define MUIA_Title_Newable                  0x80424144 /* V20 isg BOOL              */
#endif

#ifndef MUIA_Title_Sortable
#define MUIA_Title_Sortable                 0x804211f1 /* V20 isg BOOL              */ /* private */
#endif

#ifndef MUIM_Title_CreateNewButton
#define MUIM_Title_CreateNewButton          0x8042ce5c /* private */ /* V20 */
struct  MUIP_Title_CreateNewButton          { ULONG MethodID; }; /* private */
#endif

#ifndef MUIM_Title_New
#define MUIM_Title_New                      0x804247a6 /* private */ /* V20 */
struct  MUIP_Title_New                      { ULONG MethodID; }; /* private */
#endif

#ifndef MUIA_Title_Dragable
#define MUIA_Title_Dragable                 0x80423810 /* V20 isg BOOL              */ /* private */
#endif

#ifndef MUIA_Group_ChildCount
#define MUIA_Group_ChildCount               0x80420322 /* V20 isg LONG              */ /* private */
#endif

#ifndef MUIM_Text
#define MUIM_Text                           0x8042ee70 /* private */ /* V20 */
#define MUIM_TextDim                        0x80422ad7 /* private */ /* V20 */

struct  MUIP_Text                           { ULONG MethodID; LONG left;LONG top;LONG width;LONG height;STRPTR text;LONG len;STRPTR preparse;ULONG flags; }; /* private */
struct  MUIP_TextDim                        { ULONG MethodID; STRPTR text;LONG len;STRPTR preparse;ULONG flags; }; /* private */
#endif

#ifndef MUIA_CustomBackfill
#define MUIA_CustomBackfill 0x80420a63
#endif

#ifndef MUIA_Prop_Delta
#define MUIA_Prop_Delta                     0x8042c5f4 /* V4  is. LONG              */ /* private */
#endif

#if 0
struct MUI_DragImage
{
	struct BitMap *bm;
	WORD width;  /* exact width and height of bitmap */
	WORD height;
	WORD touchx; /* position of pointer click relative to bitmap */
	WORD touchy;
	ULONG flags; /* see flags below, all other flags reserved for future use */
	PLANEPTR mask;
};
#endif

#ifndef MUIM_Backfill
#define MUIM_Backfill 0x80428d73
struct  MUIP_Backfill { ULONG MethodID; LONG left; LONG top; LONG right; LONG bottom; LONG xoffset; LONG yoffset; LONG brightness; };
#endif

#ifndef MUIA_CustomBackfill
#define MUIA_CustomBackfill                 0x80420a63
#endif

#ifndef MUIF_DRAGIMAGE_SOURCEALPHA
#define MUIF_DRAGIMAGE_SOURCEALPHA   (1<<1)
#endif

#ifndef MUIM_CreateDragImage
#define MUIM_CreateDragImage                0x8042eb6f /* Custom Class */ /* V18 */
struct  MUIP_CreateDragImage                { ULONG MethodID; LONG touchx; LONG touchy; ULONG flags; }; /* Custom Class */
#endif

#ifndef MUIM_DeleteDragImage
#define MUIM_DeleteDragImage                0x80423037 /* Custom Class */ /* V18 */
struct  MUIP_DeleteDragImage                { ULONG MethodID; struct MUI_DragImage *di; }; /* Custom Class */
#endif

#ifndef MUIM_FindObject
#define MUIM_FindObject                     0x8042038f /* V13 */
struct  MUIP_FindObject                     { ULONG MethodID; Object *findme; };
#endif

#ifndef MUIA_Dtpic_Alpha
#define MUIA_Dtpic_Alpha                    0x8042b4db /* V20 isg LONG              */ /* private */
#endif

#ifndef MUIA_Dtpic_DarkenSelState
#define MUIA_Dtpic_DarkenSelState           0x80423247 /* V20 i.g BOOL              */
#endif

#ifndef MUIA_Dtpic_Fade
#define MUIA_Dtpic_Fade                     0x80420429 /* V20 isg LONG              */ /* private */
#endif

#ifndef MUIA_Dtpic_LightenOnMouse
#define MUIA_Dtpic_LightenOnMouse           0x8042966a /* V20 i.g BOOL              */
#endif

#ifndef MUIA_Dtpic_MinHeight
#define MUIA_Dtpic_MinHeight                0x80423ecc /* V20 i.g BOOL              */ /* private */
#endif

#ifndef MUIA_Dtpic_MinWidth
#define MUIA_Dtpic_MinWidth                 0x8042c417 /* V20 i.g BOOL              */ /* private */
#endif

#ifndef MUIM_GoActive
#define MUIM_GoActive                       0x8042491a /* private */ /* V8  */
struct  MUIP_GoActive                       { ULONG MethodID; ULONG flags; }; /* private */
#endif

#ifndef MUIM_GoInactive
#define MUIM_GoInactive                     0x80422c0c /* private */ /* V8  */
struct  MUIP_GoInactive                     { ULONG MethodID; ULONG flags; }; /* private */
#endif

#ifndef MUIA_List_TopPixel
#define MUIA_List_TopPixel                  0x80429df3
#endif

#ifndef MUIM_CheckShortHelp 
#define MUIM_CheckShortHelp                 0x80423c79 /* private */ /* V20 */
#endif

/* Handy MUI functions and macros */

#ifndef get
#define get(obj,attr,store) GetAttr(attr,obj,(IPTR *)store)
#endif
#ifndef set
#define set(obj,attr,value) SetAttrs(obj,attr,value,TAG_DONE)
#endif
#ifndef nnset
#define nnset(obj,attr,value) SetAttrs(obj,MUIA_NoNotify,TRUE,attr,value,TAG_DONE)
#endif

#ifndef _parent
#define _parent(_o) ((Object *)getv(_o, MUIA_Parent))
#endif

#ifndef _backspec
#define _backspec(obj)    (muiAreaData(obj)->mad_BackSpec)
#endif

#define _between(_a,_x,_b) ((_x)>=(_a) && (_x)<=(_b))
#define _isinobject(_o,_x,_y) (_between(_left(_o),(_x),_right(_o)) && _between(_top(_o),(_y),_bottom(_o)))
#define _isinwinborder(_x,_y) \
	((_between(0, (_x), _window(obj)->Width) && _between(0, (_y), _window(obj)->BorderTop)) || \
	 (_between(_window(obj)->Width - _window(obj)->BorderRight, (_x), _window(obj)->Width) && _between(0, (_y), _window(obj)->Height)) || \
	 (_between(0, (_x), _window(obj)->Width) && _between(_window(obj)->Height - _window(obj)->BorderBottom, (_y), _window(obj)->Height)) || \
	 (_between(0, (_x), _window(obj)->BorderLeft) && _between(0, (_y), _window(obj)->Height)))

#define INITTAGS (((struct opSet *)msg)->ops_AttrList)

#define FORTAG(_tagp) \
	{ \
		struct TagItem *tag, *_tags = (struct TagItem *)(_tagp); \
		while ((tag = NextTagItem(&_tags))) switch ((int)tag->ti_Tag)
#define NEXTTAG }

#define FORCHILD(_o, _a) \
	{ \
		APTR child, _cstate = (APTR)((struct MinList *)getv(_o, _a))->mlh_Head; \
		while ((child = NextObject(&_cstate)))

#define NEXTCHILD }

#undef KeyCheckMark
#define KeyCheckMark(selected,control)\
	ImageObject,\
		ImageButtonFrame,\
		MUIA_InputMode        , MUIV_InputMode_Toggle,\
		MUIA_Image_Spec       , MUII_CheckMark,\
		MUIA_Image_FreeVert   , TRUE,\
		MUIA_Selected         , selected,\
		MUIA_Background       , MUII_ButtonBack,\
		MUIA_ShowSelState     , FALSE,\
		MUIA_ControlChar      , control,\
		MUIA_CycleChain       , 1,\
		End

#undef KeyButton
#define KeyButton(name,key)\
	TextObject,\
		ButtonFrame,\
		MUIA_Font, MUIV_Font_Button,\
		MUIA_Text_Contents, name,\
		MUIA_Text_PreParse, "\33c",\
		MUIA_Text_HiChar  , key,\
		MUIA_ControlChar  , key,\
		MUIA_InputMode    , MUIV_InputMode_RelVerify,\
		MUIA_Background   , MUII_ButtonBack,\
		MUIA_CycleChain   , 1,\
		End

#define PictureButton(name, imagepath)\
		NewObject(getpicturebuttonclass(), NULL,\
				MA_PictureButton_Name, name,\
				MA_PictureButton_Path, imagepath,\
				MA_PictureButton_UserData, NULL,\
				MUIA_ShortHelp, name,\
				TAG_DONE)

APTR MakeCheck(CONST_STRPTR str, ULONG checked);
APTR MakeRect(void);
APTR MakeButton(CONST_STRPTR msg);
APTR MakeVBar(void);
APTR MakeHBar(void);
APTR MakeLabel(CONST_STRPTR msg);
APTR MakeNewString(CONST_STRPTR str, ULONG maxlen);
APTR MakeString(CONST_STRPTR def, ULONG secret);
APTR MakePrefsString(CONST_STRPTR str, CONST_STRPTR def, ULONG maxlen, ULONG id);
APTR MakePrefsStringWithWeight(CONST_STRPTR str, CONST_STRPTR def, ULONG maxlen, ULONG weight, ULONG id);
APTR MakePrefsCheck(CONST_STRPTR str, ULONG def, ULONG id);
APTR MakeDirString(CONST_STRPTR str, CONST_STRPTR def, ULONG id);
APTR MakeFileString(CONST_STRPTR str, CONST_STRPTR def, ULONG id);
APTR MakeCycle(CONST_STRPTR label, const CONST_STRPTR *entries);
APTR MakePrefsCycle(CONST_STRPTR label, const CONST_STRPTR *entries, ULONG id);
APTR MakePrefsSlider(CONST_STRPTR label, LONG min, LONG max, LONG def, ULONG id);
APTR MakeSlider(CONST_STRPTR label, LONG min, LONG max, LONG def);
APTR MakePrefsPopString(CONST_STRPTR label, CONST_STRPTR def, ULONG id);
APTR MakeCycleLocalized(CONST_STRPTR text, APTR cyclelabels, ULONG first, ULONG last);

#if defined(__MORPHOS__)
ULONG getv(APTR obj, ULONG attr);
#endif
#if defined(__AROS__)
IPTR getv(APTR obj, ULONG attr);
#include <aros-overrides.h>
#endif

#endif
