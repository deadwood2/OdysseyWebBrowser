#ifndef AROS_OVERRIDES_H
#define AROS_OVERRIDES_H

#ifdef __cplusplus
extern "C" {
#endif
IPTR DoSuperNew(struct IClass *cl, Object *obj, ULONG tag1, ...);
APTR AllocVecTaskPooled(ULONG byteSize);
VOID FreeVecTaskPooled(APTR memory);
APTR ARGB2BGRA(APTR src, ULONG stride, ULONG height);
VOID ARGB2BGRAFREE(APTR dst);
#ifdef __cplusplus
}
#endif

#undef DoMethod // these inlines somehow conflict with compilation, something abour variadic?
#undef SetAttrs
#undef CoerceMethod

#undef NewObject
#include <proto/intuition.h>

typedef struct _struct_Msg _Msg_;

/* Overrides for struct Node to struct ::Node due to OWB also having such type */
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

#undef NEWLIST
#define NEWLIST(_l)                                     \
do                                                      \
{                                                       \
    struct List *__aros_list_tmp = (struct List *)(_l), \
                *l = __aros_list_tmp;                   \
                                                        \
    l->lh_TailPred = (struct ::Node *)l;                \
    l->lh_Tail     = 0;                                 \
    l->lh_Head     = (struct ::Node *)&l->lh_Tail;      \
} while (0)

#undef REMOVE
#define REMOVE(n) Remove((struct ::Node*)n)

#undef REMHEAD
#define REMHEAD(l) ((APTR)RemHead((struct List*)l))

#undef ADDTAIL
#define ADDTAIL(l,n) AddTail((struct List*)l,(struct ::Node*)n)

#undef ADDHEAD
#define ADDHEAD(l,n) AddHead((struct List*)l,(struct ::Node*)n)

#undef REMTAIL
#define REMTAIL(l) ((APTR)RemTail((struct List*)l))

#define WA_PointerType                              (WA_Dummy + 164)
#define POINTERTYPE_NORMAL                          0
#define POINTERTYPE_MOVE                            14
#define SA_StopBlanker                              (SA_Dummy + 121)


#define MUIM_Menustrip_Popup                        (MUIB_MUI|0x00420e76) /* MUI: V20 */
#define MUIM_Menustrip_ExitChange                   (MUIB_MUI|0x0042ce4d) /* MUI: V20 */
#define MUIM_Menustrip_InitChange                   (MUIB_MUI|0x0042dcd9) /* MUI: V20 */
#define MUIA_List_DoubleClick                       (MUIB_MUI|0x00424635) /* MUI: V4  i.g BOOL */
#define MUIA_List_DragType                          (MUIB_MUI|0x00425cd3) /* MUI: V11 isg LONG */
#define MUIA_Text_Shorten                           (MUIB_MUI|0x00428bbd) /* MUI: V20 isg LONG */
#define MUIA_FrameDynamic                           (MUIB_MUI|0x004223c9) /* MUI: V20 isg BOOL */
#define MUIA_FrameVisible                           (MUIB_MUI|0x00426498) /* MUI: V20 isg BOOL */

#define MUIV_Text_Shorten_Cutoff                    1
#define MUIV_PushMethod_Delay(millis)               MIN(0x0ffffff0, (((ULONG)millis) << 8))

#define MCC_TI_TAGBASE                              ((TAG_USER)|((1307<<16)+0x712))
#define MCC_TI_ID(x)                                (MCC_TI_TAGBASE+(x))
#define MUIA_Textinput_CursorPos                    MCC_TI_ID(127)        /* V1  .sg ULONG */
#define MUIA_Textinput_MarkStart                    MCC_TI_ID(133)        /* V13 isg ULONG */
#define MUIA_Textinput_MarkEnd                      MCC_TI_ID(134)        /* V13 isg ULONG */
#define MUIA_Textinput_ResetMarkOnCursor            MCC_TI_ID(157)        /* V29 isg BOOL */

#define MUIM_Textinput_DoMarkAll                    MCC_TI_ID(11)         /* V1 */

#define NM_WHEEL_UP                                 0x7a
#define NM_WHEEL_DOWN                               0x7b
#undef XMLCALL
#define XMLCALL

typedef IPTR * ULONGPTR;

#if (AROS_BIG_ENDIAN == 1)
#define NATIVE_ARGB RECTFMT_ARGB32   /* Big Endian Archs. */
#else
#define NATIVE_ARGB RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

// redefines (to work around missing functionality, commented is actual value )

//#define MUIV_Frame_Page 20
#define MUIV_Frame_Page MUIV_Frame_Group

#endif


/*
MUIM_Application_KillPushMethod         NOT USED
MUIM_Group_GetPageChild                 NOT USED
MUIA_Title_Closable                     NOT USED
MUIA_Title_Dragable                     NOT USED
MUIM_Text                               NOT USED
MUIM_TextDim                            NOT USED
MUIA_Prop_Delta                         NOT USED
MUIA_Dtpic_Fade                         NOT USED
MUIA_Dtpic_MinHeight                    NOT USED
MUIA_Dtpic_MinWidth                     NOT USED

MUIF_PUSHMETHOD_SINGLE                  NOT IMPLEMENTED
MUII_Close                              NOT IMPLEMENTED
MUIM_Title_Close                        NOT IMPLEMENTED
MUIA_Title_Newable                      NOT IMPLEMENTED
MUIA_Title_Sortable                     NOT IMPLEMENTED
MUIM_Title_CreateNewButton              NOT IMPLEMENTED
MUIM_Title_New                          NOT IMPLEMENTED
MUIA_CustomBackfill                     NOT IMPLEMENTED
MUIM_FindObject                         NOT IMPLEMENTED
MUIA_Dtpic_Alpha                        NOT IMPLEMENTED
MUIA_Dtpic_DarkenSelState               NOT IMPLEMENTED
MUIA_Dtpic_LightenOnMouse               NOT IMPLEMENTED
MUIM_GoActive                           NOT IMPLEMENTED
MUIM_GoInactive                         NOT IMPLEMENTED
MUIA_List_TopPixel                      NOT IMPLEMENTED


MUIA_Group_ChildCount                   IMPLEMENTED
MUIF_DRAGIMAGE_SOURCEALPHA              IMPLEMENTED
MUIM_CreateDragImage                    IMPLEMENTED
MUIM_DeleteDragImage                    IMPLEMENTED
MUIM_Family_GetChild                    IMPLEMENTED
MUIM_Group_Insert                       IMPLEMENTED
MUIC_Title                              IMPLEMENTED (+/-)

*/
