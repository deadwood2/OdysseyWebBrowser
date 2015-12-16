/*
 * Copyright (C) 2010 Pleyo.  All rights reserved.
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
#include "DragData.h"
#include "DataObjectMorphOS.h"
#include "URL.h"
#include "SharedBuffer.h"
#include "WebDragData.h"
#include "WebDragData_p.h"

#include <wtf/text/CString.h>

using namespace std;
using namespace WebCore;

WebDragDataPrivate* WebDragData::platformDragData()
{
#if 0
    RefPtr<DataObject> obj = DataObject::create();
    obj->url = URL(ParsedURLString, m_url.c_str());
    obj->urlTitle = m_urlTitle.c_str();
    obj->downloadURL = URL(ParsedURLString, m_downloadURL.c_str());
    obj->fileExtension = m_fileExtension.c_str();
    for(size_t i = 0; i < m_filenames.size(); ++i) {
        obj->filenames.append(m_filenames[i]);
    }
    obj->plainText = m_plainText.c_str();
    obj->textHtml = m_textHtml.c_str();
    obj->htmlBaseUrl = URL(ParsedURLString, m_htmlBaseUrl.c_str());
    obj->fileContentFilename = m_fileContentFilename.c_str();
    obj->fileContent = SharedBuffer::create(m_fileContent.c_str(), m_fileContent.length()); 

    if(m_webDataPrivate)
        delete m_webDataPrivate;
    m_webDataPrivate = new WebDragDataPrivate();
    m_webDataPrivate->m_dragDataRef = obj.get();
    obj->ref();
    return m_webDataPrivate;
#else
    return NULL;
#endif
}
