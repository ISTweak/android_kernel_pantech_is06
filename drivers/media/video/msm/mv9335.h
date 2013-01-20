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

#ifndef MV9335_H
#define MV9335_H

#include <linux/types.h>
#include <mach/camera.h>

extern struct mv9335_reg mv9335_regs;

struct mv9335_i2c_reg_conf {
	uint8_t addr;
	uint8_t data;
	uint8_t mdelay;
};

struct mv9335_reg {
	struct mv9335_i2c_reg_conf *plltbl;
	uint16_t plltbl_size;
#ifdef F_SKYCAM_MODEL_EF12S
	struct mv9335_i2c_reg_conf *plltbl2;
	uint16_t plltbl2_size;
#endif
};

#endif

