#import <ob/OBFramework.h>
#import <proto/obframework.h>
#import "WkCertificate.h"

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#include <string.h>

extern "C" { void dprintf(const char *,...); }

#define SHALEN 32

static X509_STORE     *_store;

@interface _WkCertificate : WkCertificate
{
	OBData       *_data;
	BOOL          _decoded;
	BOOL          _valid;
	BOOL          _validOverride;
	BOOL          _selfSigned;
	BOOL          _isExpired;
	OBDictionary *_subjectName;
	OBDictionary *_issuerName;
	OBData       *_sha;
	OBString     *_serial;
	OBString     *_algorithm;
	OBString     *_validNotBefore;
	OBString     *_validNotAfter;
	int           _version;
}
@end

@interface _WkCertificateChain : WkCertificateChain
{
	OBArray *_certificates;
}
@end

@implementation _WkCertificate

- (id)initWithData:(OBData *)data
{
	if ((self = [super init]))
	{
		_data = [data copy];
	}

	return self;
}

- (void)dealloc
{
	[_data release];
	[_subjectName release];
	[_issuerName release];
	[_sha release];
	[_serial release];
	[_algorithm release];
	[_validNotBefore release];
	[_validNotAfter release];
	[super dealloc];
}

- (OBData *)certificate
{
	return _data;
}

+ (void)initialize
{
	_store = X509_STORE_new();
	if (_store)
	{
		if (1 != X509_STORE_load_locations(_store, "MOSSYS:Data/SSL/curl-ca-bundle.crt", nullptr))
		{
			X509_STORE_free(_store);
			_store = nullptr;
		}
	}
}

+ (void)shutdown
{
	if (_store)
		X509_STORE_free(_store);
}

- (OBDictionary *)_dnToDictionary:(const char *)dnString
{
	if (dnString)
	{
		OBMutableDictionary *out = [OBMutableDictionary dictionaryWithCapacity:16];
		OBArray *pairs = [[OBString stringWithUTF8String:dnString + 1] componentsSeparatedByString:@"/"];
		OBEnumerator *e = [pairs objectEnumerator];
		OBString *pair;
		while ((pair = [e nextObject]))
		{
			OBArray *kv = [pair componentsSeparatedByString:@"="];
			if ([kv count] == 2)
				[out setObject:[kv lastObject] forKey:[kv firstObject]];
		}

		return out;
	}

	return nil;
}

- (OBString *)_serialFromCert:(X509 *)cert
{
	ASN1_INTEGER *serial = X509_get_serialNumber(cert);
	BIGNUM *bn = ASN1_INTEGER_to_BN(serial, NULL);
	if (!bn)
		return nil;

	char *tmp = BN_bn2dec(bn);
	if (tmp)
	{
		OBString *out = [OBString stringWithUTF8String:tmp];
		BN_free(bn);
		OPENSSL_free(tmp);
		return out;
	}

	BN_free(bn);
	return nil;
}

- (OBString *)_convertASN1TIME:(ASN1_TIME *)t
{
	int rc;
	char date[128];
	BIO *b = BIO_new(BIO_s_mem());

	if (b)
	{
		rc = ASN1_TIME_print(b, t);
		if (rc <= 0)
		{
			BIO_free(b);
			return nil;
		}
		
		rc = BIO_gets(b, date, 128);
		BIO_free(b);

		if (rc > 0)
		{
			return [OBString stringWithUTF8String:date];
		}
	}
	
	return nil;
}

- (X509 *)_cert
{
	if (_data)
	{
		const unsigned char *bytes = (const unsigned char *)[_data bytes];
		BIO* certBio = BIO_new(BIO_s_mem());
		if (certBio)
		{
			BIO_write(certBio, bytes, int([_data length]));
			X509* cert = PEM_read_bio_X509(certBio, NULL, NULL, NULL);
			BIO_free(certBio);
			return cert;
		}
	}
	
	return nullptr;
}

