// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/fdt_iter.h
 *	Copyright (c) 2026 Vivian Wang
 */

#ifndef __LOLI_FDT_ITER_H_INC__
#define __LOLI_FDT_ITER_H_INC__

#include <string.h>

#define FDT_MAGIC 0xd00dfeedU
#define FDT_VERSION 17

#define FDT_BEGIN_NODE 0x1
#define FDT_END_NODE 0x2
#define FDT_PROP 0x3
#define FDT_NOP 0x4
#define FDT_END 0x9

struct Fdt_Iter {
	const char *fdt;
	size_t total_size;

	const char *strings;
	size_t strings_size;

	const char *struct_end;

	const char *next;
	const char *name;
	const char *data;
	size_t data_len;
	uint32_t opcode;
};

int fdt_iter_init(const char *fdt, size_t size, struct Fdt_Iter *iter);
int fdt_iter_step(struct Fdt_Iter *iter);
const char *fdt_get_compatible(const char *fdt);

#endif	// __LOLI_FDT_ITER_H_INC__
