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

#ifndef WebDragData_h
#define WebDragData_h

#include "WebKitTypes.h"
#include <locale.h>
#include <string>
#include <vector>
#include <limits.h>


class WebDragDataPrivate;

enum WebDragOperation {
    WebDragOperationNone    = 0,
    WebDragOperationCopy    = 1,
    WebDragOperationLink    = 2,
    WebDragOperationGeneric = 4,
    WebDragOperationPrivate = 8,
    WebDragOperationMove    = 16,
    WebDragOperationDelete  = 32,
    WebDragOperationEvery   = UINT_MAX
};

class WEBKIT_OWB_API WebDragData {
public:
    static WebDragData* create();
    static WebDragData* create(const WebDragData&);

    ~WebDragData();

    WebDragData& operator=(const WebDragData& d)
    {
        assign(d);
        return *this;
    }

    void assign(const WebDragData&);

    void reset();

    bool isNull() const { return !m_webDataPrivate; }

    const char* url() const;
    void setURL(const char*);

    const char* urlTitle() const;
    void setURLTitle(const char*);

    const char* downloadURL() const;
    void setDownloadURL(const char*);

    const char* fileExtension() const;
    void setFileExtension(const char*);

    bool hasFileNames() const;
    std::vector<const char*> fileNames() const;
    void setFileNames(const std::vector<const char*>);
    void appendToFileNames(const char*);

    const char* plainText() const;
    void setPlainText(const char*);

    const char* htmlText() const;
    void setHTMLText(const char*);

    const char* htmlBaseURL() const;
    void setHTMLBaseURL(const char*);

    const char* fileContentFileName() const;
    void setFileContentFileName(const char*);

    const char* fileContent() const;
    void setFileContent(const char*);

    WebDragDataPrivate* platformDragData();

private:
    WebDragData();
    WebDragData(const WebDragData& d);

    std::string m_url;
    std::string m_urlTitle;
    std::string m_downloadURL;
    std::string m_fileExtension;
    std::vector<const char*> m_filenames;
    std::string m_plainText;
    std::string m_textHtml;
    std::string m_htmlBaseUrl;
    std::string m_fileContentFilename;
    std::string m_fileContent;

    WebDragDataPrivate* m_webDataPrivate;

};

#endif
