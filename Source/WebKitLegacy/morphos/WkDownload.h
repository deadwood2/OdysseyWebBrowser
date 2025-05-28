#import <ob/OBURL.h>

@class WkMutableNetworkRequest, WkDownload, WkError;

@protocol WkDownloadDelegate <OBObject>

- (void)downloadDidBegin:(WkDownload *)download;
- (void)download:(WkDownload *)download didRedirect:(OBURL *)newURL;

// Size should be known at the time of this callback, provided the server gave us one
- (void)didReceiveResponse:(WkDownload *)download;

// Respond with a string or nil to cancel download
- (OBString *)decideFilenameForDownload:(WkDownload *)download withSuggestedName:(OBString *)suggestedName;

- (void)download:(WkDownload *)download didReceiveBytes:(size_t)bytes;

- (void)downloadDidFinish:(WkDownload *)download;
- (void)download:(WkDownload *)download didFailWithError:(WkError *)error;
- (void)downloadNeedsAuthenticationCredentials:(WkDownload *)download;

@optional
- (BOOL)downloadShouldMoveFileWhenFinished:(WkDownload *)download;
- (void)download:(WkDownload *)download completedMoveWithError:(WkError *)error;

@end

@interface WkDownload : OBObject

+ (WkDownload *)download:(OBURL *)url withDelegate:(id<WkDownloadDelegate>)delegate;
+ (WkDownload *)downloadRequest:(WkMutableNetworkRequest *)request withDelegate:(id<WkDownloadDelegate>)delegate;

- (void)start;
- (void)cancel;
- (void)cancelForResume;

- (BOOL)canResumeDownload;
- (void)resume;

- (OBURL *)url;
- (OBString *)filename;

- (void)setLogin:(OBString *)login password:(OBString *)password;

- (QUAD)size;
- (QUAD)downloadedSize;

- (BOOL)isPending;
- (BOOL)isFailed;
- (BOOL)isFinished;

// Either modifies the destination path or performs the move itself if called from downloadDidFinish: or later on
// Will obviously only work once if it's to perform an immediate rename/copy for a completed download
- (void)moveFinishedDownload:(OBString *)destinationPath;

@end
