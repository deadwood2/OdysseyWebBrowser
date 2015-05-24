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
#include "DefaultPolicyDelegate.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "WebError.h"
#include "WebFrame.h"
#include "WebFramePolicyListener.h"
#include "WebMutableURLRequest.h"
#include "WebNavigationAction.h"
#include "WebView.h"
#include "WebUtils.h"

#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include <sys/types.h> 
#include <sys/stat.h> 

#ifdef _MSC_VER 
 #ifndef S_ISDIR 
 #define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR) 
 #endif 
 #else 
 #include <unistd.h> 
#endif

#include "gui.h"
#include "utils.h"
#include <proto/dos.h>
#include <proto/intuition.h>
#include <clib/debug_protos.h>
#undef get
#undef String
#undef Read

using namespace WebCore;

// FIXME: move this enum to a separate header file when other code begins to use it.
typedef enum WebExtraNavigationType {
    WebNavigationTypePlugInRequest = WebNavigationTypeOther + 1
} WebExtraNavigationType;

DefaultPolicyDelegate::DefaultPolicyDelegate()
{
}

DefaultPolicyDelegate::~DefaultPolicyDelegate()
{
}

TransferSharedPtr<DefaultPolicyDelegate> DefaultPolicyDelegate::sharedInstance()
{
    static SharedPtr<DefaultPolicyDelegate> shared = DefaultPolicyDelegate::createInstance();
    return shared;
}

TransferSharedPtr<DefaultPolicyDelegate> DefaultPolicyDelegate::createInstance()
{
    DefaultPolicyDelegate* instance = new DefaultPolicyDelegate();
    return instance;
}


void DefaultPolicyDelegate::decidePolicyForNavigationAction(WebView* webView, 
    /*[in]*/ WebNavigationAction* actionInformation, 
    /*[in]*/ WebMutableURLRequest* request, 
	/*[in]*/ WebFrame* frame,
    /*[in]*/ WebFramePolicyListener* listener)
{
    int navType = actionInformation->type();

    bool canHandleRequest = webView->canHandleRequest(request);

	navType = actionInformation->type();

	// XXX: move that to webframeloaderlient event directly to use form
	if (navType == WebNavigationTypeFormSubmitted)
	{
		DoMethod(app, MM_OWBApp_SaveFormState, webView, frame);
	}

	if (navType == WebNavigationTypeFormResubmitted)
	{
		// XXX: move that to an app method
		ULONG ret = MUI_RequestA(app, NULL, 0, GSI(MSG_REQUESTER_NORMAL_TITLE), GSI(MSG_REQUESTER_YES_NO), GSI(MSG_DEFAULTPOLICYDELEGATE_MESSAGE), NULL);

		switch(ret)
		{
			case 1:
				listener->use();
				break;

			default:
			case 0:
				listener->ignore();
				break;
		}
	}
	else
	{
		if (canHandleRequest)
	        listener->use();
		else if (navType == WebNavigationTypePlugInRequest)
	        listener->use();
		else
		{
			char *url = (char *) request->_URL();
			if(url)
			{
		        // A file URL shouldn't fall through to here, but if it did,
		        // it would be a security risk to open it.
				if (!String(url).startsWith("file:"))
				{
		            // FIXME: Open the URL not by means of a webframe, but by handing it over to the system and letting it determine how to open that particular URL scheme.  See documentation for [NSWorkspace openURL]
		            ;
		        }
		        listener->ignore();
				free(url);
			}
	    }
	}
}

void DefaultPolicyDelegate::decidePolicyForNewWindowAction(
    /*[in]*/ WebView* /*webView*/, 
    /*[in]*/ WebNavigationAction* /*actionInformation*/, 
    /*[in]*/ WebMutableURLRequest* /*request*/, 
    /*[in]*/ const char* /*frameName*/, 
    /*[in]*/ WebFramePolicyListener* listener)
{
    listener->use();
}

bool DefaultPolicyDelegate::decidePolicyForMIMEType(
    /*[in]*/ WebView* webView, 
    /*[in]*/ const ResourceResponse& response, 
    /*[in]*/ WebMutableURLRequest* request, 
    /*[in]*/ WebFrame* frame,
    /*[in]*/ WebFramePolicyListener* listener)
{
    return DoMethod(app, MM_OWBApp_RequestPolicyForMimeType, (APTR) &response, (APTR) request, (APTR) webView, (APTR) frame, (APTR) listener);
}

void DefaultPolicyDelegate::unableToImplementPolicyWithError(
    /*[in]*/ WebView* /*webView*/, 
    /*[in]*/ WebError* error, 
    /*[in]*/ WebFrame* frame)
{
	char *frameNameStr = (char *) frame->name();
    String errorStr;
    errorStr = error->localizedDescription();

    String frameName;
	frameName = frameNameStr;

	free(frameNameStr);

    //LOG_ERROR("called unableToImplementPolicyWithError:%S inFrame:%S", errorStr ? errorStr : TEXT(""), frameName ? frameName : TEXT(""));
}
