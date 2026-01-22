// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/fdt.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_FDT_H_INC__
#define __LOLI_FDT_H_INC__

#include <efidef.h>

#pragma pack(push, 0)

typedef struct {
	uint32_t magic;
	uint32_t totalSize;
	uint32_t offDtStruct;
	uint32_t offDtStrings;
	uint32_t offMapRsvMap;
	uint32_t version;
	uint32_t lastCompVersion;
	uint32_t bootCpuidPhys;
	uint32_t sizeDtStrings;
	uint32_t sizeDtStruct;
} Fdt_Header;

#pragma pack(pop)

void fdt_fixup_and_load(Fdt_Header *fdt);
void *load_from_fdtdir(const char *path);

/* Useful helpers for accessing values inside FDT */

static inline uint32_t
read_be32(const char *data)
{
	uint8_t *p = (uint8_t*)data;
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static inline uint32_t
be32_to_cpu(uint32_t val)
{
	return read_be32((char*)&val);
}

#endif	// __LOLI_FDT_H_INC__
