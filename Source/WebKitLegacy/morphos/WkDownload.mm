#import "WkDownload_private.h"
#import "WkNetworkRequestMutable.h"
#import "WkError_private.h"
#import <ob/OBThread.h>
#import <ob/OBArray.h>
#import <ob/OBRunLoop.h>
#import <proto/obframework.h>

#undef __OBJC__
#include "WebKit.h"
#include <wtf/WallTime.h>
#include <wtf/text/WTFString.h>
#include <WebCore/CurlDownload.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/TextEncoding.h>
#include <wtf/FileSystem.h>
#include <WebProcess.h>
#define __OBJC__

#import <proto/dos.h>
extern "C" { void dprintf(const char *, ...); }

#define D(x) 

@class _WkDownload;

class WebDownload final : public WebCore::CurlDownloadListener
{
public:
	void initialize(_WkDownload *outer, OBURL *url);
	void initialize(_WkDownload *outer, WebCore::ResourceHandle* handle, const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& response);
	bool start();
	bool cancel();

	bool cancelForResume();
	bool resume();
	
	void deleteTmpFile();

    void didReceiveResponse(const WebCore::ResourceResponse& response) override;
    void didReceiveDataOfLength(int size) override;
    void didFinish() override;
    void didFail(const WebCore::ResourceError &) override;

	QUAD size() const { return m_size; }
	QUAD downloadedSize() const { return m_receivedSize; }

	void setUserPassword(const String& user, const String &password);

private:
	_WkDownload                  *m_outerObject; // weak
    RefPtr<WebCore::CurlDownload> m_download;
    QUAD                      m_size { 0 };
    QUAD                      m_receivedSize { 0 };
    WTF::String                   m_user, m_password;
};

@interface _WkDownload : WkDownload
{
	WkMutableNetworkRequest *_request;
	WebDownload              _download;
	OBURL                   *_url;
	OBString                *_filename;
	OBString                *_tmpPath;
	id<WkDownloadDelegate>   _delegate;
	bool                     _selfretained;
	bool                     _isPending;
	bool                     _isFailed;
	bool                     _isFinished;
}

- (id<WkDownloadDelegate>)delegate;
- (void)setFilename:(OBString *)f;
- (void)selfrelease;
- (void)setPending:(BOOL)pending;
- (void)setFailed:(BOOL)failed;
- (void)setFinished:(BOOL)fini;
- (void)cancelDueToAuthentication;
- (void)updateURL:(OBURL *)url;
- (void)handleFinishedWithTmpPath:(OBString *)path;

@end

void WebDownload::initialize(_WkDownload *outer, OBURL *url)
{
	m_outerObject = outer;
	m_download = adoptRef(new WebCore::CurlDownload());
	if (m_download)
	{
		WTF::URL wurl(WTF::URL(), [[url absoluteString] cString]);
		D(dprintf("initialize %s context %p\n", [[url absoluteString] cString], WebKit::WebProcess::singleton().networkingContext()));
		m_download->init(*this, wurl, WebKit::WebProcess::singleton().networkingContext());
	}
}

void WebDownload::initialize(_WkDownload *outer, WebCore::ResourceHandle*handle, const WebCore::ResourceRequest&request, const WebCore::ResourceResponse& response)
{
	m_outerObject = outer;
	m_download = adoptRef(new WebCore::CurlDownload());
	if (m_download)
	{
		auto uurl = response.url().string().utf8();
		auto umime = response.mimeType().utf8();
		auto uname = response.suggestedFilename().utf8();

		if (0 == uname.length())
			uname = WebCore::decodeURLEscapeSequences(response.url().lastPathComponent()).utf8();
		
		D(dprintf("initialize w/response %s\n", uurl.data()));
		
		[m_outerObject setFilename:[OBString stringWithUTF8String:uname.data()]];
		m_size = response.expectedContentLength();
  
        if (-1ll == m_size)
            m_size = 0;

		m_download->init(*this, handle, request, response);
	}
}

