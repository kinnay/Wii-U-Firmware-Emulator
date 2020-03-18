
#include "hardware/usb.h"
#include "common/logger.h"
#include "common/buffer.h"


void USBDevice::reset() {
	address = 0;
}

void USBDevice::setup(Buffer data) {
	this->data = Buffer();
	
	if (data.size() != 8) {
		Logger::error("USB setup packet has invalid size");
		return;
	}
	
	USBSetupPacket *packet = (USBSetupPacket *)data.get();
	if (packet->request == SET_ADDRESS) {
		address = packet->value;
	}
	else if (packet->request == GET_DESCRIPTOR) {
		Descriptor type = (Descriptor)(packet->value >> 8);
		this->data = get_descriptor(type);
		if (this->data.size() > packet->length) {
			this->data = this->data.slice(0, packet->length);
		}
	}
	else if (packet->request == SET_CONFIGURATION) {}
	else {
		Logger::warning("Unknown usb request: %i", packet->request);
	}
}

void USBDevice::write(Buffer data) {}

Buffer USBDevice::read(uint32_t length) {
	if (length != data.size()) {
		Logger::error("Unexpected usb read size: %i", length);
		return Buffer(length);
	}
	
	return data;
}


USBDummyDevice::USBDummyDevice() {
	device_descriptor.size = sizeof(device_descriptor);
	device_descriptor.type = DEVICE;
	device_descriptor.usb_version = 0x310;
	device_descriptor.device_class = 0xFF;
	device_descriptor.device_subclass = 0;
	device_descriptor.device_protocol = 0;
	device_descriptor.max_packet_size = 16;
	device_descriptor.vendor_id = 0;
	device_descriptor.product_id = 0;
	device_descriptor.manufacturer = 0;
	device_descriptor.product = 0;
	device_descriptor.serial_number = 0;
	device_descriptor.num_configurations = 0;
	
	configuration_descriptor.size = sizeof(configuration_descriptor);
	configuration_descriptor.type = CONFIGURATION;
	configuration_descriptor.total_length = sizeof(configuration_descriptor);
	configuration_descriptor.num_interfaces = 0;
	configuration_descriptor.configuration_value = 0;
	configuration_descriptor.configuration_index = 0;
	configuration_descriptor.attributes = 0;
	configuration_descriptor.max_power = 50;
}

Buffer USBDummyDevice::get_descriptor(Descriptor type) {
	if (type == DEVICE) {
		return Buffer(&device_descriptor, sizeof(device_descriptor), Buffer::CreateCopy);
	}
	else if (type == CONFIGURATION) {
		return Buffer(&configuration_descriptor, sizeof(configuration_descriptor), Buffer::CreateCopy);
	}
	else {
		Logger::warning("Unknown descriptor type: %i", type);
		return Buffer();
	}
}
