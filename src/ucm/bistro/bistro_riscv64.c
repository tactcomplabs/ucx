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
#include <stdbool.h>
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
#define X2  2
#define X5  5

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
#define JAL(_regs, _regd, _imm) (((_imm) << 12) | ((_regd) << 7) | (0x6f))

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

/**
 * @brief ORI - or immediate in source register into destination register
 *
 * @param[in] _reg  register number (0-31), @param[out] _reg register number (0-31), @param[imm] 12 bit immmediate value
 */
#define ORI(_regs, _regd, _imm) (((_imm) << 20) | ((_regs) << 15) | (0b110 << 12) | ((_regd) << 7) | (0x13))
#define XORI(_regs, _regd, _imm) (((_imm) << 20) | ((_regs) << 15) | (0b100 << 12) | ((_regd) << 7) | (0x13))
#define OR(_regs_a, _regs_b, _regd) (((_regs_b) << 20) | ((_regs_a) << 15) | (0b110 << 12) | ((_regd) << 7) | (0x33))

#define SD(_regs_a, _regs_b, _imm) ( ( 0XFFFFFFFF & (((_imm) >> 5) << 25) ) | ((_regs_b) << 20) | ((_regs_a) << 15) | (0b011 << 12) | (( 0XFFFFFFFF & ( (_imm) << 27) ) >> 20 ) | (0x23) )

ucs_status_t ucm_bistro_patch(void *func_ptr, void *hook, const char *symbol,
                              void **orig_func_p,
                              ucm_bistro_restore_point_t **rp)
{
    ucs_status_t status;
    ptrdiff_t delta = (ptrdiff_t)( ( hook - func_ptr ) + 4 );

    ucm_bistro_patch_t patch = {
	.addi  = ADDI(X2, X2, 16),
	.sd    = SD(X2, X1, 0),
        .auipc = AUIPC( ( (0b11111111111111111111 << 12) & (delta + 0x800) ) >> 12, X31),
        .jalr  = JALR(X31, X0, 0b111111111111 & delta)
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
