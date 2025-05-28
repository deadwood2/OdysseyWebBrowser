#import <ob/OBString.h>

@class OBArray, OBURL;

@interface WkUserScript : OBObject

typedef enum
{
	WkUserScript_InjectPosition_AtDocumentStart,
	WkUserScript_InjectPosition_AtDocumentEnd,
} WkUserScript_InjectPosition;

typedef enum
{
	WkUserScript_InjectInFrames_All,
	WkUserScript_InjectInFrames_Top,
} WkUserScript_InjectInFrames;

+ (WkUserScript *)userScriptWithContents:(OBString *)script injectPosition:(WkUserScript_InjectPosition)position injectInFrames:(WkUserScript_InjectInFrames)inFrames whiteList:(OBArray *)white blackList:(OBArray *)blacklist;

+ (WkUserScript *)userScriptWithContentsOfFile:(OBString *)path injectPosition:(WkUserScript_InjectPosition)position injectInFrames:(WkUserScript_InjectInFrames)inFrames whiteList:(OBArray *)white blackList:(OBArray *)blacklist;

- (WkUserScript_InjectPosition)injectPosition;
- (WkUserScript_InjectInFrames)injectInFrames;

- (OBString *)path;
- (OBString *)script;

- (OBArray * /* OBString */)whiteList;
- (OBArray * /* OBString */)blackList;

@end

@interface WkUserScripts : OBObject

+ (void)addUserScript:(WkUserScript *)script;
+ (void)removeUserScript:(WkUserScript *)script;
+ (OBArray *)userScripts;

@end
