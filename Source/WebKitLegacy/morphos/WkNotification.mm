#import "WkNotification_private.h"
#import <ob/OBURL.h>
#undef __OBJC__
#import <WebCore/UserGestureIndicator.h>
#define __OBJC__

static WTF::HashMap<WTF::UUID, id> _notificationLookup;

@implementation WkNotificationPrivate

- (id)initWithNotification:(WebCore::NotificationData&&)notification
{
	if ((self = [super init]))
	{
		_notification = WTFMove(notification);
		_notificationLookup.add(_notification->notificationID, self);
	}
	
	return self;
}

- (void)dealloc
{
	[self cancel];
	[super dealloc];
}

- (void)cancel
{
	auto it = _notificationLookup.find(_notification->notificationID);
	if (it != _notificationLookup.end())
    	_notificationLookup.remove(_notification->notificationID);
}

+ (id)notificationForNotification:(WebCore::NotificationData&&)notification
{
	auto it = _notificationLookup.find(notification.notificationID);
	if (it != _notificationLookup.end())
		return it->value;
	return nil;
}

- (OBString *)title
{
    auto utitle = _notification->title.utf8();
    return [OBString stringWithUTF8String:utitle.data()];
}

- (OBString *)body
{
    auto ubody = _notification->body.utf8();
    return [OBString stringWithUTF8String:ubody.data()];
}

- (OBString *)language
{
    auto ulanguage = _notification->language.utf8();
    return [OBString stringWithUTF8String:ulanguage.data()];
}

- (OBString *)tag
{
    auto utag = _notification->tag.utf8();
    return [OBString stringWithUTF8String:utag.data()];
}

- (OBURL *)icon
{
    auto uurl = _notification->iconURL.utf8();
    return [OBURL URLWithString:[OBString stringWithUTF8String:uurl.data()]];
}

- (BOOL)isCancelled
{
	auto it = _notificationLookup.find(_notification->notificationID);
	if (it == _notificationLookup.end())
        return YES;
    return NO;
}

// Report state to the website
- (void)notificationShown
{
	if (![self isCancelled])
	{
        WebCore::Notification::ensureOnNotificationThread(*_notification, [](auto* notification) {
            if (notification)
                notification->dispatchShowEvent();
        });
    }
}

- (void)notificationClosed
{
	if (![self isCancelled])
	{
        WebCore::Notification::ensureOnNotificationThread(*_notification, [](auto* notification) {
            if (notification)
                notification->dispatchCloseEvent();
        });
    }
}

- (void)notificationClicked
{
	if (![self isCancelled])
	{
        WebCore::Notification::ensureOnNotificationThread(*_notification, [](auto* notification) {
            if (notification)
                notification->dispatchClickEvent();
        });
    }
}

- (void)notificationNotShownDueToError
{
	if (![self isCancelled])
	{
        WebCore::Notification::ensureOnNotificationThread(*_notification, [](auto* notification) {
            if (notification)
                notification->dispatchErrorEvent();
        });
    }
}

@end

@implementation WkNotification

- (OBString *)title
{
	return nil;
}

- (OBString *)body
{
	return nil;
}

- (OBString *)language
{
	return nil;
}

- (OBString *)tag
{
	return nil;
}

- (OBURL *)icon
{
	return nil;
}

// Report state to the website
- (void)notificationShown
{
}

- (void)notificationClosed
{
}

- (void)notificationClicked
{
}

- (void)notificationNotShownDueToError
{
}

- (BOOL)isCancelled
{
	return YES;
}

@end

@implementation WkNotificationPermissionResponsePrivate

- (id)initWithCallback:(WebCore::NotificationClient::PermissionHandler&&)callback
{
	if ((self = [super init]))
	{
		_callback = std::move(callback);
	}
	
	return self;
}

- (void)respondToNotificationPermission:(WkNotificationPermission)permission
{
	switch (permission)
	{
	case WkNotificationPermission_Default:
		_callback(WebCore::Notification::Permission::Default);
		break;
	case WkNotificationPermission_Deny:
		_callback(WebCore::Notification::Permission::Denied);
		break;
	case WkNotificationPermission_Grant:
		_callback(WebCore::Notification::Permission::Granted);
		break;
	}
}

@end

@implementation WkNotificationPermissionResponse
- (void)respondToNotificationPermission:(WkNotificationPermission)permission
{
	(void)permission;
}
@end
