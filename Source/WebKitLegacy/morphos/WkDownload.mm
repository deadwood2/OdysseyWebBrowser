#import "WkDownload_private.h"
#import "WkNetworkRequestMutable.h"
#import "WkError_private.h"
#import "WkSettings.h"
#import <ob/OBThread.h>
#import <ob/OBArray.h>
#import <ob/OBRunLoop.h>
#import <proto/obframework.h>

#undef __OBJC__
#include "WebKit.h"
#include <wtf/WallTime.h>
#include <wtf/URL.h>
#include <wtf/text/WTFString.h>
#include <WebCore/CurlDownload.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/DataURLDecoder.h>
#include <WebCore/PlatformStrategies.h>
#include <WebCore/BlobRegistry.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/BlobRegistryImpl.h>
#include <WebCore/BlobDataFileReference.h>
#include <pal/text/TextEncoding.h>
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
    QUAD                          m_size { 0 };
    QUAD                          m_receivedSize { 0 };
    WTF::String                   m_user, m_password;
    bool                          m_didResume { 0 };
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
		WTF::URL wurl(WTF::URL(), String::fromUTF8([[url absoluteString] cString]));
		D(dprintf("initialize %s context %p\n", [[url absoluteString] cString], WebKit::WebProcess::singleton().networkingContext()));
		m_download->init(*this, wurl, WebKit::WebProcess::singleton().networkingContext());
	}
}

void WebDownload::initialize(_WkDownload *outer, WebCore::ResourceHandle*handle, const WebCore::ResourceRequest &request, const WebCore::ResourceResponse& response)
{
	m_outerObject = outer;
	m_download = adoptRef(new WebCore::CurlDownload());
	if (m_download)
	{
		auto uurl = response.url().string().utf8();
		auto umime = response.mimeType().utf8();
		auto uname = response.suggestedFilename().utf8();

		if (0 == uname.length())
			uname = PAL::decodeURLEscapeSequences(response.url().lastPathComponent()).utf8();
		
		D(dprintf("initialize w/response %s\n", uurl.data()));
		
		[m_outerObject setFilename:[OBString stringWithUTF8String:uname.data()]];
		m_size = response.expectedContentLength();
  
        if (-1ll == m_size)
            m_size = 0;

		m_download->init(*this, handle, request, response);

        // would leak a connection otherwise
        if (handle)
            handle->cancel();
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
        m_didResume = true;
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
			if (0 == m_size || m_didResume)
            {
                m_didResume = false;
                m_receivedSize = m_download->resumeOffset();
				m_size = m_download->resumeOffset() + response.expectedContentLength();
            }
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
				suggestedFilename = PAL::decodeURLEscapeSequences(suggestedFilename);
				
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, '\n', ' ');
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, '\r', ' ');
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, '\t', ' ');
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, ':', WTF::emptyString());
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, '\'', '_');
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, '/', '_');
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, '\\', '_');
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, '*', '_');
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, '?', '_');
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, '~', '_');
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, '[', '_');
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, ']', '_');
				suggestedFilename = makeStringByReplacingAll(suggestedFilename, ';', '_');
				
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

- (OBString *)temporaryPath
{
    return _tmpPath;
}

- (void)setTemporaryPath:(OBString *)path
{
    [_tmpPath autorelease];
    _tmpPath = [path copy];
}

@end

@interface _WkDownloadData : WkDownload
{
    WTF::URL _url;
    OBURL *_pageURL;
    id<WkDownloadDelegate> _delegate;
    OBString *_filename;
    WTF::String _downloadPath;
    BOOL _isPending;
    BOOL _isFailed;
    BOOL _isFinished;
    QUAD _size;
    QUAD _downloadedSize;
}
@end

@implementation _WkDownloadData

- (id)initWithURL:(const URL&)url pageURL:(OBURL *)pageurl filename:(OBString *)name withDelegate:(id<WkDownloadDelegate>)delegate
{
    if ((self = [super init]))
    {
        _url = url;
        _pageURL = [pageurl retain];
        _filename = [name copy];
        _delegate = delegate;
    }
    
    return self;
}

