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
#include "Icon.h"

#include <wtf/text/CString.h>
#include "GraphicsContext.h"
#include "Logging.h"
#include "MIMETypeRegistry.h"
#include <wtf/PassRefPtr.h>
#include "BitmapImage.h"

#include <cstdio>
#include <clib/debug_protos.h>
#include "utils.h"

namespace WebCore {

Icon::Icon()
    :  m_icon(0)
{
}

Icon::~Icon()
{
    m_icon = nullptr;
}


PassRefPtr<Icon> Icon::createIconForFiles(const Vector<String>& filenames)
{
    if (filenames.isEmpty())
        return 0;

    // TODO: handle currentdir if only filename is given

	String command = "GetDeficonPath PATH=\"" + filenames[0] + "\"";
    String iconPath;

    // TODO: handle different file types in selection (use family or default if different)

    // Ask Ambient which icon/mimetype should be used.

    if(rexx_send((char *) "AMBIENT", (char *) command.utf8().data()))
    {
		iconPath = String(rexx_result());
		RefPtr<Icon> icon = adoptRef(new Icon);
		icon->m_icon = Image::loadPlatformResource(iconPath.utf8().data());
		return icon.release();
    }

    return 0;
}


void Icon::paint(GraphicsContext& context, const FloatRect& rect)
{
    context.drawImage(m_icon.get(), ColorSpaceDeviceRGB, rect);
}

}
