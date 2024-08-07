/*
 * Copyright (C) 2016-2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebPaymentCoordinatorProxyCocoa.h"

#if ENABLE(APPLE_PAY)

#import "WebPaymentCoordinatorProxy.h"
#import "WebProcessPool.h"
#import <WebCore/PaymentAuthorizationStatus.h>
#import <WebCore/PaymentHeaders.h>
#import <WebCore/URL.h>
#import <pal/spi/cocoa/PassKitSPI.h>
#import <wtf/BlockPtr.h>
#import <wtf/RunLoop.h>
#import <wtf/SoftLinking.h>

#if PLATFORM(MAC)
SOFT_LINK_PRIVATE_FRAMEWORK_OPTIONAL(PassKit)
#else
SOFT_LINK_FRAMEWORK(PassKit)
#endif

SOFT_LINK_CLASS(PassKit, PKPassLibrary);
SOFT_LINK_CLASS(PassKit, PKPaymentAuthorizationViewController);
SOFT_LINK_CLASS(PassKit, PKPaymentMerchantSession);
SOFT_LINK_CLASS(PassKit, PKPaymentRequest);
SOFT_LINK_CLASS(PassKit, PKPaymentSummaryItem);
SOFT_LINK_CLASS(PassKit, PKShippingMethod);

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
SOFT_LINK_FRAMEWORK(Contacts)
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressStreetKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressSubLocalityKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressCityKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressSubAdministrativeAreaKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressStateKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressPostalCodeKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressCountryKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressISOCountryCodeKey, NSString *);
SOFT_LINK_CLASS(PassKit, PKPaymentAuthorizationResult)
SOFT_LINK_CLASS(PassKit, PKPaymentRequestPaymentMethodUpdate)
SOFT_LINK_CLASS(PassKit, PKPaymentRequestShippingContactUpdate)
SOFT_LINK_CLASS(PassKit, PKPaymentRequestShippingMethodUpdate)
SOFT_LINK_CONSTANT(PassKit, PKPaymentErrorDomain, NSString *);
SOFT_LINK_CONSTANT(PassKit, PKContactFieldPostalAddress, NSString *);
SOFT_LINK_CONSTANT(PassKit, PKContactFieldEmailAddress, NSString *);
SOFT_LINK_CONSTANT(PassKit, PKContactFieldPhoneNumber, NSString *);
SOFT_LINK_CONSTANT(PassKit, PKContactFieldName, NSString *);
SOFT_LINK_CONSTANT(PassKit, PKContactFieldPhoneticName, NSString *);
SOFT_LINK_CONSTANT(PassKit, PKPaymentErrorContactFieldUserInfoKey, NSString *);
SOFT_LINK_CONSTANT(PassKit, PKPaymentErrorPostalAddressUserInfoKey, NSString *);
#endif

typedef void (^PKCanMakePaymentsCompletion)(BOOL isValid, NSError *error);
#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
extern "C" void PKCanMakePaymentsWithMerchantIdentifierDomainAndSourceApplication(NSString *identifier, NSString *domain, NSString *sourceApplicationSecondaryIdentifier, PKCanMakePaymentsCompletion completion);
SOFT_LINK_FUNCTION_MAY_FAIL_FOR_SOURCE(WebKit, PassKit, PKCanMakePaymentsWithMerchantIdentifierDomainAndSourceApplication, void, (NSString *identifier, NSString *domain, NSString *sourceApplicationSecondaryIdentifier, PKCanMakePaymentsCompletion completion), (identifier, domain, sourceApplicationSecondaryIdentifier, completion));
#else
extern "C" void PKCanMakePaymentsWithMerchantIdentifierAndDomain(NSString *identifier, NSString *domain, PKCanMakePaymentsCompletion completion);
SOFT_LINK_FUNCTION_MAY_FAIL_FOR_SOURCE(WebKit, PassKit, PKCanMakePaymentsWithMerchantIdentifierAndDomain, void, (NSString *identifier, NSString *domain, PKCanMakePaymentsCompletion completion), (identifier, domain, completion));
#endif


@implementation WKPaymentAuthorizationViewControllerDelegate

- (instancetype)initWithPaymentCoordinatorProxy:(WebKit::WebPaymentCoordinatorProxy&)webPaymentCoordinatorProxy
{
    if (!(self = [super init]))
        return nullptr;

    _webPaymentCoordinatorProxy = &webPaymentCoordinatorProxy;

    return self;
}

- (void)invalidate
{
    _webPaymentCoordinatorProxy = nullptr;
    if (_paymentAuthorizedCompletion) {
#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
        _paymentAuthorizedCompletion(adoptNS([allocPKPaymentAuthorizationResultInstance() initWithStatus:PKPaymentAuthorizationStatusFailure errors:@[ ]]).get());
#else
        _paymentAuthorizedCompletion(PKPaymentAuthorizationStatusFailure);
#endif
        _paymentAuthorizedCompletion = nullptr;
    }
}

- (void)paymentAuthorizationViewController:(PKPaymentAuthorizationViewController *)controller willFinishWithError:(NSError *)error
{
}

- (void)paymentAuthorizationViewController:(PKPaymentAuthorizationViewController *)controller didRequestMerchantSession:(void(^)(PKPaymentMerchantSession *, NSError *))sessionBlock
{
    ASSERT(!_sessionBlock);
    _sessionBlock = sessionBlock;

    [getPKPaymentAuthorizationViewControllerClass() paymentServicesMerchantURL:^(NSURL *merchantURL, NSError *error) {
        if (error)
            LOG_ERROR("PKCanMakePaymentsWithMerchantIdentifierAndDomain error %@", error);

        dispatch_async(dispatch_get_main_queue(), ^{
            ASSERT(_sessionBlock);

            if (!_webPaymentCoordinatorProxy) {
                _sessionBlock(nullptr, nullptr);
                return;
            }

            _webPaymentCoordinatorProxy->validateMerchant(merchantURL);
        });
    }];
}

static WebCore::ApplePaySessionPaymentRequest::ShippingMethod toShippingMethod(PKShippingMethod *shippingMethod)
{
    ASSERT(shippingMethod);

    WebCore::ApplePaySessionPaymentRequest::ShippingMethod result;
    result.label = shippingMethod.label;
    result.detail = shippingMethod.detail;
    result.amount = shippingMethod.amount.stringValue;
    result.identifier = shippingMethod.identifier;

    return result;
}

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000)

- (void)paymentAuthorizationViewController:(PKPaymentAuthorizationViewController *)controller didAuthorizePayment:(PKPayment *)payment handler:(void (^)(PKPaymentAuthorizationResult *result))completion
{
#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
    if (!_webPaymentCoordinatorProxy) {
        completion(adoptNS([allocPKPaymentAuthorizationResultInstance() initWithStatus:PKPaymentAuthorizationStatusFailure errors:@[ ]]).get());
        return;
    }

    ASSERT(!_paymentAuthorizedCompletion);
    _paymentAuthorizedCompletion = completion;

    _webPaymentCoordinatorProxy->didAuthorizePayment(WebCore::Payment(payment));
#endif
}

- (void)paymentAuthorizationViewController:(PKPaymentAuthorizationViewController *)controller didSelectPaymentMethod:(PKPaymentMethod *)paymentMethod handler:(void (^)(PKPaymentRequestPaymentMethodUpdate *update))completion
{
#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
    if (!_webPaymentCoordinatorProxy) {
        completion(adoptNS([allocPKPaymentRequestPaymentMethodUpdateInstance() initWithPaymentSummaryItems:@[ ]]).get());
        return;
    }

    ASSERT(!_didSelectPaymentMethodCompletion);
    _didSelectPaymentMethodCompletion = completion;
    _webPaymentCoordinatorProxy->didSelectPaymentMethod(WebCore::PaymentMethod(paymentMethod));
#endif
}

- (void)paymentAuthorizationViewController:(PKPaymentAuthorizationViewController *)controller didSelectShippingMethod:(PKShippingMethod *)shippingMethod handler:(void (^)(PKPaymentRequestShippingMethodUpdate *update))completion {
#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
    if (!_webPaymentCoordinatorProxy) {
        completion(adoptNS([allocPKPaymentRequestShippingMethodUpdateInstance() initWithPaymentSummaryItems:@[ ]]).get());
        return;
    }

    ASSERT(!_didSelectShippingMethodCompletion);
    _didSelectShippingMethodCompletion = completion;
    _webPaymentCoordinatorProxy->didSelectShippingMethod(toShippingMethod(shippingMethod));
#endif
}

- (void)paymentAuthorizationViewController:(PKPaymentAuthorizationViewController *)controller didSelectShippingContact:(PKContact *)contact handler:(void (^)(PKPaymentRequestShippingContactUpdate *update))completion
{
#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
    if (!_webPaymentCoordinatorProxy) {
        completion(adoptNS([allocPKPaymentRequestShippingContactUpdateInstance() initWithErrors:@[ ] paymentSummaryItems:@[ ] shippingMethods:@[ ]]).get());
        return;
    }

    ASSERT(!_didSelectShippingContactCompletion);
    _didSelectShippingContactCompletion = completion;
    _webPaymentCoordinatorProxy->didSelectShippingContact(WebCore::PaymentContact(contact));
#endif
}

#endif

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED < 110000)

- (void)paymentAuthorizationViewController:(PKPaymentAuthorizationViewController *)controller didAuthorizePayment:(PKPayment *)payment completion:(void (^)(PKPaymentAuthorizationStatus))completion
{
    if (!_webPaymentCoordinatorProxy) {
        completion(PKPaymentAuthorizationStatusFailure);
        return;
    }

    ASSERT(!_paymentAuthorizedCompletion);
    _paymentAuthorizedCompletion = completion;

    _webPaymentCoordinatorProxy->didAuthorizePayment(WebCore::Payment(payment));
}

- (void)paymentAuthorizationViewController:(PKPaymentAuthorizationViewController *)controller didSelectShippingMethod:(PKShippingMethod *)shippingMethod completion:(void (^)(PKPaymentAuthorizationStatus status, NSArray<PKPaymentSummaryItem *> *summaryItems))completion
{
    if (!_webPaymentCoordinatorProxy) {
        completion(PKPaymentAuthorizationStatusFailure, @[ ]);
        return;
    }

    ASSERT(!_didSelectShippingMethodCompletion);
    _didSelectShippingMethodCompletion = completion;
    _webPaymentCoordinatorProxy->didSelectShippingMethod(toShippingMethod(shippingMethod));
}

- (void)paymentAuthorizationViewController:(PKPaymentAuthorizationViewController *)controller didSelectPaymentMethod:(PKPaymentMethod *)paymentMethod completion:(void (^)(NSArray<PKPaymentSummaryItem *> *summaryItems))completion
{
    if (!_webPaymentCoordinatorProxy) {
        completion(@[ ]);
        return;
    }

    ASSERT(!_didSelectPaymentMethodCompletion);
    _didSelectPaymentMethodCompletion = completion;

    _webPaymentCoordinatorProxy->didSelectPaymentMethod(WebCore::PaymentMethod(paymentMethod));
}

- (void)paymentAuthorizationViewController:(PKPaymentAuthorizationViewController *)controller didSelectShippingContact:(PKContact *)contact completion:(void (^)(PKPaymentAuthorizationStatus status, NSArray<PKShippingMethod *> *shippingMethods, NSArray<PKPaymentSummaryItem *> *summaryItems))completion
{
    if (!_webPaymentCoordinatorProxy) {
        completion(PKPaymentAuthorizationStatusFailure, @[ ], @[ ]);
        return;
    }

    ASSERT(!_didSelectShippingContactCompletion);
    _didSelectShippingContactCompletion = completion;
    _webPaymentCoordinatorProxy->didSelectShippingContact(WebCore::PaymentContact(contact));
}

#endif

- (void)paymentAuthorizationViewControllerDidFinish:(PKPaymentAuthorizationViewController *)controller
{
    if (!_webPaymentCoordinatorProxy)
        return;

    if (!_didReachFinalState)
        _webPaymentCoordinatorProxy->didCancelPaymentSession();

    _webPaymentCoordinatorProxy->hidePaymentUI();
}

@end

// FIXME: Once rdar://problem/24420024 has been fixed, import PKPaymentRequest_Private.h instead.
@interface PKPaymentRequest ()
@property (nonatomic, retain) NSURL *originatingURL;
@end

@interface PKPaymentRequest ()
// FIXME: Remove this once it's in an SDK.
@property (nonatomic, strong) NSArray *thumbnailURLs;
@property (nonatomic, strong) NSURL *thumbnailURL;

@property (nonatomic, assign) BOOL expectsMerchantSession;
@end

namespace WebKit {

bool WebPaymentCoordinatorProxy::platformCanMakePayments()
{
#if PLATFORM(MAC)
    if (!PassKitLibrary())
        return false;
#endif

    return [getPKPaymentAuthorizationViewControllerClass() canMakePayments];
}

void WebPaymentCoordinatorProxy::platformCanMakePaymentsWithActiveCard(const String& merchantIdentifier, const String& domainName, WTF::Function<void (bool)>&& completionHandler)
{
#if PLATFORM(MAC)
    if (!PassKitLibrary()) {
        completionHandler(false);
        return;
    }
#endif

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
    if (!canLoad_PassKit_PKCanMakePaymentsWithMerchantIdentifierDomainAndSourceApplication()) {
        RunLoop::main().dispatch([completionHandler = WTFMove(completionHandler)] {
            completionHandler(false);
        });
        return;
    }

    softLink_PassKit_PKCanMakePaymentsWithMerchantIdentifierDomainAndSourceApplication(merchantIdentifier, domainName, m_webPageProxy.process().processPool().configuration().sourceApplicationSecondaryIdentifier(), BlockPtr<void (BOOL, NSError *)>::fromCallable([completionHandler = WTFMove(completionHandler)](BOOL canMakePayments, NSError *error) mutable {
        if (error)
            LOG_ERROR("PKCanMakePaymentsWithMerchantIdentifierAndDomain error %@", error);

        RunLoop::main().dispatch([completionHandler = WTFMove(completionHandler), canMakePayments] {
            completionHandler(canMakePayments);
        });
    }).get());
#else
    if (!canLoad_PassKit_PKCanMakePaymentsWithMerchantIdentifierAndDomain()) {
        RunLoop::main().dispatch([completionHandler = WTFMove(completionHandler)] {
            completionHandler(false);
        });
        return;
    }

    softLink_PassKit_PKCanMakePaymentsWithMerchantIdentifierAndDomain(merchantIdentifier, domainName, BlockPtr<void (BOOL, NSError *)>::fromCallable([completionHandler = WTFMove(completionHandler)](BOOL canMakePayments, NSError *error) mutable {
        if (error)
            LOG_ERROR("PKCanMakePaymentsWithMerchantIdentifierAndDomain error %@", error);

        RunLoop::main().dispatch([completionHandler = WTFMove(completionHandler), canMakePayments] {
            completionHandler(canMakePayments);
        });
    }).get());
#endif
}

void WebPaymentCoordinatorProxy::platformOpenPaymentSetup(const String& merchantIdentifier, const String& domainName, WTF::Function<void (bool)>&& completionHandler)
{
#if PLATFORM(MAC)
    if (!PassKitLibrary()) {
        completionHandler(false);
        return;
    }
#endif

    auto passLibrary = adoptNS([allocPKPassLibraryInstance() init]);
    [passLibrary openPaymentSetupForMerchantIdentifier:merchantIdentifier domain:domainName completion:BlockPtr<void (BOOL)>::fromCallable([completionHandler = WTFMove(completionHandler)](BOOL result) mutable {
        RunLoop::main().dispatch([completionHandler = WTFMove(completionHandler), result] {
            completionHandler(result);
        });
    }).get()];
}

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
static RetainPtr<NSSet> toPKContactFields(const WebCore::ApplePaySessionPaymentRequest::ContactFields& contactFields)
{
    Vector<NSString *> result;

    if (contactFields.postalAddress)
        result.append(getPKContactFieldPostalAddress());
    if (contactFields.phone)
        result.append(getPKContactFieldPhoneNumber());
    if (contactFields.email)
        result.append(getPKContactFieldEmailAddress());
    if (contactFields.name)
        result.append(getPKContactFieldName());
    if (contactFields.phoneticName)
        result.append(getPKContactFieldPhoneticName());

    return adoptNS([[NSSet alloc] initWithObjects:result.data() count:result.size()]);
}
#else
static PKAddressField toPKAddressField(const WebCore::ApplePaySessionPaymentRequest::ContactFields& contactFields)
{
    PKAddressField result = 0;

    if (contactFields.postalAddress)
        result |= PKAddressFieldPostalAddress;
    if (contactFields.phone)
        result |= PKAddressFieldPhone;
    if (contactFields.email)
        result |= PKAddressFieldEmail;
    if (contactFields.name)
        result |= PKAddressFieldName;

    return result;
}
#endif

static PKPaymentSummaryItemType toPKPaymentSummaryItemType(WebCore::ApplePaySessionPaymentRequest::LineItem::Type type)
{
    switch (type) {
    case WebCore::ApplePaySessionPaymentRequest::LineItem::Type::Final:
        return PKPaymentSummaryItemTypeFinal;

    case WebCore::ApplePaySessionPaymentRequest::LineItem::Type::Pending:
        return PKPaymentSummaryItemTypePending;
    }
}

static NSDecimalNumber *toDecimalNumber(const String& amount)
{
    if (!amount)
        return [NSDecimalNumber zero];
    return [NSDecimalNumber decimalNumberWithString:amount locale:@{ NSLocaleDecimalSeparator : @"." }];
}

static RetainPtr<PKPaymentSummaryItem> toPKPaymentSummaryItem(const WebCore::ApplePaySessionPaymentRequest::LineItem& lineItem)
{
    return [getPKPaymentSummaryItemClass() summaryItemWithLabel:lineItem.label amount:toDecimalNumber(lineItem.amount) type:toPKPaymentSummaryItemType(lineItem.type)];
}

static RetainPtr<NSArray> toPKPaymentSummaryItems(const WebCore::ApplePaySessionPaymentRequest::TotalAndLineItems& totalAndLineItems)
{
    auto paymentSummaryItems = adoptNS([[NSMutableArray alloc] init]);
    for (auto& lineItem : totalAndLineItems.lineItems) {
        if (auto summaryItem = toPKPaymentSummaryItem(lineItem))
            [paymentSummaryItems addObject:summaryItem.get()];
    }

    if (auto totalItem = toPKPaymentSummaryItem(totalAndLineItems.total))
        [paymentSummaryItems addObject:totalItem.get()];

    return paymentSummaryItems;
}

static PKMerchantCapability toPKMerchantCapabilities(const WebCore::ApplePaySessionPaymentRequest::MerchantCapabilities& merchantCapabilities)
{
    PKMerchantCapability result = 0;
    if (merchantCapabilities.supports3DS)
        result |= PKMerchantCapability3DS;
    if (merchantCapabilities.supportsEMV)
        result |= PKMerchantCapabilityEMV;
    if (merchantCapabilities.supportsCredit)
        result |= PKMerchantCapabilityCredit;
    if (merchantCapabilities.supportsDebit)
        result |= PKMerchantCapabilityDebit;

    return result;
}

static RetainPtr<NSArray> toSupportedNetworks(const Vector<String>& supportedNetworks)
{
    auto result = adoptNS([[NSMutableArray alloc] initWithCapacity:supportedNetworks.size()]);
    for (auto& supportedNetwork : supportedNetworks)
        [result addObject:supportedNetwork];
    return result;
}

static PKShippingType toPKShippingType(WebCore::ApplePaySessionPaymentRequest::ShippingType shippingType)
{
    switch (shippingType) {
    case WebCore::ApplePaySessionPaymentRequest::ShippingType::Shipping:
        return PKShippingTypeShipping;

    case WebCore::ApplePaySessionPaymentRequest::ShippingType::Delivery:
        return PKShippingTypeDelivery;

    case WebCore::ApplePaySessionPaymentRequest::ShippingType::StorePickup:
        return PKShippingTypeStorePickup;

    case WebCore::ApplePaySessionPaymentRequest::ShippingType::ServicePickup:
        return PKShippingTypeServicePickup;
    }
}

static RetainPtr<PKShippingMethod> toPKShippingMethod(const WebCore::ApplePaySessionPaymentRequest::ShippingMethod& shippingMethod)
{
    RetainPtr<PKShippingMethod> result = [getPKShippingMethodClass() summaryItemWithLabel:shippingMethod.label amount:toDecimalNumber(shippingMethod.amount)];
    [result setIdentifier:shippingMethod.identifier];
    [result setDetail:shippingMethod.detail];

    return result;
}
    
#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
static RetainPtr<NSSet> toNSSet(const Vector<String>& strings)
{
    if (strings.isEmpty())
        return nil;

    auto mutableSet = adoptNS([[NSMutableSet alloc] initWithCapacity:strings.size()]);
    for (auto& string : strings)
        [mutableSet addObject:string];

    return WTFMove(mutableSet);
}
#endif

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300 && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101304) \
    || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110300 && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110300)
static PKPaymentRequestAPIType toAPIType(WebCore::ApplePaySessionPaymentRequest::Requester requester)
{
    switch (requester) {
    case WebCore::ApplePaySessionPaymentRequest::Requester::ApplePayJS:
        return PKPaymentRequestAPITypeWebJS;
    case WebCore::ApplePaySessionPaymentRequest::Requester::PaymentRequest:
        return PKPaymentRequestAPITypeWebPaymentRequest;
    }
}
#endif

RetainPtr<PKPaymentRequest> toPKPaymentRequest(WebPageProxy& webPageProxy, const WebCore::URL& originatingURL, const Vector<WebCore::URL>& linkIconURLs, const WebCore::ApplePaySessionPaymentRequest& paymentRequest)
{
    auto result = adoptNS([allocPKPaymentRequestInstance() init]);

    [result setOriginatingURL:originatingURL];

    if ([result respondsToSelector:@selector(setThumbnailURLs:)]) {
        auto thumbnailURLs = adoptNS([[NSMutableArray alloc] init]);
        for (auto& linkIconURL : linkIconURLs)
            [thumbnailURLs addObject:static_cast<NSURL *>(linkIconURL)];

        [result setThumbnailURLs:thumbnailURLs.get()];
    } else if (!linkIconURLs.isEmpty())
        [result setThumbnailURL:linkIconURLs[0]];

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300 && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101304) \
    || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110300 && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110300)
    [result setAPIType:toAPIType(paymentRequest.requester())];
#endif

    [result setCountryCode:paymentRequest.countryCode()];
    [result setCurrencyCode:paymentRequest.currencyCode()];
    [result setBillingContact:paymentRequest.billingContact().pkContact()];
    [result setShippingContact:paymentRequest.shippingContact().pkContact()];
#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
    [result setRequiredBillingContactFields:toPKContactFields(paymentRequest.requiredBillingContactFields()).get()];
    [result setRequiredShippingContactFields:toPKContactFields(paymentRequest.requiredShippingContactFields()).get()];
#else
    [result setRequiredBillingAddressFields:toPKAddressField(paymentRequest.requiredBillingContactFields())];
    [result setRequiredShippingAddressFields:toPKAddressField(paymentRequest.requiredShippingContactFields())];
#endif

    [result setSupportedNetworks:toSupportedNetworks(paymentRequest.supportedNetworks()).get()];
    [result setMerchantCapabilities:toPKMerchantCapabilities(paymentRequest.merchantCapabilities())];

    [result setShippingType:toPKShippingType(paymentRequest.shippingType())];

    auto shippingMethods = adoptNS([[NSMutableArray alloc] init]);
    for (auto& shippingMethod : paymentRequest.shippingMethods())
        [shippingMethods addObject:toPKShippingMethod(shippingMethod).get()];
    [result setShippingMethods:shippingMethods.get()];

    auto paymentSummaryItems = adoptNS([[NSMutableArray alloc] init]);
    for (auto& lineItem : paymentRequest.lineItems()) {
        if (auto summaryItem = toPKPaymentSummaryItem(lineItem))
            [paymentSummaryItems addObject:summaryItem.get()];
    }

    if (auto totalItem = toPKPaymentSummaryItem(paymentRequest.total()))
        [paymentSummaryItems addObject:totalItem.get()];

    [result setPaymentSummaryItems:paymentSummaryItems.get()];

    [result setExpectsMerchantSession:YES];

    if (!paymentRequest.applicationData().isNull()) {
        auto applicationData = adoptNS([[NSData alloc] initWithBase64EncodedString:paymentRequest.applicationData() options:0]);
        [result setApplicationData:applicationData.get()];
    }

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
    [result setSupportedCountries:toNSSet(paymentRequest.supportedCountries()).get()];
#endif

    // FIXME: Instead of using respondsToSelector, this should use a proper #if version check.
    auto& configuration = webPageProxy.process().processPool().configuration();

    if (!configuration.sourceApplicationBundleIdentifier().isEmpty() && [result respondsToSelector:@selector(setSourceApplicationBundleIdentifier:)])
        [result setSourceApplicationBundleIdentifier:configuration.sourceApplicationBundleIdentifier()];

    if (!configuration.sourceApplicationSecondaryIdentifier().isEmpty() && [result respondsToSelector:@selector(setSourceApplicationSecondaryIdentifier:)])
        [result setSourceApplicationSecondaryIdentifier:configuration.sourceApplicationSecondaryIdentifier()];

#if PLATFORM(IOS)
    if (!configuration.ctDataConnectionServiceType().isEmpty() && [result respondsToSelector:@selector(setCTDataConnectionServiceType:)])
        [result setCTDataConnectionServiceType:configuration.ctDataConnectionServiceType()];
#endif

    return result;
}

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
static PKPaymentAuthorizationStatus toPKPaymentAuthorizationStatus(WebCore::PaymentAuthorizationStatus status)
{
    switch (status) {
    case WebCore::PaymentAuthorizationStatus::Success:
        return PKPaymentAuthorizationStatusSuccess;
    case WebCore::PaymentAuthorizationStatus::Failure:
        return PKPaymentAuthorizationStatusFailure;
    case WebCore::PaymentAuthorizationStatus::PINRequired:
        return PKPaymentAuthorizationStatusPINRequired;
    case WebCore::PaymentAuthorizationStatus::PINIncorrect:
        return PKPaymentAuthorizationStatusPINIncorrect;
    case WebCore::PaymentAuthorizationStatus::PINLockout:
        return PKPaymentAuthorizationStatusPINLockout;
    }
}

static PKPaymentErrorCode toPKPaymentErrorCode(WebCore::PaymentError::Code code)
{
    switch (code) {
    case WebCore::PaymentError::Code::Unknown:
        return PKPaymentUnknownError;
    case WebCore::PaymentError::Code::ShippingContactInvalid:
        return PKPaymentShippingContactInvalidError;
    case WebCore::PaymentError::Code::BillingContactInvalid:
        return PKPaymentBillingContactInvalidError;
    case WebCore::PaymentError::Code::AddressUnserviceable:
        return PKPaymentShippingAddressUnserviceableError;
    }
}

static RetainPtr<NSError> toNSError(const WebCore::PaymentError& error)
{
    auto userInfo = adoptNS([[NSMutableDictionary alloc] init]);
    [userInfo setObject:error.message forKey:NSLocalizedDescriptionKey];

    if (error.contactField) {
        NSString *pkContactField = nil;
        NSString *postalAddressKey = nil;

        switch (*error.contactField) {
        case WebCore::PaymentError::ContactField::PhoneNumber:
            pkContactField = getPKContactFieldPhoneNumber();
            break;

        case WebCore::PaymentError::ContactField::EmailAddress:
            pkContactField = getPKContactFieldEmailAddress();
            break;

        case WebCore::PaymentError::ContactField::Name:
            pkContactField = getPKContactFieldName();
            break;

        case WebCore::PaymentError::ContactField::PhoneticName:
            pkContactField = getPKContactFieldPhoneticName();
            break;

        case WebCore::PaymentError::ContactField::PostalAddress:
            pkContactField = getPKContactFieldPostalAddress();
            break;

        case WebCore::PaymentError::ContactField::AddressLines:
            pkContactField = getPKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressStreetKey();
            break;

        case WebCore::PaymentError::ContactField::SubLocality:
            pkContactField = getPKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressSubLocalityKey();
            break;

        case WebCore::PaymentError::ContactField::Locality:
            pkContactField = getPKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressCityKey();
            break;

        case WebCore::PaymentError::ContactField::PostalCode:
            pkContactField = getPKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressPostalCodeKey();
            break;

        case WebCore::PaymentError::ContactField::SubAdministrativeArea:
            pkContactField = getPKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressSubAdministrativeAreaKey();
            break;

        case WebCore::PaymentError::ContactField::AdministrativeArea:
            pkContactField = getPKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressStateKey();
            break;

        case WebCore::PaymentError::ContactField::Country:
            pkContactField = getPKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressCountryKey();
            break;

        case WebCore::PaymentError::ContactField::CountryCode:
            pkContactField = getPKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressISOCountryCodeKey();
            break;
        }

        [userInfo setObject:pkContactField forKey:getPKPaymentErrorContactFieldUserInfoKey()];
        if (postalAddressKey)
            [userInfo setObject:postalAddressKey forKey:getPKPaymentErrorPostalAddressUserInfoKey()];
    }

    return adoptNS([[NSError alloc] initWithDomain:getPKPaymentErrorDomain() code:toPKPaymentErrorCode(error.code) userInfo:userInfo.get()]);
}

static RetainPtr<NSArray> toNSErrors(const Vector<WebCore::PaymentError>& errors)
{
    auto result = adoptNS([[NSMutableArray alloc] init]);

    for (auto error : errors) {
        if (auto nsError = toNSError(error))
            [result addObject:nsError.get()];
    }

    return result;
}
#else
static PKPaymentAuthorizationStatus toPKPaymentAuthorizationStatus(const std::optional<WebCore::PaymentAuthorizationResult>& result)
{
    if (!result)
        return PKPaymentAuthorizationStatusSuccess;

    if (result->errors.size() == 1) {
        auto& error = result->errors[0];
        switch (error.code) {
        case WebCore::PaymentError::Code::Unknown:
        case WebCore::PaymentError::Code::AddressUnserviceable:
            return PKPaymentAuthorizationStatusFailure;

        case WebCore::PaymentError::Code::BillingContactInvalid:
            return PKPaymentAuthorizationStatusInvalidBillingPostalAddress;

        case WebCore::PaymentError::Code::ShippingContactInvalid:
            if (error.contactField && error.contactField == WebCore::PaymentError::ContactField::PostalAddress)
                return PKPaymentAuthorizationStatusInvalidShippingPostalAddress;

            return PKPaymentAuthorizationStatusInvalidShippingContact;
        }
    }

    switch (result->status) {
    case WebCore::PaymentAuthorizationStatus::Success:
        return PKPaymentAuthorizationStatusSuccess;
    case WebCore::PaymentAuthorizationStatus::Failure:
        return PKPaymentAuthorizationStatusFailure;
    case WebCore::PaymentAuthorizationStatus::PINRequired:
        return PKPaymentAuthorizationStatusPINRequired;
    case WebCore::PaymentAuthorizationStatus::PINIncorrect:
        return PKPaymentAuthorizationStatusPINIncorrect;
    case WebCore::PaymentAuthorizationStatus::PINLockout:
        return PKPaymentAuthorizationStatusPINLockout;
    }

    return PKPaymentAuthorizationStatusFailure;
}
#endif

void WebPaymentCoordinatorProxy::platformCompletePaymentSession(const std::optional<WebCore::PaymentAuthorizationResult>& result)
{
    ASSERT(m_paymentAuthorizationViewController);
    ASSERT(m_paymentAuthorizationViewControllerDelegate);

    m_paymentAuthorizationViewControllerDelegate->_didReachFinalState = WebCore::isFinalStateResult(result);

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
    auto status = result ? result->status : WebCore::PaymentAuthorizationStatus::Success;
    auto pkPaymentAuthorizationResult = adoptNS([allocPKPaymentAuthorizationResultInstance() initWithStatus:toPKPaymentAuthorizationStatus(status) errors:result ? toNSErrors(result->errors).get() : @[ ]]);
    m_paymentAuthorizationViewControllerDelegate->_paymentAuthorizedCompletion(pkPaymentAuthorizationResult.get());
#else
    m_paymentAuthorizationViewControllerDelegate->_paymentAuthorizedCompletion(toPKPaymentAuthorizationStatus(result));
#endif
    m_paymentAuthorizationViewControllerDelegate->_paymentAuthorizedCompletion = nullptr;
}

void WebPaymentCoordinatorProxy::platformCompleteMerchantValidation(const WebCore::PaymentMerchantSession& paymentMerchantSession)
{
    ASSERT(m_paymentAuthorizationViewController);
    ASSERT(m_paymentAuthorizationViewControllerDelegate);

    m_paymentAuthorizationViewControllerDelegate->_sessionBlock(paymentMerchantSession.pkPaymentMerchantSession(), nullptr);
    m_paymentAuthorizationViewControllerDelegate->_sessionBlock = nullptr;
}

void WebPaymentCoordinatorProxy::platformCompleteShippingMethodSelection(const std::optional<WebCore::ShippingMethodUpdate>& update)
{
    ASSERT(m_paymentAuthorizationViewController);
    ASSERT(m_paymentAuthorizationViewControllerDelegate);

    if (update)
        m_paymentAuthorizationViewControllerDelegate->_paymentSummaryItems = toPKPaymentSummaryItems(update->newTotalAndLineItems);

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
    auto pkShippingMethodUpdate = adoptNS([allocPKPaymentRequestShippingMethodUpdateInstance() initWithPaymentSummaryItems:m_paymentAuthorizationViewControllerDelegate->_paymentSummaryItems.get()]);
    m_paymentAuthorizationViewControllerDelegate->_didSelectShippingMethodCompletion(pkShippingMethodUpdate.get());
#else
    m_paymentAuthorizationViewControllerDelegate->_didSelectShippingMethodCompletion(PKPaymentAuthorizationStatusSuccess, m_paymentAuthorizationViewControllerDelegate->_paymentSummaryItems.get());
#endif
    m_paymentAuthorizationViewControllerDelegate->_didSelectShippingMethodCompletion = nullptr;
}

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED < 110000)
static PKPaymentAuthorizationStatus toPKPaymentAuthorizationStatus(const std::optional<WebCore::ShippingContactUpdate>& update)
{
    if (!update || update->errors.isEmpty())
        return PKPaymentAuthorizationStatusSuccess;

    if (update->errors.size() == 1) {
        auto& error = update->errors[0];
        switch (error.code) {
        case WebCore::PaymentError::Code::Unknown:
        case WebCore::PaymentError::Code::AddressUnserviceable:
            return PKPaymentAuthorizationStatusFailure;

        case WebCore::PaymentError::Code::BillingContactInvalid:
            return PKPaymentAuthorizationStatusInvalidBillingPostalAddress;

        case WebCore::PaymentError::Code::ShippingContactInvalid:
            if (error.contactField && error.contactField == WebCore::PaymentError::ContactField::PostalAddress)
                return PKPaymentAuthorizationStatusInvalidShippingPostalAddress;

            return PKPaymentAuthorizationStatusInvalidShippingContact;
        }
    }

    return PKPaymentAuthorizationStatusFailure;
}
#endif

void WebPaymentCoordinatorProxy::platformCompleteShippingContactSelection(const std::optional<WebCore::ShippingContactUpdate>& update)
{
    ASSERT(m_paymentAuthorizationViewController);
    ASSERT(m_paymentAuthorizationViewControllerDelegate);

    if (update) {
        m_paymentAuthorizationViewControllerDelegate->_paymentSummaryItems = toPKPaymentSummaryItems(update->newTotalAndLineItems);

        auto shippingMethods = adoptNS([[NSMutableArray alloc] init]);
        for (auto& shippingMethod : update->newShippingMethods)
            [shippingMethods addObject:toPKShippingMethod(shippingMethod).get()];

        m_paymentAuthorizationViewControllerDelegate->_shippingMethods = WTFMove(shippingMethods);
    }

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
    auto pkShippingContactUpdate = adoptNS([allocPKPaymentRequestShippingContactUpdateInstance() initWithErrors:update ? toNSErrors(update->errors).get() : @[ ] paymentSummaryItems:m_paymentAuthorizationViewControllerDelegate->_paymentSummaryItems.get() shippingMethods:m_paymentAuthorizationViewControllerDelegate->_shippingMethods.get()]);
    m_paymentAuthorizationViewControllerDelegate->_didSelectShippingContactCompletion(pkShippingContactUpdate.get());
#else
    m_paymentAuthorizationViewControllerDelegate->_didSelectShippingContactCompletion(toPKPaymentAuthorizationStatus(update), m_paymentAuthorizationViewControllerDelegate->_shippingMethods.get(), m_paymentAuthorizationViewControllerDelegate->_paymentSummaryItems.get());
#endif
    m_paymentAuthorizationViewControllerDelegate->_didSelectShippingContactCompletion = nullptr;
}

void WebPaymentCoordinatorProxy::platformCompletePaymentMethodSelection(const std::optional<WebCore::PaymentMethodUpdate>& update)
{
    ASSERT(m_paymentAuthorizationViewController);
    ASSERT(m_paymentAuthorizationViewControllerDelegate);

    if (update)
        m_paymentAuthorizationViewControllerDelegate->_paymentSummaryItems = toPKPaymentSummaryItems(update->newTotalAndLineItems);

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000)
    auto pkPaymentMethodUpdate = adoptNS([allocPKPaymentRequestPaymentMethodUpdateInstance() initWithPaymentSummaryItems:m_paymentAuthorizationViewControllerDelegate->_paymentSummaryItems.get()]);
    m_paymentAuthorizationViewControllerDelegate->_didSelectPaymentMethodCompletion(pkPaymentMethodUpdate.get());
#else
    m_paymentAuthorizationViewControllerDelegate->_didSelectPaymentMethodCompletion(m_paymentAuthorizationViewControllerDelegate->_paymentSummaryItems.get());
#endif
    m_paymentAuthorizationViewControllerDelegate->_didSelectPaymentMethodCompletion = nullptr;
}

Vector<String> WebPaymentCoordinatorProxy::platformAvailablePaymentNetworks()
{
#if PLATFORM(MAC)
    if (!PassKitLibrary())
        return { };
#endif
    
    NSArray<PKPaymentNetwork> *availableNetworks = [getPKPaymentRequestClass() availableNetworks];
    Vector<String> result;
    result.reserveInitialCapacity(availableNetworks.count);
    for (PKPaymentNetwork network in availableNetworks)
        result.uncheckedAppend(network);
    return result;
}

} // namespace WebKit

#endif // ENABLE(APPLE_PAY)
