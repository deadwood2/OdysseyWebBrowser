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
#include "Pasteboard.h"

#include "clipboard.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <datatypes/textclass.h>
#include <datatypes/pictureclass.h>

using namespace WebCore;

/* Clipboard Text copy */

bool copyTextToClipboard(char *text, bool utf8)
{
    IFFHandle *ih;
	size_t len = strlen(text);
    bool copied = false;

	if (text && len)
	{
		char *converted = utf8 ? utf8_to_local(text) : strdup(text);

		if(converted)
		{
			if ((ih = AllocIFF()))
			{
				if (ClipboardHandle *ch = OpenClipboard(PRIMARY_CLIP))
				{
		            ih->iff_Stream = (uint32)ch;
		            InitIFFasClip(ih);

					if (0 == OpenIFF(ih, IFFF_WRITE))
					{
						if (0 == PushChunk(ih, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN))
						{
							if (0 == PushChunk(ih, 0, ID_CHRS, IFFSIZE_UNKNOWN) &&
								WriteChunkBytes(ih, (APTR)converted, strlen(converted)) > 0 &&
							    0 == PopChunk(ih))
							{
								copied = true;
							}

							if(utf8)
							{
								copied = false;

								if( 0 == PushChunk(ih, 0, ID_UTF8, IFFSIZE_UNKNOWN) &&
								    WriteChunkBytes(ih, (APTR)text, len) > 0 &&
								    0 == PopChunk(ih))
								{
									copied = true;
								}
							}

							PopChunk(ih);
		                }
		                CloseIFF(ih);
		            }
		            CloseClipboard(ch);
		        }
		        FreeIFF(ih);
		    }

			free(converted);
		}
	}

	return copied;
}

static void copycollection(String &str, CollectionItem* ci, uint32 codeSet)
{
	/*
    if (ci->ci_Next)
		copycollection(str, ci->ci_Next, codeSet);
	*/

	if (CODESET_UTF8 == codeSet)
	{
		str = String::fromUTF8((const char *)ci->ci_Data, ci->ci_Size);
	}
	else if (CODESET_LOCAL == codeSet)
	{
		char *local = (char *) malloc(ci->ci_Size + 1);

		if(local)
		{
			char *converted;

			memcpy(local, (const char *) ci->ci_Data, ci->ci_Size);
			local[ci->ci_Size] = 0;

			converted = local_to_utf8(local);

			if(converted)
			{
				str = String::fromUTF8(converted);
				free(converted);
			}

			free(local);
		}
	}
}

WTF::String pasteFromClipboard(void)
{
    String result;

	if (IFFHandle *ih = AllocIFF())
	{
		if (ClipboardHandle *ch = OpenClipboard(PRIMARY_CLIP))
		{
            ih->iff_Stream = (uint32)ch;
            InitIFFasClip(ih);

			if (0 == OpenIFF(ih, IFFF_READ))
			{
				const LONG chunks[4] = { ID_FTXT, ID_CHRS, ID_FTXT, ID_UTF8 };

				if (0 == CollectionChunks(ih, (LONG *)chunks, 2) &&
				    0 == StopOnExit(ih, ID_FTXT, ID_FORM))
				{
                    while (true)
					{
						if (IFFERR_EOC == ParseIFF(ih, IFFPARSE_SCAN))
						{
							CollectionItem *ci, *ci2;

							// First chunk is expected to have local charset
							ci = FindCollection(ih, ID_FTXT, ID_CHRS);

							// Second chunk is optional, and expected to have UTF8 charset if present
							ci2 = FindCollection(ih, ID_FTXT, ID_UTF8);

							if (ci2)
								copycollection(result, ci2, CODESET_UTF8);
							else if(ci)
								copycollection(result, ci, CODESET_LOCAL);
                        }
                        else
                            break;
					}
				}

                CloseIFF(ih);
            }
            CloseClipboard(ch);
        }
        FreeIFF(ih);
    }

    return result;
}

/********************************************************************************************/

/* Clipboard picture copy (wild copy/paste... should be cleaned up) */

#define CkErr(expression)  {if (!error) error = (expression);}
#define putbmhd(iff, bmHdr)  \
	PutCk(iff, ID_BMHD, sizeof(struct BitMapHeader), (BYTE *)bmHdr)

#define DUMP 0
#define RUN	1

#define MinRun 3
#define MaxRun 128
#define MaxDat 128

