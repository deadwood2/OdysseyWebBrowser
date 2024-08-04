/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
#include "PaymentContact.h"

#if ENABLE(APPLE_PAY)

#import "ApplePayPaymentContact.h"
#import "PassKitSPI.h"
#import "SoftLinking.h"
#import <Contacts/Contacts.h>
#import <wtf/text/StringBuilder.h>

SOFT_LINK_FRAMEWORK(Contacts)
SOFT_LINK_CLASS(Contacts, CNMutablePostalAddress)
SOFT_LINK_CLASS(Contacts, CNPhoneNumber)

#if PLATFORM(MAC)
SOFT_LINK_PRIVATE_FRAMEWORK(PassKit)
#else
SOFT_LINK_FRAMEWORK(PassKit)
#endif

SOFT_LINK_CLASS(PassKit, PKContact)

namespace WebCore {

static RetainPtr<PKContact> convert(const ApplePayPaymentContact& contact)
{
    auto result = adoptNS([allocPKContactInstance() init]);

    if (!contact.familyName.isEmpty() || !contact.givenName.isEmpty()) {
        auto name = adoptNS([[NSPersonNameComponents alloc] init]);
        [name setFamilyName:contact.familyName];
        [name setGivenName:contact.givenName];
        [result setName:name.get()];
    }

    if (!contact.emailAddress.isEmpty())
        [result setEmailAddress:contact.emailAddress];

    if (!contact.phoneNumber.isEmpty())
        [result setPhoneNumber:adoptNS([allocCNPhoneNumberInstance() initWithStringValue:contact.phoneNumber]).get()];

    if (contact.addressLines && !contact.addressLines->isEmpty()) {
        auto address = adoptNS([allocCNMutablePostalAddressInstance() init]);

        StringBuilder builder;
        for (unsigned i = 0; i < contact.addressLines->size(); ++i) {
            builder.append(contact.addressLines->at(i));
            if (i != contact.addressLines->size() - 1)
                builder.append('\n');
        }
        
        // FIXME: StringBuilder should hava a toNSString() function to avoid the extra String allocation.
        [address setStreet:builder.toString()];

        if (!contact.locality.isEmpty())
            [address setCity:contact.locality];
        if (!contact.postalCode.isEmpty())
            [address setPostalCode:contact.postalCode];
        if (!contact.administrativeArea.isEmpty())
            [address setState:contact.administrativeArea];
        if (!contact.country.isEmpty())
            [address setCountry:contact.country];
        if (!contact.countryCode.isEmpty())
            [address setISOCountryCode:contact.countryCode];

        [result setPostalAddress:address.get()];
    }

    return result;
}

static ApplePayPaymentContact convert(PKContact *contact)
{
    ASSERT(contact);

    ApplePayPaymentContact result;

    if (contact.phoneNumber)
        result.phoneNumber = contact.phoneNumber.stringValue;
    if (contact.emailAddress)
        result.emailAddress = contact.emailAddress;
    if (contact.name.givenName)
        result.givenName = contact.name.givenName;
    if (contact.name.familyName)
        result.familyName = contact.name.familyName;
    if (contact.postalAddress.street.length) {
        Vector<String> addressLines;
        String(contact.postalAddress.street).split("\n", addressLines);
        result.addressLines = WTFMove(addressLines);
    }
    if (contact.postalAddress.city)
        result.locality = contact.postalAddress.city;
    if (contact.postalAddress.postalCode)
        result.postalCode = contact.postalAddress.postalCode;
    if (contact.postalAddress.state)
        result.administrativeArea = contact.postalAddress.state;
    if (contact.postalAddress.country)
        result.country = contact.postalAddress.country;
    if (contact.postalAddress.ISOCountryCode)
        result.countryCode = contact.postalAddress.ISOCountryCode;

    return result;
}

PaymentContact PaymentContact::fromApplePayPaymentContact(const ApplePayPaymentContact& contact)
{
    return PaymentContact(convert(contact).get());
}

ApplePayPaymentContact PaymentContact::toApplePayPaymentContact() const
{
    return convert(m_pkContact.get());
}

}

#endif
