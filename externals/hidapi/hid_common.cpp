#include <stdio.h>
#include "hidapi.h"

HID_API_NAMESPACE_BEGIN

void HID_API_EXPORT hid_free_enumeration(struct hid_device_info* devs)
{
	struct hid_device_info* d = devs;
	while (d) {
		struct hid_device_info* next = d->next;
		free(d->path);
		free(d->serial_number);
		free(d->manufacturer_string);
		free(d->product_string);
		free(d);
		d = next;
	}
}

hid_device* hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t* serial_number)
{
	struct hid_device_info* devs, * cur_dev;
	const char* path_to_open = NULL;
	hid_device* handle = NULL;

	devs = hid_enumerate(vendor_id, product_id);
	cur_dev = devs;
	while (cur_dev) {
		if (cur_dev->vendor_id == vendor_id &&
			cur_dev->product_id == product_id) {
			if (serial_number) {
				if (wcscmp(serial_number, cur_dev->serial_number) == 0) {
					path_to_open = cur_dev->path;
					break;
				}
			}
			else {
				path_to_open = cur_dev->path;
				break;
			}
		}
		cur_dev = cur_dev->next;
	}

	if (path_to_open) {
		/* Open the device */
		handle = hid_open_path(path_to_open);
	}

	hid_free_enumeration(devs);

	return handle;
}

HID_API_NAMESPACE_END