- (void)dealloc
{
    [_filename release];
    [_pageURL release];
    [super dealloc];
}

- (void)finishedWithError:(WkError *)error
{
    _isFailed = error ? YES : NO;
    _isFinished = YES;
    _isPending = NO;

    if (error)
        [[OBRunLoop mainRunLoop] performSelector:@selector(download:didFailWithError:) target:_delegate withObject:self withObject:error];
    else
        [[OBRunLoop mainRunLoop] performSelector:@selector(downloadDidFinish:) target:_delegate withObject:self];
}

- (void)work
{
    [_delegate downloadDidBegin:self];

	auto result = WebCore::DataURLDecoder::decode(_url);

    if (result)
    {
        _size = result->data.size();
        [_delegate didReceiveResponse:self];

        OBString *name = [_delegate decideFilenameForDownload:self withSuggestedName:_filename];

        D(dprintf("%s: decidedpath %s from name %s\n", __PRETTY_FUNCTION__, [name cString], [_filename cString]));

        if (name)
        {
            [self setFilename:name];

            FileSystem::PlatformFileHandle downloadFileHandle;

            @synchronized (self) {
                auto [filePath, fileHandle] = FileSystem::openTemporaryFile("download"_s);
                _downloadPath = filePath;
                downloadFileHandle = fileHandle;
            }

            if (downloadFileHandle != FileSystem::invalidPlatformFileHandle)
            {
                if (-1 != FileSystem::writeToFile(downloadFileHandle, result->data.span()))
                {
                    _downloadedSize = result->data.size();
                    [_delegate download:self didReceiveBytes:_downloadedSize];
                    FileSystem::closeFile(downloadFileHandle);
                    _isFinished = YES;
                    _isPending = NO;
                    [self finishedWithError:nil];
                }
                else
                {
                    ULONG err = IoErr();
                    FileSystem::closeFile(downloadFileHandle);
                    FileSystem::deleteFile(_downloadPath);
                    [self finishedWithError:[WkError errorWithURL:[self url] errorType:WkErrorType_Write code:err]];
                }
            }
            else
            {
                [self finishedWithError:[WkError errorWithURL:[self url] errorType:WkErrorType_Write code:IoErr()]];
            }
        }
        else
        {
            [self finishedWithError:[WkError errorWithURL:[self url] errorType:WkErrorType_Cancellation code:0]];
        }
    }
    else
    {
        [self finishedWithError:[WkError errorWithURL:[self url] errorType:WkErrorType_General code:0]];
    }
}

- (void)start
{
    _isPending = true;
    [[OBRunLoop mainRunLoop] performSelector:@selector(work) target:self];
}

- (BOOL)canResumeDownload
{
    return NO;
}

- (OBURL *)url
{
    return _pageURL;
}

- (void)setFilename:(OBString *)f
{
    @synchronized (self) {
        [_filename autorelease];
        _filename = [f copy];
    }
}

- (OBString *)filename
{
    @synchronized (self) {
        return [[_filename retain] autorelease];
    }
}

- (void)setTemporaryPath:(OBString *)path
{
    // not relevant here
}

- (OBString *)temporaryPath
{
    @synchronized (self) {
        auto upath = _downloadPath.utf8();
        return [OBString stringWithUTF8String:upath.data()];
    }
}

- (void)setLogin:(OBString *)login password:(OBString *)password
{
    (void)login;
    (void)password;
}

- (QUAD)size
{
    return _size;
}

- (QUAD)downloadedSize
{
    return _downloadedSize;
}

- (BOOL)isPending
{
    return _isPending;
}

- (BOOL)isFailed
{
    return _isFailed;
}

- (BOOL)isFinished
{
    return _isFinished;
}

- (id<WkDownloadDelegate>)delegate
{
    return _delegate;
}

@end

