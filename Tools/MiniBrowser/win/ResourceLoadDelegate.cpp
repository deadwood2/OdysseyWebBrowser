/*
* Copyright (C) 2014-2015 Apple Inc. All Rights Reserved.
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
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "stdafx.h"
#include "ResourceLoadDelegate.h"

#include "PageLoadTestClient.h"
#include "MiniBrowser.h"
#include <WebKit/WebKitCOMAPI.h>
#include <comip.h>
#include <commctrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <shlwapi.h>
#include <wininet.h>

HRESULT ResourceLoadDelegate::QueryInterface(_In_ REFIID riid, _COM_Outptr_ void** ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    *ppvObject = nullptr;
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebResourceLoadDelegate*>(this);
    else if (IsEqualIID(riid, IID_IWebResourceLoadDelegate))
        *ppvObject = static_cast<IWebResourceLoadDelegate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG ResourceLoadDelegate::AddRef()
{
    return ++m_refCount;
}

ULONG ResourceLoadDelegate::Release()
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

HRESULT ResourceLoadDelegate::identifierForInitialRequest(_In_opt_ IWebView*, _In_opt_ IWebURLRequest*, _In_opt_ IWebDataSource*, unsigned long identifier)
{
    if (!m_client)
        return E_FAIL;

    m_client->pageLoadTestClient().didInitiateResourceLoad(identifier);

    return S_OK;
}

HRESULT ResourceLoadDelegate::willSendRequest(_In_opt_ IWebView*, unsigned long, _In_opt_ IWebURLRequest*, _In_opt_ IWebURLResponse*, _In_opt_ IWebDataSource*, _COM_Outptr_opt_ IWebURLRequest** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    return E_NOTIMPL;
}

HRESULT ResourceLoadDelegate::didReceiveAuthenticationChallenge(_In_opt_ IWebView*, unsigned long, _In_opt_ IWebURLAuthenticationChallenge*, _In_opt_ IWebDataSource*)
{
    return E_NOTIMPL;
}

HRESULT ResourceLoadDelegate::didCancelAuthenticationChallenge(_In_opt_ IWebView*, unsigned long, _In_opt_ IWebURLAuthenticationChallenge*, _In_opt_ IWebDataSource*)
{
    return E_NOTIMPL;
}

HRESULT ResourceLoadDelegate::didReceiveResponse(_In_opt_ IWebView*, unsigned long, _In_opt_ IWebURLResponse*, _In_opt_ IWebDataSource*)
{
    return E_NOTIMPL;
}

HRESULT ResourceLoadDelegate::didReceiveContentLength(_In_opt_ IWebView*, unsigned long, UINT, _In_opt_ IWebDataSource*)
{
    return E_NOTIMPL;
}

HRESULT ResourceLoadDelegate::didFinishLoadingFromDataSource(_In_opt_ IWebView*, unsigned long identifier, _In_opt_ IWebDataSource*)
{
    if (!m_client)
        return E_FAIL;

    m_client->pageLoadTestClient().didEndResourceLoad(identifier);

    return S_OK;
}

HRESULT ResourceLoadDelegate::didFailLoadingWithError(_In_opt_ IWebView*, unsigned long identifier, _In_opt_ IWebError*, _In_opt_ IWebDataSource*)
{
    if (!m_client)
        return E_FAIL;

    m_client->pageLoadTestClient().didEndResourceLoad(identifier);

    return S_OK;
}

HRESULT ResourceLoadDelegate::plugInFailedWithError(_In_opt_ IWebView*, _In_opt_ IWebError*, _In_opt_ IWebDataSource*)
{
    return E_NOTIMPL;
}