/* When used on global definitions, static means private.
 * This keeps these names, which are only referenced in this
 * module, from conficting with same-named objects in your program.
 */
static LONG putSize;
static char buf[ 256 ]; 	/* [TBD] should be 128?  on stack?*/

#define GetByte()	(*source++)
#define PutByte(c)	{ *dest++ = (c);   ++putSize; }

static int align(int a, int b)
{
	return (a + b - 1) & (~(b - 1));
}

static BYTE *PutDump( BYTE *dest, int nn )
{
    int i;

    PutByte( nn - 1 );
    for ( i = 0; i < nn; i++ ) PutByte( buf[ i ] );
    return ( dest );
}

static BYTE *PutRun( BYTE *dest, int nn, int cc )
{
    PutByte( -( nn - 1 ) );
    PutByte( cc );
    return ( dest );
}

#define OutDump(nn)   dest = PutDump(dest, nn)
#define OutRun(nn,cc) dest = PutRun(dest, nn, cc)

/*----------- packrow --------------------------------------------------*/
/* Given POINTERS TO POINTERS, packs one row, updating the source and
 * destination pointers.  RETURNs count of packed bytes.
 */
static LONG packrow( BYTE *pSource, BYTE *pDest, LONG rowSize )
{
    BYTE * source, *dest;
    char c, lastc = '\0';
    BOOL mode = DUMP;
    short nbuf = 0; 		/* number of chars in buffer */
    short rstart = 0; 		/* buffer index current run starts */

	source = pSource;
	dest = pDest;
    putSize = 0;
    buf[ 0 ] = lastc = c = GetByte();   /* so have valid lastc */
    nbuf = 1; rowSize--; 	/* since one byte eaten.*/


    for ( ; rowSize; --rowSize )
    {
        buf[ nbuf++ ] = c = GetByte();
        switch ( mode )
        {
            case DUMP:
                /* If the buffer is full, write the length byte,
                   then the data */
                if ( nbuf > MaxDat )
                {
                    OutDump( nbuf - 1 );
                    buf[ 0 ] = c;
                    nbuf = 1;
                    rstart = 0;
                    break;
                }

                if ( c == lastc )
                {
                    if ( nbuf - rstart >= MinRun )
                    {
                        if ( rstart > 0 ) OutDump( rstart );
                        mode = RUN;
                    }
                    else if ( rstart == 0 )
                        mode = RUN; 	/* no dump in progress,
                    				so can't lose by making these 2 a run.*/
                }
                else rstart = nbuf - 1; 		/* first of run */
                break;

            case RUN:
                if ( ( c != lastc ) || ( nbuf - rstart > MaxRun ) )
                {
                    /* output run */
                    OutRun( nbuf - 1 - rstart, lastc );
                    buf[ 0 ] = c;
                    nbuf = 1;
                    rstart = 0;
                    mode = DUMP;
                }
                break;
        }

        lastc = c;
    }

    switch ( mode )
    {
        case DUMP:
            OutDump( nbuf );
            break;
        case RUN:
            OutRun( nbuf - rstart, lastc );
            break;
    }
    return ( putSize );
}


/*
 * PutCk
 *
 * Writes one chunk of data to an iffhandle
 *
 */
static long PutCk( struct IFFHandle *iff, long id, long size, void *data )
{
    long error = 0, wlen;

	if ( !(error = PushChunk( iff, 0, id, size )) )
	{
        /* Write the actual data */

        if ( ( wlen = WriteChunkBytes( iff, data, size ) ) != size )
        {
            error = IFFERR_WRITE;
        }
        else error = PopChunk( iff );
    }
    return ( error );
}

/* chunky 2 planar routine */

