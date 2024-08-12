/**************************************************************************//**
* @file     cmdlib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2023 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __CMD_LIB_H__
#define __CMD_LIB_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WINDOWS
    typedef unsigned char             uint8_t;
    typedef unsigned short            uint16_t;
    typedef unsigned int              uint32_t;
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
int CMDProcessData(uint8_t* pData, int dataLen);
#ifdef __cplusplus
}
#endif

#endif //__CMD_LIB_H__
