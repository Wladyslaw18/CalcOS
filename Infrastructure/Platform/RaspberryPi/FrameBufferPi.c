#include "Mailbox.h"

// Screen configuration variables
uint32_t screen_width = 1920;
uint32_t screen_height = 1080;
uint32_t screen_pitch = 0;
uint32_t* lfb_pointer = 0; // Linear FrameBuffer pointer mapped by GPU

bool pi_framebuffer_init() {
    // Mailbox command buffer (Must be 16-byte aligned for Broadcom DMA!)
    volatile uint32_t mbox[35] MBOX_ALIGN;

    mbox[0] = 35 * 4;       // Buffer size in bytes
    mbox[1] = MBOX_REQUEST; // Tag request indicator

    mbox[2] = 0x00048003;   // Tag: Set Physical Display Width/Height
    mbox[3] = 8;            // Value buffer size
    mbox[4] = 8;            // Request value length
    mbox[5] = 1920;         // Width
    mbox[6] = 1080;         // Height

    mbox[7] = 0x00048004;   // Tag: Set Virtual Display Width/Height
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = 1920;
    mbox[11] = 1080;

    mbox[12] = 0x00048005;  // Tag: Set Depth
    mbox[13] = 4;
    mbox[14] = 4;
    mbox[15] = 32;          // 32 bits per pixel (RGBA)

    mbox[16] = 0x00040001;  // Tag: Allocate FrameBuffer
    mbox[17] = 8;
    mbox[18] = 8;
    mbox[19] = 4096;        // Alignment constraint
    mbox[20] = 0;           // GPU will output pointer here!

    mbox[21] = 0x00040008;  // Tag: Get Pitch
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 0;           // GPU will output pitch size here

    mbox[25] = 0;           // End tag marker

    // Fire the command to the GPU channel 8 (Property Tag Channel)
    if (mbox_call(MBOX_CH_PROP, mbox) && mbox[20] != 0) {
        // Broadcom uses bus addressing, so mask bit 30/31 to get raw ARM CPU physical address
        lfb_pointer = (uint32_t*)((uint64_t)mbox[20] & 0x3FFFFFFF);
        screen_width = mbox[5];
        screen_height = mbox[6];
        screen_pitch = mbox[24];
        return true;
    }
    
    return false; // Display request failed
}

void pi_put_pixel(int x, int y, uint32_t pixel_rgba) {
    if (!lfb_pointer || x >= (int)screen_width || y >= (int)screen_height || x < 0 || y < 0) return;
    
    // Direct memory write to the frame coordinates!
    uint32_t offset = (y * (screen_pitch / 4)) + x;
    lfb_pointer[offset] = pixel_rgba;
}
