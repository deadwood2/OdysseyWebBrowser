#import <mui/MUIGroup.h>
#import "WkCertificate.h"

@class MCCListtree, MUIText, MUIList, OBDictionary;

// 'Name:'
#define WkCertificateVerifier_Localization_Name @"wfCertName"
// 'Issued By:'
#define WkCertificateVerifier_Localization_Issued @"wfCertIssuedBy"
// 'Expires:'
#define WkCertificateVerifier_Localization_Expires @"wfCertExpires"
// 'This certificate is valid.'
#define WkCertificateVerifier_Localization_Valid @"wfCertValid"
// 'This certificate is NOT valid.'
#define WkCertificateVerifier_Localization_NotValid @"wfCertError"
// 'This certificate does NOT match the hostname.'
#define WkCertificateVerifier_Localization_NoHostMatch @"wfCertHostError"
// 'This certificate is expired.'
#define WkCertificateVerifier_Localization_Expired @"wfCertExpired"

@interface WkCertificateVerifier : MUIGroup
{
	WkCertificateChain *_certificateChain;
	OBString           *_host;
	OBDictionary       *_localization;
	MCCListtree        *_tree;
	MUIText            *_logo;
	MUIText            *_name;
	MUIText            *_issuedBy;
	MUIText            *_expires;
	MUIList            *_details;
	MUIText            *_valid;
	BOOL                _matchesHost;
}

+ (WkCertificateVerifier *)verifierForCertificateChain:(WkCertificateChain *)chain;
+ (WkCertificateVerifier *)verifierForCertificateChain:(WkCertificateChain *)chain host:(OBString *)host localizationDictionary:(OBDictionary *)localization;

- (WkCertificateChain *)certificateChain;

// Allows overloading the event, must call super to allow for regular processing
- (void)onCertificateSelected:(WkCertificate *)certificate;

@end
