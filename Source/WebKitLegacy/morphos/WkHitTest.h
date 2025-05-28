#import <ob/OBString.h>

@class OBURL;

@interface WkHitTest : OBObject

- (OBString *)selectedText;
- (OBString *)title;
- (OBString *)altText;
- (OBString *)textContent;

- (BOOL)isContentEditable;
- (OBString *)misspelledWord;
- (OBArray /* OBString */ *)guessesForMisspelledWord;
- (OBArray /* OBString */ *)availableDictionaries;
- (OBString *)enabledDictionary;
- (void)learnMissspelledWord;
- (void)ignoreMisspelledWord;
- (void)replaceMisspelledWord:(OBString *)correctWord;

- (OBURL *)linkURL;

// Image may have no URL, so the getter is here to make copy 2 clip, etc possible
- (BOOL)isImage;
- (OBURL *)imageURL;
- (OBString *)imageFileExtension;
- (OBString *)imageMimeType;
- (LONG)imageWidth;
- (LONG)imageHeight;

// Helpers
- (void)downloadLinkFile;
- (void)downloadImageFile;
- (BOOL)copyImageToClipboard;
- (BOOL)saveImageToFile:(OBString *)path;
- (void)replaceSelectedTextWidth:(OBString *)replacement;
- (void)cutSelectedText;
- (void)pasteText;
- (void)selectAll;

// Should only be used in text editors
typedef enum {
	WkHitTestImageFloat_None,
	WkHitTestImageFloat_Left,
	WkHitTestImageFloat_Right
} WkHitTestImageFloat;

- (void)setImageFloat:(WkHitTestImageFloat)imagefloat;
- (WkHitTestImageFloat)imageFloat;

@end
