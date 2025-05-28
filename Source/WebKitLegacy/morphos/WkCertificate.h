#import <ob/OBString.h>

@class OBDictionary, OBData, OBArray, OBError;

@interface WkCertificate : OBObject

// Constructs a x509 certificate from the passed in ascii data
+ (WkCertificate *)certificateWithData:(OBData *)data;
+ (WkCertificate *)certificateWithData:(const char *)data length:(ULONG)length;

// Ascii data of the certificate
- (OBData *)certificate;

// YES if the certificate is self signed
- (BOOL)isSelfSigned;

// YES if this is a root certificate
- (BOOL)isRoot;

// YES if we regard this certificate as valid on its own (ie root cert, etc)
- (BOOL)isValid;

// YES if the certificate is not valid at the current system time
- (BOOL)isExpired;

// Maps subject/issuer name fields as OBString object in the form key:@"CN" value:@"*.google.com"
- (OBDictionary *)subjectName;
- (OBDictionary *)issuerName;

// Version of the certificate
- (int)version;

// Serial # of the certificate
- (OBString *)serialNumber;

// SHA256 either as raw data or a hexadecimal string
- (OBData *)sha256;
- (OBString *)sha256Hex;

// Certificate's Algorithm
- (OBString *)algorithm;

// Expiration time as strings
- (OBString *)notValidBefore;
- (OBString *)notValidAfter;

@end

@interface WkCertificateChain : OBObject

+ (WkCertificateChain *)certificateChainWithCertificates:(OBArray *)certificates;
+ (WkCertificateChain *)certificateChainWithCertificate:(WkCertificate *)cert;

- (OBArray *)certificates;

// Verifies the whole certificate chain
- (BOOL)verify:(OBError **)outerror;
- (BOOL)verifyForHost:(OBString *)hostname error:(OBError **)outerror;

// Translates the code from OBError into a string
+ (OBString *)validationErrorStringForErrorCode:(int)e;

@end
