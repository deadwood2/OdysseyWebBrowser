#include "WebNotificationClient.h"

#if ENABLE(NOTIFICATIONS)

#include <WebCore/NotificationData.h>
#include <WebCore/ScriptExecutionContext.h>
#include <WebCore/NotificationPermissionCallback.h>
#include <wtf/CompletionHandler.h>
#include "WebPage.h"

#define D(x) 

using namespace WebCore;

namespace  WebKit {

WebNotificationClient::WebNotificationClient(WebPage *webView)
    : m_webPage(webView)
{
	D(dprintf("%s(%p)\n", __PRETTY_FUNCTION__, this));
}

bool WebNotificationClient::show(WebCore::ScriptExecutionContext& context, WebCore::NotificationData&& notification, RefPtr<WebCore::NotificationResources>&&, WTF::CompletionHandler<void()>&& onCompleted)
{
    auto page = m_webPage.get();
    if (!page)
        return false;

	if (!page->_fShowNotification)
		return false;

	D(dprintf("%s(%p): %p\n", __PRETTY_FUNCTION__, this, notification));

	page->_fShowNotification(WTFMove(notification));

    onCompleted();
    return true;
}

void WebNotificationClient::cancel(WebCore::NotificationData&& notification)
{
	D(dprintf("%s(%p): %p\n", __PRETTY_FUNCTION__, this, notification));
    auto page = m_webPage.get();
    if (!page)
        return;

	if (page->_fHideNotification)
		page->_fHideNotification(WTFMove(notification));
}

void WebNotificationClient::notificationObjectDestroyed(WebCore::NotificationData&& notification)
{
    auto page = m_webPage.get();
    if (!page)
        return;

	if (page->_fHideNotification)
		page->_fHideNotification(WTFMove(notification));
}

void WebNotificationClient::notificationControllerDestroyed()
{
	D(dprintf("%s(%p):\n", __PRETTY_FUNCTION__, this));
	// Means our WkWebView gets destroyed too, so all of the client-side notifications should be
	// cleaned by the client
    delete this;
}

void WebNotificationClient::requestPermission(WebCore::ScriptExecutionContext&context, WebCore::NotificationClient::PermissionHandler&&callback)
{
    auto page = m_webPage.get();
    if (!page)
        return;

	D(dprintf("%s(%p): %p\n", __PRETTY_FUNCTION__, this, &context));
	if (page->_fRequestNotificationPermission)
	{
		page->_fRequestNotificationPermission(context.url(), WTFMove(callback));
	}
	else
	{
		callback(NotificationClient::Permission::Denied);
	}
}

NotificationClient::Permission WebNotificationClient::checkPermission(ScriptExecutionContext* context)
{
    auto page = m_webPage.get();
    if (!page)
        return NotificationClient::Permission::Denied;

    if (!context || !context->isDocument() || !page->_fCheckNotificationPermission)
        return NotificationClient::Permission::Denied;

	auto permission = page->_fCheckNotificationPermission(context->url());

	if (permission == WebViewDelegate::NotificationPermission::Default)
		return NotificationClient::Permission::Default;

	if (permission == WebViewDelegate::NotificationPermission::Grant)
            return NotificationClient::Permission::Granted;

	return NotificationClient::Permission::Denied;
}

}

#endif
