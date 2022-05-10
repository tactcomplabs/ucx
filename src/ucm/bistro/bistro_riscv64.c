/**
 * Copyright (C) Tactical Computing Labs, LLC. 2022. ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

/* *******************************************************
 * RISC-V processors family                              *
 * ***************************************************** */
#if defined(__riscv)

#include <sys/mman.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <ucm/bistro/bistro.h>
#include <ucm/bistro/bistro_int.h>
#include <ucm/util/sys.h>
#include <ucs/sys/math.h>
#include <ucs/arch/cpu.h>
#include <ucs/debug/assert.h>

/* Registers numbers to use with the move immediate to register.
 * The destination register is X31 (highest temporary).
 * Register X28-X30 are used for block shifting and masking.
 * Register X0 is always zero */
#define X31 31
#define X30 30
#define X27 27
#define X0  0
#define X1  1

/**
 * @brief AUIPC - Add 20 bit immediate to program counter
 *
 * @param[in] _reg  register number (0-31)
 */
#define AUIPC(_imm, _rd) (((_imm) << 12) | ((_rd) << 7) | (0x17))

/**
 * @brief JALR - Add 12 bit immediate to source register, save to destination register, jump and link from destination register
 *
 * @param[in] _reg  register number (0-31), @param[out] _reg register number (0-31), @param[imm] 12 bit immmediate value
 */
#define JALR(_regs, _regd, _imm) (((_imm) << 20) | ((_regs) << 15) | (0b000 << 12) | ((_regd) << 7) | (0x67))

/**
 * @brief ADDI - Add 12 bit immediate to source register, save to destination register 
 *
 * @param[in] _reg  register number (0-31), @param[out] _reg register number (0-31), @param[imm] 12 bit immmediate value
 */
#define ADDI(_regs, _regd, _imm) (((_imm) << 20) | ((_regs) << 15) | (0b000 << 12) | ((_regd) << 7) | (0x13))

/**
 * @brief LUI - load upper 20 bit immediate to destination register
 *
 * @param[in] _reg  register number (0-31), @param[out] _reg register number (0-31), @param[imm] 12 bit immmediate value
 */
#define LUI(_regd, _imm) (((_imm) << 12) | ((_regd) << 7) | (0x37))

/**
 * @brief SLLI - left-shift immediate number of bits in source register into destination register
 *
 * @param[in] _reg  register number (0-31), @param[out] _reg register number (0-31), @param[imm] 12 bit immmediate value
 */
#define SLLI(_regs, _regd, _imm) (((_imm) << 20) | ((_regs) << 15) | (0b001 << 12) | ((_regd) << 7) | (0x13))

ucs_status_t ucm_bistro_patch(void *func_ptr, void *hook, const char *symbol,
                              void **orig_func_p,
                              ucm_bistro_restore_point_t **rp)
{
    const uintptr_t updated_hook = ((uintptr_t)hook);
    /* u-prefix means upper 32 bits of 64 bit integer */
    const uint32_t uhi = ( ( ( 0b11111111111111111111 << 12 ) & (updated_hook >> 32)) );
    const uint32_t ulo = ( ( ( 0b111111111111 ) & (updated_hook >> 32) ) );

    /* l-prefix means lower 32 bits of 64 bit integer */
    const uint32_t lhi = ( ( 0b11111111111111111111 << 12 ) & updated_hook );
    const uint32_t llo = ( 0b111111111111 & updated_hook );

    ucs_status_t status;
    ucm_bistro_patch_t patch = {
        .uhi  = LUI(X31, uhi >> 12),       /* load upper 20 bits in 64 range */
        .ulo  = ADDI(X31, X31, ulo),       /* load next upper 12 bits in 64 range */
        .sli  = SLLI(X31, X31, 32),        /* shift the upper 32 bits into position */
        .lhi  = ADDI(X31, X31, lhi >> 12), /* load the lower 20 bits in the 32 range */
        .jalr = JALR(X31, X1, llo)         /* load the first 12 bits in the 32 range, add them and jump */
    };

    if (orig_func_p != NULL) {
        return UCS_ERR_UNSUPPORTED;
    }

    status = ucm_bistro_create_restore_point(func_ptr, sizeof(ucm_bistro_patch_t), rp);
    if (UCS_STATUS_IS_ERR(status)) {
        return status;
    }

    return ucm_bistro_apply_patch(func_ptr, &patch, sizeof(ucm_bistro_patch_t));
}
#endif
