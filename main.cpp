// fnkeylogi.cpp
//
// Copyright © 2022 Mino
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the “Software”), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
// THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <exception>
#include <iostream>
#include <array>
#include <format>
#include <chrono>
#include <thread>
#include <span>
#include <string_view>

#include <Windows.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.HumanInterfaceDevice.h>

using std::span;
using std::wstring_view;
using std::cout;
using std::wcout;
using std::endl;

using winrt::hstring;

using namespace std::chrono_literals;

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Devices::HumanInterfaceDevice;
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Storage;

const std::array<uint8_t, 5> REPORT_FN_KEY_ON_USB{ 0x10, 0x01, 0x0C, 0x1C, 0x00 };
const std::array<uint8_t, 5> REPORT_FN_KEY_ON_BT{ 0x11, 0xFF, 0x0C, 0x1D, 0x00 /* off: 0x01 */ };

void hid_send_data(HidDevice& device, span<const uint8_t> to_send) {
    auto report = device.CreateOutputReport(/* reportId: 0x10 */);
    auto report_data = report.Data();

    if (report_data.Length() < to_send.size()) {
        return;
    }

    std::copy(to_send.begin(), to_send.end(), report_data.data());

    // n.b.) can throw hresult_error
    device.SendOutputReportAsync(report).get();
}

void broadcast_hid_oreport(const hstring& selector, wstring_view target_device_name, span<const uint8_t> broadcast_data) {
    auto devices = DeviceInformation::FindAllAsync(selector).get();

    for (auto i = 0; i < devices.Size(); i++) {
        auto device_info = devices.GetAt(i);
        const auto device_name = device_info.Name();

        auto device = HidDevice::FromIdAsync(device_info.Id(), FileAccessMode::ReadWrite).get();
        if (!device) {
            //wcout << L"failed to open a device: " << std::wstring_view{ device_name } << endl;
            continue;
        }

        if (device_name != target_device_name) {
            continue;
        }

        hid_send_data(device, broadcast_data);

        device.Close();
    }
}

void scan_and_configure() {
    auto usb_devices = HidDevice::GetDeviceSelector(0xFF00, 0x0001, 0x046D /* logi */, 0xC52B);
    auto bt_devices = HidDevice::GetDeviceSelector(0xFF43, 0x202);

    broadcast_hid_oreport(usb_devices, L"Logitech® Unifying Receiver", REPORT_FN_KEY_ON_USB);
    broadcast_hid_oreport(bt_devices, L"Keyboard K780", REPORT_FN_KEY_ON_BT);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    winrt::init_apartment();

    while (true) {
        try {
            scan_and_configure();
        } catch (const std::exception& ex) {
            // ignored
        } catch (const winrt::hresult_error& err) {
            // ignored; this can happen if Logitech Option is already running on the machine
            // (or sending hid output report to the wrong device...)
        }

        std::this_thread::sleep_for(5000ms);
    }

    return 0;
}