- (void)decode
{
	if (_data && !_decoded)
	{
		X509* cert = [self _cert];

		_decoded = YES;

		if (cert)
		{
			char nameBuffer[4096];

			if (X509_NAME_oneline(X509_get_subject_name(cert), nameBuffer, sizeof(nameBuffer)))
			{
				_subjectName = [[self _dnToDictionary:nameBuffer] retain];
			}

			if (X509_NAME_oneline(X509_get_issuer_name(cert), nameBuffer, sizeof(nameBuffer)))
			{
				_issuerName = [[self _dnToDictionary:nameBuffer] retain];
			}

			char *sha = static_cast<char *>(OBAlloc(SHALEN));
			if (sha)
			{
				const EVP_MD *digest = EVP_sha256();
				if (X509_digest(cert, digest, (unsigned char*) sha, NULL))
				{
					_sha = [[OBData dataWithBytesNoCopy:sha length:SHALEN freeWhenDone:YES] retain];
				}
				else
				{
					OBFree(sha);
				}
			}

			_version = ((int) X509_get_version(cert)) + 1;
			_serial = [[self _serialFromCert:cert] retain];

			int pkey_nid = X509_get_signature_nid(cert);
			if (pkey_nid != NID_undef)
			{
				const char* nidname = OBJ_nid2ln(pkey_nid);
				if (nidname && *nidname)
					_algorithm = [[OBString stringWithUTF8String:nidname] retain];
			}
			
			_validNotAfter = [[self _convertASN1TIME:X509_get_notAfter(cert)] retain];
			_validNotBefore = [[self _convertASN1TIME:X509_get_notBefore(cert)] retain];

			if (!_validOverride)
			{
				_valid = X509_check_ca(cert) > 0;
			}

			if (X509_cmp_current_time(X509_get_notAfter(cert)) != -1)
			{
				if (!_validOverride)
					_valid = NO;
				_isExpired = YES;
			}

			if (X509_cmp_current_time(X509_get_notBefore(cert)) != 1)
			{
				if (!_validOverride)
					_valid = NO;
				_isExpired = YES;
			}

			_selfSigned = X509_check_issued(cert, cert) == X509_V_OK;

			X509_free(cert);
		}
	}
}

- (BOOL)isRoot
{
	[self decode];
	return _valid && _selfSigned;
}

- (BOOL)isExpired
{
	[self decode];
	return _isExpired;
}

- (BOOL)isValid
{
	[self decode];
	return _valid;
}

- (void)setValid:(BOOL)valid
{
	_valid = valid;
	_validOverride = YES;
}

- (BOOL)isSelfSigned
{
	[self decode];
	return _selfSigned;
}

- (OBDictionary *)subjectName
{
	[self decode];
	return _subjectName;
}

- (OBDictionary *)issuerName
{
	[self decode];
	return _issuerName;
}

- (int)version
{
	[self decode];
	return _version;
}

- (OBString *)serialNumber
{
	[self decode];
	return _serial;
}

- (OBData *)sha256
{
	[self decode];
	return _sha;
}

static inline void hex_encode(const unsigned char* readbuf, void *writebuf, size_t len)
{
	for(size_t i=0; i < len; i++) {
		char *l = (char*) (2*i + ((intptr_t) writebuf));
		sprintf(l, "%02x", readbuf[i]);
	}
}

- (OBString *)sha256Hex
{
	OBData *sha = [self sha256];
	if ([sha length] == SHALEN)
	{
		char buffer[SHALEN * 2];
		hex_encode(static_cast<const unsigned char*>([sha bytes]), buffer, SHALEN);
		return [OBString stringWithCString:buffer length:(SHALEN * 2) encoding:MIBENUM_UTF_8];
	}
	
	return nil;
}

- (OBString *)algorithm
{
	[self decode];
	return _algorithm;
}

- (OBString *)notValidBefore
{
	[self decode];
	return _validNotBefore;
}

- (OBString *)notValidAfter
{
	[self decode];
	return _validNotAfter;
}

@end

@implementation WkCertificate

+ (WkCertificate *)certificateWithData:(OBData *)data
{
	return [[[_WkCertificate alloc] initWithData:data] autorelease];
}

+ (WkCertificate *)certificateWithData:(const char *)data length:(ULONG)length
{
	return [[[_WkCertificate alloc] initWithData:[OBData dataWithBytes:data length:length]] autorelease];
}

+ (void)shutdown
{
	[_WkCertificate shutdown];
}

- (OBData *)certificate
{
	return nil;
}

- (BOOL)isRoot
{
	return NO;
}

- (BOOL)isValid
{
	return NO;
}

- (BOOL)isExpired
{
	return YES;
}

- (void)setValid:(BOOL)valid
{
	(void)valid;
}

- (BOOL)isSelfSigned
{
	return YES;
}

- (OBDictionary *)subjectName
{
	return nil;
}

- (OBDictionary *)issuerName
{
	return nil;
}

- (int)version
{
	return 0;
}

- (OBString *)serialNumber
{
	return nil;
}

