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
#include <unistd.h>
#include <string>
#include "artist_13_3_pro.h"

artist_13_3_pro::artist_13_3_pro() {
    productIds.push_back(0x092b);

    for (int currentAssignedButton = BTN_0; currentAssignedButton < BTN_8; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

artist_13_3_pro::~artist_13_3_pro() {

}

std::string artist_13_3_pro::getProductName(int productId) {
    if (productId == 0x092b) {
        return "XP-Pen Artist 13.3 Pro";
    }

    return "Unknown XP-Pen Device";
}

void artist_13_3_pro::setConfig(nlohmann::json config) {
    if (!config.contains("mapping") || config["mapping"] == nullptr) {
        config["mapping"] = nlohmann::json({});

        auto addToButtonMap = [&config](int key, int eventType, std::vector<int> codes) {
            std::string evstring = std::to_string(eventType);
            config["mapping"]["buttons"][std::to_string(key)][evstring] = codes;
        };

        auto addToDialMap = [&config](int dial, int value, int eventType, std::vector<int> codes) {
            std::string strvalue = std::to_string(value);
            std::string evstring = std::to_string(eventType);
            config["mapping"]["dials"][std::to_string(dial)][strvalue][evstring] = codes;
        };

        addToButtonMap(BTN_0, EV_KEY, {KEY_B});
        addToButtonMap(BTN_1, EV_KEY, {KEY_E});
        addToButtonMap(BTN_2, EV_KEY, {KEY_SPACE});
        addToButtonMap(BTN_3, EV_KEY, {KEY_LEFTALT});
        addToButtonMap(BTN_4, EV_KEY, {KEY_V});
        addToButtonMap(BTN_5, EV_KEY, {KEY_LEFTCTRL, KEY_S});
        addToButtonMap(BTN_6, EV_KEY, {KEY_LEFTCTRL, KEY_Z});
        addToButtonMap(BTN_7, EV_KEY, {KEY_LEFTCTRL, KEY_LEFTALT, KEY_N});

        addToDialMap(REL_WHEEL, -1, EV_KEY, {KEY_LEFTCTRL, KEY_MINUS});
        addToDialMap(REL_WHEEL, 1, EV_KEY, {KEY_LEFTCTRL, KEY_EQUAL});
    }
    jsonConfig = config;

    submitMapping(jsonConfig);
}

int artist_13_3_pro::sendInitKeyOnInterface() {
    return 0x02;
}

bool artist_13_3_pro::attachToInterfaceId(int interfaceId) {
    return true;
}

bool artist_13_3_pro::attachDevice(libusb_device_handle *handle, int interfaceId) {
    if (interfaceId == 2) {
        unsigned char *buf = new unsigned char[12];

        // We need to get a few more bits of information
        if (libusb_get_string_descriptor(handle, 0x64, 0x0409, buf, 12) != 12) {
            std::cout << "Could not get descriptor" << std::endl;
            return false;
        }

        int maxWidth = (buf[3] << 8) + buf[2];
        int maxHeight = (buf[5] << 8) + buf[4];
        int maxPressure = (buf[9] << 8) + buf[8];

        unsigned short vendorId = 0x28bd;
        unsigned short productId = 0xf92b;
        unsigned short versionId = 0x0001;

        struct uinput_pen_args penArgs{
                .maxWidth = maxWidth,
                .maxHeight = maxHeight,
                .maxPressure = maxPressure,
                .maxTiltX = 60,
                .maxTiltY = 60,
                .vendorId = vendorId,
                .productId = productId,
                .versionId = versionId,
                {"XP-Pen Artist 13.3 Pro"},
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
                {"XP-Pen Artist 13.3 Pro Pad"},
        };

        uinputPens[handle] = create_pen(penArgs);
        uinputPads[handle] = create_pad(padArgs);
    }

    return true;
}

bool artist_13_3_pro::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    switch (data[0]) {
        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            handleFrameEvent(handle, data, dataLen);
            break;

        default:
            break;
    }

    return true;
}

void artist_13_3_pro::handleDigitizerEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] < 0xb0) {
        // Extract the X and Y position
        int penX = (data[3] << 8) + data[2];
        int penY = (data[5] << 8) + data[4];

        // Check to see if the pen is touching
        int pressure;
        if (0x01 & data[1]) {
            // Grab the pressure amount
            pressure = (data[7] << 8) + data[6];

            uinput_send(uinputPens[handle], EV_KEY, BTN_TOOL_PEN, 1);
            uinput_send(uinputPens[handle], EV_ABS, ABS_PRESSURE, pressure);
        } else {
            uinput_send(uinputPens[handle], EV_KEY, BTN_TOOL_PEN, 0);
        }

        // Grab the tilt values
        short tiltx = (char)data[8];
        short tilty = (char)data[9];

        // Check to see if the stylus buttons are being pressed
        if (0x02 & data[1]) {
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS, 1);
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS2, 0);
        } else if (0x04 & data[1]) {
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS, 0);
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS2, 1);
        } else {
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS, 0);
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS2, 0);
        }

        uinput_send(uinputPens[handle], EV_ABS, ABS_X, penX);
        uinput_send(uinputPens[handle], EV_ABS, ABS_Y, penY);
        uinput_send(uinputPens[handle], EV_ABS, ABS_TILT_X, tiltx);
        uinput_send(uinputPens[handle], EV_ABS, ABS_TILT_Y, tilty);

        uinput_send(uinputPens[handle], EV_SYN, SYN_REPORT, 1);
    }
}

void artist_13_3_pro::handleFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] >= 0xf0) {
        long button = data[2];
        // Only 8 buttons on this device
        long position = ffsl(data[2]);

        // Take the dial
        short dialValue = 0;
        if (0x01 & data[7]) {
            dialValue = 1;
        } else if (0x02 & data[7]) {
            dialValue = -1;
        }

        bool shouldSyn = true;
        bool dialEvent = false;

        if (dialValue != 0) {
            bool send_reset = false;
            auto dialMap = dialMapping.getDialMap(EV_REL, REL_WHEEL, dialValue);
            for (auto dmap: dialMap) {
                uinput_send(uinputPads[handle], dmap.event_type, dmap.event_value, dmap.event_data);
                if (dmap.event_type == EV_KEY) {
                    send_reset = true;
                }
            }

            uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);

            if (send_reset) {
                for (auto dmap: dialMap) {
                    // We have to handle key presses manually here because this device does not send reset events
                    if (dmap.event_type == EV_KEY) {
                        uinput_send(uinputPads[handle], dmap.event_type, dmap.event_value, 0);
                    }
                }
            }
            uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);

            shouldSyn = false;
            dialEvent = true;
        }

        if (button != 0) {
            auto padMap = padMapping.getPadMap(padButtonAliases[position - 1]);
            for (auto pmap : padMap) {
                uinput_send(uinputPads[handle], pmap.event_type, pmap.event_value, 1);
            }
            lastPressedButton[handle] = position;
        } else if (!dialEvent) {
            if (lastPressedButton.find(handle) != lastPressedButton.end() && lastPressedButton[handle] > 0) {
                auto padMap = padMapping.getPadMap(padButtonAliases[lastPressedButton[handle] - 1]);
                for (auto pmap : padMap) {
                    uinput_send(uinputPads[handle], pmap.event_type, pmap.event_value, 0);
                }
                lastPressedButton[handle] = -1;
            }
        }

        if (shouldSyn) {
            uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
        }
    }
}
