#include "udp.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"

#define DMA_LOVE    // Using DMA for payload copy and CRC32 Calc.

// 4B5B convert table
const static uint16_t __not_in_flash("tbl_4b5b") tbl_4b5b[256] = {
//const uint16_t __scratch_x ("tbl_4b5b") tbl_4b5b[256] = {
    0b1111011110, 0b1111001001, 0b1111010100, 0b1111010101, 0b1111001010, 0b1111001011, 0b1111001110, 0b1111001111, 0b1111010010, 0b1111010011, 0b1111010110, 0b1111010111, 0b1111011010, 0b1111011011, 0b1111011100, 0b1111011101,
    0b0100111110, 0b0100101001, 0b0100110100, 0b0100110101, 0b0100101010, 0b0100101011, 0b0100101110, 0b0100101111, 0b0100110010, 0b0100110011, 0b0100110110, 0b0100110111, 0b0100111010, 0b0100111011, 0b0100111100, 0b0100111101,
    0b1010011110, 0b1010001001, 0b1010010100, 0b1010010101, 0b1010001010, 0b1010001011, 0b1010001110, 0b1010001111, 0b1010010010, 0b1010010011, 0b1010010110, 0b1010010111, 0b1010011010, 0b1010011011, 0b1010011100, 0b1010011101,
    0b1010111110, 0b1010101001, 0b1010110100, 0b1010110101, 0b1010101010, 0b1010101011, 0b1010101110, 0b1010101111, 0b1010110010, 0b1010110011, 0b1010110110, 0b1010110111, 0b1010111010, 0b1010111011, 0b1010111100, 0b1010111101,
    0b0101011110, 0b0101001001, 0b0101010100, 0b0101010101, 0b0101001010, 0b0101001011, 0b0101001110, 0b0101001111, 0b0101010010, 0b0101010011, 0b0101010110, 0b0101010111, 0b0101011010, 0b0101011011, 0b0101011100, 0b0101011101,
    0b0101111110, 0b0101101001, 0b0101110100, 0b0101110101, 0b0101101010, 0b0101101011, 0b0101101110, 0b0101101111, 0b0101110010, 0b0101110011, 0b0101110110, 0b0101110111, 0b0101111010, 0b0101111011, 0b0101111100, 0b0101111101,
    0b0111011110, 0b0111001001, 0b0111010100, 0b0111010101, 0b0111001010, 0b0111001011, 0b0111001110, 0b0111001111, 0b0111010010, 0b0111010011, 0b0111010110, 0b0111010111, 0b0111011010, 0b0111011011, 0b0111011100, 0b0111011101,
    0b0111111110, 0b0111101001, 0b0111110100, 0b0111110101, 0b0111101010, 0b0111101011, 0b0111101110, 0b0111101111, 0b0111110010, 0b0111110011, 0b0111110110, 0b0111110111, 0b0111111010, 0b0111111011, 0b0111111100, 0b0111111101,
    0b1001011110, 0b1001001001, 0b1001010100, 0b1001010101, 0b1001001010, 0b1001001011, 0b1001001110, 0b1001001111, 0b1001010010, 0b1001010011, 0b1001010110, 0b1001010111, 0b1001011010, 0b1001011011, 0b1001011100, 0b1001011101,
    0b1001111110, 0b1001101001, 0b1001110100, 0b1001110101, 0b1001101010, 0b1001101011, 0b1001101110, 0b1001101111, 0b1001110010, 0b1001110011, 0b1001110110, 0b1001110111, 0b1001111010, 0b1001111011, 0b1001111100, 0b1001111101,
    0b1011011110, 0b1011001001, 0b1011010100, 0b1011010101, 0b1011001010, 0b1011001011, 0b1011001110, 0b1011001111, 0b1011010010, 0b1011010011, 0b1011010110, 0b1011010111, 0b1011011010, 0b1011011011, 0b1011011100, 0b1011011101,
    0b1011111110, 0b1011101001, 0b1011110100, 0b1011110101, 0b1011101010, 0b1011101011, 0b1011101110, 0b1011101111, 0b1011110010, 0b1011110011, 0b1011110110, 0b1011110111, 0b1011111010, 0b1011111011, 0b1011111100, 0b1011111101,
    0b1101011110, 0b1101001001, 0b1101010100, 0b1101010101, 0b1101001010, 0b1101001011, 0b1101001110, 0b1101001111, 0b1101010010, 0b1101010011, 0b1101010110, 0b1101010111, 0b1101011010, 0b1101011011, 0b1101011100, 0b1101011101,
    0b1101111110, 0b1101101001, 0b1101110100, 0b1101110101, 0b1101101010, 0b1101101011, 0b1101101110, 0b1101101111, 0b1101110010, 0b1101110011, 0b1101110110, 0b1101110111, 0b1101111010, 0b1101111011, 0b1101111100, 0b1101111101,
    0b1110011110, 0b1110001001, 0b1110010100, 0b1110010101, 0b1110001010, 0b1110001011, 0b1110001110, 0b1110001111, 0b1110010010, 0b1110010011, 0b1110010110, 0b1110010111, 0b1110011010, 0b1110011011, 0b1110011100, 0b1110011101,
    0b1110111110, 0b1110101001, 0b1110110100, 0b1110110101, 0b1110101010, 0b1110101011, 0b1110101110, 0b1110101111, 0b1110110010, 0b1110110011, 0b1110110110, 0b1110110111, 0b1110111010, 0b1110111011, 0b1110111100, 0b1110111101,
};

