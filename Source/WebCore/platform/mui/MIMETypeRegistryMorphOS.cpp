/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
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
#include "MIMETypeRegistry.h"

#include "gui.h"

namespace WebCore {

struct ExtensionMap {
    const char* mimeType;
    const char* extension;
};

static const char textPlain[] = "text/plain";
static const char textHtml[] = "text/html";
static const char imageJpeg[] = "image/jpeg";
static const char octetStream[] = "application/octet-stream";

static const ExtensionMap extensionMap [] = {
	{ textPlain, "txt" },
    { textPlain, "text" },
    { textHtml, "html" },
    { textHtml, "htm" },
    { "text/css", "css" },
    { "text/xml", "xml" },
    { "text/xsl", "xsl" },
    { "image/gif", "gif" },
    { "image/png", "png" },
    { imageJpeg, "jpeg" },
    { imageJpeg, "jpg" },
    { imageJpeg, "jfif" },
    { imageJpeg, "pjpeg" },
    { "image/webp", "webp" },
    { "image/bmp", "bmp" },
    { "application/xhtml+xml", "xhtml" },
    { "application/x-javascript", "js" },
    { "application/json", "json" },
    { octetStream, "exe" },
    { octetStream, "com" },
    { octetStream, "bin" },
    { "application/zip", "zip" },
    { "application/gzip", "gz" },
    { "application/pdf", "pdf" },
    { "application/postscript", "ps" },
    { "image/x-icon", "ico" },
    { "image/tiff", "tiff" },
    { "image/x-xbitmap", "xbm" },
    { "image/svg+xml", "svg" },
    { "application/rss+xml", "rss" },
    { "application/rdf+xml", "rdf" },
	{ "application/x-shockwave-flash", "swf" },
	{ "application/x-futuresplash", "spl" },

	// Media types

    // Ogg
    { "application/ogg", "ogx" },
    { "audio/ogg", "ogg" },
    { "audio/ogg", "oga" },
    { "video/ogg", "ogv" },

    // Annodex
    { "application/annodex", "anx" },
    { "audio/annodex", "axa" },
    { "video/annodex", "axv" },
    { "audio/speex", "spx" },

    // WebM
    { "video/webm", "webm" },
    { "audio/webm", "webm" },

    // MPEG
    { "audio/mpeg", "m1a" },
    { "audio/mpeg", "m2a" },
    { "audio/mpeg", "m1s" },
    { "audio/mpeg", "mpa" },
    { "video/mpeg", "mpg" },
    { "video/mpeg", "m15" },
    { "video/mpeg", "m1s" },
    { "video/mpeg", "m1v" },
    { "video/mpeg", "m75" },
    { "video/mpeg", "mpa" },
    { "video/mpeg", "mpeg" },
    { "video/mpeg", "mpm" },
    { "video/mpeg", "mpv" },

    // MPEG playlist
    { "application/vnd.apple.mpegurl", "m3u8" },
    { "application/mpegurl", "m3u8" },
    { "application/x-mpegurl", "m3u8" },
    { "audio/mpegurl", "m3url" },
    { "audio/x-mpegurl", "m3url" },
    { "audio/mpegurl", "m3u" },
    { "audio/x-mpegurl", "m3u" },

    // MPEG-4
	{ "video/x-mp4", "mp4" },
	{ "video/mp4",   "mp4" },
    { "video/x-m4v", "m4v" },
    { "audio/x-m4a", "m4a" },
    { "audio/x-m4b", "m4b" },
    { "audio/x-m4p", "m4p" },
    { "audio/mp4", "m4a" },

    // MP3
    { "audio/mp3", "mp3" },
    { "audio/x-mp3", "mp3" },
    { "audio/x-mpeg", "mp3" },

    // MPEG-2
    { "video/x-mpeg2", "mp2" },
    { "video/mpeg2", "vob" },
    { "video/mpeg2", "mod" },
    { "video/m2ts", "m2ts" },
    { "video/x-m2ts", "m2t" },
    { "video/x-m2ts", "ts" },

    // 3GP/3GP2
    { "audio/3gpp", "3gpp" },
    { "audio/3gpp2", "3g2" },
    { "application/x-mpeg", "amc" },

    // AAC
    { "audio/aac", "aac" },
    { "audio/aac", "adts" },
    { "audio/x-aac", "m4r" },

    // CoreAudio File
    { "audio/x-caf", "caf" },
    { "audio/x-gsm", "gsm" },

    // ADPCM
	{ "audio/x-wav", "wav" },

    { 0, 0 }
};

String MIMETypeRegistry::getMIMETypeForExtension(const String &ext)
{
    String s = ext.lower();
    const ExtensionMap *e = extensionMap;
    while (e->extension) {
        if (s == e->extension)
            return e->mimeType;
        ++e;
    }

    if(ext != "")
    {
		APTR n;
		ITERATELIST(n, &mimetype_list)
		{
		    struct mimetypenode *mn = (struct mimetypenode *) n;
		    String extensions = mn->extensions;
		    Vector<String> listExtensions;
		    extensions.split(" ", true, listExtensions);

		    for(size_t i = 0; i < listExtensions.size(); i++)
		    {
				if(listExtensions[i] == s)
				{
				    return String(mn->mimetype);
				}
		    }
		}
    }

    return String();
}

Vector<String> MIMETypeRegistry::getExtensionsForMIMEType(const String& type)
{
    Vector<String> extensions;
    String s = type.lower();
    const ExtensionMap *e = extensionMap;
    while (e->extension) {
	if (s == e->mimeType)
	    extensions.append(e->extension);
        ++e;
    }

    APTR n;
    ITERATELIST(n, &mimetype_list)
    {
		struct mimetypenode *mn = (struct mimetypenode *) n;
		
		if(s == mn->mimetype)
		{
		    Vector<String> newExtensions;
		    String strExtensions = mn->extensions;
		    strExtensions.split(" ", false, newExtensions);
		    
		    if(newExtensions.size())
				extensions = newExtensions;
		    
		    break;
		}
    }
    
    return extensions;
}

bool MIMETypeRegistry::isApplicationPluginMIMEType(const String&)
{
    return false;
}


#if ENABLE(MEDIA_SOURCE)
bool MIMETypeRegistry::isSupportedMediaSourceMIMEType(const String&, const String&)
{
    return false;
}
#endif

}
