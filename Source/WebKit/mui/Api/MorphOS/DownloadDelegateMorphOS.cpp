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

#if defined(__AROS__) /* Variadic + C++ local object problem */
#define NO_INLINE_STDARG
#endif

#include "config.h"
#include "ResourceResponse.h"
#include "WebDownload.h"
#include "WebDownloadPrivate.h"
#include "WebURLResponse.h"
#include "WebError.h"
#include "DownloadDelegateMorphOS.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include <wtf/CurrentTime.h>
#include "URL.h"

#include <dos/dos.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <proto/utility.h>
#include <clib/debug_protos.h>

#include <stdio.h>

#include "gui.h"
#include "utils.h"
#include "owbcommand.h"

#undef set

#define D(x)

using namespace WebCore;

DownloadDelegateMorphOS::DownloadDelegateMorphOS()
{
}

DownloadDelegateMorphOS::~DownloadDelegateMorphOS()
{
	D(kprintf("DownloadDelegateMorphOS::~DownloadDelegateMorphOS\n"));
}

void DownloadDelegateMorphOS::decideDestinationWithSuggestedFilename(WebDownload *download, const char* suggestedFilename)
{
    D(kprintf("DownloadDelegateMorphOS::decideDestinationWithSuggestedFilename(%s)\n", fileName));

    char* fileName = utf8_to_local(suggestedFilename);

    if(fileName)
    {
		WebDownloadPrivate* priv = download->getWebDownloadPrivate();

		priv->dl = (struct downloadnode *) DoMethod(app, MM_OWBApp_Download, priv->requestUri.utf8().data(), fileName, (APTR) download);

		if(priv->dl)
		{
			priv->dl->size = priv->totalSize;
			if(!priv->mimetype.isEmpty())
				priv->dl->mimetype = strdup(priv->mimetype.utf8().data());
			priv->dl->webdownload = download;

			if(priv->dl->path && priv->dl->filename)
			{
				char *localUri;
				unsigned long size = strlen(priv->dl->path) + strlen(priv->dl->filename) + 2;
				
				localUri = (char *) malloc(size);

				if(localUri)
				{
					BPTR l;
					bool overwrite = true;
					bool resume = false;

					strcpy(localUri, priv->dl->path);
					AddPart(localUri, priv->dl->filename, size);

					if(!priv->quiet)
					{
						if((l=Lock(localUri, ACCESS_READ)))
						{
                            struct FileInfoBlock *fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);

							if(fib)
							{
								if(Examine(l, fib))
								{
									struct DateTime dt;
									char message[8192];
									TEXT tmpdate[50];
									TEXT tmptime[50];
									String truncatedURI = truncate(String(localUri), 64);

									dt.dat_Stamp.ds_Days    = fib->fib_Date.ds_Days;
									dt.dat_Stamp.ds_Minute  = fib->fib_Date.ds_Minute;
									dt.dat_Stamp.ds_Tick    = fib->fib_Date.ds_Tick;
									dt.dat_Format           = FORMAT_DEF;
									dt.dat_Flags            = 0;
									dt.dat_StrDay           = NULL;
									dt.dat_StrDate          = tmpdate;
									dt.dat_StrTime          = tmptime;

									if(!DateToStr(&dt))
									{
										strcpy((char *)tmpdate, "Unknown");
										strcpy((char *)tmptime, "Unknown");
									}

									snprintf(message, sizeof(message), GSI(MSG_DOWNLOADDELEGATE_OVERWRITE_OR_RESUME),
																	   truncatedURI.latin1().data(),
																	   priv->totalSize,
																	   fib->fib_Size, tmpdate, tmptime);

									int ret = MUI_RequestA(app, NULL, 0, GSI(MSG_REQUESTER_NORMAL_TITLE), GSI(MSG_DOWNLOADDELEGATE_OVERWRITE_OR_RESUME_ACTIONS), message, NULL);

									switch(ret)
									{
										case 1:
											resume = false;
											overwrite = true;
									 		break;
										case 2:
											resume = true;
											overwrite = false;
											priv->startOffset = (unsigned long long) fib->fib_Size;
											priv->currentSize = priv->startOffset;
											break;
										default:
										case 0:
											resume = false;
											overwrite = false;
											break;
									}
								}

								FreeDosObject(DOS_FIB, fib);
							}

							UnLock(l);
						}
					}

					D(kprintf("destination %s overwrite %d resume %d\n", localUri, overwrite, resume));

					download->setDestination(localUri, overwrite, resume);

					free(localUri);
				}
			}
		}
		free(fileName);
    }
}

void DownloadDelegateMorphOS::didCancelAuthenticationChallenge(WebDownload* download, WebURLAuthenticationChallenge* challenge)
{
    D(kprintf("DownloadDelegateMorphOS %p - didCancelAuthenticationChallenge %p\n", download, challenge));
}

void DownloadDelegateMorphOS::didCreateDestination(WebDownload* download, const char *destination)
{
    D(kprintf("DownloadDelegateMorphOS %p - didCreateDestination %s\n", download, destination));
}

