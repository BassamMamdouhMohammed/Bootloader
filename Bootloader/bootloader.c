/*
 * bootloader.c
 * LinkedIn: https://www.linkedin.com/in/bassam-mamdouh
 * Created on: Mar 6, 2023
 * Author: Bassam Mamdouh Mohammed
 * Platform: Nucleo-F446RE
 * Version: 1.0.0
 */


#include "Bootloader.h"

static uint8_t BL_HostBuffer[BL_HOST_BUFFER_RX_LENGTH];

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

static void Bootloader_UART_Transmit(uint8_t *HostBuffer,uint8_t msgLength);

static uint8_t Bootloader_CRC_verify(uint8_t *Data, uint32_t DataLength, uint32_t Host_CRC);
static void Bootloader_Send_Ack(uint8_t msgLength);
static void Bootloader_Send_NAck();
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
		if(HAL_ERROR == HAL_Status)
		{
			BL_PrintMessage("HAL_ERROR happend 2section \r\n");
		}
		else
		{
			switch(BL_HostBuffer[1])
			{
				case BL_GET_VERSION_CMD:
					
					break;
				case BL_GET_HELP_CMD:
			
					break;
				case BL_GET_CID_CMD:
					BL_PrintMessage("BL_GET_CID_CMD \r\n");
					break;
				case BL_GET_RDP_STATUS_CMD:
					BL_PrintMessage("BL_GET_RDP_STATUS_CMD \r\n");
					break;
				case BL_GO_TO_ADDRESS_CMD:
					BL_PrintMessage("BL_GO_TO_ADDRESS_CMD \r\n");
					break;
				case BL_FLASH_ERASE_CMD:
					BL_PrintMessage("BL_FLASH_ERASE_CMD \r\n");
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
	for(Iterator = 0; Iterator < DataLength; ++Iterator){
		DataBuffer = (uint32_t)Data[Iterator];
		BL_calc_CRC = HAL_CRC_Accumulate(BL_CRC_HANDLE,&DataBuffer,1);
	}
	__HAL_CRC_DR_RESET(BL_CRC_HANDLE);
	
	if(Data == NULL || (BL_calc_CRC != Host_CRC)){
		CRC_Status = CRC_FAILED;
	} else {}
		
	return CRC_Status;
}

static void Bootloader_UART_Transmit(uint8_t *HostBuffer,uint8_t msgLength)
{
	HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART,HostBuffer,msgLength,HAL_MAX_DELAY);
}

static void Bootloader_GetVersion(uint8_t *HostBuffer)
{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("BL_GET_VERSION_CMD \r\n");
#endif	
	uint8_t BootloaderVersion[4] = {BL_VENDOR_ID,BL_SW_MAJOR_VERSION,
									BL_SW_MINOR_VERSION,BL_SW_PATCH_VERSION};
	uint16_t HostPacketLen = HostBuffer[0];
	uint32_t HostCRC = *((uint32_t *)((HostBuffer + HostPacketLen) - BL_CRC_SIZE_BYTES));
									
	if(CRC_PASSED == Bootloader_CRC_verify((uint8_t *)&HostBuffer[0],HostPacketLen - 4,HostCRC))
	{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("CRC Verification Passed \r\n");
#endif	
		Bootloader_Send_Ack(4);
		Bootloader_UART_Transmit((uint8_t *)&BootloaderVersion[0],4);
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("Bootloader Version. %d.%d.%d \r\n",
		                                   BootloaderVersion[0],BootloaderVersion[1],BootloaderVersion[2]);
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
	BL_PrintMessage("BL_GET_HELP_CMD \r\n");
#endif		
	uint16_t HostPacketLen = HostBuffer[0];
	uint32_t HostCRC = *((uint32_t *)((HostBuffer + HostPacketLen) - BL_CRC_SIZE_BYTES));
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
	BL_PrintMessage("BL_GET_HELP_CMD \r\n");
#endif		
	uint16_t HostPacketLen = HostBuffer[0];
	uint32_t HostCRC = *((uint32_t *)((HostBuffer + HostPacketLen) - BL_CRC_SIZE_BYTES));
	uint16_t MCU_ID = 0;
	if(CRC_PASSED == Bootloader_CRC_verify((uint8_t *)&HostBuffer[0],HostPacketLen - 4,HostCRC))
	{
#if (BL_INFO_DEBUG == BL_DEBUG_ENABLE)
	BL_PrintMessage("CRC Verification Passed \r\n");
#endif	
		MCU_ID = (uint16_t)(DBGMCU->IDCODE & 0x00000FFF);
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

static void Bootloader_GetSectorProtectionStatus(uint8_t *HostBuffer)
{
	
}


static void Bootloader_JumpToAddress(uint8_t *HostBuffer)
{
	
}
static void Bootloader_EraseFlash(uint8_t *HostBuffer)
{
	
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


