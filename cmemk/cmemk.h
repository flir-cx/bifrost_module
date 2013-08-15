/*
 * Copyright (c) 2011, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file    cmem.h
 * @brief   Describes the interface to the contiguous memory allocator.
 *
 * The cmem user interface library wraps file system calls to an associated
 * kernel module (cmemk.ko), which needs to be loaded in order for calls to
 * this library to succeed.
 *
 * The following is an example of installing the cmem kernel module:
 * 
 * @verbatim /sbin/insmod cmemk.ko pools=4x30000,2x500000 phys_start=0x0 phys_end=0x3000000 @endverbatim
 *     - phys_start and phys_end must be specified in hexadecimal format
 *     - phys_start is "inclusive" and phys_end is "exclusive", i.e.,
 *       phys_end should be "end address + 1".
 *     - pools must be specified using decimal format (for both number and
 *       size), since using hexadecimal format would visually clutter the
 *       specification due to the use of "x" as a token separator
 *     - it's possible to insmod cmemk.ko without specifying any memory blocks,
 *       in which case CMEM_getPhys() and CMEM_cache*() APIs can still be
 *       utilized by an application.
 *
 * This particular command creates 2 pools. The first pool is created with 4
 * buffers of size 30000 bytes and the second pool is created with 2 buffers
 * of size 500000 bytes. The CMEM pool buffers start at 0x0 and end at
 * 0x2FFFFFF (max). 
 *
 * There is also support for a 2nd contiguous memory block to be specified,
 * with all the same features supported for the 2nd block as with the 1st.
 * This 2nd block is specified with *_1 parameters.  The following example
 * expands upon the first example above:
 * 
 * @verbatim /sbin/insmod cmemk.ko pools=4x30000,2x500000 phys_start=0x0 phys_end=0x3000000
    pools_1=4x65536 phys_start_1=0x80000000 phys_end_1=0x80010000 @endverbatim
 *
 * This particular command, in addition to the pools explained above,
 * creates 1 pool (with 4 buffers of size 64KB) in a 2nd memory block which
 * starts at 0x80000000 and ends at 0x8000FFFF (specified as "end + 1" on the
 * insmod command).
 *
 * In order to access this 2nd memory block, new APIs have been added to
 * CMEM which allow specification of the block ID.
 *
 * There are two more configuration "switches" for the cmemk.ko kernel module,
 * which can be specified on the 'insmod' (or 'modprobe') command lines:
 *     useHeapIfPoolUnavailable=[0|1]
 *     allowOverlap=[0|1]
 *
 * 'useHeapIfPoolUnavailable', when set to 1, will cause pool-based allocations
 * to fallback to a heap-based allocation if no pool buffer of sufficient size
 * is available (the CMEM heap is described below).
 *
 * 'allowOverlap', when set to 1, causes cmemk.ko to not fail when it detects
 * that a CMEM memory block location conflicts with the Linux kernel memory,
 * and instead an informational message is printed on the console.  When set to
 * 0, cmemk.ko insertion will fail when this condition is detected.  The
 * overlap detection is fairly crude, however, checking only that the end of
 * the kernel's memory (assigned by way of the u-boot 'bootargs' parameter
 * "mem=##M") is not above the beginning location of a CMEM memory block.  For
 * example, on most TI processor-based systems the kernel's memory starts at
 * 0x80000000 and ends at (0x80000000 + ##M), so a CMEM block starting at
 * 0x1000 would be detected as overlapping since the beginning location of that
 * block is not greater than the end location of the kernel's memory.  To
 * allow this situation, cmemk.ko should be inserted using "allowOverlap=1".
 *
 * Pool buffers are aligned on a module-dependent boundary, and their sizes are
 * rounded up to this same boundary.  This applies to each buffer within a
 * pool.  The total space used by an individual pool will therefore be greater
 * than (or equal to) the exact amount requested in the installation of the
 * module.
 *
 * The poolid used in the driver calls would be 0 for the first pool and 1 for
 * the second pool.
 *
 * Pool allocations can be requested explicitly by pool number, or more
 * generally by just a size.  For size-based allocations, the pool which best
 * fits the requested size is automatically chosen.  Some CMEM APIs (newer
 * ones) accept a blockid as a parameter, in order to specify which of the
 * multiple blocks to operate on.  For 'legacy' APIs (ones that existed before
 * the support for multiple blocks) where a blockid is still needed, block 0
 * is assumed.
 *
 * There is also support for a general purpose heap.  In addition to the 2
 * pools described above, a general purpose heap block is created from which
 * allocations of any size can be requested.  Internally, allocation sizes are
 * rounded up to a module-dependent boundary and allocation addresses are
 * aligned either to this same boundary or to the requested alignment
 * (whichever is greater).
 *
 * The size of the heap block is the amount of CMEM memory remaining after all
 * pool allocations.  If more heap space is needed than is available after pool
 * allocations, you must reduce the amount of CMEM memory granted to the pools.
 *
 * Buffer allocation is tracked at the file descriptor level by way of a
 * 'registration' list.  The initial allocator of a buffer (the process that
 * calls CMEM_alloc()) is automatically added to the registration list,
 * and further processes can become registered for the same buffer by way
 * of the CMEM_registerAlloc() API (and unregister with the
 * CMEM_unregister() API).  This registration list for each buffer
 * allows for buffer ownership tracking and cleanup on a
 * per-file-descriptor basis, so that when a process exits or dies without
 * having explicitly freed/unregistered its buffers, they get automatically
 * unregistered (and freed when no more registered file descriptors exist).
 * Only when the last registered file descriptor frees a buffer (either
 * explictily, or by auto-cleanup) does a buffer actually get freed back to
 * the kernel module.
 *
 * Since the CMEM interface library doesn't use the GT tracing facility, there
 * is one configuration option available for the CMEM module to control
 * whether the debug or release interface library is used for building the
 * application.  This config parameter is named 'debug' and is of type bool,
 * and the default value is 'false'.
 *
 * The following line is an example of enabling usage of the debug interface
 * library:
 *     var cmem = xdc.useModule('ti.sdo.linuxutils.cmem.CMEM');
 *     cmem.debug = true;
 * This will enable "CMEM Debug" statements to be printed to stdout.
 */