- (OBData *)sha256
{
	return nil;
}

- (OBString *)sha256Hex
{
	return nil;
}

- (OBString *)algorithm
{
	return nil;
}

- (OBString *)notValidBefore
{
	return nil;
}

- (OBString *)notValidAfter
{
	return nil;
}

@end

@implementation _WkCertificateChain

- (id)initWithCertificates:(OBArray *)certs
{
	if ((self = [super init]))
	{
		if ([certs count])
			_certificates = [certs retain];
	}
	
	return self;
}

- (void)dealloc
{
	[_certificates release];
	[super dealloc];
}

- (OBArray *)certificates
{
	return _certificates;
}

- (BOOL)verify:(OBError **)outerror
{
	if (outerror)
		*outerror = nil;

	ULONG chainCount = [_certificates count];

	if (chainCount > 1)
	{
		STACK_OF(X509)* x509_ca_stack = sk_X509_new_null();
		if (x509_ca_stack)
		{
			X509 *xcert;
			BOOL ok = YES;

			for (ULONG i = 0; i < [_certificates count]; i++)
			{
				_WkCertificate *wkcert = [_certificates objectAtIndex:i];
				xcert = [wkcert _cert];

				if (!xcert)
				{
					// failed
					ok = NO;
					if (outerror)
						*outerror = [OBError errorWithDomain:@"SSLVerification" code:X509_V_ERR_UNSPECIFIED userInfo:nil];
					break;
				}

				if (i < [_certificates count] - 1)
				{
					sk_X509_push(x509_ca_stack, xcert);
				}
			}

			if (ok)
			{
				X509_STORE_CTX *ctx = X509_STORE_CTX_new();
				if (ctx)
				{
					if (1 == X509_STORE_CTX_init(ctx, _store, xcert, x509_ca_stack))
					{
						int rc = X509_verify_cert(ctx);
						ok = rc == 1;
						if (!ok && outerror)
							*outerror = [OBError errorWithDomain:@"SSLVerification" code:X509_STORE_CTX_get_error(ctx) userInfo:nil];
						if (ok)
						{
							for (ULONG i = 0; i < [_certificates count]; i++)
							{
								_WkCertificate *wkcert = [_certificates objectAtIndex:i];
								[wkcert setValid:YES];
							}
						}
					}
					else
					{
						ok = NO;
						if (outerror)
							*outerror = [OBError errorWithDomain:@"SSLVerification" code:X509_V_ERR_OUT_OF_MEM userInfo:nil];
					}

					X509_STORE_CTX_free(ctx);
				}
				else
				{
					ok = NO;
					if (outerror)
						*outerror = [OBError errorWithDomain:@"SSLVerification" code:X509_V_ERR_OUT_OF_MEM userInfo:nil];
				}
			}
			
			if (xcert)
				X509_free(xcert);
			sk_X509_pop_free(x509_ca_stack, X509_free);
			return ok;
		}
	}
	
	return NO;
}

static int dnsnamecmp(const char *hostname, const char *match)
{
	if (match[0] == '*' && match[1] == '.')
	{
		int hlen = (int)strlen(hostname);
		int mlen = (int)strlen(match);
		// the "naked" domain is not valid
		if (hlen < mlen)
			return -1;
		// wildcard only covers one level of subdomains
		if (index(hostname, '.') < &hostname[hlen - mlen + 1])
			return -1;
		// compare suffix
		hostname += hlen - mlen + 1;
		match++;
	}
	return strcasecmp(hostname, match);
}

// based on https://github.com/iSECPartners/ssl-conservatory/blob/master/openssl/everything-you-wanted-to-know-about-openssl.pdf

typedef enum
{
        MatchFound,
        MatchNotFound,
        NoSANPresent,
        MalformedCertificate,
        Error
} HostnameValidationResult;

