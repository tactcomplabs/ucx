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
#define X0  0
#define X1  1

/**
 * @brief Add 20 bit immediate to program counter
 *
 * @param[in] _reg  register number (0-31)
 */
//#define AUIPC(_imm, _rd) (((_imm) << 12) | ((_rd) << 7) | (0x17))
#define AUIPC(_imm, _rd) (((_imm) ) | ((_rd) << 7) | (0x17))

/**
 * @brief JALR
 *
 */
#define JALR(_regs, _regd, _imm) (((_imm) << 20) | ((_regs) << 15) | ((_regd) << 7) | (0x67))


ucs_status_t ucm_bistro_patch(void *func_ptr, void *hook, const char *symbol,
                              void **orig_func_p,
                              ucm_bistro_restore_point_t **rp)
{
    const ptrdiff_t delta = (ptrdiff_t)( (hook - func_ptr) );
    const ptrdiff_t hi = ( ( 0b11111111111111111111 << 12 ) & delta );
    const ptrdiff_t lo = ( 0b111111111111 & delta );

    ucm_bistro_patch_t patch = {
        .auipc   = AUIPC( hi , X31 ),
        .jalr    = JALR(X31, X1, lo )
    };

    ucs_status_t status;

    if (orig_func_p != NULL) {
        return UCS_ERR_UNSUPPORTED;
    }

    status = ucm_bistro_create_restore_point(func_ptr, sizeof(patch), rp);
    if (UCS_STATUS_IS_ERR(status)) {
        return status;
    }

    return ucm_bistro_apply_patch(func_ptr, &patch, sizeof(patch));
}
#endif