/**
 *  @defgroup   ti_sdo_linuxutils_cmem_CMEM  Contiguous Memory Manager
 *
 *  This is the API for the Contiguous Memory Manager.
 */

#ifndef ti_sdo_linuxutils_cmem_CMEMK_H
#define ti_sdo_linuxutils_cmem_CMEMK_H

#if defined (__cplusplus)
extern "C" {
#endif


#define CMEM_VERSION    0x03000100U

/* ioctl cmd "flavors" */
#define CMEM_WB                         0x00010000
#define CMEM_INV                        0x00020000
#define CMEM_HEAP                       0x00040000  /**< operation applies to heap */
#define CMEM_POOL                       0x00000000  /**< operation applies to a pool */
#define CMEM_CACHED                     0x00080000  /**< allocated buffer is cached */
#define CMEM_NONCACHED                  0x00000000  /**< allocated buffer is not cached */
#define CMEM_PHYS                       0x00100000

#define CMEM_IOCMAGIC                   0x0000fe00

/* supported "base" ioctl cmds for the driver. */
#define CMEM_IOCALLOC                   1
#define CMEM_IOCALLOCHEAP               2
#define CMEM_IOCFREE                    3
#define CMEM_IOCGETPHYS                 4
#define CMEM_IOCGETSIZE                 5
#define CMEM_IOCGETPOOL                 6
#define CMEM_IOCCACHE                   7
#define CMEM_IOCGETVERSION              8
#define CMEM_IOCGETBLOCK                9
#define CMEM_IOCREGUSER                 10
#define CMEM_IOCGETNUMBLOCKS            11
/*
 * New ioctl cmds should use integers greater than the largest current cmd
 * in order to not break backward compatibility.
 */

/* supported "flavors" to "base" ioctl cmds for the driver. */
#define CMEM_IOCCACHEWBINV              CMEM_IOCCACHE | CMEM_WB | CMEM_INV
#define CMEM_IOCCACHEWB                 CMEM_IOCCACHE | CMEM_WB
#define CMEM_IOCCACHEINV                CMEM_IOCCACHE | CMEM_INV
#define CMEM_IOCALLOCCACHED             CMEM_IOCALLOC | CMEM_CACHED
#define CMEM_IOCALLOCHEAPCACHED         CMEM_IOCALLOCHEAP | CMEM_CACHED
#define CMEM_IOCFREEHEAP                CMEM_IOCFREE | CMEM_HEAP
#define CMEM_IOCFREEPHYS                CMEM_IOCFREE | CMEM_PHYS
#define CMEM_IOCFREEHEAPPHYS            CMEM_IOCFREE | CMEM_HEAP | CMEM_PHYS

#define CMEM_IOCCMDMASK                 0x000000ff

/**
 */
union CMEM_AllocUnion {
    struct {                    /**< */
        size_t size;
        size_t align;
        int blockid;
    } alloc_heap_inparams;      /**< */
    struct {                    /**< */
        int poolid;
        int blockid;
    } alloc_pool_inparams;      /**< */
    struct {                    /**< */
        int poolid;
        int blockid;
    } get_size_inparams;        /**< */
    struct {                    /**< */
        size_t size;
        int blockid;
    } get_pool_inparams;        /**< */
    struct {                    /**< */
        unsigned long physp;
        size_t size;
    } alloc_pool_outparams;     /**< */
    struct {                    /**< */
        unsigned long physp;
        size_t size;
    } get_block_outparams;      /**< */
    struct {                    /**< */
        int poolid;
        size_t size;
    } free_outparams;           /**< */
    unsigned long physp;
    unsigned long virtp;
    size_t size;
    int poolid;
    int blockid;
};

#if defined (__cplusplus)
}
#endif

#endif
/*
 *  @(#) ti.sdo.linuxutils.cmem; 2, 2, 0,1; 6-24-2011 16:00:37; /db/atree/library/trees/linuxutils/linuxutils.git/src/ linuxutils-i04
 */

