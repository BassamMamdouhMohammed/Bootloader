/*
 * bootloader.c
 * LinkedIn: https://www.linkedin.com/in/bassam-mamdouh
 * Created on: Mar 6, 2023
 * Author: Bassam Mamdouh Mohammed
 * Platform: Nucleo-F446RE
 * Version: 1.0.0
 */

/*--------------------------- INCLUDES --------------------------------------*/
#include "Bootloader.h"
/*--------------------------- Global Variables Definitions ------------------*/
static uint8_t BL_HostBuffer[BL_HOST_BUFFER_RX_LENGTH];
static uint8_t Bootloader_Supported_CMDs[12] = 
	{
		BL_GET_VERSION_CMD,
		BL_GET_HELP_CMD,
		BL_GET_CID_CMD,
		BL_GET_RDP_STATUS_CMD,
		BL_GO_TO_ADDRESS_CMD,
		BL_FLASH_ERASE_CMD,
		BL_MEM_WRITE_CMD,
		BL_EN_RW_PROTECT_CMD,
		BL_MEM_READ_CMD,
		BL_READ_SECTOR_STATUS_CMD,
		BL_READ_OTP_CMD,
		BL_DIS_RW_PROTECT_CMD,
	};
/*--------------------------- Static Functions Prototypes -------------------*/
static void Bootloader_GetVersion(uint8_t *HostBuffer);
static void Bootloader_GetHelp(uint8_t *HostBuffer);
static void Bootloader_GetSectorProtectionStatus(uint8_t *HostBuffer);
static void Bootloader_GetChip_IdentificationNumber(uint8_t *HostBuffer);

static void Bootloader_JumpToAddress(uint8_t *HostBuffer);
static void Bootloader_EraseFlash(uint8_t *HostBuffer);
static void Bootloader_MemoryWrite(uint8_t *HostBuffer);
static void Bootloader_MemoryRead(uint8_t *HostBuffer);

static void Bootloader_ReadProtectionLevel(uint8_t *HostBuffer);
static void Bootloader_Enable_RW_Protection(uint8_t *HostBuffer);
static void Bootloader_Disable_RW_Protection(uint8_t *HostBuffer);

/*------------------------- static helper functions -----------------------------*/
static void Bootloader_UART_Transmit(uint8_t *HostBuffer,uint8_t msgLength);
static void Bootloader_JumpToUserApplication(void);
static uint8_t Bootloader_CRC_verify(uint8_t *Data, uint32_t DataLength, uint32_t Host_CRC);
static void Bootloader_Send_Ack(uint8_t msgLength);
static void Bootloader_Send_NAck();
static uint8_t Bootloader_JumpAddressVerify(uint32_t address);
/*-------------------------- Bootloader functionality Implementatoin ------------------*/
void BL_PrintMessage(char *format, ...)
{
	char Message[100] = {0};
	va_list args;
	va_start(args, format);
	vsprintf(Message,format,args);
	#if (BL_ENABLE_DEBUG_METHOD == BL_ENABLE_UART_DEBUG_MESSAGE)
	HAL_UART_Transmit(BL_DEBUG_UART,(uint8_t *)Message,sizeof(Message),HAL_MAX_DELAY);
	#elif (BL_ENABLE_DEBUG_METHOD == BL_ENABLE_SPI_DEBUG_MESSAGE)
		/*Transmit data through SPI*/
	#elif (BL_ENABLE_DEBUG_METHOD == BL_ENABLE_CAN_DEBUG_MESSAGE)
		/*Transmit data through CAN*/
	#endif
	va_end(args);

}
/*--------------------------- Static Functions Definitions -------------------*/
BL_STATUS BL_UART_FetchHostCommand(void)
{
	BL_STATUS Status = BL_OK;
	HAL_StatusTypeDef HAL_Status = HAL_OK;
	uint8_t DataLength = 0;
	memset(BL_HostBuffer,0,BL_HOST_BUFFER_RX_LENGTH);
	HAL_Status = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART,BL_HostBuffer,1,HAL_MAX_DELAY);
	if(HAL_ERROR == HAL_Status)
	{
		BL_PrintMessage("HAL_ERROR happend 1section \r\n");
	}
	else
	{
		DataLength = BL_HostBuffer[0];
		HAL_Status = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART,&BL_HostBuffer[1],DataLength,HAL_MAX_DELAY);
		if(HAL_OK != HAL_Status)
		{
			BL_PrintMessage("HAL_ERROR happend 2section \r\n");
			HAL_Status = HAL_ERROR;
		}
		else
		{
			switch(BL_HostBuffer[1])
			{
				case BL_GET_VERSION_CMD:
					Bootloader_GetVersion(BL_HostBuffer);
				break;
				case BL_GET_HELP_CMD:
					Bootloader_GetHelp(BL_HostBuffer);
				break;
				case BL_GET_CID_CMD:
					Bootloader_GetChip_IdentificationNumber(BL_HostBuffer);
				break;
				case BL_GET_RDP_STATUS_CMD:
					BL_PrintMessage("BL_GET_RDP_STATUS_CMD \r\n");
				break;
				case BL_GO_TO_ADDRESS_CMD:
					Bootloader_JumpToAddress(BL_HostBuffer);
				break;
				case BL_FLASH_ERASE_CMD:
					Bootloader_EraseFlash(BL_HostBuffer);
				break;
				case BL_MEM_WRITE_CMD:
					BL_PrintMessage("BL_MEM_WRITE_CMD \r\n");
				break;
				case BL_EN_RW_PROTECT_CMD:
					BL_PrintMessage("BL_EN_RW_PROTECT_CMD \r\n");
				break;
				case BL_MEM_READ_CMD:
					BL_PrintMessage("BL_MEM_READ_CMD \r\n");
				break;
				case BL_READ_SECTOR_STATUS_CMD:
					BL_PrintMessage("BL_READ_SECTOR_STATUS_CMD \r\n");
				break;
				case BL_READ_OTP_CMD:
					BL_PrintMessage("BL_READ_OTP_CMD \r\n");
				break;
				case BL_DIS_RW_PROTECT_CMD:
					BL_PrintMessage("BL_DIS_RW_PROTECT_CMD \r\n");
				break;
				default: BL_PrintMessage("Invalid Bootloader Command \r\n");
				break;
			}
		}
	}
	return Status;
}


