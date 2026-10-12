
#include <WebCore/CryptoClient.h>
#include <wtf/TZoneMalloc.h>

namespace WebKit {

class WebPage;

class WebCryptoClient:  public WebCore::CryptoClient {
    WTF_MAKE_TZONE_ALLOCATED(WebCryptoClient);
public:
    WebCryptoClient() = default;
    ~WebCryptoClient() = default;
    std::optional<Vector<uint8_t>> wrapCryptoKey(const Vector<uint8_t>&) const override;
    std::optional<Vector<uint8_t>> unwrapCryptoKey(const Vector<uint8_t>&) const override;
private:
};

}
