
#include "Aura.hpp"

#if defined(__EMSCRIPTEN__)
// Aura is not supported on Emscripten/Web
#include <emscripten/emscripten.h>

// Emscripten bridge: forward the requested colour to the host page's WebAura
// (WebHID) instance. The host registers the target via Module.onAuraColor(r,g,b).
EM_JS(void, FE_AuraSetColor, (int r, int g, int b), {
    var m = typeof Module !== 'undefined' ? Module : null;
    if (m && typeof m.onAuraColor === 'function') m.onAuraColor(r, g, b);
});

struct Aura::Impl { void* dev = nullptr; };
Aura::Aura() : impl(std::make_unique<Aura::Impl>()) {}
Aura::~Aura() {}
bool Aura::IsOpen() const { return false; }
bool Aura::SetColor(char r, char g, char b, bool force) {
    FE_AuraSetColor((int)(unsigned char)r, (int)(unsigned char)g, (int)(unsigned char)b);
    auraInitialized = true;
    return true;
}
#else

#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#else
#include <hidapi/hidapi.h>
#endif

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

#pragma pack(push, 1)
struct AuraInitReport
{
	uint8_t reportId = 0x5A;
	uint8_t cmd      = 0xBC;
	uint8_t mode     = 0x01;
	uint8_t reserved[61] = { 0 };
};

struct AuraColorReport
{
	uint8_t reportId     = 0x5A;
	uint8_t cmd          = 0xBC;
	uint8_t mode         = 0x01;
	uint8_t apply        = 0x01;
	uint8_t reserved1[5] = { 0 };   // bytes 4-8
	uint8_t r            = 0;
	uint8_t g            = 0;
	uint8_t b            = 0;
	uint8_t reserved2[52] = { 0 };  // pad out to 64 total
};
#pragma pack(pop)

static_assert(sizeof(AuraInitReport)  == 64, "AuraInitReport must be exactly 64 bytes");
static_assert(sizeof(AuraColorReport) == 64, "AuraColorReport must be exactly 64 bytes");

struct Aura::Impl {
	void *dev = NULL;

	uint8_t lastR = 0, lastG = 0, lastB = 0;

#ifdef _WIN32
	HANDLE OpenAura(USHORT vid, USHORT pid, USHORT page, USHORT usage)
	{
		GUID guid;
		HidD_GetHidGuid(&guid);
		HDEVINFO devInfo = SetupDiGetClassDevs(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

		SP_DEVICE_INTERFACE_DATA ifData{ sizeof(ifData) };
		for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devInfo, nullptr, &guid, i, &ifData); i++)
		{
			DWORD size = 0;
			SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, nullptr, 0, &size, nullptr);
			std::vector<BYTE> buf(size);
			auto detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)buf.data();
			detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
			if (!SetupDiGetDeviceInterfaceDetailW(devInfo, &ifData, detail, size, nullptr, nullptr)) continue;

			HANDLE h = CreateFileW(detail->DevicePath, GENERIC_WRITE | GENERIC_READ,
									FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
			if (h == INVALID_HANDLE_VALUE) continue;

			HIDD_ATTRIBUTES attr{ sizeof(attr) };
			HidD_GetAttributes(h, &attr);

			PHIDP_PREPARSED_DATA pp;
			HIDP_CAPS caps{};
			if (attr.VendorID == vid && attr.ProductID == pid && HidD_GetPreparsedData(h, &pp))
			{
				HidP_GetCaps(pp, &caps);
				HidD_FreePreparsedData(pp);
				if (caps.UsagePage == page && caps.Usage == usage) return h;
			}
			CloseHandle(h);
		}
		return nullptr;
	}
#else
#if !defined(EMSCRIPTEN)
	hid_device* OpenAura(uint16_t vid, uint16_t pid, uint16_t page, uint16_t usage)
    {
        struct hid_device_info* devs = hid_enumerate(vid, pid);
        struct hid_device_info* cur_dev = devs;
        const char* path_to_open = nullptr;

        while (cur_dev) {
            if (cur_dev->usage_page == page && cur_dev->usage == usage) {
                path_to_open = cur_dev->path;
                break;
            }
            cur_dev = cur_dev->next;
        }

        hid_device* handle = nullptr;
        if (path_to_open) {
            handle = hid_open_path(path_to_open);
        }

        hid_free_enumeration(devs);
        return handle;
    }
#endif
#endif

	bool SetFeature(void* dev, void* data, size_t size) {
#ifdef _WIN32
		return HidD_SetFeature((HANDLE)dev, data, (ULONG)size);
#else
		return hid_send_feature_report((hid_device*)dev, (unsigned char*)data, size) >= 0;
#endif
	}
};

Aura::Aura() : impl(std::make_unique<Aura::Impl>()) {
	impl->dev = (void*)impl->OpenAura(0x0B05, 0x19B6, 0xFF31, 0x76);
}

Aura::~Aura() {
#ifdef _WIN32
	if (impl->dev) CloseHandle(impl->dev);
#else
	if (impl->dev) {
        hid_close((hid_device*)impl->dev);
    }
    hid_exit();
#endif
}

bool Aura::IsOpen() const { return impl->dev != NULL; }

bool Aura::SetColor(char r, char g, char b, bool force) {
	if (!IsOpen()) return false;
	
	if (!force && r == impl->lastR && g == impl->lastG && b == impl->lastB)
		return true; // Just preend it's ok the colour is the same hmm? but what if we wanna reset it

	AuraInitReport init;
	impl->SetFeature(impl->dev, &init, sizeof(init));

	AuraColorReport report;
	report.r = r;
	report.g = g;
	report.b = b;
	return impl->SetFeature(impl->dev, &report, sizeof(report));
}

#endif // !__EMSCRIPTEN__