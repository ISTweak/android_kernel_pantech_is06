/* Copyright (c) 2010, PANTECH. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include "mv9335.h"

static const struct mv9335_i2c_reg_conf mv9335_pll_setup_tbl[] = {
	{ 0x80, 0x00, 0 },
	{ 0xd0, 0x30, 0 },
	{ 0x40, 0x01, 0 },
	{ 0x41, 0x40, 0 },
	{ 0x42, 0x05, 0 },
	{ 0x49, 0x03, 0 },
	{ 0x80, 0x03, 0 },
	/* MANDATORY: 10ms + margin */
	{ 0x4a, 0x01, 50 } 
};

#ifdef F_SKYCAM_MODEL_EF12S
/* Use different PLL configuration for flash programming mode. */
static const struct mv9335_i2c_reg_conf mv9335_pll_setup_tbl2[] = {
	{ 0x80, 0x00, 0 },
	{ 0xd0, 0x30, 0 },
	{ 0x40, 0x01, 0 },
	{ 0x41, 0x40, 0 },
	{ 0x42, 0x05, 0 },
	{ 0x49, 0x03, 0 },
	{ 0x80, 0x02, 0 },
	/* Do not poll 'REG_MV9335_HW_VER' in flash programming mode.
	 * We can't assume that MV9335 is operating correctly or not. 
	 * MANDATORY: 100ms + margin */
	{ 0x4a, 0x01, 200 }
};
#endif

struct mv9335_reg mv9335_regs = {
	.plltbl = (struct mv9335_i2c_reg_conf *)&mv9335_pll_setup_tbl[0],
	.plltbl_size = ARRAY_SIZE(mv9335_pll_setup_tbl),
#ifdef F_SKYCAM_MODEL_EF12S
	.plltbl2 = (struct mv9335_i2c_reg_conf *)&mv9335_pll_setup_tbl2[0],
	.plltbl2_size = ARRAY_SIZE(mv9335_pll_setup_tbl2),
#endif
};