// NRZI convert table
const static uint16_t __not_in_flash("tbl_nrzi") tbl_nrzi[2048] = {
//const uint16_t __scratch_y ("tbl_nrzi") tbl_nrzi[2048] = {
    0x000, 0x3f0, 0x3f8, 0x008, 0x3fc, 0x00c, 0x004, 0x3f4, 0x3fe, 0x00e, 0x006, 0x3f6, 0x002, 0x3f2, 0x3fa, 0x00a, 0x3ff, 0x00f, 0x007, 0x3f7, 0x003, 0x3f3, 0x3fb, 0x00b, 0x001, 0x3f1, 0x3f9, 0x009, 0x3fd, 0x00d, 0x005, 0x3f5, 
    0x200, 0x1f0, 0x1f8, 0x208, 0x1fc, 0x20c, 0x204, 0x1f4, 0x1fe, 0x20e, 0x206, 0x1f6, 0x202, 0x1f2, 0x1fa, 0x20a, 0x1ff, 0x20f, 0x207, 0x1f7, 0x203, 0x1f3, 0x1fb, 0x20b, 0x201, 0x1f1, 0x1f9, 0x209, 0x1fd, 0x20d, 0x205, 0x1f5, 
    0x300, 0x0f0, 0x0f8, 0x308, 0x0fc, 0x30c, 0x304, 0x0f4, 0x0fe, 0x30e, 0x306, 0x0f6, 0x302, 0x0f2, 0x0fa, 0x30a, 0x0ff, 0x30f, 0x307, 0x0f7, 0x303, 0x0f3, 0x0fb, 0x30b, 0x301, 0x0f1, 0x0f9, 0x309, 0x0fd, 0x30d, 0x305, 0x0f5, 
    0x100, 0x2f0, 0x2f8, 0x108, 0x2fc, 0x10c, 0x104, 0x2f4, 0x2fe, 0x10e, 0x106, 0x2f6, 0x102, 0x2f2, 0x2fa, 0x10a, 0x2ff, 0x10f, 0x107, 0x2f7, 0x103, 0x2f3, 0x2fb, 0x10b, 0x101, 0x2f1, 0x2f9, 0x109, 0x2fd, 0x10d, 0x105, 0x2f5, 
    0x380, 0x070, 0x078, 0x388, 0x07c, 0x38c, 0x384, 0x074, 0x07e, 0x38e, 0x386, 0x076, 0x382, 0x072, 0x07a, 0x38a, 0x07f, 0x38f, 0x387, 0x077, 0x383, 0x073, 0x07b, 0x38b, 0x381, 0x071, 0x079, 0x389, 0x07d, 0x38d, 0x385, 0x075, 
    0x180, 0x270, 0x278, 0x188, 0x27c, 0x18c, 0x184, 0x274, 0x27e, 0x18e, 0x186, 0x276, 0x182, 0x272, 0x27a, 0x18a, 0x27f, 0x18f, 0x187, 0x277, 0x183, 0x273, 0x27b, 0x18b, 0x181, 0x271, 0x279, 0x189, 0x27d, 0x18d, 0x185, 0x275, 
    0x080, 0x370, 0x378, 0x088, 0x37c, 0x08c, 0x084, 0x374, 0x37e, 0x08e, 0x086, 0x376, 0x082, 0x372, 0x37a, 0x08a, 0x37f, 0x08f, 0x087, 0x377, 0x083, 0x373, 0x37b, 0x08b, 0x081, 0x371, 0x379, 0x089, 0x37d, 0x08d, 0x085, 0x375, 
    0x280, 0x170, 0x178, 0x288, 0x17c, 0x28c, 0x284, 0x174, 0x17e, 0x28e, 0x286, 0x176, 0x282, 0x172, 0x17a, 0x28a, 0x17f, 0x28f, 0x287, 0x177, 0x283, 0x173, 0x17b, 0x28b, 0x281, 0x171, 0x179, 0x289, 0x17d, 0x28d, 0x285, 0x175, 
    0x3c0, 0x030, 0x038, 0x3c8, 0x03c, 0x3cc, 0x3c4, 0x034, 0x03e, 0x3ce, 0x3c6, 0x036, 0x3c2, 0x032, 0x03a, 0x3ca, 0x03f, 0x3cf, 0x3c7, 0x037, 0x3c3, 0x033, 0x03b, 0x3cb, 0x3c1, 0x031, 0x039, 0x3c9, 0x03d, 0x3cd, 0x3c5, 0x035, 
    0x1c0, 0x230, 0x238, 0x1c8, 0x23c, 0x1cc, 0x1c4, 0x234, 0x23e, 0x1ce, 0x1c6, 0x236, 0x1c2, 0x232, 0x23a, 0x1ca, 0x23f, 0x1cf, 0x1c7, 0x237, 0x1c3, 0x233, 0x23b, 0x1cb, 0x1c1, 0x231, 0x239, 0x1c9, 0x23d, 0x1cd, 0x1c5, 0x235, 
    0x0c0, 0x330, 0x338, 0x0c8, 0x33c, 0x0cc, 0x0c4, 0x334, 0x33e, 0x0ce, 0x0c6, 0x336, 0x0c2, 0x332, 0x33a, 0x0ca, 0x33f, 0x0cf, 0x0c7, 0x337, 0x0c3, 0x333, 0x33b, 0x0cb, 0x0c1, 0x331, 0x339, 0x0c9, 0x33d, 0x0cd, 0x0c5, 0x335, 
    0x2c0, 0x130, 0x138, 0x2c8, 0x13c, 0x2cc, 0x2c4, 0x134, 0x13e, 0x2ce, 0x2c6, 0x136, 0x2c2, 0x132, 0x13a, 0x2ca, 0x13f, 0x2cf, 0x2c7, 0x137, 0x2c3, 0x133, 0x13b, 0x2cb, 0x2c1, 0x131, 0x139, 0x2c9, 0x13d, 0x2cd, 0x2c5, 0x135, 
    0x040, 0x3b0, 0x3b8, 0x048, 0x3bc, 0x04c, 0x044, 0x3b4, 0x3be, 0x04e, 0x046, 0x3b6, 0x042, 0x3b2, 0x3ba, 0x04a, 0x3bf, 0x04f, 0x047, 0x3b7, 0x043, 0x3b3, 0x3bb, 0x04b, 0x041, 0x3b1, 0x3b9, 0x049, 0x3bd, 0x04d, 0x045, 0x3b5, 
    0x240, 0x1b0, 0x1b8, 0x248, 0x1bc, 0x24c, 0x244, 0x1b4, 0x1be, 0x24e, 0x246, 0x1b6, 0x242, 0x1b2, 0x1ba, 0x24a, 0x1bf, 0x24f, 0x247, 0x1b7, 0x243, 0x1b3, 0x1bb, 0x24b, 0x241, 0x1b1, 0x1b9, 0x249, 0x1bd, 0x24d, 0x245, 0x1b5, 
    0x340, 0x0b0, 0x0b8, 0x348, 0x0bc, 0x34c, 0x344, 0x0b4, 0x0be, 0x34e, 0x346, 0x0b6, 0x342, 0x0b2, 0x0ba, 0x34a, 0x0bf, 0x34f, 0x347, 0x0b7, 0x343, 0x0b3, 0x0bb, 0x34b, 0x341, 0x0b1, 0x0b9, 0x349, 0x0bd, 0x34d, 0x345, 0x0b5, 
    0x140, 0x2b0, 0x2b8, 0x148, 0x2bc, 0x14c, 0x144, 0x2b4, 0x2be, 0x14e, 0x146, 0x2b6, 0x142, 0x2b2, 0x2ba, 0x14a, 0x2bf, 0x14f, 0x147, 0x2b7, 0x143, 0x2b3, 0x2bb, 0x14b, 0x141, 0x2b1, 0x2b9, 0x149, 0x2bd, 0x14d, 0x145, 0x2b5, 
    0x3e0, 0x010, 0x018, 0x3e8, 0x01c, 0x3ec, 0x3e4, 0x014, 0x01e, 0x3ee, 0x3e6, 0x016, 0x3e2, 0x012, 0x01a, 0x3ea, 0x01f, 0x3ef, 0x3e7, 0x017, 0x3e3, 0x013, 0x01b, 0x3eb, 0x3e1, 0x011, 0x019, 0x3e9, 0x01d, 0x3ed, 0x3e5, 0x015, 
    0x1e0, 0x210, 0x218, 0x1e8, 0x21c, 0x1ec, 0x1e4, 0x214, 0x21e, 0x1ee, 0x1e6, 0x216, 0x1e2, 0x212, 0x21a, 0x1ea, 0x21f, 0x1ef, 0x1e7, 0x217, 0x1e3, 0x213, 0x21b, 0x1eb, 0x1e1, 0x211, 0x219, 0x1e9, 0x21d, 0x1ed, 0x1e5, 0x215, 
    0x0e0, 0x310, 0x318, 0x0e8, 0x31c, 0x0ec, 0x0e4, 0x314, 0x31e, 0x0ee, 0x0e6, 0x316, 0x0e2, 0x312, 0x31a, 0x0ea, 0x31f, 0x0ef, 0x0e7, 0x317, 0x0e3, 0x313, 0x31b, 0x0eb, 0x0e1, 0x311, 0x319, 0x0e9, 0x31d, 0x0ed, 0x0e5, 0x315, 
    0x2e0, 0x110, 0x118, 0x2e8, 0x11c, 0x2ec, 0x2e4, 0x114, 0x11e, 0x2ee, 0x2e6, 0x116, 0x2e2, 0x112, 0x11a, 0x2ea, 0x11f, 0x2ef, 0x2e7, 0x117, 0x2e3, 0x113, 0x11b, 0x2eb, 0x2e1, 0x111, 0x119, 0x2e9, 0x11d, 0x2ed, 0x2e5, 0x115, 
    0x060, 0x390, 0x398, 0x068, 0x39c, 0x06c, 0x064, 0x394, 0x39e, 0x06e, 0x066, 0x396, 0x062, 0x392, 0x39a, 0x06a, 0x39f, 0x06f, 0x067, 0x397, 0x063, 0x393, 0x39b, 0x06b, 0x061, 0x391, 0x399, 0x069, 0x39d, 0x06d, 0x065, 0x395, 
    0x260, 0x190, 0x198, 0x268, 0x19c, 0x26c, 0x264, 0x194, 0x19e, 0x26e, 0x266, 0x196, 0x262, 0x192, 0x19a, 0x26a, 0x19f, 0x26f, 0x267, 0x197, 0x263, 0x193, 0x19b, 0x26b, 0x261, 0x191, 0x199, 0x269, 0x19d, 0x26d, 0x265, 0x195, 
    0x360, 0x090, 0x098, 0x368, 0x09c, 0x36c, 0x364, 0x094, 0x09e, 0x36e, 0x366, 0x096, 0x362, 0x092, 0x09a, 0x36a, 0x09f, 0x36f, 0x367, 0x097, 0x363, 0x093, 0x09b, 0x36b, 0x361, 0x091, 0x099, 0x369, 0x09d, 0x36d, 0x365, 0x095, 
    0x160, 0x290, 0x298, 0x168, 0x29c, 0x16c, 0x164, 0x294, 0x29e, 0x16e, 0x166, 0x296, 0x162, 0x292, 0x29a, 0x16a, 0x29f, 0x16f, 0x167, 0x297, 0x163, 0x293, 0x29b, 0x16b, 0x161, 0x291, 0x299, 0x169, 0x29d, 0x16d, 0x165, 0x295, 
    0x020, 0x3d0, 0x3d8, 0x028, 0x3dc, 0x02c, 0x024, 0x3d4, 0x3de, 0x02e, 0x026, 0x3d6, 0x022, 0x3d2, 0x3da, 0x02a, 0x3df, 0x02f, 0x027, 0x3d7, 0x023, 0x3d3, 0x3db, 0x02b, 0x021, 0x3d1, 0x3d9, 0x029, 0x3dd, 0x02d, 0x025, 0x3d5, 
    0x220, 0x1d0, 0x1d8, 0x228, 0x1dc, 0x22c, 0x224, 0x1d4, 0x1de, 0x22e, 0x226, 0x1d6, 0x222, 0x1d2, 0x1da, 0x22a, 0x1df, 0x22f, 0x227, 0x1d7, 0x223, 0x1d3, 0x1db, 0x22b, 0x221, 0x1d1, 0x1d9, 0x229, 0x1dd, 0x22d, 0x225, 0x1d5, 
    0x320, 0x0d0, 0x0d8, 0x328, 0x0dc, 0x32c, 0x324, 0x0d4, 0x0de, 0x32e, 0x326, 0x0d6, 0x322, 0x0d2, 0x0da, 0x32a, 0x0df, 0x32f, 0x327, 0x0d7, 0x323, 0x0d3, 0x0db, 0x32b, 0x321, 0x0d1, 0x0d9, 0x329, 0x0dd, 0x32d, 0x325, 0x0d5, 
    0x120, 0x2d0, 0x2d8, 0x128, 0x2dc, 0x12c, 0x124, 0x2d4, 0x2de, 0x12e, 0x126, 0x2d6, 0x122, 0x2d2, 0x2da, 0x12a, 0x2df, 0x12f, 0x127, 0x2d7, 0x123, 0x2d3, 0x2db, 0x12b, 0x121, 0x2d1, 0x2d9, 0x129, 0x2dd, 0x12d, 0x125, 0x2d5, 
    0x3a0, 0x050, 0x058, 0x3a8, 0x05c, 0x3ac, 0x3a4, 0x054, 0x05e, 0x3ae, 0x3a6, 0x056, 0x3a2, 0x052, 0x05a, 0x3aa, 0x05f, 0x3af, 0x3a7, 0x057, 0x3a3, 0x053, 0x05b, 0x3ab, 0x3a1, 0x051, 0x059, 0x3a9, 0x05d, 0x3ad, 0x3a5, 0x055, 
    0x1a0, 0x250, 0x258, 0x1a8, 0x25c, 0x1ac, 0x1a4, 0x254, 0x25e, 0x1ae, 0x1a6, 0x256, 0x1a2, 0x252, 0x25a, 0x1aa, 0x25f, 0x1af, 0x1a7, 0x257, 0x1a3, 0x253, 0x25b, 0x1ab, 0x1a1, 0x251, 0x259, 0x1a9, 0x25d, 0x1ad, 0x1a5, 0x255, 
    0x0a0, 0x350, 0x358, 0x0a8, 0x35c, 0x0ac, 0x0a4, 0x354, 0x35e, 0x0ae, 0x0a6, 0x356, 0x0a2, 0x352, 0x35a, 0x0aa, 0x35f, 0x0af, 0x0a7, 0x357, 0x0a3, 0x353, 0x35b, 0x0ab, 0x0a1, 0x351, 0x359, 0x0a9, 0x35d, 0x0ad, 0x0a5, 0x355, 
    0x2a0, 0x150, 0x158, 0x2a8, 0x15c, 0x2ac, 0x2a4, 0x154, 0x15e, 0x2ae, 0x2a6, 0x156, 0x2a2, 0x152, 0x15a, 0x2aa, 0x15f, 0x2af, 0x2a7, 0x157, 0x2a3, 0x153, 0x15b, 0x2ab, 0x2a1, 0x151, 0x159, 0x2a9, 0x15d, 0x2ad, 0x2a5, 0x155, 
    0x3ff, 0x00f, 0x007, 0x3f7, 0x003, 0x3f3, 0x3fb, 0x00b, 0x001, 0x3f1, 0x3f9, 0x009, 0x3fd, 0x00d, 0x005, 0x3f5, 0x000, 0x3f0, 0x3f8, 0x008, 0x3fc, 0x00c, 0x004, 0x3f4, 0x3fe, 0x00e, 0x006, 0x3f6, 0x002, 0x3f2, 0x3fa, 0x00a, 
    0x1ff, 0x20f, 0x207, 0x1f7, 0x203, 0x1f3, 0x1fb, 0x20b, 0x201, 0x1f1, 0x1f9, 0x209, 0x1fd, 0x20d, 0x205, 0x1f5, 0x200, 0x1f0, 0x1f8, 0x208, 0x1fc, 0x20c, 0x204, 0x1f4, 0x1fe, 0x20e, 0x206, 0x1f6, 0x202, 0x1f2, 0x1fa, 0x20a, 
    0x0ff, 0x30f, 0x307, 0x0f7, 0x303, 0x0f3, 0x0fb, 0x30b, 0x301, 0x0f1, 0x0f9, 0x309, 0x0fd, 0x30d, 0x305, 0x0f5, 0x300, 0x0f0, 0x0f8, 0x308, 0x0fc, 0x30c, 0x304, 0x0f4, 0x0fe, 0x30e, 0x306, 0x0f6, 0x302, 0x0f2, 0x0fa, 0x30a, 
    0x2ff, 0x10f, 0x107, 0x2f7, 0x103, 0x2f3, 0x2fb, 0x10b, 0x101, 0x2f1, 0x2f9, 0x109, 0x2fd, 0x10d, 0x105, 0x2f5, 0x100, 0x2f0, 0x2f8, 0x108, 0x2fc, 0x10c, 0x104, 0x2f4, 0x2fe, 0x10e, 0x106, 0x2f6, 0x102, 0x2f2, 0x2fa, 0x10a, 
    0x07f, 0x38f, 0x387, 0x077, 0x383, 0x073, 0x07b, 0x38b, 0x381, 0x071, 0x079, 0x389, 0x07d, 0x38d, 0x385, 0x075, 0x380, 0x070, 0x078, 0x388, 0x07c, 0x38c, 0x384, 0x074, 0x07e, 0x38e, 0x386, 0x076, 0x382, 0x072, 0x07a, 0x38a, 
    0x27f, 0x18f, 0x187, 0x277, 0x183, 0x273, 0x27b, 0x18b, 0x181, 0x271, 0x279, 0x189, 0x27d, 0x18d, 0x185, 0x275, 0x180, 0x270, 0x278, 0x188, 0x27c, 0x18c, 0x184, 0x274, 0x27e, 0x18e, 0x186, 0x276, 0x182, 0x272, 0x27a, 0x18a, 
    0x37f, 0x08f, 0x087, 0x377, 0x083, 0x373, 0x37b, 0x08b, 0x081, 0x371, 0x379, 0x089, 0x37d, 0x08d, 0x085, 0x375, 0x080, 0x370, 0x378, 0x088, 0x37c, 0x08c, 0x084, 0x374, 0x37e, 0x08e, 0x086, 0x376, 0x082, 0x372, 0x37a, 0x08a, 
    0x17f, 0x28f, 0x287, 0x177, 0x283, 0x173, 0x17b, 0x28b, 0x281, 0x171, 0x179, 0x289, 0x17d, 0x28d, 0x285, 0x175, 0x280, 0x170, 0x178, 0x288, 0x17c, 0x28c, 0x284, 0x174, 0x17e, 0x28e, 0x286, 0x176, 0x282, 0x172, 0x17a, 0x28a, 
    0x03f, 0x3cf, 0x3c7, 0x037, 0x3c3, 0x033, 0x03b, 0x3cb, 0x3c1, 0x031, 0x039, 0x3c9, 0x03d, 0x3cd, 0x3c5, 0x035, 0x3c0, 0x030, 0x038, 0x3c8, 0x03c, 0x3cc, 0x3c4, 0x034, 0x03e, 0x3ce, 0x3c6, 0x036, 0x3c2, 0x032, 0x03a, 0x3ca, 
    0x23f, 0x1cf, 0x1c7, 0x237, 0x1c3, 0x233, 0x23b, 0x1cb, 0x1c1, 0x231, 0x239, 0x1c9, 0x23d, 0x1cd, 0x1c5, 0x235, 0x1c0, 0x230, 0x238, 0x1c8, 0x23c, 0x1cc, 0x1c4, 0x234, 0x23e, 0x1ce, 0x1c6, 0x236, 0x1c2, 0x232, 0x23a, 0x1ca, 
    0x33f, 0x0cf, 0x0c7, 0x337, 0x0c3, 0x333, 0x33b, 0x0cb, 0x0c1, 0x331, 0x339, 0x0c9, 0x33d, 0x0cd, 0x0c5, 0x335, 0x0c0, 0x330, 0x338, 0x0c8, 0x33c, 0x0cc, 0x0c4, 0x334, 0x33e, 0x0ce, 0x0c6, 0x336, 0x0c2, 0x332, 0x33a, 0x0ca, 
    0x13f, 0x2cf, 0x2c7, 0x137, 0x2c3, 0x133, 0x13b, 0x2cb, 0x2c1, 0x131, 0x139, 0x2c9, 0x13d, 0x2cd, 0x2c5, 0x135, 0x2c0, 0x130, 0x138, 0x2c8, 0x13c, 0x2cc, 0x2c4, 0x134, 0x13e, 0x2ce, 0x2c6, 0x136, 0x2c2, 0x132, 0x13a, 0x2ca, 
    0x3bf, 0x04f, 0x047, 0x3b7, 0x043, 0x3b3, 0x3bb, 0x04b, 0x041, 0x3b1, 0x3b9, 0x049, 0x3bd, 0x04d, 0x045, 0x3b5, 0x040, 0x3b0, 0x3b8, 0x048, 0x3bc, 0x04c, 0x044, 0x3b4, 0x3be, 0x04e, 0x046, 0x3b6, 0x042, 0x3b2, 0x3ba, 0x04a, 
    0x1bf, 0x24f, 0x247, 0x1b7, 0x243, 0x1b3, 0x1bb, 0x24b, 0x241, 0x1b1, 0x1b9, 0x249, 0x1bd, 0x24d, 0x245, 0x1b5, 0x240, 0x1b0, 0x1b8, 0x248, 0x1bc, 0x24c, 0x244, 0x1b4, 0x1be, 0x24e, 0x246, 0x1b6, 0x242, 0x1b2, 0x1ba, 0x24a, 
    0x0bf, 0x34f, 0x347, 0x0b7, 0x343, 0x0b3, 0x0bb, 0x34b, 0x341, 0x0b1, 0x0b9, 0x349, 0x0bd, 0x34d, 0x345, 0x0b5, 0x340, 0x0b0, 0x0b8, 0x348, 0x0bc, 0x34c, 0x344, 0x0b4, 0x0be, 0x34e, 0x346, 0x0b6, 0x342, 0x0b2, 0x0ba, 0x34a, 
    0x2bf, 0x14f, 0x147, 0x2b7, 0x143, 0x2b3, 0x2bb, 0x14b, 0x141, 0x2b1, 0x2b9, 0x149, 0x2bd, 0x14d, 0x145, 0x2b5, 0x140, 0x2b0, 0x2b8, 0x148, 0x2bc, 0x14c, 0x144, 0x2b4, 0x2be, 0x14e, 0x146, 0x2b6, 0x142, 0x2b2, 0x2ba, 0x14a, 
    0x01f, 0x3ef, 0x3e7, 0x017, 0x3e3, 0x013, 0x01b, 0x3eb, 0x3e1, 0x011, 0x019, 0x3e9, 0x01d, 0x3ed, 0x3e5, 0x015, 0x3e0, 0x010, 0x018, 0x3e8, 0x01c, 0x3ec, 0x3e4, 0x014, 0x01e, 0x3ee, 0x3e6, 0x016, 0x3e2, 0x012, 0x01a, 0x3ea, 
    0x21f, 0x1ef, 0x1e7, 0x217, 0x1e3, 0x213, 0x21b, 0x1eb, 0x1e1, 0x211, 0x219, 0x1e9, 0x21d, 0x1ed, 0x1e5, 0x215, 0x1e0, 0x210, 0x218, 0x1e8, 0x21c, 0x1ec, 0x1e4, 0x214, 0x21e, 0x1ee, 0x1e6, 0x216, 0x1e2, 0x212, 0x21a, 0x1ea, 
    0x31f, 0x0ef, 0x0e7, 0x317, 0x0e3, 0x313, 0x31b, 0x0eb, 0x0e1, 0x311, 0x319, 0x0e9, 0x31d, 0x0ed, 0x0e5, 0x315, 0x0e0, 0x310, 0x318, 0x0e8, 0x31c, 0x0ec, 0x0e4, 0x314, 0x31e, 0x0ee, 0x0e6, 0x316, 0x0e2, 0x312, 0x31a, 0x0ea, 
    0x11f, 0x2ef, 0x2e7, 0x117, 0x2e3, 0x113, 0x11b, 0x2eb, 0x2e1, 0x111, 0x119, 0x2e9, 0x11d, 0x2ed, 0x2e5, 0x115, 0x2e0, 0x110, 0x118, 0x2e8, 0x11c, 0x2ec, 0x2e4, 0x114, 0x11e, 0x2ee, 0x2e6, 0x116, 0x2e2, 0x112, 0x11a, 0x2ea, 
    0x39f, 0x06f, 0x067, 0x397, 0x063, 0x393, 0x39b, 0x06b, 0x061, 0x391, 0x399, 0x069, 0x39d, 0x06d, 0x065, 0x395, 0x060, 0x390, 0x398, 0x068, 0x39c, 0x06c, 0x064, 0x394, 0x39e, 0x06e, 0x066, 0x396, 0x062, 0x392, 0x39a, 0x06a, 
    0x19f, 0x26f, 0x267, 0x197, 0x263, 0x193, 0x19b, 0x26b, 0x261, 0x191, 0x199, 0x269, 0x19d, 0x26d, 0x265, 0x195, 0x260, 0x190, 0x198, 0x268, 0x19c, 0x26c, 0x264, 0x194, 0x19e, 0x26e, 0x266, 0x196, 0x262, 0x192, 0x19a, 0x26a, 
    0x09f, 0x36f, 0x367, 0x097, 0x363, 0x093, 0x09b, 0x36b, 0x361, 0x091, 0x099, 0x369, 0x09d, 0x36d, 0x365, 0x095, 0x360, 0x090, 0x098, 0x368, 0x09c, 0x36c, 0x364, 0x094, 0x09e, 0x36e, 0x366, 0x096, 0x362, 0x092, 0x09a, 0x36a, 
    0x29f, 0x16f, 0x167, 0x297, 0x163, 0x293, 0x29b, 0x16b, 0x161, 0x291, 0x299, 0x169, 0x29d, 0x16d, 0x165, 0x295, 0x160, 0x290, 0x298, 0x168, 0x29c, 0x16c, 0x164, 0x294, 0x29e, 0x16e, 0x166, 0x296, 0x162, 0x292, 0x29a, 0x16a, 
    0x3df, 0x02f, 0x027, 0x3d7, 0x023, 0x3d3, 0x3db, 0x02b, 0x021, 0x3d1, 0x3d9, 0x029, 0x3dd, 0x02d, 0x025, 0x3d5, 0x020, 0x3d0, 0x3d8, 0x028, 0x3dc, 0x02c, 0x024, 0x3d4, 0x3de, 0x02e, 0x026, 0x3d6, 0x022, 0x3d2, 0x3da, 0x02a, 
    0x1df, 0x22f, 0x227, 0x1d7, 0x223, 0x1d3, 0x1db, 0x22b, 0x221, 0x1d1, 0x1d9, 0x229, 0x1dd, 0x22d, 0x225, 0x1d5, 0x220, 0x1d0, 0x1d8, 0x228, 0x1dc, 0x22c, 0x224, 0x1d4, 0x1de, 0x22e, 0x226, 0x1d6, 0x222, 0x1d2, 0x1da, 0x22a, 
    0x0df, 0x32f, 0x327, 0x0d7, 0x323, 0x0d3, 0x0db, 0x32b, 0x321, 0x0d1, 0x0d9, 0x329, 0x0dd, 0x32d, 0x325, 0x0d5, 0x320, 0x0d0, 0x0d8, 0x328, 0x0dc, 0x32c, 0x324, 0x0d4, 0x0de, 0x32e, 0x326, 0x0d6, 0x322, 0x0d2, 0x0da, 0x32a, 
    0x2df, 0x12f, 0x127, 0x2d7, 0x123, 0x2d3, 0x2db, 0x12b, 0x121, 0x2d1, 0x2d9, 0x129, 0x2dd, 0x12d, 0x125, 0x2d5, 0x120, 0x2d0, 0x2d8, 0x128, 0x2dc, 0x12c, 0x124, 0x2d4, 0x2de, 0x12e, 0x126, 0x2d6, 0x122, 0x2d2, 0x2da, 0x12a, 
    0x05f, 0x3af, 0x3a7, 0x057, 0x3a3, 0x053, 0x05b, 0x3ab, 0x3a1, 0x051, 0x059, 0x3a9, 0x05d, 0x3ad, 0x3a5, 0x055, 0x3a0, 0x050, 0x058, 0x3a8, 0x05c, 0x3ac, 0x3a4, 0x054, 0x05e, 0x3ae, 0x3a6, 0x056, 0x3a2, 0x052, 0x05a, 0x3aa, 
    0x25f, 0x1af, 0x1a7, 0x257, 0x1a3, 0x253, 0x25b, 0x1ab, 0x1a1, 0x251, 0x259, 0x1a9, 0x25d, 0x1ad, 0x1a5, 0x255, 0x1a0, 0x250, 0x258, 0x1a8, 0x25c, 0x1ac, 0x1a4, 0x254, 0x25e, 0x1ae, 0x1a6, 0x256, 0x1a2, 0x252, 0x25a, 0x1aa, 
    0x35f, 0x0af, 0x0a7, 0x357, 0x0a3, 0x353, 0x35b, 0x0ab, 0x0a1, 0x351, 0x359, 0x0a9, 0x35d, 0x0ad, 0x0a5, 0x355, 0x0a0, 0x350, 0x358, 0x0a8, 0x35c, 0x0ac, 0x0a4, 0x354, 0x35e, 0x0ae, 0x0a6, 0x356, 0x0a2, 0x352, 0x35a, 0x0aa, 
    0x15f, 0x2af, 0x2a7, 0x157, 0x2a3, 0x153, 0x15b, 0x2ab, 0x2a1, 0x151, 0x159, 0x2a9, 0x15d, 0x2ad, 0x2a5, 0x155, 0x2a0, 0x150, 0x158, 0x2a8, 0x15c, 0x2ac, 0x2a4, 0x154, 0x15e, 0x2ae, 0x2a6, 0x156, 0x2a2, 0x152, 0x15a, 0x2aa, 
};

