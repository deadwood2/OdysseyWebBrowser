#import "WkError_private.h"
#import "WkCertificate.h"
#import <ob/OBFramework.h>
#undef __OBJC__
#import <WebCore/ResourceError.h>
#import <WebCore/CertificateInfo.h>
#define __OBJC__

@interface WkErrorPrivate : WkError
{
	OBURL *_url;
	OBString *_domain;
	OBString *_description;
	WkCertificateChain *_certificates;
	WkErrorType _type;
	int _code;
}
@end

@implementation WkErrorPrivate

- (id)initWithResourceError:(const WebCore::ResourceError &)error
{
	if ((self = [super init]))
	{
		auto udescription = error.localizedDescription().utf8();
		_description = [[OBString stringWithUTF8String:udescription.data()] retain];

		_code = error.errorCode();

		auto udomain = error.domain().utf8();
		_domain = [[OBString stringWithUTF8String:udomain.data()] retain];

		auto uurl = error.failingURL().string().utf8();
		_url = [[OBURL URLWithString:[OBString stringWithUTF8String:uurl.data()]] retain];

		const auto& certInfo = error.certificateInfo();

		if (certInfo)
		{
			const auto& chain = certInfo->certificateChain();
			if (chain.size())
			{
				OBMutableArray *certArray = [OBMutableArray arrayWithCapacity:chain.size()];
				// NOTE: we want root > intermediate > client cert order
				for (int i = chain.size() - 1; i >= 0; i--)
				{
					const auto &cert = chain[i];
					[certArray addObject:[WkCertificate certificateWithData:(const char*)cert.data() length:cert.size()]];
				}

				_certificates = [[WkCertificateChain certificateChainWithCertificates:certArray] retain];
			}
		}
		
		if (error.isSSLCertVerificationError())
			_type = WkErrorType_SSLCertification;
		else if (error.isSSLConnectError())
			_type = WkErrorType_SSLConnection;
		else switch (error.type())
		{
		case WebCore::ResourceErrorBase::Type::Null: _type = WkErrorType_Null; break;
		case WebCore::ResourceErrorBase::Type::General: _type = WkErrorType_General; break;
		case WebCore::ResourceErrorBase::Type::AccessControl: _type = WkErrorType_AccessControl; break;
		case WebCore::ResourceErrorBase::Type::Cancellation: _type = WkErrorType_Cancellation; break;
		case WebCore::ResourceErrorBase::Type::Timeout: _type = WkErrorType_Timeout; break;
		}
	}

	return self;
}

- (id)initWithURL:(OBURL *)url errorType:(WkErrorType)type code:(int)code
{
	if ((self = [super init]))
	{
		_code = code;
		_type = type;
		_url = [url retain];
		switch (_type)
		{
		case WkErrorType_Null: _domain = @"null"; _description = @""; break;
		case WkErrorType_General: _domain = @"general"; _description = OBL(@"General Error", @"Error description"); break;
		case WkErrorType_AccessControl: _domain = @"access"; _description = OBL(@"Access Control Error", @"Error description"); break;
		case WkErrorType_Cancellation: _domain = @"cancellation"; _description = OBL(@"Cancelled", @"Error description"); break;
		case WkErrorType_Timeout: _domain = @"timeout"; _description = OBL(@"Timed out", @"Error description"); break;
		case WkErrorType_SSLConnection: _domain = @"ssl"; _description = OBL(@"SSL Error", @"Error description"); break;
		case WkErrorType_SSLCertification: _domain = @"sslcertificate"; _description = OBL(@"Certificate Error", @"Error description"); break;
		case WkErrorType_SourcePath: _domain = @"file"; _description = OBL(@"I/O Error: invalid source", @"Error description"); break;
		case WkErrorType_DestinationPath: _domain = @"file"; _description = OBL(@"I/O Error: invalid destination", @"Error description"); break;
		case WkErrorType_Rename: _domain = @"file"; _description = OBL(@"I/O Error: file rename", @"Error description"); break;
		case WkErrorType_Read: _domain = @"file"; _description = OBL(@"I/O Error: file read", @"Error description"); break;
		case WkErrorType_Write: _domain = @"file"; _description = OBL(@"I/O Error: file write", @"Error description"); break;
		}
	}
	
	return self;
}

- (void)dealloc
{
	[_url release];
	[_domain release];
	[_certificates release];
	[_description release];
	[super dealloc];
}

- (OBString *)domain
{
	return _domain;
}

- (OBString *)localizedDescription
{
	return _description;
}

- (OBURL *)URL
{
	return _url;
}

- (int)errorCode
{
	return _code;
}

- (WkErrorType)type
{
	return _type;
}

- (WkCertificateChain *)certificates
{
	return _certificates;
}

@end

@implementation WkError

+ (WkError *)errorWithResourceError:(const WebCore::ResourceError &)error
{
	return [[[WkErrorPrivate alloc] initWithResourceError:error] autorelease];
}

+ (WkError *)errorWithURL:(OBURL *)url errorType:(WkErrorType)type code:(int)code
{
	return [[[WkErrorPrivate alloc] initWithURL:url errorType:type code:code] autorelease];
}

- (OBString *)domain
{
	return nil;
}

- (OBURL *)URL
{
	return nil;
}

- (OBString *)localizedDescription
{
	return nil;
}

- (int)errorCode
{
	return -1;
}

- (WkErrorType)type
{
	return WkErrorType_Null;
}

- (WkCertificateChain *)certificates
{
	return nil;
}

@end