@interface _WkDownloadBlob : WkDownload
{
    WTF::URL _url;
    OBURL *_pageURL;
    id<WkDownloadDelegate> _delegate;
    OBString *_filename;
    BOOL _isPending;
    BOOL _isFailed;
    BOOL _isFinished;
    QUAD _size;
    QUAD _downloadedSize;
    WTF::String _downloadPath;
}
@end

@implementation _WkDownloadBlob

- (id)initWithURL:(const URL&)url pageURL:(OBURL *)pageurl filename:(OBString *)name withDelegate:(id<WkDownloadDelegate>)delegate
{
    if ((self = [super init]))
    {
        _url = url;
        _pageURL = [pageurl retain];
        _delegate = delegate;
        _filename = [name copy];
    }
    
    return self;
}

- (void)dealloc
{
    [_pageURL release];
    [_filename release];
    [super dealloc];
}

- (void)finishedWithError:(WkError *)error
{
    _isFailed = error ? YES : NO;
    _isFinished = YES;
    _isPending = NO;

    if (error)
        [[OBRunLoop mainRunLoop] performSelector:@selector(download:didFailWithError:) target:_delegate withObject:self withObject:error];
    else
        [[OBRunLoop mainRunLoop] performSelector:@selector(downloadDidFinish:) target:_delegate withObject:self];
}

- (void)work
{
    auto registry = WebCore::platformStrategies()->blobRegistry()->blobRegistryImpl();
    [_delegate downloadDidBegin:self];

    Vector<RefPtr<WebCore::BlobDataFileReference>> blobReferences = registry->filesInBlob(_url);

    // no files, just data chunks
    if (blobReferences.size() == 0)
    {
        auto data = registry->getBlobDataFromURL(_url);
        if (nullptr != data)
        {
            UQUAD size = 0;
            auto items = data->items();
            for (auto& item : items)
            {
                size += item.data()->size();
            }

            _size = size;
            [_delegate didReceiveResponse:self];

            OBString *name = [_delegate decideFilenameForDownload:self withSuggestedName:_filename];

            D(dprintf("%s: decidedpath %s from name %s\n", __PRETTY_FUNCTION__, [name cString], [_filename cString]));

            if (name)
            {
                [self setFilename:name];

                FileSystem::PlatformFileHandle downloadFileHandle;

                @synchronized (self) {
                    auto [filePath, fileHandle] = FileSystem::openTemporaryFile("download"_s);
                    _downloadPath = filePath;
                    downloadFileHandle = fileHandle;
                }

                if (downloadFileHandle != FileSystem::invalidPlatformFileHandle)
                {
                    for (auto& item : items)
                    {
						D(dprintf("%s: item offset %lld size %d data %p\n", __PRETTY_FUNCTION__, item.offset(), item.data()->size(), item.data()->data()->data()));
                    
                        if (-1 != FileSystem::writeToFile(downloadFileHandle, item.data()->span()))
						{
							_downloadedSize += item.data()->size();
							[_delegate download:self didReceiveBytes:item.data()->size()];
						}
						else
						{
							ULONG err = IoErr();
							FileSystem::closeFile(downloadFileHandle);
							FileSystem::deleteFile(_downloadPath);
							[self finishedWithError:[WkError errorWithURL:[self url] errorType:WkErrorType_Write code:err]];
							return;
						}
					}

					FileSystem::closeFile(downloadFileHandle);
					_isFinished = YES;
					_isPending = NO;
					[self finishedWithError:nil];
					return;
                }
                else
                {
					ULONG err = IoErr();
					[self finishedWithError:[WkError errorWithURL:[self url] errorType:WkErrorType_Write code:err]];
                }
            }
            else
            {
                [self finishedWithError:[WkError errorWithURL:[self url] errorType:WkErrorType_Cancellation code:0]];
            }
        }
        else
        {
            [self finishedWithError:[WkError errorWithURL:[self url] errorType:WkErrorType_General code:0]];
        }
    }
    else
    {
        [self finishedWithError:[WkError errorWithURL:[self url] errorType:WkErrorType_Cancellation code:0]];
    }
}

