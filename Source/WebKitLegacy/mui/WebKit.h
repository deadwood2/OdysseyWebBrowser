/*
* Copyright (C) 2009 Pleyo.  All rights reserved.                               
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
* THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS ASIS AND ANY       
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

#ifndef WebKit_h
#define WebKit_h

#include "WebKitDefines.h"
#include "WebKitTypes.h"

#include "DOMCoreClasses.h"
#include "DOMHTMLClasses.h"
#include "DOMRange.h"
#include "JSActionDelegate.h"
#include "SharedObject.h"
#include "SharedPtr.h"
#include "TransferSharedPtr.h"
#include "WebBackForwardList.h"
#include "WebDownloadDelegate.h"
#include "WebDownload.h"
#include "WebDragData.h"
#include "WebEditingDelegate.h"
#include "WebError.h"
#include "WebFrame.h"
#include "WebFrameLoadDelegate.h"
#include "WebFramePolicyListener.h"
#include "WebHistoryDelegate.h"
#include "WebHistoryItem.h"
#include "WebHitTestResults.h"
#include "WebInspector.h"
#include "WebMutableURLRequest.h"
#include "WebNavigationAction.h"
#include "WebNavigationData.h"
#include "WebNotificationDelegate.h"
#include "WebObject.h"
#include "WebPolicyDelegate.h"
#include "WebPreferences.h"
#include "WebResourceLoadDelegate.h"
#include "WebScriptObject.h"
#include "WebScriptWorld.h"
#include "WebSecurityOrigin.h"
#include "WebURLAuthenticationChallenge.h"
#include "WebURLAuthenticationChallengeSender.h"
#include "WebURLCredential.h"
#include "WebURLResponse.h"
#include "WebValue.h"
#include "WebView.h"
#include "WebWorkersPrivate.h"

#include "WebDatabaseManager.h"

#if ENABLE(ICONDATABASE)
#include "WebIconDatabase.h"
#endif

#include "WebWindow.h"
#include "WebWindowAlert.h"
#include "WebWindowConfirm.h"
#include "WebWindowPrompt.h"

#endif//WebKit_h
