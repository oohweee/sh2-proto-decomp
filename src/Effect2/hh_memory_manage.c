/*
 * hh_memory_manage.c: memory blocks of the Effect2 system.
 *
 * The effect system keeps four memory blocks, indexed by MemoryBlock_Type:
 *
 *   0 : packet buffer           (0x60000 bytes, carved from the effect work area)
 *   1 : object work             (0x80000 bytes, carved from the effect work area)
 *   2 : texture buffer          (4 x 0x44800 bytes, carved from the effect work area)
 *   3 : always-resident texture buffer (_texture_buffer_always, 0x44800 bytes, bss)
 *
 * Blocks 0-2 are laid out back to back from MemShareGetEffect2WorkAddr(), each
 * one starting on a 64-byte boundary (see
 * HH_MemoryManager_DesignateSize_Alignment64Address_Calculator). Block 3 is
 * static and always present.
 *
 * Matching: the two asserts bake "hh_memory_manage.c:243" and "hh_memory_manage.c:276" into
 * their messages, so they have to stay on those lines; the blank lines below keep them there.
 */

#include "sh2.h"

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 137
/* Always-resident texture buffer (memory block 3). */
static u_long128 _texture_buffer_always[1][0x44800 / sizeof(u_long128)] __attribute__((aligned(64)));

/* Start address of each memory block, indexed by MemoryBlock_Type. */
static void *_pMemroyBlock_Table[4] = {
    NULL,
    NULL,
    NULL,
    _texture_buffer_always,
};

static void *AllocateMemoryBlock_Get(unsigned int MemoryBlock_Type);

/**
 * Register the start address of a memory block.
 * @param MemoryBlock_Type block number (0..3)
 * @param pAddress         start address of the block
 * @return 1
 */
static unsigned int MemoryBlock_Allocate(unsigned int MemoryBlock_Type, void *pAddress) {
    unsigned int result;

    _pMemroyBlock_Table[MemoryBlock_Type] = pAddress;
    result = 1;
    return result;
}

/**
 * Check whether a memory block has been allocated.
 * @param MemoryBlock_Type block number (0..3)
 * @return 1 if allocated, 0 if not
 */
static unsigned int AllocateMemoryBlock_Check(unsigned int MemoryBlock_Type) {
    unsigned int result;

    result = 0;
    if (AllocateMemoryBlock_Get(MemoryBlock_Type)) {
        result = 1;
    }
    return result;
}

/**
 * Get the start address of a memory block.
 * @param MemoryBlock_Type block number (0..3)
 * @return start address (NULL if not allocated)
 */
static void *AllocateMemoryBlock_Get(unsigned int MemoryBlock_Type) {
    return _pMemroyBlock_Table[MemoryBlock_Type];
}

/**
 * Release a memory block.
 * @param MemoryBlock_Type block number (0..3)
 * @return 1 if it was allocated, 0 if not
 */
static unsigned int AllocateMemoryBlock_Release(unsigned int MemoryBlock_Type) {
    unsigned int result;

    result = 0;
    if (_pMemroyBlock_Table[MemoryBlock_Type]) {
        _pMemroyBlock_Table[MemoryBlock_Type] = NULL;
        result = 1;
    }
    return result;
}

/**
 * Public wrapper of AllocateMemoryBlock_Check.
 * @param MemoryBlock_Type block number (0..3)
 * @return 1 if allocated, 0 if not
 */
unsigned int HH_MemoryManager_AllocateMemoryBlock_Check(unsigned int MemoryBlock_Type) {
    return AllocateMemoryBlock_Check(MemoryBlock_Type);
}

/**
 * Public wrapper of AllocateMemoryBlock_Get.
 * @param MemoryBlock_Type block number (0..3)
 * @return start address (NULL if not allocated)
 */
void *HH_MemoryManager_AllocateMemoryBlock_Get(unsigned int MemoryBlock_Type) {
    return AllocateMemoryBlock_Get(MemoryBlock_Type);
}

/**
 * Address just past Number items of Size bytes from pAddress, rounded up to 16 bytes.
 * @param pAddress start address (must be 16-byte aligned)
 * @param Size     size of one item
 * @param Number   number of items
 * @return next 16-byte aligned address
 */
