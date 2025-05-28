#include "WebNotificationClient.h"

#if ENABLE(NOTIFICATIONS)

#include <WebCore/NotificationPermissionCallback.h>
#include <WebCore/ScriptExecutionContext.h>
#include "WebPage.h"

#define D(x) 

using namespace WebCore;

namespace  WebKit {

WebNotificationClient::WebNotificationClient(WebPage *webView)
    : m_webPage(webView)
{
	D(dprintf("%s(%p)\n", __PRETTY_FUNCTION__, this));
}

bool WebNotificationClient::show(Notification* notification)
{
	if (!m_webPage->_fShowNotification)
		return false;

	D(dprintf("%s(%p): %p\n", __PRETTY_FUNCTION__, this, notification));
	auto it = m_notificationContextMap.find(notification->scriptExecutionContext());

	if (it == m_notificationContextMap.end())
	{
		it = m_notificationContextMap.add(notification->scriptExecutionContext(), Vector<RefPtr<WebCore::Notification>>()).iterator;
	}

	if (it != m_notificationContextMap.end())
	{
		it->value.append(notification);
	}
	
	m_webPage->_fShowNotification(notification);

    return true;
}

void WebNotificationClient::cancel(Notification* notification)
{
	D(dprintf("%s(%p): %p\n", __PRETTY_FUNCTION__, this, notification));
	if (m_webPage->_fHideNotification)
		m_webPage->_fHideNotification(notification);
}

void WebNotificationClient::clearNotifications(ScriptExecutionContext* context)
{
	D(dprintf("%s(%p): %p\n", __PRETTY_FUNCTION__, this, context));
	auto it = m_notificationContextMap.find(context);
	if (it != m_notificationContextMap.end())
	{
		for (auto nit = it->value.begin(); nit != it->value.end(); nit++)
		{
			WebCore::Notification *notification = nit->get();
			notification->finalize();
			if (m_webPage->_fHideNotification)
				m_webPage->_fHideNotification(notification);
		}

		m_notificationContextMap.remove(it);
	}
}

void WebNotificationClient::notificationObjectDestroyed(Notification* notification)
{
	if (m_webPage->_fHideNotification)
		m_webPage->_fHideNotification(notification);
	
	auto it = m_notificationContextMap.find(notification->scriptExecutionContext());
	if (it != m_notificationContextMap.end())
	{
		size_t index = it->value.find(notification);
		if (index != notFound)
			it->value.remove(index);
	}
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
	// TODO: call WkWebView via page
	D(dprintf("%s(%p): %p\n", __PRETTY_FUNCTION__, this, &context));
	if (m_webPage->_fRequestNotificationPermission)
	{
		m_webPage->_fRequestNotificationPermission(context.url(), WTFMove(callback));
	}
	else
	{
		callback(Notification::Permission::Denied);
	}
}

NotificationClient::Permission WebNotificationClient::checkPermission(ScriptExecutionContext* context)
{
    if (!context || !context->isDocument() || !m_webPage->_fCheckNotificationPermission)
        return NotificationClient::Permission::Denied;

	auto permission = m_webPage->_fCheckNotificationPermission(context->url());

	if (permission == WebViewDelegate::NotificationPermission::Default)
		return NotificationClient::Permission::Default;

	if (permission == WebViewDelegate::NotificationPermission::Grant)
            return NotificationClient::Permission::Granted;

	return NotificationClient::Permission::Denied;
}

}

#endif