static void chunky2planar( unsigned char	*chunky , unsigned char	  *planar , int width)
{
	int				stupid;
	char 			*bufftemp;
	int				mod = align( width , 16 );
	unsigned char	*buffptr;
	int				planarIdx = 0;
	int				i;

	bufftemp = (char *) malloc( mod * 3 );
	buffptr = (unsigned char *) bufftemp;

    /* First loop.  Convert internal to DKB */

	for (i = 0; i < width; i++)
    {
		bufftemp[i]             = chunky[i * 4 + 1];
		bufftemp[i + mod ]		= chunky[i * 4 + 2];
		bufftemp[i + mod * 2]	= chunky[i * 4 + 3];
	}

	width = align( width , 16 );

	/* now for second loop.  DKB to IFF.  Gotta handle the weird interleaving */

	for (stupid = 0; stupid < 3; stupid++)
	{
		/* RGB component */

		int	shift;  /* amount to shift when placing in pixel byte */

		for (shift = 0; shift < 8; shift++)
		{
			/* Bitplane number */

			int	mask = 1 << shift;
			int phase = 0;
			int	pixel = 0;
			int	w;

			for (w = 0; w < width; w++)
			{
				/* width */

				pixel <<= 1;	/* make room for next bit */
				pixel += ( buffptr[ w ] & mask ) ? 1:0;		  /* insert next bit */
				phase++;

				if (phase == 8)
                {
					planar[ planarIdx] = pixel;
					planarIdx++;
					pixel = phase = 0;

				}
			}
		}

		/* next component */

		buffptr += width;
	}

	/* Ok!  second loop done.  Got bits sorted into bytes.  Now we have an IFF24 line. */

	free(bufftemp);
}

/*---------- putbody ---------------------------------------------------*/

static long putbody( struct IFFHandle  *iff,
				ChkImage	*img,
				BYTE  *mask,
				struct BitMapHeader	  *MyBitMapHeader)
{
	int	error;
	int	bytesPerPlane = align( img->width , 16 ) >> 3;
	int	iRow;
	unsigned char	*planarData = (unsigned char *) calloc( 1 , bytesPerPlane * 24 );

	/* buffering */

	int				bufferSize = 1<<11;
	unsigned char	buffer[ 1<<11 ];
	int				bytes = 0;

	if	( !planarData )
	{
		return  IFFERR_WRITE;
	}

    /* Write out a BODY chunk header */

	if ( (error = PushChunk( iff, NULL, ID_BODY, IFFSIZE_UNKNOWN ) ) )
	{
		free( planarData );
		return ( error );
	}

    /* Write out the BODY contents */
	for ( iRow = 0 ; iRow < MyBitMapHeader->bmh_Height; iRow++ )
    {
		/* convert from chunky 2 planar */

		chunky2planar( img->data.b + img->width * iRow * 4 , planarData , img->width );

		if	( !MyBitMapHeader->bmh_Compression )
		{
			if ( WriteChunkBytes( iff, planarData , bytesPerPlane * 24 ) != bytesPerPlane * 24)
				error = IFFERR_WRITE;
		}
		else
		{
			/* we have to compress each plane separately */

			int	i;

			for	(i=0;i<24;i++)
			{
				bytes += packrow( (BYTE *) (planarData +  bytesPerPlane * i) , (BYTE *) (buffer + bytes) , bytesPerPlane );

				if	( bytes > bufferSize / 2 )
				{
					/* filled half of the buffer. Write */

                    if ( WriteChunkBytes( iff, buffer , bytes ) != bytes)
						error = IFFERR_WRITE;

					bytes = 0;
				}
			}
		}
    }

	/* write last portion of data */

	if	( bytes )
	{
		if ( WriteChunkBytes( iff, buffer , bytes ) != bytes)
			error = IFFERR_WRITE;
	}

	/* free used memory */

	free( planarData );

    /* Finish the chunk */
    error = PopChunk( iff );

    return ( error );
}


static LONG	saveilbm( struct IFFHandle	*iff,
               struct BitMapHeader	*MyBitMapHeader,
			   ChkImage	*img,
               UBYTE	*MaskPlane,
               ULONG	transparentColor,
               char	*filename )
{
	LONG error = 0L;

    if ( OpenIFF( iff,
                  IFFF_WRITE ) == 0 )
    {
        error = PushChunk( iff, ID_ILBM, ID_FORM, IFFSIZE_UNKNOWN );

        if ( filename )
        {
            int	Length;
            Length	= strlen( filename );
            CkErr( PutCk( iff, ID_NAME, Length, filename ) );
        }

        CkErr( putbmhd( iff, MyBitMapHeader ) );

        /* Write out the BODY
         */

        CkErr( putbody( iff,
						img,
						(BYTE *) MaskPlane,
						MyBitMapHeader ) );

        CkErr( PopChunk( iff ) ); 	/* close out the FORM */
        CloseIFF( iff );
    }

    return ( error );
}

/*****************************************************************************/

