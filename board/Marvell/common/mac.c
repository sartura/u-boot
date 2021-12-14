// SPDX-License-Identifier: GPL-2.0+
/*
 * Marvell hw_info (mac) command implementation
 * Helper command for interfacing with the Marvell hw_info parameters
 *
 * Copyright (c) 2021 Sartura Ltd.
 * Copyright (c) 2018 Marvell International Ltd.
 *
 * Author: Luka Kovacic <luka.kovacic@sartura.hr>
 */

#include <command.h>
#include <common.h>
#include <dm.h>
#include <env.h>
#include <env_internal.h>
#include <i2c_eeprom.h>
#include <stdlib.h>

#define HW_INFO_MAX_ENV_SIZE		0x1F0
#define HW_INFO_ENV_OFFSET		0xA
#define HW_INFO_ENV_SEP			0x20

#define HW_INFO_MAX_NAME_LEN		32

#define HW_INFO_MERGED_VARIABLE		"read_board_hw_info"

/**
 * read_eeprom_offset() - Read data from the SPI flash
 * @buf: Buffer to write in
 * @offset: Offset from the flash start
 *
 * Read EEPROM data into the buffer from offset to HW_INFO_MAX_ENV_SIZE.
 */
static int read_eeprom_offset(struct udevice *hwinfo_dev, char *buf, int offset)
{
	int ret;

	ret = i2c_eeprom_read(hwinfo_dev, offset, buf, HW_INFO_MAX_ENV_SIZE);
	if (ret) {
		printf("Error - unable to read hw_info environment from EEPROM.\n");
		return ret;
	}

	return ret;
}

/**
 * cmd_hw_info_dump() - Dump the hw_info parameters
 *
 * This function prints all Marvell hw_info parameters, which are stored in
 * the SPI flash.
 */
static int cmd_hw_info_dump(struct udevice *hwinfo_dev)
{
	char buffer[HW_INFO_MAX_ENV_SIZE];
	struct hsearch_data htab;
	char *res = NULL;
	ssize_t len;
	int ret = 0;

	ret = read_eeprom_offset(hwinfo_dev, buffer, HW_INFO_ENV_OFFSET);
	if (ret)
		goto err;

	memset(&htab, 0, sizeof(htab));
	if (!hcreate_r(HW_INFO_MAX_ENV_SIZE, &htab)) {
		ret = -ENOMEM;
		goto err;
	}
	if (!himport_r(&htab, buffer, HW_INFO_MAX_ENV_SIZE,
		       HW_INFO_ENV_SEP, H_NOCLEAR, 0, 0, NULL)) {
		ret = -EFAULT;
		goto err_htab;
	}

	len = hexport_r(&htab, '\n', H_HIDE_DOT, &res, 0, 0, NULL);
	if (len > 0) {
		printf("Parameters (hw_info):\n");
		puts(res);
		free(res);
		ret = 0;
		goto ret_htab;
	}
ret_htab:
	hdestroy_r(&htab);
	return ret;
err_htab:
	hdestroy_r(&htab);
err:
	printf("## Error: cannot store hw_info parameters to SPI flash\n");
	return ret;
}

/**
 * cmd_hw_info_read() - Import the hw_info parameters into U-Boot env
 * @print_env: Print U-Boot environment after new parameters are imported
 *
 * This function reads the Marvell hw_info parameters from EEPROM and
 * imports them into the U-Boot env.
 */
static int cmd_hw_info_read(struct udevice *hwinfo_dev, bool print_env)
{
	char buffer[HW_INFO_MAX_ENV_SIZE];
	char *res = NULL;
	ssize_t len;
	int ret = 0;

	ret = read_eeprom_offset(hwinfo_dev, buffer, HW_INFO_ENV_OFFSET);
	if (ret)
		goto err;

	if (!himport_r(&env_htab, buffer, HW_INFO_MAX_ENV_SIZE,
		       HW_INFO_ENV_SEP, H_NOCLEAR, 0, 0, NULL)) {
		ret = -EFAULT;
		goto err;
	}

	printf("Successfully imported the Marvell hw_info parameters.\n");
	if (!print_env)
		return 0;

	len = hexport_r(&env_htab, '\n', H_HIDE_DOT, &res, 0, 0, NULL);
	if (len > 0) {
		printf("Updated environment:\n");
		puts(res);
		free(res);
		return 0;
	}
err:
	printf("## Error: cannot import hw_info parameters\n");
	return ret;
}

