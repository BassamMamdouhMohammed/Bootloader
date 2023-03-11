/*
 * bootloader.h
 * LinkedIn: https://www.linkedin.com/in/bassam-mamdouh
 * Created on: Mar 6, 2023
 * Author: Bassam Mamdouh Mohammed
 * Platform: Nucleo-F446RE
 * Version: 1.0.0
 */


#ifndef BOOTLOADER_H
#define BOOTLOADER_H

/*---------------------------------- INCLUDES ---------------------------------------- */
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <main.h>
#include "usart.h"
#include "crc.h"
/*---------------------------------- MACRO DEFINITION -------------------------------- */
#define BL_DEBUG_UART                  (&huart2)
#define BL_HOST_COMMUNICATION_UART     (&huart1)
#define BL_ENABLE_DEBUG_METHOD         (BL_ENABLE_UART_DEBUG_MESSAGE)
#define BL_CRC_HANDLE                  (&hcrc)
#define BL_HOST_BUFFER_RX_LENGTH       (200)
#define BL_DEBUG_ENABLE                (0x01)
#define BL_DEBUG_DISABLE               (0x00)
#define BL_INFO_DEBUG                  (BL_DEBUG_ENABLE)
#define BL_ENABLE_UART_DEBUG_MESSAGE   (0x00)
#define BL_ENABLE_SPI_DEBUG_MESSAGE    (0x01)
#define BL_ENABLE_CAN_DEBUG_MESSAGE    (0x02)

#define BL_GET_VERSION_CMD             (0x10)
#define BL_GET_HELP_CMD                (0x11)
#define BL_GET_CID_CMD                 (0x12)
#define BL_GET_RDP_STATUS_CMD          (0x13)
#define BL_GO_TO_ADDRESS_CMD           (0x14)
#define BL_FLASH_ERASE_CMD             (0x15)
#define BL_MEM_WRITE_CMD               (0x16)
#define BL_EN_RW_PROTECT_CMD           (0x17)
#define BL_MEM_READ_CMD                (0x18)
#define BL_READ_SECTOR_STATUS_CMD      (0x19)
#define BL_READ_OTP_CMD                (0x20)
#define BL_DIS_RW_PROTECT_CMD          (0x21)

#define BL_VENDOR_ID                    (127)
#define BL_SW_MAJOR_VERSION             (1)
#define BL_SW_MINOR_VERSION             (0)
#define BL_SW_PATCH_VERSION             (0)

#define BL_CRC_SIZE_BYTES               (4)
#define CRC_FAILED                      (0x00)
#define CRC_PASSED                      (0x01)
#define BL_ACK                          (0xAB)
#define BL_NACK                         (0xCD)
/*---------------------------------- MACRO FUNCTION ---------------------------------- */

/*---------------------------------- USER DATA TYPE ---------------------------------- */
typedef enum{
	BL_ERROR = 0,
	BL_OK
}BL_STATUS;
/*---------------------------------- APIs PROTOTYPE ---------------------------------- */
void BL_PrintMessage(char *format, ...);
BL_STATUS BL_UART_FetchHostCommand(void);

#endif /* BOOTLOADER_H */