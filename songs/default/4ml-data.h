#define TOTAL_VOICES 7

const unsigned char mmml_data[] = {
	// header
	0x00,0x12,0x00,0x27,0x00,0x3E,0x00,0x55,0x00,0x6A,0x00,0x7C,0x00,0x9A,0x00,0xBC,
	0x00,0xCA,
	// data
	0xF0,0x02,0xE7,0xF5,0x05,0xD1,0xF4,0x64,0xF2,0x00,0xD1,0xF4,0x62,0xF2,0x00,0xF1,
	0xF0,0xFF,0x00,0xF1,0xFF,0xF0,0x02,0xE6,0xF5,0x01,0xF7,0x32,0xD2,0xF4,0x6B,0xF2,
	0x00,0xD2,0xF4,0x69,0xF2,0x00,0xF1,0xF0,0xFF,0x00,0xF1,0xFF,0xF0,0x02,0xE6,0xF5,
	0x01,0xF7,0x96,0xD2,0xF4,0x6F,0xF2,0x00,0xD2,0xF4,0x6D,0xF2,0x00,0xF1,0xF0,0xFF,
	0x00,0xF1,0xFF,0xF0,0x02,0xE6,0xF5,0x01,0xD3,0xF4,0x66,0xF2,0x00,0xD3,0xF4,0x64,
	0xF2,0x00,0xF1,0xF0,0xFF,0x00,0xF1,0xFF,0xF0,0x02,0xF5,0x00,0xF4,0x64,0xF2,0x01,
	0xF4,0x62,0xF2,0x01,0xF1,0xF0,0xFF,0x00,0xF1,0xFF,0xF5,0x06,0xF0,0x10,0xD4,0xE3,
	0x34,0xD3,0xE2,0x84,0xD4,0xE3,0x54,0xE2,0x34,0xE3,0x14,0xE2,0x54,0xD3,0xE3,0x84,
	0xE2,0x14,0xF1,0xF0,0xFF,0x00,0xF1,0xFF,0xF0,0x08,0xE7,0x14,0xE5,0x44,0xE4,0x64,
	0xE7,0x44,0x24,0xE6,0x54,0xE7,0x44,0x14,0x44,0x37,0x25,0x0D,0x44,0x54,0x24,0xE5,
	0x44,0xE7,0x14,0x44,0xF1,0xF0,0xFF,0x00,0xF1,0xFF,0xF0,0x02,0xE6,0x13,0x04,0x14,
	0x09,0xE5,0x14,0x0A,0x14,0x04,0xF1,0xFF,0xF0,0x08,0xE4,0xD4,0xF7,0x00,0x55,0xE2,
	0xF7,0xC8,0xC5,0xE4,0xF7,0xC8,0x85,0xE2,0xF7,0x00,0x55,0xD3,0xE4,0xF7,0x00,0x85,
	0xD4,0xE2,0xF7,0xC8,0x85,0xD3,0xE4,0xF7,0xC8,0xC5,0xE2,0xF7,0x00,0x85,0xF1,0xFF,
};