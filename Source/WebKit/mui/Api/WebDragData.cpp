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
#include "WebDragData.h"
#include "WebDragData_p.h"

using namespace std;

WebDragData* WebDragData::create()
{
    return new WebDragData();
}

WebDragData* WebDragData::create(const WebDragData& data)
{
    return new WebDragData(data);
}

WebDragData::WebDragData()
    : m_webDataPrivate(0)
{
}

WebDragData::WebDragData(const WebDragData& d)
    : m_webDataPrivate(0) 
{ 
    assign(d); 
}

WebDragData::~WebDragData() 
{ 
    reset(); 
}

void WebDragData::reset()
{
    m_url = "";
    m_urlTitle = "";
    m_downloadURL = "";
    m_fileExtension = "";
    m_filenames.clear();
    m_plainText = "";
    m_textHtml = "";
    m_htmlBaseUrl = "";
    m_fileContentFilename = "";
    m_fileContent = "";
    if(m_webDataPrivate)
        delete m_webDataPrivate;
}

void WebDragData::assign(const WebDragData& data) {
    m_url = data.url();
    m_urlTitle = data.urlTitle();
    m_downloadURL = data.downloadURL();
    m_fileExtension = data.fileExtension();
    m_filenames = data.fileNames();
    m_plainText = data.plainText();
    m_textHtml = data.htmlText();
    m_htmlBaseUrl = data.htmlBaseURL();
    m_fileContentFilename = data.fileContentFileName();
    m_fileContent = data.fileContent(); 
}

const char* WebDragData::url() const
{
    return m_url.c_str();
}

void WebDragData::setURL(const char* url)
{
    m_url = url;
}

const char* WebDragData::urlTitle() const
{
    return m_urlTitle.c_str();
}

void WebDragData::setURLTitle(const char* urlTitle)
{
    m_urlTitle = urlTitle;
}

const char* WebDragData::downloadURL() const
{
    return m_downloadURL.c_str();
}

void WebDragData::setDownloadURL(const char* downloadURL)
{
    m_downloadURL = downloadURL;
}

const char* WebDragData::fileExtension() const
{
    return m_fileExtension.c_str();
}

void WebDragData::setFileExtension(const char* fileExtension)
{
    m_fileExtension = fileExtension;
}

bool WebDragData::hasFileNames() const
{
    return !m_filenames.empty();
}

vector<const char*> WebDragData::fileNames() const
{
    return m_filenames;
}

void WebDragData::setFileNames(const vector<const char*> fileNames)
{
    m_filenames.clear();
    m_filenames = fileNames;
}

void WebDragData::appendToFileNames(const char* fileName)
{
    m_filenames.push_back(fileName);
}

const char* WebDragData::plainText() const
{
    return m_plainText.c_str();
}

void WebDragData::setPlainText(const char* plainText)
{
    m_plainText = plainText;
}

const char* WebDragData::htmlText() const
{
    return m_textHtml.c_str();
}

void WebDragData::setHTMLText(const char* htmlText)
{
    m_textHtml = htmlText;
}

const char* WebDragData::htmlBaseURL() const
{
    return m_htmlBaseUrl.c_str();
}

void WebDragData::setHTMLBaseURL(const char* htmlBaseURL)
{
    m_htmlBaseUrl = htmlBaseURL;
}

const char* WebDragData::fileContentFileName() const
{
    return m_fileContentFilename.c_str();
}

void WebDragData::setFileContentFileName(const char* fileName)
{
    m_fileContentFilename = fileName;
}

const char* WebDragData::fileContent() const
{
    return m_fileContent.c_str();
}

void WebDragData::setFileContent(const char* fileContent)
{
    m_fileContent = fileContent;
}