static uint32_t crc_table[256];
//static uint8_t  data_8b[DEF_UDP_BUF_SIZE];
static uint8_t __scratch_x ("data_8b") data_8b[DEF_UDP_BUF_SIZE];
static uint16_t ip_identifier = 0;
static uint32_t ip_chk_sum1, ip_chk_sum2, ip_chk_sum3;

// Etherent Frame
static const uint16_t  eth_type            = 0x0800; // IP

// IPv4 Header
static const uint8_t   ip_version          = 4;      // IP v4
static const uint8_t   ip_head_len         = 5;
static const uint8_t   ip_type_of_service  = 0;
static const uint16_t  ip_total_len        = 20 + DEF_UDP_LEN;

static uint32_t DMA_UDP;
static dma_channel_config c0;


static void _make_crc_table(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (uint32_t j = 0; j < 8; j++) {
            c = c & 1 ? (c >> 1) ^ 0xEDB88320 : (c >> 1);
        }
        crc_table[i] = c;
    }
}


void udp_init(void) {

#ifdef DMA_LOVE
    DMA_UDP = dma_claim_unused_channel(true);
    c0 = dma_channel_get_default_config(DMA_UDP);
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_8);
    channel_config_set_read_increment(&c0, true);
    channel_config_set_write_increment(&c0, true);
