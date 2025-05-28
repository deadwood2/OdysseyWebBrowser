#import "WkNotification.h"

#undef __OBJC__
#define __MORPHOS_DISABLE
#import "WebKit.h"
#import <wtf/CompletionHandler.h>
#import <WebCore/Notification.h>
#import <WebCore/NotificationClient.h>
#import <WebCore/NotificationPermissionCallback.h>
#define __OBJC__

@interface WkNotificationPrivate : WkNotification
{
	RefPtr<WebCore::Notification> _notification;
}

- (id)initWithNotification:(RefPtr<WebCore::Notification>)notification;
+ (id)notificationForNotification:(WebCore::Notification *)notification;

- (void)cancel;

@end

@interface WkNotificationPermissionResponsePrivate : WkNotificationPermissionResponse
{
	WebCore::NotificationClient::PermissionHandler _callback;
}

- (id)initWithCallback:(WebCore::NotificationClient::PermissionHandler&&)callback;

@end
