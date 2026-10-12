#import "WkWebView.h"

@class WkWebInspectorView;

@protocol WkWebInspectorViewDelegate <OBObject>

- (void)webInspectorViewDestroyed:(WkWebInspectorView *)view;
- (OBString *)webInspectorView:(WkWebInspectorView *)view wantsToSaveFile:(OBString *)file;
- (void)webInspectorView:(WkWebInspectorView *)view changedURL:(OBString *)url;

@end

@interface WkWebInspectorView : WkWebView
{
	id _inspectorPrivate;
}

- (WkWebView *)inspectedView;

- (void)setInspectorDelegate:(id<WkWebInspectorViewDelegate>)delegate;
- (id<WkWebInspectorViewDelegate>)inspectorDelegate;

- (void)close;

@end