static void Bootloader_GetVersion(uint8_t *HostBuffer)
{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("Read Bootloader version from MCU \r\n");
#endif	
	uint16_t HostPacketLen = HostBuffer[0] + 1;
	/* assigns the value of the crc32 to HostCRC */
	uint32_t HostCRC = *((uint32_t *)((HostBuffer + HostPacketLen) - BL_CRC_SIZE_BYTES));
	uint8_t BootloaderVersion[4] = {BL_VENDOR_ID,BL_SW_MAJOR_VERSION,
									BL_SW_MINOR_VERSION,BL_SW_PATCH_VERSION};
	/* CRC32 verification checking */
	if(CRC_PASSED == Bootloader_CRC_verify((uint8_t *)&HostBuffer[0],HostPacketLen - 4,HostCRC))
	{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("CRC Verification Passed \r\n");
#endif	
		Bootloader_Send_Ack(4);
		Bootloader_UART_Transmit((uint8_t *)&BootloaderVersion[0],4);
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("Bootloader Version. %d.%d.%d \r\n",
		                                   BootloaderVersion[1],BootloaderVersion[2],BootloaderVersion[3]);
#endif			
	}
	else{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("CRC Verification Failed \r\n");
#endif	
		Bootloader_Send_NAck();
	}
}
static void Bootloader_GetHelp(uint8_t *HostBuffer)
{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("list commands supported by bootloader \r\n");
#endif		
	/* calculate CRC32 */
	uint16_t HostPacketLen = HostBuffer[0];
	uint32_t HostCRC = *((uint32_t *)((HostBuffer + HostPacketLen) - BL_CRC_SIZE_BYTES));
	/* checking CRC32 */
	if(CRC_PASSED == Bootloader_CRC_verify((uint8_t *)&HostBuffer[0],HostPacketLen - 4,HostCRC))
	{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("CRC Verification Passed \r\n");
#endif	
		Bootloader_Send_Ack(12);
		Bootloader_UART_Transmit((uint8_t *)&Bootloader_Supported_CMDs[0],12);
	}
	else{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("CRC Verification Failed \r\n");
#endif	
		Bootloader_Send_NAck();
	}	
}