bool WebDownload::start()
{
	if (!m_download)
		return false;

	m_download->setUserPassword(m_user, m_password);
	m_download->start();
	
	D(dprintf("%s\n", __PRETTY_FUNCTION__));
	
	[[m_outerObject delegate] downloadDidBegin:m_outerObject];
	
	return true;
}

bool WebDownload::cancel()
{
	D(dprintf("%s: dl %p\n", __PRETTY_FUNCTION__, m_download));
	if (!m_download)
		return true;
	m_download->setDeleteTmpFile(true);
	if (!m_download->cancel())
		return false;
	m_download->setListener(nullptr);
	m_download = nullptr;
	return true;
}

#pragma GCC push_options
#pragma GCC optimize ("O0") // or crash when resuming :|

bool WebDownload::cancelForResume()
{
	D(dprintf("%s: dl %p\n", __PRETTY_FUNCTION__, m_download));
	if (!m_download)
		return true;
	m_download->setDeleteTmpFile(false);
	if (!m_download->cancel())
		return false;
	D(dprintf("%s: dl %p done\n", __PRETTY_FUNCTION__, m_download));
	return true;
}

bool WebDownload::resume()
{
	if (m_download && m_download->isCancelled())
	{
		m_download->resume();
		m_receivedSize = m_download->resumeOffset();
		return true;
	}

	return false;
}

#pragma GCC pop_options

void WebDownload::didReceiveResponse(const WebCore::ResourceResponse& response)
{
	[m_outerObject retain];
	if (m_download)
	{
		if (response.httpStatusCode() < 400)
		{
			// (add received size to handle resuming)
			// try to keep m_size if already set! (see the case in which we dl from a pending response)
			// the response here is often bogus in that case :|
			if (0 == m_size || m_receivedSize)
				m_size = m_download->resumeOffset() + response.expectedContentLength();
			[[m_outerObject delegate] didReceiveResponse:m_outerObject];

			// redirection
			auto uurl = response.url().string().utf8();
			[m_outerObject updateURL:[OBURL URLWithString:[OBString stringWithUTF8String:uurl.data()]]];

			OBString *path = [m_outerObject filename];

			D(dprintf("%s: outer filename %s\n", __PRETTY_FUNCTION__, [path cString]));

			if (0 == [path length])
			{
				String suggestedFilename = response.suggestedFilename();
				if (suggestedFilename.isEmpty())
					suggestedFilename = response.url().lastPathComponent().toString();
				suggestedFilename = WebCore::decodeURLEscapeSequences(suggestedFilename);
				
				suggestedFilename.replace('\n', ' ');
				suggestedFilename.replace('\r', ' ');
				suggestedFilename.replace('\t', ' ');
				suggestedFilename.replace(':', String());
				suggestedFilename.replace('\'', '_');
				suggestedFilename.replace('/', '_');
				suggestedFilename.replace('\\', '_');
				suggestedFilename.replace('*', '_');
				suggestedFilename.replace('?', '_');
				suggestedFilename.replace('~', '_');
				suggestedFilename.replace('[', '_');
				suggestedFilename.replace(']', '_');
				suggestedFilename.replace(';', '_');
				
				auto usuggestedFilename = suggestedFilename.utf8();
				path = [[m_outerObject delegate] decideFilenameForDownload:m_outerObject withSuggestedName:[OBString stringWithUTF8String:usuggestedFilename.data()]];
			}
			else
			{
				path = [[m_outerObject delegate] decideFilenameForDownload:m_outerObject withSuggestedName:path];
			}
			
			D(dprintf("%s: path %s\n", __PRETTY_FUNCTION__, [path cString]));
			if (path)
			{
				[m_outerObject setFilename:path];
				// m_download->setDestination(WTF::String::fromUTF8([path cString]));
			}
			else
			{
				[m_outerObject cancel];
			}
		}
		else if (response.isUnauthorized())
		{
			[m_outerObject cancelDueToAuthentication];
		}
		// todo: maybe proxy auth?
		else
		{
			[m_outerObject cancel];
		}
	}
	[m_outerObject autorelease];
}

