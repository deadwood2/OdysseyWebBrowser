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

#include <string.h>

#include <proto/dos.h>
#include <proto/asl.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <clib/macros.h>

#include <clib/debug_protos.h>

#include "gui.h"
#include "asl.h"

ULONG asl_run_multiple(STRPTR p, struct TagItem *tags, char *** files, ULONG remember_path)
{
	ULONG count = 0;
	STRPTR dir, file;

	*files = NULL;
	file = NULL;
	dir = NULL;

	if (p)
	{
		dir = (STRPTR) AllocVecTaskPooled(strlen(p) + 1);

		if (dir)
		{
			ULONG len;
			BPTR l;

			memset(dir, 0, strlen(p) + 1);

			l = Lock(p, ACCESS_READ);

			if(l)
			{
				struct FileInfoBlock fi;

				if(Examine(l, &fi))
				{

					if(fi.fib_DirEntryType > 0)
					{
						len = strlen(p) + 1;
                        stccpy(dir, p, len);
						file = NULL;
					}
					else
					{
						file = FilePart(p);
						len = file - p + 1;
						stccpy(dir, p, len);
					}
				}
				UnLock(l);
			}
			else
			{
				file = FilePart(p);
				len = file - p + 1;
				stccpy(dir, p, len);
			}
		}
	}

	if (dir || !p)
	{
		struct FileRequester *req;

		req = (struct FileRequester *)MUI_AllocAslRequestTags(ASL_FileRequest,
			ASLFR_DoPatterns, TRUE,
			ASLFR_DoMultiSelect, TRUE,
			file == dir ? TAG_IGNORE : ASLFR_InitialDrawer, dir,
			file ? ASLFR_InitialFile : TAG_IGNORE, file,
			TAG_MORE, tags
		);

		if (req)
		{
			set(app, MUIA_Application_Sleep, TRUE);

			if (MUI_AslRequestTags(req, TAG_DONE))
			{
				count = req->fr_NumArgs;

				*files = (char **) AllocVecTaskPooled(count*sizeof(char *));

				if(*files)
				{
					int i;
					struct WBArg *ap;
					char buf[256];

					for (ap=req->fr_ArgList,i=0;i<req->fr_NumArgs;i++,ap++)
					{
						if(NameFromLock(ap->wa_Lock, buf, sizeof(buf)))
						{
							if(remember_path)
								set(app, MA_OWBApp_CurrentDirectory, buf);

							AddPart((STRPTR) buf, (STRPTR) ap->wa_Name, sizeof(buf));

							(*files)[i] = (char *) AllocVecTaskPooled(strlen(buf)+1);

							if((*files)[i])
							{
								strcpy((*files)[i], buf);
								//kprintf("selected file %d <%s>\n", i, (*files)[i]);
							}
						}
					}
				}
			}

			set(app, MUIA_Application_Sleep, FALSE);

			MUI_FreeAslRequest(req);
		}

		if (dir)
		{
			FreeVecTaskPooled(dir);
		}
	}

	return count;
}

void asl_free(ULONG count, char ** files)
{
	ULONG i;
	for(i = 0; i < count; i++)
	{
		FreeVecTaskPooled(files[i]);
	}
	FreeVecTaskPooled(files);
}

char * asl_run(STRPTR p, struct TagItem *tags, ULONG remember_path)
{
	STRPTR result, dir, file;

	file = NULL;
	dir = NULL;

	if (p)
	{
		dir = (STRPTR) AllocVecTaskPooled(strlen(p) + 1);

		if (dir)
		{
			ULONG len;
			BPTR l;

			memset(dir, 0, strlen(p) + 1);

			l = Lock(p, ACCESS_READ);

			if(l)
			{
				struct FileInfoBlock fi;

				if(Examine(l, &fi))
				{

					if(fi.fib_DirEntryType > 0)
					{
						len = strlen(p) + 1;
                        stccpy(dir, p, len);
						file = NULL;
					}
					else
					{
						file = FilePart(p);
						len = file - p + 1;
						stccpy(dir, p, len);
					}
				}
				UnLock(l);
			}
			else
			{
				file = FilePart(p);
				len = file - p + 1;
				stccpy(dir, p, len);
			}
		}
	}

	result = NULL;

	if (dir || !p)
	{
		struct FileRequester *req;

		req = (struct FileRequester *)MUI_AllocAslRequestTags(ASL_FileRequest,
			ASLFR_DoPatterns, TRUE,
			file == dir ? TAG_IGNORE : ASLFR_InitialDrawer, dir,
			file ? ASLFR_InitialFile : TAG_IGNORE, file,
			TAG_MORE, tags
		);

		if (req)
		{
			set(app, MUIA_Application_Sleep, TRUE);

			if (MUI_AslRequestTags(req, TAG_DONE))
			{
				ULONG tlen;

				tlen = strlen(req->fr_Drawer) + strlen(req->fr_File) + 8;

				result = (STRPTR) AllocVecTaskPooled(tlen);

				if (result)
				{
					strcpy(result, req->fr_Drawer);
					AddPart(result, req->fr_File, tlen);

					if(remember_path)
						set(app, MA_OWBApp_CurrentDirectory, req->fr_Drawer);
				}
			}

			set(app, MUIA_Application_Sleep, FALSE);

			MUI_FreeAslRequest(req);
		}

		if (dir)
		{
			FreeVecTaskPooled(dir);
		}
	}

	return result;
}