- (void)start
{
    _isPending = true;
    [[OBRunLoop mainRunLoop] performSelector:@selector(work) target:self];
}

- (BOOL)canResumeDownload
{
    return NO;
}

- (OBURL *)url
{
    return _pageURL;
}

- (void)setFilename:(OBString *)f
{
    [_filename autorelease];
    _filename = [f copy];
}

- (OBString *)filename
{
    return _filename;
}

- (void)setTemporaryPath:(OBString *)path
{
}

- (OBString *)temporaryPath
{
    @synchronized (self) {
        auto upath = _downloadPath.utf8();
        return [OBString stringWithUTF8String:upath.data()];
    }
}

- (void)setLogin:(OBString *)login password:(OBString *)password
{
    (void)login;
    (void)password;
}

- (QUAD)size
{
    return _size;
}

- (QUAD)downloadedSize
{
    return _downloadedSize;
}

- (BOOL)isPending
{
    return _isPending;
}

- (BOOL)isFailed
{
    return _isFailed;
}

- (BOOL)isFinished
{
    return _isFinished;
}

- (id<WkDownloadDelegate>)delegate
{
    return _delegate;
}

@end
@implementation WkDownload

+ (WkDownload *)download:(OBURL *)url withDelegate:(id<WkDownloadDelegate>)delegate
{
	D(dprintf("%s: url %s\n", __PRETTY_FUNCTION__, [[url absoluteString] cString]));

	return [[[_WkDownload alloc] initWithURL:url withDelegate:delegate] autorelease];
}

+ (WkDownload *)downloadRequest:(WkMutableNetworkRequest *)request withDelegate:(id<WkDownloadDelegate>)delegate
{
	return [[[_WkDownload alloc] initWithRequest:request withDelegate:delegate] autorelease];
}

+ (WkDownload *)downloadWithHandle:(WebCore::ResourceHandle*)handle request:(const WebCore::ResourceRequest&)request response:(const WebCore::ResourceResponse&)response withDelegate:(id<WkDownloadDelegate>)delegate
{
	D(dprintf("%s: url %s\n", __PRETTY_FUNCTION__, request.url().string().utf8().data()));
	return [[[_WkDownload alloc] initWithHandle:handle request:request response:response withDelegate:delegate] autorelease];
}

+ (WkDownload *)downloadWithDataURL:(const URL&)url pageURL:(OBURL *)pageurl filename:(OBString *)name withDelegate:(id<WkDownloadDelegate>)delegate
{
	D(dprintf("%s: url %s\n", __PRETTY_FUNCTION__, url.string().utf8().data()));
    return [[[_WkDownloadData alloc] initWithURL:url pageURL:pageurl filename:name withDelegate:delegate] autorelease];
}