void DownloadDelegateMorphOS::didReceiveAuthenticationChallenge(WebDownload* download, WebURLAuthenticationChallenge* challenge)
{
    D(kprintf("DownloadDelegateMorphOS %p - didReceiveAuthenticationChallenge %p\n", download, challenge));
}

void DownloadDelegateMorphOS::didReceiveDataOfLength(WebDownload* download, unsigned length)
{
    D(kprintf("DownloadDelegateMorphOS %p - didReceiveDataOfLength %p %d\n", download, length));

	WebDownloadPrivate* priv = download->getWebDownloadPrivate();

	double currenttime = currentTime();

	if(priv->dl)
	{
		priv->dl->done = priv->currentSize + length;

		if(currenttime > priv->dl->lastupdatetime + 0.1)
		{
			priv->dl->lastupdatetime = currenttime;
			DoMethod(app, MM_OWBApp_DownloadUpdate, priv->dl);
		}
	}
}

void DownloadDelegateMorphOS::didReceiveResponse(WebDownload* download, WebURLResponse* response)
{
    D(kprintf("DownloadDelegateMorphOS %p - didReceiveResponse %p\n", download, response));

	WebDownloadPrivate* priv = download->getWebDownloadPrivate();

	priv->totalSize = response->expectedContentLength();
	priv->mimetype = response->resourceResponse().mimeType();
}

void DownloadDelegateMorphOS::willResumeWithResponse(WebDownload* download, WebURLResponse* response, long long fromByte)
{
    D(kprintf("DownloadDelegateMorphOS %p - willResumeWithResponse %p, %q\n", download, response, fromByte));
}

WebMutableURLRequest* DownloadDelegateMorphOS::willSendRequest(WebDownload* download, WebMutableURLRequest* request, WebURLResponse* redirectResponse)
{
    D(kprintf("DownloadDelegateMorphOS %p - willSendRequest %p %p\n", download, request, redirectResponse));
    return request;
}

bool DownloadDelegateMorphOS::shouldDecodeSourceDataOfMIMEType(WebDownload*, const char*)
{
    return false;
}

void DownloadDelegateMorphOS::didBegin(WebDownload* download)
{
    D(kprintf("DownloadDelegateMorphOS %p - didBegin\n", download));
    registerDownload(download);
	
	DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_Downloads, FALSE);
/*
    if(delegate)
		DoMethod(delegate, MUIM_DownloadDelegate_DidBeginDownload, download);
*/
}

void DownloadDelegateMorphOS::didFinish(WebDownload* download)
{
    D(kprintf("DownloadDelegateMorphOS %p - didFinish\n", download));

	WebDownloadPrivate* priv = download->getWebDownloadPrivate();
	char cpath[1024];
	char ccomment[80];
	char *ptr;
	
	ptr = strdup(priv->destinationPath.latin1().data());
	stccpy(cpath, ptr, sizeof(cpath));
	free(ptr);

	URL url = URL(ParsedURLString, priv->requestUri);
	url.setPass("");
	ptr = strdup(url.string().utf8().data());
	stccpy(ccomment, ptr, sizeof(ccomment));
	free(ptr);

	DoMethod(app, MM_OWBApp_DownloadDone, priv->dl);

	D(kprintf("DownloadDelegateMorphOS %p - SetComment(%s, %s)\n", download, (STRPTR) priv->destinationPath.latin1().data(), (STRPTR) priv->requestUri.utf8().data()));

	SetComment((STRPTR) cpath, (STRPTR) ccomment);

	if(!priv->command.isEmpty())
	{
		D(kprintf("Check DownloadDelegateMorphOS %p - Command (%s)\n", download, (STRPTR) priv->command.latin1().data()));

		OWBCommand cmd(priv->command, ACTION_AMIGADOS);
		cmd.execute();
	}
    unregisterDownload(download);

	delete download;
}

void DownloadDelegateMorphOS::didFailWithError(WebDownload* download, WebError* error)
{
	char *errormsg = (char *) error->localizedDescription();

	D(kprintf("DownloadDelegateMorphOS %p - didFailWithError(%s)\n", download, errormsg));

	WebDownloadPrivate* priv = download->getWebDownloadPrivate();

	if(!priv->destinationPath.isEmpty())
	{
		char cpath[1024];
		char ccomment[80];
		String comment = String("FAILED ") + priv->requestUri ;
		char *ptr;

		ptr = strdup(priv->destinationPath.latin1().data());
		stccpy(cpath, ptr, sizeof(cpath));
		free(ptr);

		URL url = URL(ParsedURLString, priv->requestUri);
		url.setPass("");
		comment = String("FAILED ") + url.string();
		ptr = strdup(comment.utf8().data());
		stccpy(ccomment, ptr, sizeof(ccomment));
		free(ptr);

		SetComment((STRPTR) cpath, (STRPTR) ccomment);
	}

	DoMethod(app, MM_OWBApp_DownloadError, priv->dl, errormsg);

	free(errormsg);

    unregisterDownload(download);

	delete download;
}
