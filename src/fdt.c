// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/fdt.c
 *	Copyright (C) 2025 Yao Zi <ziyao@disroot.org>
 */

#include <string.h>

#include <efi.h>
#include <eficall.h>
#include <efiboot.h>
#include <efidevicetree.h>

#include <fdt.h>
#include <fdt_iter.h>
#include <interaction.h>
#include <memory.h>
#include <misc.h>
#include <file.h>

static Fdt_Header *
get_current_devicetree(void)
{
	Fdt_Header *fdt = NULL;

	for (uint_native i = 0; i < gST->numberOfTableEntries; i++) {
		Efi_Configuration_Table *t = gST->configurationTable + i;

		Efi_Guid dtbGuid = EFI_DTB_TABLE_GUID;
		if (!memcmp(&dtbGuid, &t->vendorGuid, sizeof(Efi_Guid))) {
			fdt = t->vendorTable;
			break;
		}
	}

	if (!fdt)
		return NULL;

	return fdt;
}

void
fdt_fixup_and_load(Fdt_Header *fdt)
{
	/* TODO: check compatibility */
	size_t fdtSize = be32_to_cpu(fdt->totalSize);

	pr_info("devicetree: size %lu\n", fdtSize);

	/* TODO: don't use a hard size limit */
	size_t copySize = fdtSize + 4096;
	Fdt_Header *copy = malloc_type(copySize, EFI_ACPI_RECLAIM_MEMORY);
	memcpy(copy, fdt, fdtSize);

	Efi_Dt_Fixup_Protocol *dtp = NULL;
	Efi_Guid dtFixupGuid = EFI_DT_FIXUP_PROTOCOL_GUID;
	int ret = efi_call(gBS->locateProtocol, &dtFixupGuid, NULL, (void **)&dtp);

	if (dtp) {
		ret = efi_method(dtp, fixup, copy, &copySize,
				 EFI_DT_APPLY_FIXES | EFI_DT_RESERVE_MEMORY);
		if (ret == EFI_SUCCESS)
			pr_info("devicetree: applied fixes\n");
		else
			pr_info("devicetree: failed to apply fixes\n");
	} else {
		pr_info("EFI_DT_FIXUP_PROTOCOL isn't supported, "
			"apply no fixup\n");
	}

	efi_install_configuration_table(EFI_DTB_TABLE_GUID, copy);
}

static void *
search_fdtdir(const wchar_t *path, const char *compatible, int depth)
{
	size_t pathLen = wcslen(path);
	Efi_File_Protocol *file = file_open(path);
	void *result = NULL;

	if (!file)
		return NULL;

	Efi_File_Info *info = NULL;
	if (file_get_info(file, &info) != EFI_SUCCESS)
		goto close;

	if (!(info->attribute & EFI_FILE_DIRECTORY)) {
		// File
		size_t fdtSize = info->fileSize;
		void *fdt = malloc(fdtSize);
		Efi_Status ret = efi_method(file, read, &fdtSize, fdt);
		if (ret != EFI_SUCCESS)
			goto close;

		const char *newCompatible = fdt_get_compatible(fdt);
		if (newCompatible && strcmp(newCompatible, compatible) == 0)
			result = fdt;
		else
			free(fdt);
	} else {
		// Directory
		if (depth <= 0)
			goto close; // Reached depth limit

		size_t bufSize = 0;
		Efi_File_Info *buf = NULL;

		for (;;) {
			size_t dataSize = bufSize;
			Efi_Status ret = efi_method(file, read, &dataSize, buf);

			if (dataSize == 0)
				break; // Done reading

			if (ret == TO_EFI_ERRNO(EFI_BUFFER_TOO_SMALL)) {
				size_t newSize = dataSize;
				if (dataSize < bufSize * 2)
					newSize = bufSize * 2;
				buf = realloc(buf, bufSize, newSize);
				bufSize = newSize;
				continue;
			}

			if (ret != EFI_SUCCESS)
				break;

			if (buf->fileName[0] == 0 || buf->fileName[0] == '.')
				continue;

			size_t newLen = pathLen + wcslen(buf->fileName) + 2;
			wchar_t *newPath = malloc(newLen);
			wcscpy(newPath, path);
			newPath[pathLen] = '\\';
			wcscpy(newPath + pathLen + 1, buf->fileName);

			// Search subdir or file
			result = search_fdtdir(newPath, compatible, depth - 1);
			if (result)
				break;
		}

		free(buf);
	}

close:
	free(info);
	file_close(file);
	return result;
}

#define FDTDIR_MAX_DEPTH 4

void *
load_from_fdtdir(const char *path)
{
	Fdt_Header *fdt = get_current_devicetree();
	if (!fdt) {
		pr_info("devicetree: No current FDT to match with\n");
		return NULL;
	}

	const char *compatible = fdt_get_compatible((void*)fdt);
	if (!compatible) {
		pr_info("devicetree: Current FDT has no compatible prop\n");
		return NULL;
	}

	wchar_t *wpath = malloc((str2wcs(NULL, path) + 1) * sizeof(wchar_t));
	str2wcs(wpath, path);
	void *result = search_fdtdir(wpath, compatible, FDTDIR_MAX_DEPTH);
	free(wpath);

	pr_info("devicetree: Found %s for %s\n",
		result ? "match" : "no match",
		compatible);

	return result;
}