/*
* Tries to find a match for hostname in the certificate's Subject Alternative Name extension.
*
* Returns MatchFound if a match was found.
* Returns MatchNotFound if no matches were found.
* Returns MalformedCertificate if any of the hostnames had a NUL character embedded in it.
* Returns NoSANPresent if the SAN extension was not present in the certificate.
*/
static HostnameValidationResult matches_subject_alternative_name(const char *hostname, const X509 *server_cert)
{
	HostnameValidationResult result = MatchNotFound;
	int i;
	int san_names_nb = -1;
	STACK_OF(GENERAL_NAME) *san_names = NULL;

	// Try to extract the names within the SAN extension from the certificate
	san_names = (STACK_OF(GENERAL_NAME) *)X509_get_ext_d2i(server_cert, NID_subject_alt_name, 0, 0);
	if (!san_names)
	{
		return NoSANPresent;
	}
	san_names_nb = sk_GENERAL_NAME_num(san_names);
	// Check each name within the extension
	for (i=0; i<san_names_nb; i++)
	{
		const GENERAL_NAME *current_name = sk_GENERAL_NAME_value(san_names, i);
		if (current_name->type == GEN_DNS)
		{
			// Current name is a DNS name, let's check it
			const char *dns_name = (char *) ASN1_STRING_get0_data(current_name->d.dNSName);

			// Make sure there isn't an embedded NUL character in the DNS name
			if (ASN1_STRING_length(current_name->d.dNSName) != (int)strlen(dns_name))
			{
				result = MalformedCertificate;
				break;
			}
			else
			{
				// Compare expected hostname with the DNS name
				if (!dnsnamecmp(hostname, dns_name))
				{
						result = MatchFound;
						break;
				}
			}
		}
	}
	sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);
	return result;
}

/*
* Tries to find a match for hostname in the certificate's Common Name field.
*
* Returns MatchFound if a match was found.
* Returns MatchNotFound if no matches were found.
* Returns MalformedCertificate if the Common Name had a NUL character embedded in it.
* Returns Error if the Common Name could not be extracted.
*/
static HostnameValidationResult matches_common_name(const char *hostname, const X509 *server_cert)
{
	int common_name_loc = -1;
	X509_NAME_ENTRY *common_name_entry = NULL;
	ASN1_STRING *common_name_asn1 = NULL;
	const char *common_name_str;

	// Find the position of the CN field in the Subject field of the certificate
	common_name_loc = X509_NAME_get_index_by_NID(X509_get_subject_name((X509 *) server_cert), NID_commonName, -1);
	if (common_name_loc < 0)
	{
		return Error;
	}
	// Extract the CN field
	common_name_entry = X509_NAME_get_entry(X509_get_subject_name((X509 *) server_cert), common_name_loc);
	if (!common_name_entry)
	{
		return Error;
	}
	// Convert the CN field to a C string
	common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
	if (!common_name_asn1)
	{
		return Error;
	}
	common_name_str = (const char *) ASN1_STRING_get0_data(common_name_asn1);
	// Make sure there isn't an embedded NUL character in the CN
	if (ASN1_STRING_length(common_name_asn1) != (int)strlen(common_name_str))
	{
		return MalformedCertificate;
	}
	// Compare expected hostname with the CN
	if (!dnsnamecmp(hostname, common_name_str))
	{
		return MatchFound;
	}
	else
	{
		return MatchNotFound;
	}
}

/*
* Validates the server's identity by looking for the expected hostname in the
* server's certificate. As described in RFC 6125, it first tries to find a match
* in the Subject Alternative Name extension. If the extension is not present in
* the certificate, it checks the Common Name instead.
*
* Returns MatchFound if a match was found.
* Returns MatchNotFound if no matches were found.
* Returns MalformedCertificate if any of the hostnames had a NUL character embedded in it.
* Returns Error if there was an error.
*/
static HostnameValidationResult validate_hostname(const char *hostname, const X509 *server_cert)
{
	HostnameValidationResult result;

	if (!hostname || !server_cert)
		return Error;

	// First try the Subject Alternative Names extension
	result = matches_subject_alternative_name(hostname, server_cert);
	if (result == NoSANPresent)
	{
		// Extension was not found: try the Common Name
		result = matches_common_name(hostname, server_cert);
	}
	return result;
}

- (BOOL)verifyForHost:(OBString *)hostname error:(OBError **)outerror
{
	if (outerror)
		*outerror = nil;

	if (![self verify:outerror])
		return NO;
		
	_WkCertificate *lastCertificate = (_WkCertificate *)[_certificates lastObject];
	if ([lastCertificate isKindOfClass:[_WkCertificate class]])
	{
		auto *cert = [lastCertificate _cert];
		if (cert && MatchFound == validate_hostname([hostname cString], cert))
		{
			return YES;
		}

		if (outerror)
			*outerror = [OBError errorWithDomain:@"SSLVerification" code:X509_V_ERR_HOSTNAME_MISMATCH userInfo:nil];
		return NO;
	}

	return NO;
}

@end

@implementation WkCertificateChain : OBObject

