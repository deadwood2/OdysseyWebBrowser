#undef __OBJC__
#import "WebKit.h"
#import <WebCore/FileChooser.h>
#define __OBJC__
#import "WkFileDialog.h"

@interface WkFileDialogResponseHandlerPrivate : OBObject<WkFileDialogResponseHandler, WkFileDialogSettings>
{
	RefPtr<WebCore::FileChooser> _chooser;
}

- (id)initWithChooser:(WebCore::FileChooser&)chooser;

@end
