/*
 * Copyright (C) 2020-2022 Jacek Piszczek
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MIMETypeRegistry.h"
#include <wtf/NeverDestroyed.h>
#include <wtf/HashMap.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringView.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// TODO: use code for Iris

static const HashMap<String, String, ASCIICaseInsensitiveHash> hCommonMediaTypes(std::initializer_list<HashMap<String, String, ASCIICaseInsensitiveHash>::KeyValuePairType>{
    { "bmp"_s, "image/bmp"_s },
    { "css"_s, "text/css"_s },
    { "gif"_s, "image/gif"_s },
    { "html"_s, "text/html"_s },
    { "htm"_s, "text/html"_s },
    { "ico"_s, "image/x-icon"_s },
    { "jpeg"_s, "image/jpeg"_s },
    { "jpg"_s, "image/jpeg"_s },
    { "js"_s, "application/x-javascript"_s },
    { "pdf"_s, "application/pdf"_s },
    { "png"_s, "image/png"_s },
    { "rss"_s, "application/rss+xml"_s },
    { "svg"_s, "image/svg+xml"_s },
    { "swf"_s, "application/x-shockwave-flash"_s },
    { "text"_s, "text/plain"_s },
    { "txt"_s, "text/plain"_s },
    { "xbm"_s, "image/x-xbitmap"_s },
    { "xml"_s, "text/xml"_s },
    { "xsl"_s, "text/xsl"_s },
    { "xhtml"_s, "application/xhtml+xml"_s },
    { "wml"_s, "text/vnd.wap.wml"_s },
    { "wmlc"_s, "application/vnd.wap.wmlc"_s },
	{ "htm"_s, "text/html"_s },
	{ "html"_s, "text/html"_s },
	{ "shtm"_s, "text/html"_s },
	{ "shtml"_s, "text/html"_s },
	{ "php"_s, "text/html"_s },
#if USE(WEBP)
    { "webp"_s, "image/webp"_s },
#endif
});

String MIMETypeRegistry::mimeTypeForExtension(const String& extension)
{
	auto it = hCommonMediaTypes.find(extension);
	if (it != hCommonMediaTypes.end())
	{
		return it->value;
	}
    return emptyString();
}

Vector<String> MIMETypeRegistry::extensionsForMIMEType(const String& type)
{
	Vector<String> out;
	
	for (auto& kvpair : hCommonMediaTypes)
	{
		if (equalIgnoringASCIICase(type, kvpair.value))
		{
			out.append(kvpair.key);
		}
	}
	
	return out;
}

bool MIMETypeRegistry::isApplicationPluginMIMEType(const String&)
{
    return false;
}

String MIMETypeRegistry::preferredExtensionForMIMEType(const String& mimeType)
{
    for (auto& kvpair : hCommonMediaTypes)
    {
        if (equalIgnoringASCIICase(mimeType, kvpair.value))
            return kvpair.key;
    }
    return emptyString();
}

} // namespace WebCore
