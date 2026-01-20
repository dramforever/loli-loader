// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/fdt_iter.c
 *	Copyright (c) 2026 Vivian Wang
 */

#include <efi.h>
#include <string.h>
#include <fdt.h>
#include <fdt_iter.h>

int
fdt_iter_init(const char *fdt, size_t size, struct Fdt_Iter *iter)
{
	if (iter)
		memset(iter, 0, sizeof(*iter));
	else
		return -1;

	if (!fdt || size < sizeof(Fdt_Header))
		return -1;

	Fdt_Header *header = (Fdt_Header*)fdt;
	if (be32_to_cpu(header->magic) != FDT_MAGIC)
		return -1;

	if (be32_to_cpu(header->version) < FDT_VERSION ||
	    be32_to_cpu(header->lastCompVersion) > FDT_VERSION)
		return -1;

	size_t total_size = be32_to_cpu(header->totalSize);

	size_t struct_offset = be32_to_cpu(header->offDtStruct);
	size_t struct_size = be32_to_cpu(header->sizeDtStruct);

	if (struct_offset >= total_size ||
	    struct_size == 0 ||
	    total_size - struct_offset < struct_size)
		return -1;

	size_t strings_offset = be32_to_cpu(header->offDtStrings);
	size_t strings_size = be32_to_cpu(header->sizeDtStrings);

	if (strings_offset >= total_size ||
	    total_size - strings_offset < strings_size)
		return -1;

	while (strings_size > 0 && *(fdt + strings_offset + strings_size))
		strings_size --;

	if (strings_size == 0)
		return -1;

	iter->fdt = fdt;
	iter->total_size = total_size;

	iter->strings = fdt + strings_offset;
	iter->strings_size = strings_size;

	iter->struct_end = fdt + struct_offset + struct_size;
	iter->next = fdt + struct_offset;

	return 0;
}

int
fdt_iter_step(struct Fdt_Iter *iter)
{
	if (iter->next >= iter->struct_end)
		return -1;

	/* Will be overwritten if appropriate later */
	iter->name = NULL;
	iter->data = NULL;
	iter->data_len = 0;

	while (1) {
		if (iter->struct_end - iter->next < 4)
			return -1;

		iter->opcode = read_be32(iter->next);
		iter->next += 4;

		size_t maxlen, len;

		switch (iter->opcode) {
		case FDT_BEGIN_NODE:
			maxlen = iter->struct_end - iter->next;
			len = strnlen(iter->next, maxlen);
			if (len >= maxlen)
				return -1;
			len ++;
			len = (len + 3) & ~3; /* Add padding */
			if (len >= maxlen)
				return -1;

			iter->name = iter->next;
			iter->next += len;
			return 1;
		case FDT_END_NODE:
			return 1;
		case FDT_PROP:
			if (iter->struct_end - iter->next < 8)
				return -1;
			len = read_be32(iter->next);
			maxlen = iter->struct_end - iter->next;
			size_t nameoff = read_be32(iter->next + 4);

			iter->next += 8;

			if (nameoff >= iter->strings_size)
				return -1;

			iter->name = iter->strings + nameoff;

			if (len > maxlen)
				return -1;

			iter->data = iter->next;
			iter->data_len = len;

			len = (len + 3) & ~3; /* Add padding */

			if (len > maxlen)
				return -1;

			iter->next += len;
			return 1;
		case FDT_NOP:
			iter->next += 4;
			continue; /* Skip this opcode and retry */
		case FDT_END:
			return 0;
		default:
			return -1;
		}
	}
}
