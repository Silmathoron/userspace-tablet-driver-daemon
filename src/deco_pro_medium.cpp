/*
userspace_tablet_driver_daemon
Copyright (C) 2021 - Aren Villanueva <https://github.com/kurikaesu/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include "deco_pro_medium.h"

deco_pro_medium::deco_pro_medium()
: deco_pro() {
    productIds.push_back(0x0904);
}

std::string deco_pro_medium::getProductName(int productId) {
    if (productId == 0x0904) {
        return "XP-Pen Deco Pro M";
    }

    return deco_pro::getProductName(productId);
}

bool deco_pro_medium::attachDevice(libusb_device_handle *handle, int interfaceId) {
    unsigned char* buf = new unsigned char[12];

    // We need to get a few more bits of information
    if (libusb_get_string_descriptor(handle, 0x64, 0x0409, buf, 12) != 12) {
        std::cout << "Could not get descriptor" << std::endl;
        return false;
    }

    int maxWidth = (buf[3] << 8) + buf[2];
    int maxHeight = (buf[5] << 8) + buf[4];
    int maxPressure = (buf[9] << 8) + buf[8];

    unsigned short vendorId = 0x28bd;
    unsigned short productId = 0xf904;
    unsigned short versionId = 0x0001;

    if (interfaceId == 2) {
        struct uinput_pen_args penArgs{
                .maxWidth = maxWidth,
                .maxHeight = maxHeight,
                .maxPressure = maxPressure,
                .maxTiltX = 60,
                .maxTiltY = 60,
                .vendorId = vendorId,
                .productId = productId,
                .versionId = versionId,
                {"XP-Pen Deco Pro M"},
        };

        struct uinput_pad_args padArgs{
                .padButtonAliases = padButtonAliases,
                .hasWheel = true,
                .hasHWheel = true,
                .wheelMax = 1,
                .hWheelMax = 1,
                .vendorId = vendorId,
                .productId = productId,
                .versionId = versionId,
                {"XP-Pen Deco Pro M Pad"},
        };

        uinputPens[handle] = create_pen(penArgs);
        uinputPads[handle] = create_pad(padArgs);
    }

    if (interfaceId == 0) {
        struct uinput_pointer_args pointerArgs{
                .wheelMax = 1,
                .vendorId = vendorId,
                .productId = productId,
                .versionId = versionId,
                {"XP-Pen Deco Pro M Pointer"},
        };

        uinputPointers[handle] = create_pointer(pointerArgs);
    }

    return true;
}