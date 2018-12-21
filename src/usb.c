/*
 * The GPL License (GPLv3)
 *
 * Copyright © 2016 Thomas "Ventto" Venriès  <thomas.venries@gmail.com>
 *
 * Pearlfan is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Pearlfan is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Pearlfan.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "defutils.h"
#include "usb.h"

libusb_device_handle *pfan_open(int vid, int pid)
{
    libusb_device_handle *dev_handle =
        libusb_open_device_with_vid_pid(NULL, vid, pid);

    if (!dev_handle) {
        fprintf(stderr, "Device can not be opened or found.\n");
        fprintf(stderr, "You may need permission.\n\n");
        return NULL;
    }

    if (libusb_kernel_driver_active(dev_handle, 0) == 1) {
        if (libusb_detach_kernel_driver(dev_handle, 0) != 0) {
            libusb_close(dev_handle);
            fprintf(stderr, "Can not dettach the device from the driver.\n\n");
            return NULL;
        }
    }

    if (libusb_claim_interface(dev_handle, 0) < 0) {
        libusb_close(dev_handle);
        fprintf(stderr, "Cannot claim the interface.\n\n");
        return NULL;
    }

    return dev_handle;
}

void pfan_close(libusb_device_handle *dev_handle)
{
    libusb_close(dev_handle);
    libusb_exit(NULL);
}

static int pfan_usb_send(libusb_device_handle *dev, void *data)
{
    uint8_t buf[8];
    int bytes = 0;
    int l = 0;

    memset((void *)buf, 0, 8);
    // what is this magic number?
    buf[7] = 0x2;

    fprintf(stderr, "USB transfer: %04x %04x %04x %04x\n",
        ntohs(*(unsigned short *)(data+0)),
        ntohs(*(unsigned short *)(data+2)),
        ntohs(*(unsigned short *)(data+4)),
        ntohs(*(unsigned short *)(data+6)));
    bytes = libusb_control_transfer(dev, 0x21, 9, 0x200, 0, data, 8, 1000);

    if (bytes < 0) {
        return bytes;
    }

    // The libusb_interrupt_transfer() seems to be an ACK for the
    // libusb_control_transfer() above as we discard any data received.
    int ret;

    if ((ret = libusb_interrupt_transfer(dev, 0x81, buf, 8, &l, 1000)) < 0) {
        return ret;
    }

    return bytes;
}

static uint64_t encode_effect(const char id, const unsigned char effect[3])
{
    unsigned short opts = 0;

    opts |= effect[PFAN_CLOSE];
    opts |= (effect[PFAN_OPEN] << 4);
    opts |= (id << 8);
    opts |= (effect[PFAN_BEFORECLOSE] << 12);

    return 0xAF0000022A034040;
    // what is this magic 64-bit/8 byte number?
    return 0x00000055000010A0 | (opts << 16);
}

int pfan_send(libusb_device_handle *dev_handle, int img_n,
              uint8_t effects[PFAN_MAX_DISPLAY][3],
              uint16_t displays[PFAN_MAX_DISPLAY][PFAN_MAX_W])
{
    int bytes = 0;
    int ret = 0;

    for (uint8_t i = 0; i < img_n; i++) {
        uint64_t effect = encode_effect(i, effects[i]);

        // send the effects for this image
        ret = pfan_usb_send(dev_handle, &effect);

        if (ret < 0) {
            return ret;
        } else if (ret < 8) {
            break;
        }

        bytes += 8;

        // send the image (how do we send colour information?)
        // 39 = 156 / 4 so sending 4 columns at a time, a column is encoded in
        // 2 bytes ........ ........
        for (uint8_t j = 0; j < (PFAN_MAX_W / 4); ++j) {
            displays[i][(j * 4) + 0] = (unsigned short)0x2340;
            displays[i][(j * 4) + 1] = (unsigned short)0x22a4;
            displays[i][(j * 4) + 2] = (unsigned short)0xa422;
            displays[i][(j * 4) + 3] = (unsigned short)0x93a4;
            ret = pfan_usb_send(dev_handle, &displays[i][j * 4]);

            if (ret < 0) {
                return ret;
            } else if (ret < 8) {
                break;
            }

            bytes += 8;
        }
    }

    return bytes;
}
