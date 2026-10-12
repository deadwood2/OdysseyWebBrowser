#include "WebKit.h"
#include "WebCryptoClient.h"
#include <WebCore/SerializedCryptoKeyWrap.h>
#include <WebCore/WrappedCryptoKey.h>
#include <wtf/TZoneMallocInlines.h>

namespace WebKit {

WTF_MAKE_TZONE_ALLOCATED_IMPL(WebCryptoClient);

std::optional<Vector<uint8_t>> WebCryptoClient::wrapCryptoKey(const Vector<uint8_t>& key) const
{
    auto masterKey = WebCore::defaultWebCryptoMasterKey();
    if (!masterKey)
        return std::nullopt;
    Vector<uint8_t> wrappedKey;
    if (!WebCore::wrapSerializedCryptoKey(WTFMove(*masterKey), key, wrappedKey))
        return std::nullopt;
    return wrappedKey;
}

std::optional<Vector<uint8_t>> WebCryptoClient::unwrapCryptoKey(const Vector<uint8_t>& serializedKey) const
{
    auto wrappedKey = WebCore::readSerializedCryptoKey(serializedKey);
    if (!wrappedKey)
        return std::nullopt;
    if (auto masterKey = WebCore::defaultWebCryptoMasterKey())
        return WebCore::unwrapCryptoKey(*masterKey, *wrappedKey);
    return std::nullopt;
}

}