void *HH_MemoryManager_DesignateSize_Alignment16Address_Calculator(void *pAddress, unsigned int Size,
                                                                   unsigned int Number) {
    void *result;
    unsigned int Next_Address;

    if (((unsigned int)pAddress & 0xF) == 0) {
        Next_Address = (unsigned int)pAddress;
        Next_Address += Size * Number;
        if (Next_Address & 0xF) {
            Next_Address = Next_Address - (Next_Address & 0xF) + 0x10;
        }
        result = (void *)Next_Address;
        return result;
    } else {
        assert_dw(0);
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 254
/**
 * Address just past Number items of Size bytes from pAddress, rounded up to 64 bytes. Same as the
 * 16-byte version; the DMA-friendly one used for the effect memory blocks.
 * @param pAddress start address (must be 64-byte aligned)
 * @param Size     size of one item
 * @param Number   number of items
 * @return next 64-byte aligned address
 */
void *HH_MemoryManager_DesignateSize_Alignment64Address_Calculator(void *pAddress, unsigned int Size,
                                                                   unsigned int Number) {
    void *result;
    unsigned int Next_Address;

    if (((unsigned int)pAddress & 0x3F) == 0) {
        Next_Address = (unsigned int)pAddress;
        Next_Address += Size * Number;
        if (Next_Address & 0x3F) {
            Next_Address = Next_Address - (Next_Address & 0x3F) + 0x40;
        }
        result = (void *)Next_Address;
        return result;
    } else {
        assert_dw(0);
    }
}

/**
 * Carve blocks 0-2 out of the effect work area and report the total size.
 * @return 1 on success, 0 if there is no work area
 */
unsigned int HH_MemoryManager_MemoryBlock_All_Allocate(void) {
    unsigned int result;
    void *pAddress;
    unsigned int Base;
    unsigned int End;

    result = 0;
    if ((pAddress = MemShareGetEffect2WorkAddr())) { /* Matching: the original asks twice. */
        result = 1;
    } else if ((pAddress = MemShareGetEffect2WorkAddr())) {
        result = 1;
    }
    if (result) {
        Base = (unsigned int)pAddress;
        MemoryBlock_Allocate(0, pAddress);
        pAddress = HH_MemoryManager_DesignateSize_Alignment64Address_Calculator(pAddress, 0x60000, 1);
        MemoryBlock_Allocate(1, pAddress);
        pAddress = HH_MemoryManager_DesignateSize_Alignment64Address_Calculator(pAddress, 0x80000, 1);
        MemoryBlock_Allocate(2, pAddress);
        pAddress = HH_MemoryManager_DesignateSize_Alignment64Address_Calculator(pAddress, 0x44800, 4);
        End = (unsigned int)pAddress;
        printf("All Allocate Size = %d kB\n", (End - Base) / 1024);
    }
    return result;
}

/**
 * Allocate the packet block (0) and the object work block (1).
 * @return 1 on success, 0 if there is no work area
 */
unsigned int HH_MemoryManager_MemoryBlock_Allocate_Packet_and_ObjectWork(void) {
    unsigned int result;
    void *pAddress;

    result = 0;
    if ((pAddress = MemShareGetEffect2WorkAddr())) {
        result = 1;
    } else if ((pAddress = MemShareGetEffect2WorkAddr())) {
        result = 1;
    }
    if (result) {
        MemoryBlock_Allocate(0, pAddress);
        pAddress = HH_MemoryManager_DesignateSize_Alignment64Address_Calculator(pAddress, 0x60000, 1);
        MemoryBlock_Allocate(1, pAddress);
        HH_Class_Object_Initialize();
    }
    return result;
}

/**
 * Allocate the texture buffer block (2) and set up texture entry levels 2 and 3.
 * @return 1 on success, 0 if there is no work area
 */
unsigned int HH_MemoryManager_MemoryBlock_Allocate_TextureBuffer(void) {
    unsigned int result;
    void *pAddress;

    result = 0;
    if ((pAddress = MemShareGetEffect2WorkAddr())) {
        result = 1;
    } else if ((pAddress = MemShareGetEffect2WorkAddr())) {
        result = 1;
    }
    if (result) {
        pAddress = HH_MemoryManager_DesignateSize_Alignment64Address_Calculator(pAddress, 0x60000, 1);
        pAddress = HH_MemoryManager_DesignateSize_Alignment64Address_Calculator(pAddress, 0x80000, 1);
        MemoryBlock_Allocate(2, pAddress);
        HH_Effect_Object_Texture_DesignateEntryLevel_Initialize(2);
        HH_Effect_Object_Texture_DesignateEntryLevel_Initialize(3);
    }
    return result;
}

/**
 * Release blocks 0-2.
 * @return 1 if every block was allocated
 */
unsigned int HH_MemoryManager_MemoryBlock_All_Discard(void) {
    unsigned int result;

    result = 1;
    HH_Effect_Object_Texture_DesignateEntryLevel_Discard(2);
    result *= AllocateMemoryBlock_Release(0);
    result *= AllocateMemoryBlock_Release(1);
    result *= AllocateMemoryBlock_Release(2);
    return result;
}
