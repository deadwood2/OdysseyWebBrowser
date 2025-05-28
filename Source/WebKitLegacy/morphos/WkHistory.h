#import <ob/OBArray.h>

@class OBString, OBURL;

@interface WkBackForwardListItem : OBObject

// URL of the webpage
- (OBURL *)URL;
// Initial request that created this item
- (OBURL *)initialURL;
// Title of the webpage
- (OBString *)title;

@end

#if defined(__clang__)
	#if __has_feature(objc_arc)
		#define __wkListType <__covariant WkBackForwardListItem>
	#else
		#define __wkListType
	#endif
#else
	#define __wkListType
#endif

@interface WkBackForwardList : OBObject

- (WkBackForwardListItem *)backItem;
- (WkBackForwardListItem *)forwardItem;
- (WkBackForwardListItem *)currentItem;

- (OBArray __wkListType *)backList;
- (OBArray __wkListType *)forwardList;

@end
