/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "stm32469i_discovery_sd.h"

#if GFX_USE_GFILE && GFILE_NEED_FATFS

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */

DSTATUS disk_initialize (
    BYTE drv                /* Physical drive nmuber (0..) */
)
{
	if ( drv )
		return STA_NOINIT;

	if ( BSP_SD_IsDetected() == SD_NOT_PRESENT )
		return STA_NODISK;

	if ( BSP_SD_Init() == MSD_OK )
		return 0;

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
    BYTE drv        /* Physical drive nmuber (0..) */
)
{
  if ( drv )
		return STA_NOINIT;
	
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
    BYTE drv,        /* Physical drive nmuber (0..) */
    BYTE *buff,        /* Data buffer to store read data */
    DWORD sector,    /* Sector address (LBA) */
    UINT count        /* Number of sectors to read (1..255) */
)
{
  DRESULT res;
	if ( drv || !count )
		return RES_PARERR;

	res = (DRESULT)BSP_SD_ReadBlocks((uint32_t *)buff, sector, 512, count);
	
	if ( res == 0x00 )
		return RES_OK;
	return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
    BYTE drv,            /* Physical drive nmuber (0..) */
    const BYTE *buff,    /* Data to be written */
    DWORD sector,        /* Sector address (LBA) */
    UINT count            /* Number of sectors to write (1..255) */
)
{
  DRESULT res;
	if ( drv || !count )
		return RES_PARERR;

	res = (DRESULT)BSP_SD_WriteBlocks((uint32_t *)buff, sector, 512, count);

	if ( res == 0 )
		return RES_OK;
	return RES_ERROR;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
    BYTE drv,        /* Physical drive nmuber (0..) */
    BYTE ctrl,        /* Control code */
    void *buff        /* Buffer to send/receive control data */
)
{
  SD_CardInfo cardinfo;
	DRESULT res;
	if ( drv )
		return RES_PARERR;

	switch( ctrl )
	{
	case CTRL_SYNC:
//		res = ( SD_WaitReady() == SD_RESPONSE_NO_ERROR ) ? RES_OK : RES_ERROR
		res = RES_OK;
		break;
	case GET_BLOCK_SIZE:
		*(WORD*)buff = _MAX_SS;
		res = RES_OK;
		break;
	case GET_SECTOR_COUNT:
			BSP_SD_GetCardInfo( &cardinfo );
			*(DWORD*)buff = cardinfo.CardCapacity;
			res = ( *(DWORD*)buff > 0 ) ? RES_OK : RES_PARERR;
		break;
	case CTRL_ERASE_SECTOR:
		res = ( BSP_SD_Erase( ((DWORD*)buff)[ 0 ], ((DWORD*)buff)[ 1 ] ) == MSD_OK )
			? RES_OK : RES_PARERR;
		break;
	default:
		res = RES_PARERR;
		break;
	}

	return res;
}

DWORD get_fattime(void) {
	return 0;
}

#endif // GFX_USE_GFILE && GFILE_NEED_FATFS && GFX_USE_OS_CHIBIOS


