
#include "Aura.hpp"

#include <vector>
#include <stdint.h>

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
};

Aura::Aura() : impl(std::make_unique<Aura::Impl>()) {
	impl->dev = impl->OpenAura(0x0B05, 0x19B6, 0xFF31, 0x76);
}

Aura::~Aura() {
	if (impl->dev) CloseHandle(impl->dev);
}

bool Aura::IsOpen() const { return impl->dev != NULL; }

bool Aura::SetColor(char r, char g, char b, bool force) {
	if (!IsOpen()) return false;
	
	if (!force && r == impl->lastR && g == impl->lastG && b == impl->lastB)
		return true; // Just preend it's ok the colour is the same hmm? but what if we wanna reset it

	AuraInitReport init;
	HidD_SetFeature(impl->dev, &init, sizeof(init));

	AuraColorReport report;
	report.r = r;
	report.g = g;
	report.b = b;
	return HidD_SetFeature(impl->dev, &report, sizeof(report));
}