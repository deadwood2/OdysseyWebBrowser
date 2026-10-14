/*  BSD License Copyright (C) 2020-2023 Jacek Piszczek. All rights reserved.
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY . AND ITS CONTRIBUTORS “AS IS” AND ANY EXPRESS
    OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
    DAMAGE. */

#include <WebCore/DocumentLoader.h>
#include <WebCore/LocalFrame.h>
#include <WebCore/FrameLoader.h>
#include "ABPFilterParser.h"
#include "WebFrame.h"
#include "WebFrameLoaderClient.h"

namespace WayfarerAdBlock {

ABP::ABPFilterParser g_urlFilter;
std::vector<char>    g_urlFilterData;

void initialize()
{
	WTF::String easyListPath = "PROGDIR:Conf/easylist.txt"_s;
	WTF::String easyListSerializedPath = "PROGDIR:Conf/easylist.dat"_s;

	WTF::FileSystemImpl::PlatformFileHandle fh = WTF::FileSystemImpl::openFile(easyListSerializedPath, WTF::FileSystemImpl::FileOpenMode::Read);

	if (WTF::FileSystemImpl::invalidPlatformFileHandle != fh)
	{
		long long size = WTF::FileSystemImpl::fileSize(fh).value_or(0);
		if (size > 0ll)
		{
			g_urlFilterData.resize(size + 1);
			if (size == WTF::FileSystemImpl::readFromFile(fh, std::span { reinterpret_cast<uint8_t*>(&g_urlFilterData[0]), static_cast<size_t>(size) }))
			{
				g_urlFilterData[size] = 0; // terminate just in case
				g_urlFilter.deserialize(&g_urlFilterData[0]);
			}
			else
			{
				g_urlFilterData.clear();
			}
		}

		WTF::FileSystemImpl::closeFile(fh);
	}
	else
	{
		WTF::FileSystemImpl::PlatformFileHandle fh = WTF::FileSystemImpl::openFile(easyListPath, WTF::FileSystemImpl::FileOpenMode::Read);

		if (WTF::FileSystemImpl::invalidPlatformFileHandle != fh)
		{
			long long size = WTF::FileSystemImpl::fileSize(fh).value_or(0);
			if (size > 0ll)
			{
				char *buffer = (char *)malloc(size + 1);
				if (buffer)
				{
					if (size == WTF::FileSystemImpl::readFromFile(fh, std::span { reinterpret_cast<uint8_t*>(buffer), static_cast<size_t>(size) }))
					{
						buffer[size] = 0; // terminate, parser expects this to be a null-term string
//dprintf("Parsing easylist.txt; this will take a while... and will be faster on next launch!\n");
						g_urlFilter.parse(buffer);
						int ssize;
						char *sbuffer = g_urlFilter.serialize(&ssize, false);
						WTF::FileSystemImpl::PlatformFileHandle dfh = WTF::FileSystemImpl::openFile(easyListSerializedPath, WTF::FileSystemImpl::FileOpenMode::Truncate);
						if (WTF::FileSystemImpl::invalidPlatformFileHandle != dfh)
						{
							if (ssize != WTF::FileSystemImpl::writeToFile(dfh, std::span { reinterpret_cast<const uint8_t*>(sbuffer), static_cast<size_t>(ssize) }))
							{
								WTF::FileSystemImpl::closeFile(dfh);
								WTF::FileSystemImpl::deleteFile(easyListSerializedPath);
							}
							else
							{
								WTF::FileSystemImpl::closeFile(dfh);
							}
						}
						else
						{
//							dprintf(">> failed opening easylist.dat for write\n");
						}
						delete[] sbuffer;
					}
					
					free(buffer);
				}
			}
			WTF::FileSystemImpl::closeFile(fh);
		}
	}
}

bool shouldAllowRequest(const char *url, const char *mainPageURL, WebCore::DocumentLoader& loader)
{
//	WebFrame *frame = static_cast<WebFrameLoaderClient&>(loader.frame()->loader().client()).webFrame();
//	if (!frame)
//		return false;
//	WebPage *page = frame->page();
//	if (!page)
//		return false;
//
//	if (!page->adBlockingEnabled())
//		return true;

	if (g_urlFilter.matches(url, ABP::FONoFilterOption, mainPageURL))
	{
		return false;
	}

	return true;
}

}