void WebDownload::didReceiveDataOfLength(int size)
{
	m_receivedSize += size;
	[[m_outerObject delegate] download:m_outerObject didReceiveBytes:size];
}

void WebDownload::didFinish()
{
	if (m_download)
	{
		auto tmpPath = m_download->tmpFilePath();
		auto uTmpPath = tmpPath.utf8();
		
		m_download->setDeleteTmpFile(false);
		m_download->setListener(nullptr);
		m_download = nullptr;

		D(dprintf("%s: tmp '%s'\n", __PRETTY_FUNCTION__, uTmpPath.data()));

		[m_outerObject setFinished:YES];
		[m_outerObject setPending:NO];
		[m_outerObject handleFinishedWithTmpPath:[OBString stringWithUTF8String:uTmpPath.data()]];
		[m_outerObject selfrelease];
	}
}

void WebDownload::didFail(const WebCore::ResourceError& error)
{
	[m_outerObject setPending:NO];
	[m_outerObject setFailed:YES];
	
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));

	// allow resuming
	if (m_download)
	{
		m_download->cancel();
		m_download->setDeleteTmpFile(false);
	}

	[[m_outerObject delegate] download:m_outerObject didFailWithError:[WkError errorWithResourceError:error]];
	[m_outerObject selfrelease];
}

void WebDownload::setUserPassword(const String& user, const String &password)
{
	m_user = user;
	m_password = password;
	
	if (m_download)
		m_download->setUserPassword(m_user, m_password);
}

@implementation _WkDownload

- (WkDownload *)initWithURL:(OBURL *)url withDelegate:(id<WkDownloadDelegate>)delegate
{
	if ((self = [super init]))
	{
		_url = [url retain];
		_delegate = [delegate retain];
		_isPending = false;
		_isFailed = false;
		_download.initialize(self, _url);
	}
	
	return self;
}

- (WkDownload *)initWithRequest:(WkMutableNetworkRequest *)request withDelegate:(id<WkDownloadDelegate>)delegate
{
	if ((self = [super init]))
	{
		_delegate = [delegate retain];
		_request = [request retain];
		_url = [[request URL] retain];
		_isPending = false;
		_isFailed = false;
		_download.initialize(self, _url);
	}
	
	return self;
}

- (WkDownload *)initWithHandle:(WebCore::ResourceHandle*)handle request:(const WebCore::ResourceRequest&)request response:(const WebCore::ResourceResponse&)response withDelegate:(id<WkDownloadDelegate>)delegate
{
	if ((self = [super init]))
	{
		auto uurl = response.url().string().utf8();
		_url = [[OBURL URLWithString:[OBString stringWithUTF8String:uurl.data()]] retain];
		_delegate = [delegate retain];
		_isPending = false;
		_isFailed = false;
		_download.initialize(self, handle, request, response);
	}
	return self;
}

- (void)dealloc
{
	[self cancel];
	[_url release];
	[_request release];
	[_delegate release];
	[_filename release];
	if (_tmpPath)
	{
		DeleteFile([_tmpPath nativeCString]);
	}
	[_tmpPath release];
	[super dealloc];
}

- (id<WkDownloadDelegate>)delegate
{
	return _delegate;
}

- (void)start
{
	if (_download.start())
	{
		_isPending = true;
		@synchronized (self) {
			if (!_selfretained)
			{
				[self retain];
				_selfretained = YES;
			}
		}
	}
}

- (void)selfrelease
{
	@synchronized (self) {
		if (_selfretained)
		{
			[self autorelease];
			_selfretained = NO;
		}
	}
}

