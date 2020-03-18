
#pragma once

#include "common/buffer.h"

#include <cstdint>


enum USBVendor {
	VENDOR_NINTENDO = 0x57E
};

enum USBNintendoProduct {
	NINTENDO_BLUETOOTH = 0x305
};


struct USBDeviceDescriptor {
	uint8_t size;
	uint8_t type;
	uint16_t usb_version;
	uint8_t device_class;
	uint8_t device_subclass;
	uint8_t device_protocol;
	uint8_t max_packet_size;
	uint16_t vendor_id;
	uint16_t product_id;
	uint16_t device_version;
	uint8_t manufacturer;
	uint8_t product;
	uint8_t serial_number;
	uint8_t num_configurations;
};

struct USBConfigurationDescriptor {
	uint8_t size;
	uint8_t type;
	uint16_t total_length;
	uint8_t num_interfaces;
	uint8_t configuration_value;
	uint8_t configuration_index;
	uint8_t attributes;
	uint8_t max_power;
};

struct USBSetupPacket {
	uint8_t type;
	uint8_t request;
	uint16_t value;
	uint16_t offset;
	uint16_t length;
};


class USBDevice {
public:
	enum Request {
		SET_ADDRESS = 5,
		GET_DESCRIPTOR = 6,
		SET_CONFIGURATION = 9
	};
	
	enum Descriptor {
		DEVICE = 1,
		CONFIGURATION = 2 
	};
	
	void reset();
	
	virtual Buffer get_descriptor(Descriptor type) = 0;
	
	void setup(Buffer data);
	void write(Buffer data);
	Buffer read(uint32_t length);
	
	int address;
	
private:
	Buffer data;
};


class USBDummyDevice : public USBDevice {
public:
	USBDummyDevice();
	
	Buffer get_descriptor(Descriptor type);

private:
	USBDeviceDescriptor device_descriptor;
	USBConfigurationDescriptor configuration_descriptor;
};