/**
 * mac_read_from_eeprom() - Read the parameters from EEPROM.
 *
 * This function reads the content of the Marvell hw_info parameters from the
 * EEPROM and imports them into the U-Boot environment.
 * This includes MAC addresses and board serial numbers.
 *
 * The import is performed only once.
 *
 * This function is a part of the U-Boot mac command and must be executed
 * after EEPROM initialization.
 */
int mac_read_from_eeprom(void)
{
	struct udevice *hwinfo_dev;
	ofnode hw_info_node;

	hw_info_node = ofnode_by_compatible(ofnode_null(), "marvell,hw-info");
	if (!ofnode_valid(hw_info_node)) {
		printf("Missing hw-info DT node!\n");
		return -ENODEV;
	}

	if (uclass_get_device_by_ofnode(UCLASS_I2C_EEPROM, hw_info_node, &hwinfo_dev)) {
		printf("Missing hw-info EEPROM device!\n");
		return -ENODEV;
	}

	if (env_get_ulong(HW_INFO_MERGED_VARIABLE, 10, 0) == 0) {
		if (env_set_ulong(HW_INFO_MERGED_VARIABLE, 1))
			return -ENOENT;
		return cmd_hw_info_read(hwinfo_dev, false);
	}

	return 0;
}

/**
 * print_platform_help() - Print the platform specific help
 *
 * Extend the existing U-Boot mac command description by also printing
 * platform specific help text.
 */
static void print_platform_help(void)
{
	printf("\nNote: arguments mac [id|num|errata|date|ports|port_number]\n"
	       "are unavailable on Marvell Armada A37xx platforms.\n"
	       "Use mac [read|save {parameter}] instead.\n"
	       "Available parameters:\n"
	       "pcb_slm\tPCB SLM number\n"
	       "pcb_rev\tPCB revision number\n"
	       "eco_rev\tECO revision number\n"
	       "pcb_sn\tPCB SN\n"
	       "ethaddr\tfirst MAC address\n"
	       "eth[1-9]addr\tsecond-ninth MAC address\n");
}

/**
 * do_mac() - Standard U-Boot mac command implementation
 * @cmdtp: U-Boot command table
 * @flag: Execution flags
 * @argc: Count of arguments
 * @argv: Arguments
 *
 * This function implements the standard U-Boot mac command in a mostly
 * compatible way.
 * To conform to the general command structure as much as possible, the
 * command description from cmd/mac.c is followed.
 * Where not possible, convenient or sensible additional comments for the user
 * are added.
 */
int do_mac(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	const char *cmd = argv[1];
	struct udevice *hwinfo_dev;
	ofnode hw_info_node;
	int ret = 0;

	hw_info_node = ofnode_by_compatible(ofnode_null(), "marvell,hw-info");
	if (!ofnode_valid(hw_info_node)) {
		printf("Missing hw-info DT node!\n");
		return -ENODEV;
	}

	if (uclass_get_device_by_ofnode(UCLASS_I2C_EEPROM, hw_info_node, &hwinfo_dev)) {
		printf("Missing hw-info EEPROM device!\n");
		return -ENODEV;
	}

	if (argc == 1) {
		ret = cmd_hw_info_dump(hwinfo_dev);
		if (ret)
			return -EINVAL;
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(cmd, "read")) {
		if (cmd_hw_info_read(hwinfo_dev, true))
			return -EINVAL;
	} else {
		ret = cmd_usage(cmdtp);
		print_platform_help();
		return ret;
	}

	return CMD_RET_SUCCESS;
}