- (void)cancel
{
	_isPending = false;
	_download.cancel();
	[_delegate download:self didFailWithError:[WkError errorWithURL:[self url] errorType:WkErrorType_Cancellation code:0]];
}

- (void)cancelDueToAuthentication
{
	_isPending = false;
	_download.cancelForResume();
	[_delegate downloadNeedsAuthenticationCredentials:self];
}

- (void)cancelForResume
{
	_isPending = false;
	_download.cancelForResume();
	[_delegate download:self didFailWithError:[WkError errorWithURL:[self url] errorType:WkErrorType_Cancellation code:0]];
}

- (BOOL)canResumeDownload
{
	return !_isPending && !_isFinished;
}

- (void)resume
{
	if (_download.resume())
	{
		_isPending = YES;
		_isFinished = NO;
		_isFailed = NO;
		@synchronized (self) {
			if (!_selfretained)
			{
				[self retain];
				_selfretained = YES;
			}
		}
	}
}

- (OBURL *)url
{
	return _url;
}

- (OBString *)filename
{
	return _filename;
}

- (void)setFilename:(OBString *)filename
{
	[_filename autorelease];
	_filename = [filename retain];
}

- (void)updateURL:(OBURL *)url
{
	if (![_url isEqual:url])
	{
		[_url autorelease];
		_url = [url retain];
		[_delegate download:self didRedirect:_url];
		
		D(dprintf("%s: >> %s\n", __PRETTY_FUNCTION__, [[_url absoluteString] cString]));
	}
}

- (QUAD)size
{
	return _download.size();
}

- (QUAD)downloadedSize
{
	return _download.downloadedSize();
}

- (BOOL)isPending
{
	return _isPending;
}

- (BOOL)isFailed
{
	return _isFailed;
}

- (void)setPending:(BOOL)pending
{
	_isPending = pending;
}

- (void)setFailed:(BOOL)failed
{
	_isFailed = failed;
}

- (void)setFinished:(BOOL)fini
{
	_isFinished = fini;
}

- (BOOL)isFinished
{
	return _isFinished;
}

- (void)handleFinishedWithTmpPath:(OBString *)path
{
	_tmpPath = [[path absolutePath] retain];
	[_delegate downloadDidFinish:self];

	if (![_delegate respondsToSelector:@selector(downloadShouldMoveFileWhenFinished:)] || [_delegate downloadShouldMoveFileWhenFinished:self])
	{
		[self moveFinishedDownload:_filename];
	}
}

- (void)setLogin:(OBString *)login password:(OBString *)password
{
	_download.setUserPassword(String::fromUTF8([login cString]), String::fromUTF8([password cString]));
}

- (void)moveCompletedWithError:(WkError *)error
{
	if ([_delegate respondsToSelector:@selector(download:completedMoveWithError:)])
	{
		[_delegate download:self completedMoveWithError:error];
	}
}

