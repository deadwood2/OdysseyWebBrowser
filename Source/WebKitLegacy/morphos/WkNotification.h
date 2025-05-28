#import <ob/OBString.h>

@class WkNotification, WkWebView, OBURL;

typedef enum {
	WkNotificationPermission_Default, // User did not respond / not previously asked
	WkNotificationPermission_Grant,   // User granted permission
	WkNotificationPermission_Deny     // User denied permission
} WkNotificationPermission;

@interface WkNotificationPermissionResponse : OBObject

- (void)respondToNotificationPermission:(WkNotificationPermission)permission;

@end

@protocol WkNotificationDelegate <OBObject>

- (void)webView:(WkWebView *)view wantsPermissionToDisplayNotificationsWithResponse:(WkNotificationPermissionResponse *)response forHost:(OBString *)host;
- (WkNotificationPermission)webViewWantsToCheckPermissionToDisplayNotifications:(WkWebView *)view;
- (void)webView:(WkWebView *)view wantsToDisplayNotification:(WkNotification *)notification;
- (void)webView:(WkWebView *)view cancelledNotification:(WkNotification *)notification;

@end

@interface WkNotification : OBObject

- (OBString *)title;
- (OBString *)body;
- (OBString *)language;
- (OBString *)tag;
- (OBURL *)icon;

// Report state to the website
- (void)notificationShown;
- (void)notificationClosed;
- (void)notificationClicked;
- (void)notificationNotShownDueToError;

- (BOOL)isCancelled;

@end
