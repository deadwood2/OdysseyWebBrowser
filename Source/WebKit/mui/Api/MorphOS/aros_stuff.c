#if defined(__AROS__)

#include <utility/tagitem.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>

#include <aros/debug.h>

IPTR DoSuperNew(struct IClass *cl, Object *obj, ULONG tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = (IPTR)DoSuperMethod(cl, obj, OM_NEW, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
}

#include <zlib.h>
/* This is a workaround for a weird linking problem where uncompress from zlib cannot be found */
int __uncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
    return uncompress(dest, destLen, source, sourceLen);
}

#include <stdlib.h>
APTR AllocVecTaskPooled(ULONG byteSize)
{
    return calloc(1, byteSize);
}

VOID FreeVecTaskPooled(APTR memory)
{
    free(memory);
}

APTR ARGB2BGRA(APTR src, ULONG stride, ULONG height)
{
    APTR _return = AllocVec(stride * height, MEMF_ANY);
    ULONG * dstptr = (ULONG *)_return;
    ULONG * srcptr = (ULONG *)src;
    ULONG x, y, pixelsperline = stride / 4, srcval, dstval;

    for (y = 0; y < height; y++)
        for (x = 0; x < pixelsperline; x++)
        {
            srcval = (*srcptr);
            dstval = 0;
            dstval |= ((srcval & 0x000000FF) >> 0)  << 24;
            dstval |= ((srcval & 0x0000FF00) >> 8)  << 16;
            dstval |= ((srcval & 0x00FF0000) >> 16) << 8;
            dstval |= ((srcval & 0xFF000000) >> 24) << 0;

            (*dstptr) = dstval;
            srcptr++;
            dstptr++;
        }


    return _return;
}

VOID ARGB2BGRAFREE(APTR dst)
{
    FreeVec(dst);
}


#include <proto/debug.h>
#include <proto/muimaster.h>
#include <libraries/debug.h>

#include <stdio.h>
#include <setjmp.h>

static APTR oldtraphandler = NULL;
static ULONG trapnumber = 0;
static APTR crashlocation = 0;

extern jmp_buf bailout_env;
extern Object *app;

void aros_bailout_jump()
{
    longjmp(bailout_env, -1);
}

static void aros_crash_handler()
{
    char msg[512];
    char submsg[256];

    snprintf(submsg, sizeof(submsg), "PC: 0x%p\nModule: - unknown -\nFunction: - unknown -", crashlocation);

    struct Library * DebugBase = OpenLibrary((CONST_STRPTR)"debug.library", 0L);

    if (DebugBase)
    {
        STRPTR modname, symname;
        APTR symaddr;

        if (DecodeLocation(crashlocation,
                    DL_ModuleName , &modname,
                    DL_SymbolName , &symname, DL_SymbolStart  , &symaddr,
                    TAG_DONE))
        {
            if (modname && symname)
            {
                snprintf(submsg, sizeof(submsg), "PC: 0x%p\nModule: %s\nFunction %s (0x%p) Offset 0x%p\n",
                    crashlocation, modname, symname, symaddr, (APTR)((IPTR)crashlocation - (IPTR)symaddr));
            }
        }

        CloseLibrary(DebugBase);
    }

    snprintf(msg, sizeof(msg), "Software failure in OWB has been detected.\n%s\n\nYou can either:\n - Crash: a hit will follow to dump stackframe\n - Quit: the application should quit properly and give all memory back.", submsg);

    ULONG ret = MUI_RequestA(app, NULL, 0, (CONST_STRPTR)"OWB Fatal Error",
                (CONST_STRPTR)"*_Quit|_Crash", (CONST_STRPTR)msg, NULL);
    switch(ret)
    {
        case 0:
            Alert(trapnumber);
            break;

        case 1:
            aros_bailout_jump();
            break;
    }
}

static void aros_trap_handler(ULONG trapNum, BYTE * p)
{
    /* Restore original handler in case we double-crash */
    struct Task * thistask = FindTask(NULL);

    thistask->tc_TrapCode = oldtraphandler;
    trapnumber = trapNum | AT_DeadEnd;

#if defined (__i386__)
    /* Set the eip to aros_crash_handler */
    /* NOTE: This changes private structure and might stop working when the structure changes! */
    crashlocation = (APTR)*((ULONG*)(p + 48));
    *((ULONG*)(p + 48)) = (ULONG)aros_crash_handler;
#else
#error program counter setting code missing for your architecture
#endif
}

void aros_register_trap_handler()
{
    struct Task * thistask = FindTask(NULL);

    oldtraphandler = thistask->tc_TrapCode;
    thistask->tc_TrapCode = (APTR)aros_trap_handler;
}

/* This changes the behavior of allocators to bailout mode where
 * as much as possible is done to shutdown the application ASAP
 * without additional crashes
 */
static int __memorybailout = 0;

int aros_is_memory_bailout()
{
    return __memorybailout;
}

int aros_memory_allocation_error(size_t size, size_t alignment)
{
    char msg[1024];
    CONST_STRPTR title = (CONST_STRPTR)"OWB Fatal Error";

    snprintf(msg, sizeof(msg), "Failed to allocate %u bytes aligned to %u bytes.\n\nUnfortunately, WebKit doesn't check allocations and crashes in such situation.\n\nYou can either:\n - Crash: a hit will follow to dump stackframe and allocated heap memory will be freed\n - Retry the allocation: free some memory elsewhere first and then press retry.\n - Quit: the application should quit properly and give all memory back.",
            (unsigned int)size, (unsigned int)alignment);

    ULONG ret = 0;

    if (app)
        ret = MUI_RequestA(app, NULL, 0, title, (CONST_STRPTR)"*_Retry|_Quit|_Crash", (CONST_STRPTR)msg, NULL);
    else
    {
        /* If the app is not set, the bailout_env is probably not set either */
        struct EasyStruct es;
        es.es_StructSize = sizeof(es);
        es.es_Flags = 0;
        es.es_Title = title;
        es.es_TextFormat = (CONST_STRPTR)msg;
        es.es_GadgetFormat = (CONST_STRPTR)"_Retry|_Crash";

        ret = EasyRequestArgs(0, &es, 0, 0);
    }

    switch(ret)
    {
        case 0:
            Alert(AG_NoMemory | AT_DeadEnd);
            break;

        case 1:
            break;

        case 2:
            __memorybailout = 1;
            break;
    }

    return ret;
}

static struct MsgPort * timerMP = NULL;
static struct IORequest * timerIO = NULL;
struct Library * TimerBase = NULL;
int aros_timer_open()
{
    timerMP = CreateMsgPort();
    if (timerMP != NULL)
    {
        timerIO = CreateIORequest(timerMP, sizeof(struct timerequest));
        if (timerIO != NULL)
        {
            if (OpenDevice((CONST_STRPTR)"timer.device", UNIT_VBLANK, timerIO, 0) == 0)
            {
                TimerBase = (struct Library *)timerIO->io_Device;
                return 1;
            }
        }
    }

    return 0;
}

void aros_timer_close()
{
    if (timerIO != NULL)
    {
        CloseDevice(timerIO);
        DeleteIORequest(timerIO);
    }
    if (timerMP != NULL)
        DeleteMsgPort(timerMP);
}
#endif