+ (WkCertificateChain *)certificateChainWithCertificates:(OBArray *)certificates
{
	return [[[_WkCertificateChain alloc] initWithCertificates:certificates] autorelease];
}

+ (WkCertificateChain *)certificateChainWithCertificate:(WkCertificate *)cert
{
	return [[[_WkCertificateChain alloc] initWithCertificates:[OBArray arrayWithObject:cert]] autorelease];
}

- (OBArray *)certificates
{
	return nil;
}

- (BOOL)verify:(OBError **)outerror
{
	if (outerror)
		*outerror = nil;
	return NO;
}

- (BOOL)verifyForHost:(OBString *)hostname error:(OBError **)outerror
{
	(void)hostname;
	if (outerror)
		*outerror = nil;
	return NO;
}

+ (OBString *)validationErrorStringForErrorCode:(int)e
{
	switch ((int) e)
	{
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
			return @"ERR_UNABLE_TO_GET_ISSUER_CERT";
		case X509_V_ERR_UNABLE_TO_GET_CRL:
			return @"ERR_UNABLE_TO_GET_CRL";
		case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
			return @"ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE";
		case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
			return @"ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE";
		case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
			return @"ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY";
		case X509_V_ERR_CERT_SIGNATURE_FAILURE:
			return @"ERR_CERT_SIGNATURE_FAILURE";
		case X509_V_ERR_CRL_SIGNATURE_FAILURE:
			return @"ERR_CRL_SIGNATURE_FAILURE";
		case X509_V_ERR_CERT_NOT_YET_VALID:
			return @"ERR_CERT_NOT_YET_VALID";
		case X509_V_ERR_CERT_HAS_EXPIRED:
			return @"ERR_CERT_HAS_EXPIRED";
		case X509_V_ERR_CRL_NOT_YET_VALID:
			return @"ERR_CRL_NOT_YET_VALID";
		case X509_V_ERR_CRL_HAS_EXPIRED:
			return @"ERR_CRL_HAS_EXPIRED";
		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
			return @"ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD";
		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
			return @"ERR_ERROR_IN_CERT_NOT_AFTER_FIELD";
		case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
			return @"ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD";
		case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
			return @"ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD";
		case X509_V_ERR_OUT_OF_MEM:
			return @"ERR_OUT_OF_MEM";
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
			return @"ERR_DEPTH_ZERO_SELF_SIGNED_CERT";
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
			return @"ERR_SELF_SIGNED_CERT_IN_CHAIN";
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
			return @"ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY";
		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
			return @"ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE";
		case X509_V_ERR_CERT_CHAIN_TOO_LONG:
			return @"ERR_CERT_CHAIN_TOO_LONG";
		case X509_V_ERR_CERT_REVOKED:
			return @"ERR_CERT_REVOKED";
		case X509_V_ERR_INVALID_CA:
			return @"ERR_INVALID_CA";
		case X509_V_ERR_PATH_LENGTH_EXCEEDED:
			return @"ERR_PATH_LENGTH_EXCEEDED";
		case X509_V_ERR_INVALID_PURPOSE:
			return @"ERR_INVALID_PURPOSE";
		case X509_V_ERR_CERT_UNTRUSTED:
			return @"ERR_CERT_UNTRUSTED";
		case X509_V_ERR_CERT_REJECTED:
			return @"ERR_CERT_REJECTED";
		case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
			return @"ERR_SUBJECT_ISSUER_MISMATCH";
		case X509_V_ERR_AKID_SKID_MISMATCH:
			return @"ERR_AKID_SKID_MISMATCH";
		case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
			return @"ERR_AKID_ISSUER_SERIAL_MISMATCH";
		case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
			return @"ERR_KEYUSAGE_NO_CERTSIGN";
		case X509_V_ERR_INVALID_EXTENSION:
			return @"ERR_INVALID_EXTENSION";
		case X509_V_ERR_INVALID_POLICY_EXTENSION:
			return @"ERR_INVALID_POLICY_EXTENSION";
		case X509_V_ERR_NO_EXPLICIT_POLICY:
			return @"ERR_NO_EXPLICIT_POLICY";
		case X509_V_ERR_APPLICATION_VERIFICATION:
			return @"ERR_APPLICATION_VERIFICATION";
		case X509_V_ERR_INVALID_CALL:
			return @"ERR_INVALID_CALL";
		case X509_V_ERR_HOSTNAME_MISMATCH:
			return @"ERR_HOSTNAME_MISMATCH";
		default:
			return @"ERR_UNKNOWN";
	}
}

@end
