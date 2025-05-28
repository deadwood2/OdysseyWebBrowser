#import <ob/OBURL.h>

@class WkCertificateChain;

typedef enum
{
	WkErrorType_Null,
	WkErrorType_General,
	WkErrorType_AccessControl,
	WkErrorType_Cancellation,
	WkErrorType_Timeout,
	WkErrorType_SSLConnection,
	WkErrorType_SSLCertification,
	WkErrorType_SourcePath,
	WkErrorType_DestinationPath,
	WkErrorType_Rename,
	WkErrorType_Read,
	WkErrorType_Write,
} WkErrorType;

@interface WkError : OBObject

// error domain (textual)
- (OBString *)domain;

- (OBString *)localizedDescription;

// address related to the error
- (OBURL *)URL;

// error code
- (int)errorCode;

// error type
- (WkErrorType)type;

// associated certificate
- (WkCertificateChain *)certificates;

@end
