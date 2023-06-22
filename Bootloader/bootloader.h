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
#define BL_ENABLE_UART_DEBUG_MESSAGE   (0x00)
/* Enable & Disable debugging info in any terminal ex.RealTerm */
#define BL_DEBUG_ENABLE                (0x01)
#define BL_DEBUG_DISABLE               (0x00)
/* Communication protocol used to transmit and receive commands*/
#define BL_DEBUG_UART                  (&huart2) 
#define BL_HOST_COMMUNICATION_UART     (&huart1)
#define BL_CRC_HANDLE                  (&hcrc)
#define BL_ENABLE_DEBUG_METHOD         (BL_ENABLE_UART_DEBUG_MESSAGE)
#define BL_INFO_DEBUG                  (BL_DEBUG_ENABLE)
/* Buffer to receive bootloader frame info in Bytes */
#define BL_HOST_BUFFER_RX_LENGTH       (200)
/*Bootloader supported commands to perform various operations */
/* Generic commands (bootloader info) */
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
/* bootloader version info */
#define BL_VENDOR_ID                    (127)
#define BL_SW_MAJOR_VERSION             (1)
#define BL_SW_MINOR_VERSION             (0)
#define BL_SW_PATCH_VERSION             (0)
/* crc32 size in bootloader frame & it's expected conditions */
#define BL_CRC_SIZE_BYTES               (4)
#define CRC_FAILED                      (0x00)
#define CRC_PASSED                      (0x01)
/* bootloader acknowledge */
#define BL_ACK                          (0xAB)
#define BL_NACK                         (0xCD)

#define NULL_POINTER                    (0x00)
/* jump to address conditions for validaty */
#define ADDRESS_VALID                   (0x01)
#define ADDRESS_INVALID                 (0x00)
#define SECTOR_VALID                    (0x01) /* Erase Flash Memory conditions */
#define SECTOR_INVALID                  (0x01)
#define FLASH_MAX_SECTOR_NB             (0x07) /* STM32F446RE Number Of Sectors */
#define FLASH_MASS_ERASE                (0xFF)
#define SUCCESSFUL_ERASE                (0x03)
#define FAILED_ERASE                    (0x02)

#define HAL_SUCCESSFUL_ERASE            (0xFFFFFFFFU)
#define NUCLEO_F446RE_SRAM1_SIZE        (112 * 1024)
#define NUCLEO_F446RE_SRAM2_SIZE        (16 * 1024)
#define NUCLEO_F446RE_FLASH_SIZE        (1024 * 1024)
#define NUCLEO_F446RE_SRAM1_END         (SRAM1_BASE + NUCLEO_F446RE_SRAM1_SIZE)
#define NUCLEO_F446RE_SRAM2_END         (SRAM2_BASE + NUCLEO_F446RE_SRAM2_SIZE)
#define NUCLEO_F446RE_FLASH_END         (FLASH_BASE + NUCLEO_F446RE_FLASH_SIZE)

#define FLASH_SECTOR2_BASE_ADDRESS      (0x08008000U) /* Start address for App.c */
typedef void (*pMainApp)(void);
typedef void (*JumpPTR)(void);
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