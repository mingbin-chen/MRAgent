/**************************************************************************//**
* @file     cmdlib.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2023 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdlib.h"

extern void dumpDataMsg(char* title, uint8_t* data, int dataLen);
extern void UpdateMsgData(char* msg);
extern void UpdateMsgData2(char* msg);
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define CMD_PARSE_MSG       0
#define CMD_PARA_NUMBER     20  
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static uint8_t packageBuffer[512];
static char msgBuffer[128];
static int packageBufferIndex = 0;
static char* arr[CMD_PARA_NUMBER];
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static int split(char** arr, char* str, const char* del) {
    int reVal = 0;    
    char* s = NULL;
    
#ifdef _WINDOWS
    char* next_token = NULL;
    s = (char*)strtok_s(str, del, &next_token);
#else
    s = strtok(str, del);
#endif
    
    while (s != NULL) {
        reVal++;
        *arr++ = s;
        
#ifdef _WINDOWS
        s = (char*)strtok_s(NULL, del, &next_token);
#else
        s = (char*)strtok(NULL, del);
#endif
    }
    return reVal;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
static int CMDParserData(char* pData, int dataLen)
{
    UpdateMsgData(" ***** CMDParserData: [");
    UpdateMsgData((char*)pData);
    UpdateMsgData("] *****\r\n");

    const char* del = ";";
    int reval = 0;
    reval = split(arr, pData, del);
    
    for(int i = 0; i< reval; i++)
    {
        snprintf(msgBuffer, sizeof(msgBuffer), "  - %s\r\n", *(arr + i));
        UpdateMsgData((char*)msgBuffer);
    }
    UpdateMsgData("\r\n");
    return dataLen;
}
static int shiftData(uint8_t* pData, int dataLen, int srcIndex)
{
    memcpy(pData, pData + srcIndex, dataLen - srcIndex);
    //for (int i = srcIndex; i < (dataLen - srcIndex); i++)
    //{
    //    pData[i-srcIndex] = pData[i];
    //}
    return dataLen - srcIndex;
}
int CMDProcessData(uint8_t* pData, int dataLen)
{
    #if(CMD_PARSE_MSG)
    UpdateMsgData("\r\nCMDProcessData: {");
    UpdateMsgData((char*)pData);
    UpdateMsgData("}\r\n");
    #endif

    int lastIndex = 0;
    int getParseFlag = 0;

    memcpy(packageBuffer + packageBufferIndex, pData, dataLen);
    packageBufferIndex = packageBufferIndex + dataLen;
    packageBuffer[packageBufferIndex] = 0x0;

    #if(CMD_PARSE_MSG)
    UpdateMsgData("TOTAL: {");
    UpdateMsgData((char*)pData);
    UpdateMsgData("}\r\n");
    #endif

    for (int i = 0; i < packageBufferIndex - 1; i++)
    {
        if ((packageBuffer[i] == 0x0D) && (packageBuffer[i + 1] == 0x0A))
        {
            int pureDataLen = 0;
            getParseFlag = 1;
            packageBuffer[i] = 0x0;

            pureDataLen = i - lastIndex - 2;

            if(pureDataLen > 0)
            {
                packageBuffer[i - 1] = 0x0;
                CMDParserData((char*)packageBuffer + lastIndex + 1, pureDataLen);
            }     
            
            int leftData = packageBufferIndex - i;

            if (leftData > 2)
                lastIndex = i + 2;
            else
                lastIndex = 0;

            #if(CMD_PARSE_MSG)
            snprintf(msgBuffer, sizeof(msgBuffer), " -- GET: i = %d, leftData = %d, packageBufferIndex = %d, lastIndex = %d\r\n", 
                     i, leftData, packageBufferIndex, lastIndex);
            UpdateMsgData(msgBuffer);
            #endif
        }
        else
        {

        }
    }
    if (getParseFlag == 1)
    {
        if (lastIndex > 0)
            packageBufferIndex = shiftData(packageBuffer, packageBufferIndex, lastIndex);
        else
            packageBufferIndex = 0;
    }

    packageBuffer[packageBufferIndex] = 0x0;

    #if(CMD_PARSE_MSG)
    snprintf(msgBuffer, sizeof(msgBuffer), "CMDProcessData END(packageBufferIndex = %d): {", packageBufferIndex);
    UpdateMsgData(msgBuffer);

    UpdateMsgData((char*)packageBuffer);
    UpdateMsgData("}\r\n");
    #endif

    return 0;
}

/*** * Copyright (C) 2023 Far Easy Pass LTD. All rights reserved. ***/