static ULONG WriteIFF(struct IFFHandle *MyHandle, ChkImage *img, char *fname)
{
    struct BitMapHeader	MyBitMapHeader;
	ULONG				Result = FALSE;

	/* initialize bitmap header */

	MyBitMapHeader.bmh_Width = img->width;
	MyBitMapHeader.bmh_Height = img->height;
	MyBitMapHeader.bmh_Left = MyBitMapHeader.bmh_Top = 0;  /* Default position is (0,0).*/
	MyBitMapHeader.bmh_Depth = 24;
	MyBitMapHeader.bmh_Masking = 0;
	MyBitMapHeader.bmh_Compression = 1;
	MyBitMapHeader.bmh_Transparent = 0;
	MyBitMapHeader.bmh_XAspect = 1;
	MyBitMapHeader.bmh_YAspect = 1;
	MyBitMapHeader.bmh_PageWidth = img->width;
	MyBitMapHeader.bmh_PageHeight = img->height;

	if ( saveilbm( MyHandle,		/* IFFHandle */
				   &MyBitMapHeader,	/* BitMapHeader */
				   img,				/* ChkImage */
				   NULL,			/* masking */
				   0,				/* transparency */
				   fname ) == 0 )
	{
		Result	= TRUE;
	}

    return ( Result );
}

bool copyImageToClipboard(ChkImage *img)
{
	struct IFFHandle *MyHandle;
	bool result = false;
	char *fname = "";

	if((MyHandle = AllocIFF()))
    {
		if((MyHandle->iff_Stream = (ULONG) OpenClipboard(0)))
        {
			InitIFFasClip(MyHandle);
			result = WriteIFF(MyHandle, img, fname) != FALSE;

			CloseClipboard((struct ClipboardHandle*) MyHandle->iff_Stream);
        }

		FreeIFF(MyHandle);
    }

	return result;
}

/*****************************************************************************************************/

/* Clipboard Monitoring */

static struct IOClipReq *clipboardOpen(ULONG unit)
{
	struct MsgPort *mp;
	struct IOStdReq *ior;

	if ((mp = CreatePort(0L, 0L)))
	{
		if ((ior = (struct IOStdReq *) CreateExtIO(mp, sizeof(struct IOClipReq))))
		{
			if (!(OpenDevice("clipboard.device", unit, (struct IORequest *) ior, 0L)))
			{
				return((struct IOClipReq *)ior);
			}
			DeleteExtIO((struct IORequest *) ior);
		}
	    DeletePort(mp);
	}
	return(NULL);
}

static void clipboardClose(struct IOClipReq *ior)
{
	struct MsgPort *mp;

	mp = ior->io_Message.mn_ReplyPort;

	CloseDevice((struct IORequest *)ior);
	DeleteExtIO((struct IORequest *)ior);
	DeletePort(mp);
}

static struct Hook changeHook;
static struct IOClipReq *clipReq;

ULONG clipHook (struct Hook * h, VOID * o, struct ClipHookMsg * msg)
{
        Pasteboard::createForGlobalSelection()->clear();
	//Pasteboard::generalPasteboard()->clear();
	return (0);
}

bool installClipboardMonitor (void)
{
	struct IOClipReq *clipIO;

	/* Open clipboard unit 0 */

	if ((clipIO = clipboardOpen( 0L )))
	{
	    /* Fill out the IORequest */
		clipIO->io_Data    = (char *) &changeHook;
		clipIO->io_Length  = 1;
	    clipIO->io_Command = CBD_CHANGEHOOK;

	    /* Prepare the hook */
		changeHook.h_Entry = (APTR)HookEntry;
		changeHook.h_SubEntry = (APTR) clipHook;
		changeHook.h_Data = NULL;

	    /* Start the hook */
		if (DoIO ((struct IORequest *) clipIO))
		{
			printf ("Unable to set clipboard hook\n");
		}

		clipReq = clipIO;
	}

	return clipReq != NULL;
}

void removeClipboardMonitor (void)
{
	if(clipReq)
	{
		/* Fill out the IO request */
		clipReq->io_Data = (char *) &changeHook;
		clipReq->io_Length = 0;
		clipReq->io_Command = CBD_CHANGEHOOK;

		/* Stop the hook */
		if (DoIO ((struct IORequest *) clipReq))
		{
			printf ("Unable to stop clipboard hook\n");
		}

		clipboardClose(clipReq);
		clipReq = NULL;
	}
}