+ (WkDownload *)downloadWithBlobURL:(const URL&)url pageURL:(OBURL *)pageurl filename:(OBString *)name withDelegate:(id<WkDownloadDelegate>)delegate
{
	D(dprintf("%s: url %s\n", __PRETTY_FUNCTION__, url.string().utf8().data()));
    return [[[_WkDownloadBlob alloc] initWithURL:url pageURL:pageurl filename:name withDelegate:delegate] autorelease];
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

- (OBString *)temporaryPath
{
    return nil;
}

- (id<WkDownloadDelegate>)delegate
{
    return nil;
}

- (void)setLogin:(OBString *)login password:(OBString *)password
{
	(void)login;
	(void)password;
}

- (void)moveCompletedWithError:(WkError *)error
{
	if ([[self delegate] respondsToSelector:@selector(download:completedMoveWithError:)])
	{
		[[self delegate] download:self completedMoveWithError:error];
	}
}

- (OBString *)_extensionFromFileName:(OBString *)name
{
	OBRange r = [name rangeOfString:@"." options:OBBackwardsSearch];

	if (!OBEmptyRange(r))
	{
		OBString *s = [name substringFromIndex:r.location + 1];
		if ([s length])
			return s;
	}

	return nil;
}

- (OBString *)_shortenedFileName:(OBString *)path
{
    if (!path)
        return nil;

    OBString *name = [path filePart];
    OBString *pathPart = [path pathPart];
    LONG nameLength = [name length];

    // dos calls don't like null input
    if (!name || !pathPart)
        return nil;

    LONG maxlen = 107;
    GetFileSysAttr([pathPart nativeCString], FQA_MaxFileNameLength, &maxlen, sizeof(maxlen));

    OBString *extension = [self _extensionFromFileName:name];
    LONG extensionLength = [extension length];

    if (extensionLength)
    {
        if (extensionLength - 2 > maxlen)
            return [pathPart stringByAddingPathComponent:[name substringToIndex:maxlen - 1]];

        name = [name substringToIndex:std::min(maxlen - (extensionLength + 2), nameLength - (extensionLength + 1))];
        name = [name stringByAppendingFormat:@".%@", extension];
    }
    else
    {
        name = [name substringToIndex:std::min(maxlen, LONG([name length]))];
    }

    name = [pathPart stringByAddingPathComponent:name];

    return name;
}

- (void)threadMove:(OBArray *)srcDest
{
	OBString *src = [srcDest firstObject];
	OBString *dest = [srcDest lastObject];

    dest = [self _shortenedFileName:dest];
    if (!dest || ![dest pathPart])
    {
        [[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:[self url] errorType:WkErrorType_Cancellation code:ERROR_LINE_TOO_LONG]];
        return;
    }

	BPTR lSrc = Lock([src nativeCString], ACCESS_READ);

	if (!lSrc)
	{
		[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:[self url] errorType:WkErrorType_SourcePath code:IoErr()]];
		return;
	}

	BPTR lDst = Lock([[dest pathPart] nativeCString], ACCESS_READ);
	if (!lDst)
	{
		[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:[self url] errorType:WkErrorType_DestinationPath code:IoErr()]];

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
			[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:[self url] errorType:WkErrorType_Cancellation code:IoErr()]];
			return;
		}

		lSrc = Open([src nativeCString], MODE_OLDFILE);
		if (!lSrc)
		{
			OBFree(buffer);
			[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:[self url] errorType:WkErrorType_SourcePath code:IoErr()]];
			return;
		}

        lDst = Open([dest nativeCString], MODE_NEWFILE);

        if (!lDst)
        {
            ULONG err = IoErr();
            OBFree(buffer);
            Close(lSrc);
            [[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:[self url] errorType:WkErrorType_DestinationPath code:err]];
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
				error = [WkError errorWithURL:[self url] errorType:WkErrorType_Read code:IoErr()];
				break;
			}
			
			auto lenOut = Write(lDst, buffer, len);
			if (lenOut != len)
			{
				error = [WkError errorWithURL:[self url] errorType:WkErrorType_Write code:IoErr()];
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
            ULONG err = IoErr();
            [[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:[WkError errorWithURL:[self url] errorType:WkErrorType_Rename code:err]];
            DeleteFile([src nativeCString]);
            return;
        }
	}

	[[OBRunLoop mainRunLoop] performSelector:@selector(setFilename:) target:self withObject:dest];
	[[OBRunLoop mainRunLoop] performSelector:@selector(moveCompletedWithError:) target:self withObject:nil];
}

- (void)moveFinishedDownload:(OBString *)destinationPath
{
    D(dprintf("%s: %d -> %s (tmp %s)\n", __func__, [self isFinished], [destinationPath cString], [[self temporaryPath] cString]));
	if ([self isFinished])
	{
        OBString *tmp = [[self temporaryPath] copy];
		if (tmp)
		{
            [self setTemporaryPath:nil];
			[OBThread startWithObject:self selector:@selector(threadMove:) argument:[OBArray arrayWithObjects:tmp, destinationPath, nil]];
		}
	}
	else
	{
        [self setFilename:[destinationPath absolutePath]];
	}
}

@end