- (void)threadMove:(OBArray *)srcDest
{
	OBString *src = [srcDest firstObject];
	OBString *dest = [srcDest lastObject];
	BPTR lSrc = Lock([src nativeCString], ACCESS_READ);

	if (!lSrc)
	{
		[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:_url errorType:WkErrorType_SourcePath code:IoErr()]];
		return;
	}

	BPTR lDst = Lock([[dest pathPart] nativeCString], ACCESS_READ);
	if (!lDst)
	{
		[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:_url errorType:WkErrorType_DestinationPath code:IoErr()]];

		if (lSrc)
			UnLock(lSrc);
		DeleteFile([src nativeCString]);
		return;
	}

	auto sl = SameLock(lSrc, lDst);
	
	if (lSrc)
		UnLock(lSrc);
	if (lDst)
		UnLock(lDst);

	if (LOCK_DIFFERENT == sl)
	{
		char *buffer = (char *)OBAlloc(512 * 1024);
		if (!buffer)
		{
			[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:_url errorType:WkErrorType_Cancellation code:IoErr()]];
			return;
		}
		lSrc = Open([src nativeCString], MODE_OLDFILE);
		if (!lSrc)
		{
			OBFree(buffer);
			[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:_url errorType:WkErrorType_SourcePath code:IoErr()]];
			return;
		}
		lDst = Open([dest nativeCString], MODE_NEWFILE);

		if (!lDst)
		{
			OBFree(buffer);
			Close(lSrc);
			[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:_url errorType:WkErrorType_DestinationPath code:IoErr()]];
			return;
		}
		
		WkError *error = nil;
		for (;;)
		{
			auto len = Read(lSrc, buffer, 512 * 1024);

			if (len == 0)
				break;
				
			if (len < 0)
			{
				error = [WkError errorWithURL:_url errorType:WkErrorType_Read code:IoErr()];
				break;
			}
			
			auto lenOut = Write(lDst, buffer, len);
			if (lenOut != len)
			{
				error = [WkError errorWithURL:_url errorType:WkErrorType_Write code:IoErr()];
			}
		}
		
		OBFree(buffer);

		Close(lSrc);
		Close(lDst);
		
		DeleteFile([src nativeCString]);

		if (error)
		{
			[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:error];
			return;
		}
	}
	else if (LOCK_SAME_VOLUME == sl)
	{
		if (!Rename([src nativeCString], [dest nativeCString]))
		{
			[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:_url errorType:WkErrorType_Rename code:IoErr()]];
			DeleteFile([src nativeCString]);
			return;
		}
	}

	[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:nil];
}

- (void)moveFinishedDownload:(OBString *)destinationPath
{
    D(dprintf("%s: %d -> %s (tmp %s)\n", __func__, _isFinished, [destinationPath cString], [_tmpPath cString]));
	if (_isFinished)
	{
		if (_tmpPath)
		{
			OBString *tmp = [_tmpPath copy];

			[_tmpPath release];
			_tmpPath = nil;

			[OBThread startWithObject:self selector:@selector(threadMove:) argument:[OBArray arrayWithObjects:tmp, destinationPath, nil]];
		}
	}
	else
	{
		[_filename autorelease];
		_filename = [[destinationPath absolutePath] retain];
	}
}

@end

@implementation WkDownload

+ (WkDownload *)download:(OBURL *)url withDelegate:(id<WkDownloadDelegate>)delegate
{
	return [[[_WkDownload alloc] initWithURL:url withDelegate:delegate] autorelease];
}

+ (WkDownload *)downloadRequest:(WkMutableNetworkRequest *)request withDelegate:(id<WkDownloadDelegate>)delegate
{
	return [[[_WkDownload alloc] initWithRequest:request withDelegate:delegate] autorelease];
}

+ (WkDownload *)downloadWithHandle:(WebCore::ResourceHandle*)handle request:(const WebCore::ResourceRequest&)request response:(const WebCore::ResourceResponse&)response withDelegate:(id<WkDownloadDelegate>)delegate
{
	return [[[_WkDownload alloc] initWithHandle:handle request:request response:response withDelegate:delegate] autorelease];
}

- (void)start
{

}

- (void)cancel
{

}

- (void)cancelForResume
{

}

- (BOOL)canResumeDownload
{
	return 0;
}

- (void)resume
{

}

- (QUAD)size
{
	return 0;
}

- (QUAD)downloadedSize
{
	return 0;
}

- (BOOL)isPending
{
	return 0;
}

- (BOOL)isFailed
{
	return 0;
}

- (BOOL)isFinished
{
	return 0;
}

- (OBURL *)url
{
	return nil;
}

- (OBString *)filename
{
	return nil;
}

- (void)setLogin:(OBString *)login password:(OBString *)password
{
	(void)login;
	(void)password;
}

- (void)moveFinishedDownload:(OBString *)destinationPath
{
	(void)destinationPath;
}

@end
