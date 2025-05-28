#include "config.h"

#if ENABLE(MEDIA_STREAM)
#include "RealtimeMediaSourceCenter.h"
#include "CaptureDeviceManager.h"

namespace WebCore {

class AudioCaptureDeviceManagerMorphOS final : public CaptureDeviceManager {
private:
	const Vector<CaptureDevice>& captureDevices() final { static Vector<CaptureDevice> vDummy; return vDummy; }
	std::optional<CaptureDevice> captureDeviceWithPersistentID(CaptureDevice::DeviceType type, const String& id) final { return std::nullopt; }
};

class VideoCaptureDeviceManagerMorphOS final : public CaptureDeviceManager {
private:
	const Vector<CaptureDevice>& captureDevices() final { static Vector<CaptureDevice> vDummy; return vDummy; }
	std::optional<CaptureDevice> captureDeviceWithPersistentID(CaptureDevice::DeviceType, const String&) final { return std::nullopt; }
};

class DisplayCaptureDeviceManagerMorphOS final : public CaptureDeviceManager {
private:
	const Vector<CaptureDevice>& captureDevices() final { static Vector<CaptureDevice> vDummy; return vDummy; }
	std::optional<CaptureDevice> captureDeviceWithPersistentID(CaptureDevice::DeviceType, const String&) final { return std::nullopt; }
};

class RealtimeVideoSourceFactoryMorphOS : public VideoCaptureFactory {
public:
    CaptureSourceOrError createVideoCaptureSource(const CaptureDevice& device, String&& hashSalt, const MediaConstraints* constraints) final
    {
        return { "Not supported"_s };
    }

private:
    CaptureDeviceManager& videoCaptureDeviceManager() final {
		static VideoCaptureDeviceManagerMorphOS captureDevice;
		return captureDevice; }
};

class RealtimeDisplaySourceFactoryMorphOS : public DisplayCaptureFactory {
public:
    CaptureSourceOrError createDisplayCaptureSource(const CaptureDevice& device, const MediaConstraints* constraints) final
    {
        return { "Not supported"_s };
    }
private:
    CaptureDeviceManager& displayCaptureDeviceManager() final {
		static DisplayCaptureDeviceManagerMorphOS captureDevice;
		return captureDevice; }
};

class RealtimeAudioSourceFactoryMorphOS final : public AudioCaptureFactory {
public:
    CaptureSourceOrError createAudioCaptureSource(const CaptureDevice& device, String&& hashSalt, const MediaConstraints* constraints) final
    {
		dprintf("%s\n", __PRETTY_FUNCTION__);
        return { "Not supported"_s };
    }
private:
    CaptureDeviceManager& audioCaptureDeviceManager() final {
		static AudioCaptureDeviceManagerMorphOS captureDevice;
		return captureDevice;
    }
    const Vector<CaptureDevice>& speakerDevices() const final {
		static Vector<CaptureDevice> devices;
		dprintf("%s\n", __PRETTY_FUNCTION__);
		return devices;
	}
};

AudioCaptureFactory& RealtimeMediaSourceCenter::defaultAudioCaptureFactory()
{
	static RealtimeAudioSourceFactoryMorphOS factory;
    return factory;
}

VideoCaptureFactory& RealtimeMediaSourceCenter::defaultVideoCaptureFactory()
{
	static RealtimeVideoSourceFactoryMorphOS factory;
    return factory;
}

DisplayCaptureFactory& RealtimeMediaSourceCenter::defaultDisplayCaptureFactory()
{
	static RealtimeDisplaySourceFactoryMorphOS factory;
    return factory;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