static void Bootloader_GetChip_IdentificationNumber(uint8_t *HostBuffer)
{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("Read MCU chip Identification number \r\n");
#endif		
	/* calculate CRC32 */
	uint16_t HostPacketLen = HostBuffer[0];
	uint32_t HostCRC = *((uint32_t *)((HostBuffer + HostPacketLen) - BL_CRC_SIZE_BYTES));
	uint16_t MCU_ID = 0;
	/* checking CRC32 */
	if(CRC_PASSED == Bootloader_CRC_verify((uint8_t *)&HostBuffer[0],HostPacketLen - 4,HostCRC))
	{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("CRC Verification Passed \r\n");
#endif	
		MCU_ID = (uint16_t)(DBGMCU->IDCODE & 0x00000FFF);
		/* Report chip identification number to HOST */
		Bootloader_Send_Ack(2);
		Bootloader_UART_Transmit((uint8_t *)&MCU_ID,2);
	}
	else{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("CRC Verification Failed \r\n");
#endif	
		Bootloader_Send_NAck();
	}	
}

static void Bootloader_JumpToAddress(uint8_t *HostBuffer)
{
	uint16_t HostPacketLen = 0;
	uint32_t HostCRC = 0;
	uint32_t HostJumpAddress = 0x00;
	uint8_t AddrStatus = ADDRESS_VALID; 
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("Jump Bootloader to Specific Address \r\n");
#endif
	/* calculate CRC32 */
	 HostPacketLen = HostBuffer[0];
	 HostCRC = *((uint32_t *)((HostBuffer + HostPacketLen) - BL_CRC_SIZE_BYTES));
	/* checking CRC32 */
	if(CRC_PASSED == Bootloader_CRC_verify((uint8_t *)&HostBuffer[0],HostPacketLen - 4,HostCRC))
	{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("CRC Verification Passed \r\n");
#endif
		Bootloader_Send_Ack(1);
		HostJumpAddress = *((uint32_t *)&HostBuffer[2]);
		AddrStatus = Bootloader_JumpAddressVerify(HostJumpAddress);
		if(ADDRESS_VALID == AddrStatus)
		{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("Address Verification Passed \r\n");
#endif			
			Bootloader_UART_Transmit((uint8_t *)&AddrStatus, 1);
			/* Prepare the address to jump */
			JumpPTR JumpAddr = (JumpPTR)(HostJumpAddress + 1); 
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("Jump to : 0x%X \r\n",JumpAddr);
#endif
		JumpAddr();
		}
		else
		{
			Bootloader_UART_Transmit((uint8_t *)&AddrStatus, 1);
		}
	}
	else
	{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("CRC Verification Failed \r\n");
#endif	
		Bootloader_Send_NAck();
	}
}
static void Bootloader_EraseFlash(uint8_t *HostBuffer)
{
	uint16_t HostPacketLen = 0;
	uint32_t HostCRC = 0;
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("Mass erase or sector erase of the user flash \r\n");
#endif
	/* calculate CRC32 */
	 HostPacketLen = HostBuffer[0];
	 HostCRC = *((uint32_t *)((HostBuffer + HostPacketLen) - BL_CRC_SIZE_BYTES));
	/* checking CRC32 */
	if(CRC_PASSED == Bootloader_CRC_verify((uint8_t *)&HostBuffer[0],HostPacketLen - 4,HostCRC))
	{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("CRC Verification Passed \r\n");
#endif
		Bootloader_Send_Ack(1);	
	}
	else
	{
		
	}
}
static void Bootloader_MemoryWrite(uint8_t *HostBuffer)
{
	
}
static void Bootloader_MemoryRead(uint8_t *HostBuffer)
{
	
}

static void Bootloader_ReadProtectionLevel(uint8_t *HostBuffer)
{
	
}
static void Bootloader_Enable_RW_Protection(uint8_t *HostBuffer)
{
	
}
static void Bootloader_Disable_RW_Protection(uint8_t *HostBuffer)
{
	
}
static void Bootloader_GetSectorProtectionStatus(uint8_t *HostBuffer)
{
	
}

/*-------------------------- static heper funtions Definition -------------------------*/
static void Bootloader_JumpToUserApplication(void)
{
	/* get main stack pionter value of main application */
	uint32_t MSP_Value = *((volatile uint32_t *) FLASH_SECTOR2_BASE_ADDRESS);
	/* reset handler definition funtion of main application */
	uint32_t MainAppAddr = *((volatile uint32_t *)(FLASH_SECTOR2_BASE_ADDRESS + 4));
	/* Fetch the reset handler address of the user application */
	pMainApp ResetHandlerAddr = (pMainApp)MainAppAddr;
	/* Assigns the given value to the Main Stack Pointer */
	__set_MSP(MSP_Value);
	/* Resets the RCC clock configuration to the default reset state. */
	HAL_RCC_DeInit();
	/* Jump to Application Reset Handler */
	ResetHandlerAddr();
}

