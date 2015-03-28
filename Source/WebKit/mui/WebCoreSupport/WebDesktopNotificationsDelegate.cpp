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
#include <Document.h>
#include "WebDesktopNotificationsDelegate.h"
#include "WebSecurityOrigin.h"
#include "WebView.h"
#include <wtf/text/CString.h>

#include <clib/debug_protos.h>
#define D(x)

#if ENABLE(NOTIFICATIONS) || ENABLE(LEAGCY_NOTIFICATIONS)

using namespace WebCore;

WebDesktopNotificationsDelegate::WebDesktopNotificationsDelegate(WebView* webView)
    : m_webView(webView)
{
}

bool WebDesktopNotificationsDelegate::show(Notification* object)
{
    if (object->scriptExecutionContext()->isWorkerGlobalScope())
	return false;

    object->setPendingActivity(object);
    
    kprintf("show [%s]\n%s\n", object->title().latin1().data(), object->body().latin1().data());

    return true;
}

void WebDesktopNotificationsDelegate::cancel(Notification* object)
{
  D(kprintf("cancel\n"));
}

void WebDesktopNotificationsDelegate::notificationObjectDestroyed(Notification*)
{
  D(kprintf("objectdestroyed\n"));
}

void WebDesktopNotificationsDelegate:: notificationControllerDestroyed()
{
  D(kprintf("controllerdestroyed\n"));
}

#if ENABLE(LEGACY_NOTIFICATIONS)
void WebDesktopNotificationsDelegate::requestPermission(WebCore::ScriptExecutionContext* context, PassRefPtr<VoidCallback> callback)
{
  D(kprintf("requestPermission (legacy)\n"));

	NotificationClient::Permission permission = checkPermission(context);
	if (permission != NotificationClient::PermissionNotAllowed) {
		if (callback)
			callback->handleEvent();
		return;
	}
}
#endif

void WebDesktopNotificationsDelegate::requestPermission(WebCore::ScriptExecutionContext* context, PassRefPtr<NotificationPermissionCallback> callback)
{
  D(kprintf("requestPermission\n"));

    NotificationClient::Permission permission = checkPermission(context);
	if (permission != NotificationClient::PermissionNotAllowed) {
		if (callback)
			callback->handleEvent(Notification::permissionString(permission));
		return;
	}
}

void WebDesktopNotificationsDelegate::cancelRequestsForPermission(ScriptExecutionContext*)
{
  D(kprintf("cancelRequestsForPermission\n"));
}

NotificationClient::Permission WebDesktopNotificationsDelegate::checkPermission(WebCore::ScriptExecutionContext*)
{
  D(kprintf("checkPermission\n"));
    /*
    int out = 0;
    if (hasNotificationDelegate())
      notificationDelegate()->checkNotificationPermission(SecurityOrigin::create(url)->toString(), &out);*/
	//return NotificationClient::PermissionNotAllowed; // (NotificationClient::Permission) out;
	return NotificationClient::PermissionAllowed;
}

bool WebDesktopNotificationsDelegate::hasPendingPermissionRequests(WebCore::ScriptExecutionContext*) const
{
    D(kprintf("hasPendingPermissionRequests\n"));
    return false;
}
#endif  // ENABLE(NOTIFICATIONS)
