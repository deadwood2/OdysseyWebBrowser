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

#include <WebCore/Document.h>
#include "WebDesktopNotificationsDelegate.h"
#include "WebSecurityOrigin.h"
#include "WebView.h"
#include <WebCore/NotificationPermissionCallback.h>
#include <wtf/text/CString.h>

#include <clib/debug_protos.h>
#define D(x)

#if ENABLE(NOTIFICATIONS) || ENABLE(LEAGCY_NOTIFICATIONS)

using namespace WebCore;

WebDesktopNotificationsDelegate::WebDesktopNotificationsDelegate(WebView* webView)
    : m_webView(webView)
{
}

// OWB has no notification UI yet: notifications are logged and reported
// as shown, as in OWB 2.34.
bool WebDesktopNotificationsDelegate::show(ScriptExecutionContext& context, NotificationData&& data, RefPtr<NotificationResources>&&, CompletionHandler<void()>&& callback)
{
    if (context.isWorkerGlobalScope()) {
        callback();
        return false;
    }

    kprintf("show [%s]\n%s\n", data.title.latin1().data(), data.body.latin1().data());
    callback();
    return true;
}

void WebDesktopNotificationsDelegate::cancel(NotificationData&&)
{
    D(kprintf("cancel\n"));
}

void WebDesktopNotificationsDelegate::notificationObjectDestroyed(NotificationData&&)
{
    D(kprintf("objectdestroyed\n"));
}

void WebDesktopNotificationsDelegate::notificationControllerDestroyed()
{
    D(kprintf("controllerdestroyed\n"));
}

void WebDesktopNotificationsDelegate::requestPermission(ScriptExecutionContext& context, PermissionHandler&& permissionHandler)
{
    D(kprintf("requestPermission\n"));
    // Always answer: an unanswered request leaves the page's promise pending.
    permissionHandler(checkPermission(&context));
}

NotificationClient::Permission WebDesktopNotificationsDelegate::checkPermission(ScriptExecutionContext*)
{
    D(kprintf("checkPermission\n"));
    return NotificationClient::Permission::Granted;
}

#endif  // ENABLE(NOTIFICATIONS)