static void Bootloader_Send_Ack(uint8_t msgLength)
{
	uint8_t AckValue[2] = {0};
	AckValue[0] = BL_ACK;
	AckValue[1] = msgLength;
	HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART,(uint8_t *)AckValue,2,HAL_MAX_DELAY);
}

static void Bootloader_Send_NAck()
{
	uint8_t NACKvalue = BL_NACK;
	HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART,&NACKvalue,1,HAL_MAX_DELAY);
}

static uint8_t Bootloader_CRC_verify(uint8_t *Data, uint32_t DataLength, uint32_t Host_CRC)
{
	uint8_t CRC_Status = CRC_PASSED;
	uint32_t BL_calc_CRC = 0;
	uint8_t Iterator = 0;
	uint32_t DataBuffer = 0;
	/* calulate CRC32 */
	if(NULL != Data)
	{
		for(Iterator = 0; Iterator < DataLength; ++Iterator){
		DataBuffer  = (uint32_t)Data[Iterator];
		BL_calc_CRC = HAL_CRC_Accumulate(BL_CRC_HANDLE,&DataBuffer,1);
		}
		/* Reset the CRC Calculation Unit */
		__HAL_CRC_DR_RESET(BL_CRC_HANDLE);
		/* check if the calculated crc is equal to host crc(python script) */
		if(BL_calc_CRC != Host_CRC){
			CRC_Status = CRC_FAILED;
		} else {}
	}
	else
	{
		CRC_Status = NULL_POINTER;
	}
	return CRC_Status;
}

static void Bootloader_UART_Transmit(uint8_t *HostBuffer,uint8_t msgLength)
{
	HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART,HostBuffer,msgLength,HAL_MAX_DELAY);
}
static uint8_t Bootloader_JumpAddressVerify(uint32_t address)
{
	uint8_t addr_status = ADDRESS_VALID;
	if((address >= SRAM1_BASE) && (address <= NUCLEO_F446RE_SRAM1_END)){}
	else if((address >= SRAM2_BASE) && (address <= NUCLEO_F446RE_SRAM2_END)){}
	else if((address >= SRAM1_BASE) && (address <= NUCLEO_F446RE_SRAM1_END)){}	
	else if((address >= FLASH_BASE) && (address <= NUCLEO_F446RE_FLASH_END)){}	
	else
	{
		addr_status = ADDRESS_INVALID;
	}
	return addr_status;
}
static uint8_t FlashErase(uint8_t sectorNb, uint8_t Nbofsectors)
{
	uint8_t sectorStatus = SECTOR_VALID;
	FLASH_EraseInitTypeDef pEraseInit;
	uint8_t rem_sectors = 0;
	uint32_t SectorError = 0;
	if(Nbofsectors < FLASH_MAX_SECTOR_NB)
	{
		if((sectorNb <= (FLASH_MAX_SECTOR_NB - 1)) || (FLASH_MASS_ERASE == sectorNb))
		{
			if(FLASH_MASS_ERASE == sectorNb)
			{
				pEraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE;
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
				BL_PrintMessage("Flash Mass Erase activation \r\n");
#endif
			}
			else
			{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
				BL_PrintMessage("Sector Erase activation \r\n");
#endif			
				rem_sectors = FLASH_MAX_SECTOR_NB - sectorNb;
				if(Nbofsectors > rem_sectors)
				{
					Nbofsectors = rem_sectors;
				}else{}
				pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS; /* Sectors erase only */
				pEraseInit.Sector = sectorNb;        /* Initial FLASH sector to erase when Mass erase is disabled */
				pEraseInit.NbSectors = Nbofsectors; /* Number of sectors to be erased. */					
			}
			pEraseInit.Banks = FLASH_BANK_1; /* Bank 1  */
			pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3; /* Device operating range: 2.7V to 3.6V */		
			/* Unlock the FLASH control register access */
			HAL_FLASH_Unlock();		
			/* Perform a mass erase or erase the specified FLASH memory sectors */
			HAL_FLASHEx_Erase(&pEraseInit, &SectorError);	
			if(HAL_SUCCESSFUL_ERASE == SectorError)
			{
				sectorStatus = SUCCESSFUL_ERASE;
			}				
			else
			{
				sectorStatus = FAILED_ERASE;
			}
			HAL_FLASH_Lock();
		}
		else
		{
			sectorStatus = SECTOR_INVALID;
		}
	}
	else
	{
		sectorStatus = SECTOR_INVALID;
	}
	return sectorStatus;
}