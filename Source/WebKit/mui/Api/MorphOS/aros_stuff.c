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

#endif