#else
    _make_crc_table();
#endif
}


void __time_critical_func(udp_packet_gen)(uint32_t *buf, uint8_t *udp_payload) {
    uint16_t udp_chksum = 0;
    uint32_t i, j, idx = 0;

    // Calculate the ip check sum
    ip_chk_sum1 = 0x0000C512 + ip_identifier + ip_total_len + (DEF_IP_ADR_SRC1 << 8) + DEF_IP_ADR_SRC2 + (DEF_IP_ADR_SRC3 << 8) + DEF_IP_ADR_SRC4 +
                  (DEF_IP_DST_DST1 << 8) + DEF_IP_DST_DST2 + (DEF_IP_DST_DST3 << 8) + DEF_IP_DST_DST4;
    ip_chk_sum2 = (ip_chk_sum1 & 0x0000FFFF) + (ip_chk_sum1 >> 16);
    ip_chk_sum3 = ~((ip_chk_sum2 & 0x0000FFFF) + (ip_chk_sum2 >> 16));

    //==========================================================================
    ip_identifier++;

    // Preamble
    for (i = 0; i < 7; i++) {
        data_8b[idx++] = 0x55;
    }
    // SFD
    data_8b[idx++] = 0xD5;
    // Destination MAC Address
    data_8b[idx++] = (DEF_ETH_DST_MAC >> 40) & 0xFF;
    data_8b[idx++] = (DEF_ETH_DST_MAC >> 32) & 0xFF;
    data_8b[idx++] = (DEF_ETH_DST_MAC >> 24) & 0xFF;
    data_8b[idx++] = (DEF_ETH_DST_MAC >> 16) & 0xFF;
    data_8b[idx++] = (DEF_ETH_DST_MAC >>  8) & 0xFF;
    data_8b[idx++] = (DEF_ETH_DST_MAC >>  0) & 0xFF;
    // Source MAC Address
    data_8b[idx++] = (DEF_ETH_SRC_MAC >> 40) & 0xFF;
    data_8b[idx++] = (DEF_ETH_SRC_MAC >> 32) & 0xFF;
    data_8b[idx++] = (DEF_ETH_SRC_MAC >> 24) & 0xFF;
    data_8b[idx++] = (DEF_ETH_SRC_MAC >> 16) & 0xFF;
    data_8b[idx++] = (DEF_ETH_SRC_MAC >>  8) & 0xFF;
    data_8b[idx++] = (DEF_ETH_SRC_MAC >>  0) & 0xFF;
    // Ethernet Type
    data_8b[idx++] = (eth_type >>  8) & 0xFF;
    data_8b[idx++] = (eth_type >>  0) & 0xFF;
    // IP Header
    data_8b[idx++] = (ip_version << 4) | (ip_head_len & 0x0F);
    data_8b[idx++] = (ip_type_of_service >>  0) & 0xFF;
    data_8b[idx++] = (ip_total_len >>  8) & 0xFF;
    data_8b[idx++] = (ip_total_len >>  0) & 0xFF;
    data_8b[idx++] = (ip_identifier >>  8) & 0xFF;
    data_8b[idx++] = (ip_identifier >>  0) & 0xFF;
    data_8b[idx++] = 0x00;
    data_8b[idx++] = 0x00;
    data_8b[idx++] = 0x80;
    data_8b[idx++] = 0x11;
    // IP Check SUM
    data_8b[idx++] = (ip_chk_sum3 >>  8) & 0xFF;
    data_8b[idx++] = (ip_chk_sum3 >>  0) & 0xFF;
    // IP Source
    data_8b[idx++] = DEF_IP_ADR_SRC1;
    data_8b[idx++] = DEF_IP_ADR_SRC2;
    data_8b[idx++] = DEF_IP_ADR_SRC3;
    data_8b[idx++] = DEF_IP_ADR_SRC4;
    // IP Destination
    data_8b[idx++] = DEF_IP_DST_DST1;
    data_8b[idx++] = DEF_IP_DST_DST2;
    data_8b[idx++] = DEF_IP_DST_DST3;
    data_8b[idx++] = DEF_IP_DST_DST4;
    // UDP header
    data_8b[idx++] = (DEF_UDP_SRC_PORTNUM >>  8) & 0xFF;
    data_8b[idx++] = (DEF_UDP_SRC_PORTNUM >>  0) & 0xFF;
    data_8b[idx++] = (DEF_UDP_DST_PORTNUM >>  8) & 0xFF;
    data_8b[idx++] = (DEF_UDP_DST_PORTNUM >>  0) & 0xFF;
    data_8b[idx++] = (DEF_UDP_LEN >>  8) & 0xFF;
    data_8b[idx++] = (DEF_UDP_LEN >>  0) & 0xFF;
    data_8b[idx++] = (udp_chksum >>  8) & 0xFF;
    data_8b[idx++] = (udp_chksum >>  0) & 0xFF;

    // UDP payload
#ifdef DMA_LOVE
    // DMA使用
    channel_config_set_read_increment(&c0, true);
    channel_config_set_write_increment(&c0, true);
    dma_channel_configure (
        DMA_UDP,                // Channel to be configured
        &c0,                    // The configuration we just created
        &data_8b[idx],          // Destination address
        udp_payload,            // Source address
        DEF_UDP_PAYLOAD_SIZE,   // Number of transfers
        true                    // Start yet
    );
    dma_channel_wait_for_finish_blocking(DMA_UDP);  // 転送完了待機
    idx += DEF_UDP_PAYLOAD_SIZE;
#else
    // forループコピー
    for (i = 0; i < DEF_UDP_PAYLOAD_SIZE; i++) {
        data_8b[idx++] = udp_payload[i];
    }
#endif


    //==========================================================================
    // FCS Calc
    //==========================================================================
#ifdef DMA_LOVE
    // DMA転送によるCRC演算
    channel_config_set_read_increment(&c0, true);
    channel_config_set_write_increment(&c0, false); // 転送先はNULLのためインクリ不要
    dma_channel_configure (
        DMA_UDP,                // Channel to be configured
        &c0,                    // The configuration we just created
        NULL,                   // Destination address
        &data_8b[8],            // Source address
        (idx-8),                // Number of transfers
        false                   // Don't start yet
    );
    dma_sniffer_enable(DMA_UDP, 1, true);                   // CRC Mode = Calculate a CRC-32 (IEEE802.3 polynomial) with bit reversed data
    //dma_sniffer_set_byte_swap_enabled(true);              // 1Byte単位の転送なのでSwapはなくてもOK
    hw_set_bits(&dma_hw->sniff_ctrl,                        // おまじない
               (DMA_SNIFF_CTRL_OUT_INV_BITS | DMA_SNIFF_CTRL_OUT_REV_BITS));
    dma_hw->sniff_data = 0xffffffff;                        // CRCシード初期化
    dma_channel_set_read_addr(DMA_UDP, &data_8b[8], true);  // 転送開始
    dma_channel_wait_for_finish_blocking(DMA_UDP);          // 転送完了待機
    uint32_t crc = dma_hw->sniff_data;                      // CRC演算結果取得

    // CRC結果格納（FCS）
    data_8b[idx++] = (crc >>  0) & 0xFF;
    data_8b[idx++] = (crc >>  8) & 0xFF;
    data_8b[idx++] = (crc >> 16) & 0xFF;
    data_8b[idx++] = (crc >> 24) & 0xFF;
#else
    // テーブル演算によるCRC計算
    uint32_t crc = 0xffffffff;
    for (i = 8; i < idx; i++) {
        crc = (crc >> 8) ^ crc_table[(crc ^ data_8b[i]) & 0xFF];
    }
    crc ^= 0xffffffff;
    data_8b[idx++] = (crc >>  0) & 0xFF;
    data_8b[idx++] = (crc >>  8) & 0xFF;
    data_8b[idx++] = (crc >> 16) & 0xFF;
    data_8b[idx++] = (crc >> 24) & 0xFF;
#endif

    //==========================================================================
    // Encording 4b5b & NRZI Encoder
    //  高速化のため8bit単位で処理
    //==========================================================================
    uint16_t ob = 0;
    uint32_t ans;

    ans = tbl_nrzi[0b1000111000];           // [9:5]=K, [4:0]=J
    buf[0] = ans;
    ob = (ans << 1) & 0x400;
    
    for (i = 1; i < DEF_UDP_BUF_SIZE; i++) {
        ans = tbl_nrzi[ob + tbl_4b5b[data_8b[i]]];
        ob = (ans << 1) & 0x400;
        buf[i] = ans;
    }

    buf[DEF_UDP_BUF_SIZE] = tbl_nrzi[ob + 0b0011101101];      // [9:5]=R, [4:0]=T
}
