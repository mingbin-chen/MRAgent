/**************************************************************************//**
* @file     mrlib.c
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
#include "mrlib.h"
#include "aes.h"

#ifdef _WINDOWS
#else
#include "util.h"
#include "mrDrv.h"
#endif

extern void dumpDataMsg(char* title, uint8_t* data, int dataLen);
extern void UpdateMsgData(char* msg);
extern void UpdateMsgData2(char* msg);
#ifdef _WINDOWS
extern time_t getUTCTime(void);
extern int32_t MRUartWrite(const uint8_t* buf, const int32_t len);
#else
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define ENABLE_CHECK_PACKAGE_MSG  0//1
#define ENABLE_PARSE_PACKAGE_MSG  1

#define AES_BATCH_LEN  16

#define MANAGE_VECHICLE_NUM  5

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
// 128bit AES KEY
//static uint8_t aeskey[16] = { 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00 };
//static uint8_t aeskey[16] = { 0x01, 0x00, 0x05, 0x00, 0x01, 0x00, 0x05, 0x00, 0x01, 0x00, 0x05, 0x00, 0x01, 0x00, 0x05, 0x00 };
//static uint8_t aeskey[16] = { 0x01, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x0A, 0x00 };
static uint8_t combineAeskey[16] = { 0 };
static uint8_t aesBuffer[512];
static uint8_t packageBuffer[512];
static int packageBufferIndex = 0;
static int pakageLen = 0;
static int cmdLen = 0;

static uint8_t aesKey[4];
static uint8_t ackId;
static uint8_t opCode;
static uint16_t baseId;
static char msgBuffer[128];

static b2sBaseStatusCmd b2sBaseStatusCmdItem;
static b2sVehicleStatusCmd b2sVehicleStatusCmdItem;
static vehicleStatusData vehicleStatusDataItem[MANAGE_VECHICLE_NUM];

static vehicleStatusBaseData vehicleStatusBaseDataItem;
static vehicleStatusBaseData vehicleStatusBkpBaseDataItem;
static vehicleStatusData bkpVehicleStatusDataItem[MANAGE_VECHICLE_NUM];

static b2sVehicleReportCmd b2sVehicleReportCmdItem;
static vehicleReportData vehicleReportDataItem[MANAGE_VECHICLE_NUM];

static b2sVehicleVerCmd b2sVehicleVerCmdItem;
static vehicleVerData vehicleVerDataItem[MANAGE_VECHICLE_NUM];

static b2sParaDownloadAck b2sParaDownloadAckItem;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/


/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
static int counterTick = 0;
int paraCmdGeomangReinitFlag;
int mrLocalBaseID = 0;
uint8_t mrLocalBroadcastCH;
uint8_t mrLocalWorkCH;
int mrBkpBaseID;
uint8_t mrBkpBroadcastCH;
uint8_t mrBkpWorkCH;

static void resetPackageValue(void)
{
    pakageLen = 0;
    cmdLen = 0;
}

static uint16_t reverseU16(uint16_t value) {
    uint16_t result = 0;
    for (int i = 0; i < sizeof(uint16_t); i++) {
        result = (result << 8) + (value & 0xFF);
        value >>= 8;
    }
    return result;
}
static uint32_t reverseU32(uint32_t value) {
    uint32_t result = 0;
    for (int i = 0; i < sizeof(uint32_t); i++) {
        result = (result << 8) + (value & 0xFF);
        value >>= 8;
    }
    return result;
}
int MRTimerCallback(void)
{
    if(counterTick == (1000/50))
    { 
        //UpdateMsgData("MRTimerCallback\r\n");
        counterTick = 0;
    }
    else
    {
        counterTick++;
    }
    return 0;
}

int genAckCheckSum(uint8_t* destData, int startIndex, int dataLen)
{
    int checksum = 0;
    for (int i = startIndex; i < dataLen; i++)
    {
        checksum = checksum + destData[i];
    }

    return checksum;
}

uint8_t destAllContain[256];
uint8_t destAllData[256];
int MRPrintCmd(uint8_t* pSrcData, int dataLen)
{
    UpdateMsgData("\r\n******************************   PARSET START ******************************\r\n");
    dumpDataMsg("-- MRPrintCmd -- ", pSrcData, dataLen);
    int readIndex = 0;

    int destAllContainIndex = 0;

    while (readIndex < dataLen)
    {
        uint16_t cmdLen = 0; 
        uint16_t totalLen = 0;
        uint8_t* pData = pSrcData + readIndex;
        uint8_t cmdId = pData[0];

        UpdateMsgData("\r\n");
        switch (cmdId)
        {
            case MR_B2S_CMD_REG:
                {
                    b2sRegCmd* pb2sRegCmd = (b2sRegCmd*)pData;
                    cmdLen = pb2sRegCmd->cmdLen;

                    snprintf(msgBuffer,sizeof(msgBuffer), " GET MR_B2S_CMD_REG(0x%02x): cmd len = %d(0x%04x)\r\n", pb2sRegCmd->cmdId, cmdLen, cmdLen);
                    UpdateMsgData(msgBuffer);
                    totalLen = cmdLen + sizeof(pb2sRegCmd->cmdId) + sizeof(pb2sRegCmd->cmdLen);
                    dumpDataMsg(" # MR_B2S_CMD_REG #", pData, totalLen);
                    /*
                    * typedef struct
                    {
                        uint8_t     cmdId;          //1#    0   命令字          B1  0xd0
                        uint8_t     cmdLen;         //      1   命令長度        B1  後續命令長度
                        uint16_t    initPasswd;     //2#    2   初始通信密碼    B2
                        uint8_t     ver[14];        //3#    4   基站版本資訊    B14 EG:
                                                    //                              V3.0-SV1.39.63
                        uint8_t     time[20];       //4#    18  編譯時間        B20 EG:
                                                    //                              Jan 17 2013 16:31:02
                        uint8_t     baseMark;       //5#    38  基站標誌        B1  0—卡式基站
                                                    //                              1—基礎基站
                                                    //                              2—分體式基站
                                                    //                              3—集中式基站
                                                    //                              4—引導屏基站
                                                    //                              5—900mhz_A形基站
                                                    //                              6—900mhz_C形基站
                                                    //                              7—400mhz_C形基站
                        uint16_t    checksum;       //6#    39  校驗字          B2  校驗字
                    }b2sRegCmd;
                    */
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - initPasswd : 0x%04x\r\n", reverseU16(pb2sRegCmd->initPasswd));
                    UpdateMsgData(msgBuffer);
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - ver        : [%s]\r\n", pb2sRegCmd->ver);
                    UpdateMsgData(msgBuffer);
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - time       : [%s]\r\n", pb2sRegCmd->time);
                    UpdateMsgData(msgBuffer);
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - baseMark   : 0x%02x\r\n", pb2sRegCmd->baseMark);
                    UpdateMsgData(msgBuffer);
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - checksum   : 0x%04x\r\n", reverseU16(pb2sRegCmd->checksum));
                    UpdateMsgData(msgBuffer);

                    //
                    readIndex = readIndex + totalLen;

                    #ifndef _WINDOWS
                    if(GetTimeReadyFlag())
                    #endif
                    {
                        uint8_t destData[512];
                        int cmdLen = MRSendRegCmdAck(destData, sizeof(destData));
                        if (cmdLen > 0)
                        {
                            dumpDataMsg("MRSendRegCmdAck", destData, cmdLen);
                            #ifdef _WINDOWS
                            MRUartWrite(destData, cmdLen);
                            #else
                            mrDrvWrite(destData, cmdLen);
                            #endif
                        }
                    }
                    #ifndef _WINDOWS
                    else
                    {
                        snprintf(msgBuffer,sizeof(msgBuffer), " !!! TIME is not sync, IGNORE send REG ACK !!!\r\n");
                        UpdateMsgData(msgBuffer);
                    }
                    #endif
                }
                break;     

            case MR_B2S_CMD_BASE_STATUS:
                {
                    b2sBaseStatusCmd* pb2sBaseStatusCmd = (b2sBaseStatusCmd*)pData;
                    memcpy(&b2sBaseStatusCmdItem, pData, sizeof(b2sBaseStatusCmdItem));

                    cmdLen = pb2sBaseStatusCmd->cmdLen;

                    snprintf(msgBuffer,sizeof(msgBuffer), " GET MR_B2S_CMD_BASE_STATUS(0x%02x): cmd len = %d(0x%02x)\r\n", pb2sBaseStatusCmd->cmdId, cmdLen, cmdLen);
                    UpdateMsgData(msgBuffer);

                    totalLen = cmdLen + sizeof(pb2sBaseStatusCmd->cmdId) + sizeof(pb2sBaseStatusCmd->cmdLen);
                    dumpDataMsg(" # MR_B2S_CMD_BASE_STATUS #", pData, totalLen);
                    //---  
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - baseStatus = %d\r\n", pb2sBaseStatusCmd->baseStatus);
                    UpdateMsgData(msgBuffer);
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - FourGRSSIValue = %d\r\n", pb2sBaseStatusCmd->FourGRSSIValue);
                    UpdateMsgData(msgBuffer);
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - Sub1ghzRSSIValue = %d\r\n", pb2sBaseStatusCmd->Sub1ghzRSSIValue);
                    UpdateMsgData(msgBuffer);
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - baseDataType = %d\r\n", pb2sBaseStatusCmd->baseDataType);
                    UpdateMsgData(msgBuffer);
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - checksum = %d\r\n", pb2sBaseStatusCmd->checksum);
                    UpdateMsgData(msgBuffer);
                    //
                    readIndex = readIndex + totalLen;

                    {                        
                        int destContainLen;
                        uint8_t* startContain = destAllContain + destAllContainIndex;
                        int startContainLen = sizeof(destAllContain) - destAllContainIndex;

                        destContainLen = MRGetBaseStatusAck(startContain, startContainLen);
                        destAllContainIndex = destAllContainIndex + destContainLen;
                    }
                }
                break;

            case MR_B2S_CMD_VEHICLE_STATUS:
                {
                    b2sVehicleStatusCmd* pb2sVehicleStatusCmd = (b2sVehicleStatusCmd*)pData;
                    memcpy(&b2sVehicleStatusCmdItem, pData, sizeof(b2sVehicleStatusCmdItem));

                    cmdLen = reverseU16(pb2sVehicleStatusCmd->cmdLen);
                    snprintf(msgBuffer,sizeof(msgBuffer), " GET MR_B2S_CMD_VEHICLE_STATUS(0x%02x): cmd len = %d(0x%04x)\r\n", pb2sVehicleStatusCmd->cmdId, cmdLen, cmdLen);
                    UpdateMsgData(msgBuffer);

                    totalLen = cmdLen + sizeof(pb2sVehicleStatusCmd->cmdId) + sizeof(pb2sVehicleStatusCmd->cmdLen);
                    dumpDataMsg(" # MR_B2S_CMD_VEHICLE_STATUS #", pData, totalLen);
                    //---
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - baseNum = %d\r\n", pb2sVehicleStatusCmd->baseNum);
                    UpdateMsgData(msgBuffer);

                    /* Vehicle data of local base station */
                    uint8_t* pBaseStatusDataStart = &(pb2sVehicleStatusCmd->vehicleDataStart);
                    vehicleStatusBaseData* pVehicleStatusData = (vehicleStatusBaseData*)(pBaseStatusDataStart);
                    vehicleStatusBaseDataItem.baseId = pVehicleStatusData->baseId;
                    vehicleStatusBaseDataItem.vehicleNum = pVehicleStatusData->vehicleNum;
                    int vehicleNum = vehicleStatusBaseDataItem.vehicleNum;
                    snprintf(msgBuffer, sizeof(msgBuffer), "    - <local> baseId = 0x%04x , vehicleNum = %d\r\n", reverseU16(vehicleStatusBaseDataItem.baseId), vehicleStatusBaseDataItem.vehicleNum);
                    UpdateMsgData(msgBuffer);

                    // Point shift to locaal vehicle data
                    uint8_t* pVehicleDataStart = pBaseStatusDataStart + sizeof(vehicleStatusBaseData);

                    /*
                    uint16_t vehicleId;      //2Byte         車檢器組ID
                    uint8_t  vehicleStatus;  //1Bye          車檢器狀態                                     
                    uint32_t timeStamp;      //4Bye          車檢器狀態時間戳
                    */
                    UpdateMsgData2("---------------------------------------------------------------------------------------------\r\n");
                    for (int i = 0; i < vehicleNum; i++)
                    {
                        vehicleStatusData* pVehicleStatusData = (vehicleStatusData * )(pVehicleDataStart + i * sizeof(vehicleStatusData));
                        snprintf(msgBuffer,sizeof(msgBuffer), "      [%d] local vehicleId = 0x%04x\r\n", i+1, reverseU16(pVehicleStatusData->vehicleId));
                        UpdateMsgData(msgBuffer);
                        UpdateMsgData2(msgBuffer);
                        snprintf(msgBuffer,sizeof(msgBuffer), "          local vehicleStatus = 0x%02x                 <<-----------<< \r\n", pVehicleStatusData->vehicleStatus);
                        UpdateMsgData(msgBuffer);
                        UpdateMsgData2(msgBuffer);
                        snprintf(msgBuffer,sizeof(msgBuffer), "          local vehicletimeStamp = 0x%08x (%d)\r\n", reverseU32(pVehicleStatusData->timeStamp), reverseU32(pVehicleStatusData->timeStamp));
                        UpdateMsgData(msgBuffer);
                        UpdateMsgData2(msgBuffer);

                        //
                        vehicleStatusDataItem[i].vehicleId = pVehicleStatusData->vehicleId;
                    }
                    //uint8_t checksum = *(pVehicleDataStart + vehicleNum * sizeof(vehicleStatusData));

                    /* Vehicle data of backup base station */
                    if (vehicleNum > 0)
                    {
                        // Point shift to bkp vehicle base data
                        pVehicleDataStart = pVehicleDataStart + sizeof(vehicleNum * sizeof(vehicleStatusData)) - 1;
                    }

                    pVehicleStatusData = (vehicleStatusBaseData*)(pVehicleDataStart);
                    vehicleStatusBkpBaseDataItem.baseId = pVehicleStatusData->baseId;
                    vehicleStatusBkpBaseDataItem.vehicleNum = pVehicleStatusData->vehicleNum;
                    int bkpVehicleNum = vehicleStatusBkpBaseDataItem.vehicleNum;
                    snprintf(msgBuffer, sizeof(msgBuffer), "    - <bkp> baseId = 0x%04x , vehicleNum = %d\r\n", reverseU16(vehicleStatusBkpBaseDataItem.baseId), vehicleStatusBkpBaseDataItem.vehicleNum);
                    UpdateMsgData(msgBuffer);

                    // Point shift to bkp vehicle data
                    uint8_t* pBkpVehicleDataStart = pVehicleDataStart + sizeof(vehicleStatusBaseData);

                    /*
                    uint16_t vehicleId;      //2Byte         車檢器組ID
                    uint8_t  vehicleStatus;  //1Bye          車檢器狀態
                    uint32_t timeStamp;      //4Bye          車檢器狀態時間戳
                    */
                    for (int i = 0; i < bkpVehicleNum; i++)
                    {
                        vehicleStatusData* pBkpVehicleStatusData = (vehicleStatusData*)(pBkpVehicleDataStart + i * sizeof(vehicleStatusData));
                        snprintf(msgBuffer, sizeof(msgBuffer), "      [%d] bkp vehicleId = 0x%04x\r\n", i + 1, reverseU16(pBkpVehicleStatusData->vehicleId));
                        UpdateMsgData(msgBuffer);
                        UpdateMsgData2(msgBuffer);
                        snprintf(msgBuffer, sizeof(msgBuffer), "          bkp vehicleStatus = 0x%02x                 <<-----------<< \r\n", pBkpVehicleStatusData->vehicleStatus);
                        UpdateMsgData(msgBuffer);
                        UpdateMsgData2(msgBuffer);
                        snprintf(msgBuffer, sizeof(msgBuffer), "          bkp vehicleTimeStamp = 0x%08x (%d)\r\n", reverseU32(pBkpVehicleStatusData->timeStamp), reverseU32(pBkpVehicleStatusData->timeStamp));
                        UpdateMsgData(msgBuffer);
                        UpdateMsgData2(msgBuffer);

                        //
                        bkpVehicleStatusDataItem[i].vehicleId = pBkpVehicleStatusData->vehicleId;
                    }
                    uint8_t checksum = *(pBkpVehicleDataStart + bkpVehicleNum * sizeof(vehicleStatusData));
                    
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - checksum = 0x%02x\r\n", checksum);
                    UpdateMsgData(msgBuffer);
                    //
                    readIndex = readIndex + totalLen;

                    {
                        int destContainLen;
                        uint8_t* startContain = destAllContain + destAllContainIndex;
                        int startContainLen = sizeof(destAllContain) - destAllContainIndex;

                        destContainLen = MRGetVehicleStatusAck(startContain, startContainLen);
                        destAllContainIndex = destAllContainIndex + destContainLen;
                    }
                }
                break;

            case MR_B2S_CMD_VEHICLE_REPORT: ; //
                {
                    b2sVehicleReportCmd* pb2sVehicleReportCmd = (b2sVehicleReportCmd*)pData;
                    memcpy(&b2sVehicleReportCmdItem, pData, sizeof(b2sVehicleReportCmdItem));

                    cmdLen = reverseU16(pb2sVehicleReportCmd->cmdLen);
                    snprintf(msgBuffer,sizeof(msgBuffer), " GET MR_B2S_CMD_VEHICLE_REPORT(0x%02x): cmd len = %d(0x%04x)\r\n", pb2sVehicleReportCmd->cmdId, cmdLen, cmdLen);
                    UpdateMsgData(msgBuffer);

                    totalLen = cmdLen + sizeof(pb2sVehicleReportCmd->cmdId) + sizeof(pb2sVehicleReportCmd->cmdLen);
                    dumpDataMsg(" # MR_B2S_CMD_VEHICLE_REPORT #", pData, totalLen);
                    //---
                    int vehicleStatus0 = pb2sVehicleReportCmd->dataStatus_;
                    int vehicleNum0 = pb2sVehicleReportCmd->vehicleReportNum_;
                    int vehicleStatus = pb2sVehicleReportCmd->dataStatus;
                    int vehicleNum = pb2sVehicleReportCmd->vehicleReportNum;
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - Geomang0 Status & Num = %2d %2d\r\n", vehicleStatus0, vehicleNum0);
                    UpdateMsgData(msgBuffer);
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - Geomang1 Status & Num = %2d %2d\r\n", vehicleStatus, vehicleNum);
                    UpdateMsgData(msgBuffer);
                    //
                    uint8_t* vehicleReportStart = &(pb2sVehicleReportCmd->vehicleReportStart);
                    /*
                    uint16_t vehicleId;     //2Byte         車檢器組ID
                    uint8_t  vehicleStatus; //1Bye          車檢器狀態
                    uint16_t xValue;        //6Bye          車檢器X,Y,Z軸磁場值(每2字節代表一個軸)
                    uint16_t yValue;
                    uint16_t zValue;
                    */
                    for (int i = 0; i < vehicleNum; i++)
                    {
                        vehicleReportData* pVehicleReportData = (vehicleReportData * )(vehicleReportStart + i * sizeof(vehicleReportData));
                        snprintf(msgBuffer,sizeof(msgBuffer), "      [%d] vehicleId = 0x%04x\r\n", i+1, reverseU16(pVehicleReportData->vehicleId));
                        UpdateMsgData(msgBuffer);
                        snprintf(msgBuffer,sizeof(msgBuffer), "          vehicleStatus = 0x%02x\r\n", pVehicleReportData->vehicleStatus);
                        UpdateMsgData(msgBuffer);
                        snprintf(msgBuffer,sizeof(msgBuffer), "          Value : X = 0x%04x,  Y = 0x%04x,  Z = 0x%04x\r\n", reverseU16(pVehicleReportData->xValue), reverseU16(pVehicleReportData->yValue), reverseU16(pVehicleReportData->zValue));
                        UpdateMsgData(msgBuffer);
                        //
                        vehicleReportDataItem[i].vehicleId = pVehicleReportData->vehicleId;
                    }
                    uint8_t checksum = *(vehicleReportStart + vehicleNum * sizeof(vehicleReportData));
                    snprintf(msgBuffer,sizeof(msgBuffer), "    - checksum = 0x%02x\r\n", checksum);
                    UpdateMsgData(msgBuffer);
                    //
                    readIndex = readIndex + totalLen;

                    {
                        int destContainLen;
                        uint8_t* startContain = destAllContain + destAllContainIndex;
                        int startContainLen = sizeof(destAllContain) - destAllContainIndex;

                        destContainLen = MRGetVehicleReportAck(startContain, startContainLen);
                        destAllContainIndex = destAllContainIndex + destContainLen;
                    }
                }
                break;

            case MR_B2S_CMD_VEHICLE_VER:; //b2sVehicleVerCmd; //MR_B2S_CMD_VEHICLE_VER
            {
                 b2sVehicleVerCmd* pb2sVehicleVerCmd = (b2sVehicleVerCmd*)pData;
                 memcpy(&b2sVehicleVerCmdItem, pData, sizeof(b2sVehicleVerCmdItem));

                 cmdLen = reverseU16(pb2sVehicleVerCmd->cmdLen);
                 snprintf(msgBuffer,sizeof(msgBuffer), " GET MR_B2S_CMD_VEHICLE_VER(0x%02x): cmd len = %d(0x%04x)\r\n", pb2sVehicleVerCmd->cmdId, cmdLen, cmdLen);
                 UpdateMsgData(msgBuffer);

                 totalLen = cmdLen + sizeof(pb2sVehicleVerCmd->cmdId) + sizeof(pb2sVehicleVerCmd->cmdLen);
                 dumpDataMsg(" # MR_B2S_CMD_VEHICLE_VER #", pData, totalLen);
                 //---
                 int vehicleNum = pb2sVehicleVerCmd->vehicleNum;
                 snprintf(msgBuffer,sizeof(msgBuffer), "    - vehicleNum = %d\r\n", vehicleNum);
                 UpdateMsgData(msgBuffer);
                 //
                 uint8_t* vehicleVerStart = &(pb2sVehicleVerCmd->vehicleVerStart);
                 /*
                uint16_t vehicleId;    //3     第一個車檢器組ID    B2  車位號
                uint8_t  verStr[12];    //4     第一個分咪表的版本   B12 形如：HV3.0 - SV1.41
                                        //                      表示硬體3.0版本；軟體1.41版本
                 */
                 for (int i = 0; i < vehicleNum; i++)
                 {
                     vehicleVerData* pVehicleVerData = (vehicleVerData*)(vehicleVerStart + i * sizeof(vehicleVerData));
                     snprintf(msgBuffer,sizeof(msgBuffer), "      [%d] vehicleId = 0x%04x\r\n", i + 1, reverseU16(pVehicleVerData->vehicleId));
                     UpdateMsgData(msgBuffer);
                     snprintf(msgBuffer,sizeof(msgBuffer), "          verStr = [%s]\r\n", pVehicleVerData->verStr);
                     UpdateMsgData(msgBuffer);    
                     //
                     vehicleVerDataItem[i].vehicleId = pVehicleVerData->vehicleId;
                 }
                 uint8_t checksum = *(vehicleVerStart + vehicleNum * sizeof(vehicleVerData));
                 snprintf(msgBuffer,sizeof(msgBuffer), "    - checksum = 0x%02x\r\n", checksum);
                 UpdateMsgData(msgBuffer);
                 //
                 readIndex = readIndex + totalLen;

                 {
                     int destContainLen;
                     uint8_t* startContain = destAllContain + destAllContainIndex;
                     int startContainLen = sizeof(destAllContain) - destAllContainIndex;

                     destContainLen = MRGetVehicleVerAck(startContain, startContainLen);
                     destAllContainIndex = destAllContainIndex + destContainLen;
                 }
             }
             break;

             case MR_B2S_ACK_PARA_DOWNLOAD:; //b2sParaDownloadAck; //MR_B2S_ACK_PARA_DOWNLOAD
             {   
                 b2sParaDownloadAck* pb2sParaDownloadAck = (b2sParaDownloadAck*)pData;
                 memcpy(&b2sParaDownloadAckItem, pData, sizeof(b2sParaDownloadAckItem));

                 cmdLen = pb2sParaDownloadAck->cmdLen;
                 snprintf(msgBuffer,sizeof(msgBuffer), " GET MR_B2S_ACK_PARA_DOWNLOAD(0x%02x): cmd len = %d(0x%02x)\r\n", pb2sParaDownloadAck->cmdId, cmdLen, cmdLen);
                 UpdateMsgData(msgBuffer);

                 totalLen = cmdLen + sizeof(pb2sParaDownloadAck->cmdId) + sizeof(pb2sParaDownloadAck->cmdLen);
                 dumpDataMsg(" # MR_B2S_ACK_PARA_DOWNLOAD #", pData, totalLen);
                 //---
                 snprintf(msgBuffer,sizeof(msgBuffer), "    - baseId = 0x%04x\r\n", reverseU16(pb2sParaDownloadAck->baseId));
                 UpdateMsgData(msgBuffer);
                 int paraNum = pb2sParaDownloadAck->paraNum;
                 snprintf(msgBuffer,sizeof(msgBuffer), "    - paraNum = %d\r\n", paraNum);
                 UpdateMsgData(msgBuffer);
                 //
                 uint8_t* paraDataStart = &(pb2sParaDownloadAck->paraDataStart);

                 for (int i = 0; i < paraNum; i++)
                 {
                     paraDownloadData* pparaDownloadData = (paraDownloadData*)(paraDataStart + i * sizeof(paraDownloadData));
                     snprintf(msgBuffer,sizeof(msgBuffer), "      [%d] code = 0x%02x\r\n", i + 1, pparaDownloadData->code);
                     UpdateMsgData(msgBuffer);
                     snprintf(msgBuffer,sizeof(msgBuffer), "          successFlag = 0x%02x\r\n", pparaDownloadData->successFlag);
                     UpdateMsgData(msgBuffer);                     
                 }
                 uint8_t checksum = *(paraDataStart + paraNum * sizeof(paraDownloadData));
                 snprintf(msgBuffer,sizeof(msgBuffer), "    - checksum = 0x%02x\r\n", checksum);
                 UpdateMsgData(msgBuffer);
                 //
                 readIndex = readIndex + totalLen;
                 
                 {
                     int destContainLen;
                     uint8_t* startContain = destAllContain + destAllContainIndex;
                     int startContainLen = sizeof(destAllContain) - destAllContainIndex;

                     if (paraCmdGeomangReinitFlag)
                     {
                         destContainLen = MRGetParaDLCmdReinit(startContain, startContainLen);
                         //paraCmdGeomangReinitFlag = 0;
                     }
                     else
                     {
                         destContainLen = MRGetParaDownloadCmd(startContain, startContainLen);
                         paraCmdGeomangReinitFlag = 0;
                     }
                     destAllContainIndex = destAllContainIndex + destContainLen;
                 }
             }
             break;

            default:
                snprintf(msgBuffer,sizeof(msgBuffer), " NOT SUPPORT (0x%02x): BREAK (readIndex = %d v.s %d)!!!\r\n", cmdId, readIndex, dataLen);
                //UpdateMsgData(msgBuffer);
                readIndex = 0xFFFF;
                break;
        }     
    }
    UpdateMsgData("------------------------------   PARSE FINISH ------------------------------\r\n");

    if (destAllContainIndex > 0)
    {
        #if(0)
        dumpDataMsg("Dest All ACK Contain", destAllContain, destAllContainIndex);
        #endif

        uint8_t destData[128];
        int destDataLen;

        destDataLen = MRSendAck(destData, sizeof(destData), destAllContain, destAllContainIndex, "ACK Encrypt");

        #ifdef _WINDOWS
        MRUartWrite(destData, destDataLen);
        #else
        mrDrvWritee(destData, destDataLen);
        #endif
        UpdateMsgData("------------------------------   ACK FINISH ------------------------------\r\n\r\n\r\n");
    }
    return dataLen;
}

int MRParserData(uint8_t* pData, int dataLen)
{
    #if(ENABLE_PARSE_PACKAGE_MSG)
    dumpDataMsg("-- MRParserData -- ", pData, dataLen);
    #endif
    uint8_t* cmdStart = pData + sizeof(packageHeader);

    packageHeader* ppackageHeader = (packageHeader*)pData;

    //
    memcpy(aesKey, ppackageHeader->aesKey, sizeof(ppackageHeader->aesKey));
    aesKey[3] = 0x0;
    baseId = reverseU16(ppackageHeader->id.baseId);
    mrLocalBaseID = (int)baseId;
    opCode = ppackageHeader->id.opCode;
    ackId = ppackageHeader->ackId;    
    //
    int cmdLen = reverseU16(ppackageHeader->cmdLen); //dataLen - sizeof(packageHeader);

    snprintf(msgBuffer, sizeof(msgBuffer), "-- Header: opCode = 0x%02x, baseId = 0x%04x, ackId = 0x%02x, cmdLen = %d --\r\n", opCode, baseId, ackId, cmdLen);
    UpdateMsgData(msgBuffer);

    // Generate 16 Bytes AES Key
    int i = 0;
    while (i < 16) {
        combineAeskey[i] = opCode;
        combineAeskey[i+1] = (uint8_t)baseId >> 8;
        combineAeskey[i+2] = (uint8_t)baseId;
        combineAeskey[i+3] = 0x00;
        i=i+4;
    }
    //dumpDataMsg("combineAesKey", combineAeskey, 16);

    uint8_t* destData = NULL;
    int     destDataLen = 0;
    if (ackId == MR_B2S_CMD_REG_WORD)
    {
        MRAESDecrypt(cmdStart, cmdLen, &destData, &destDataLen);
    }
    else if (ackId == MR_B2S_CMD_BASE_STATUS_WORD)
    {
        MRAESDecrypt(cmdStart+2, cmdLen-2, &destData, &destDataLen);
    }
    #if(ENABLE_PARSE_PACKAGE_MSG)
    dumpDataMsg("CMD Decrypt", destData, destDataLen);
    #endif
    MRPrintCmd(destData, destDataLen);

    return dataLen;
}

int MRProcessData(uint8_t* pData, int dataLen)
{
    #if(ENABLE_CHECK_PACKAGE_MSG)
    dumpDataMsg("MRProcessData", pData, dataLen);
    #endif
    if (packageBufferIndex == 0)
    {
        for (int i = 0; i < dataLen; i++)
        {
            if (pData[i] == MR_HEADER_TYPE)
            {
                int copyLen = dataLen - i;
                #if(ENABLE_CHECK_PACKAGE_MSG)
                dumpDataMsg("~ CREATE ~", pData + i, copyLen);
                #endif
                memcpy(packageBuffer + packageBufferIndex, pData + i, copyLen);
                
                packageBufferIndex = packageBufferIndex + copyLen;

                resetPackageValue();
            }
        }        
    }
    else
    {
        #if(ENABLE_CHECK_PACKAGE_MSG)
        dumpDataMsg("~ APPEND ~", pData, dataLen);
        #endif
        memcpy(packageBuffer + packageBufferIndex, pData, dataLen);
        packageBufferIndex = packageBufferIndex + dataLen;
    }
    if( (pakageLen == 0) && (packageBufferIndex > sizeof(packageHeader)) )
    {
        packageHeader* ppackageHeader = (packageHeader*)packageBuffer;

        cmdLen = reverseU16(ppackageHeader->cmdLen);
        pakageLen = cmdLen + sizeof(packageHeader);
        #if(ENABLE_CHECK_PACKAGE_MSG)
        snprintf(msgBuffer,sizeof(msgBuffer), "-- GET Header: cmd len = %d(0x%04x), pakageLen = %d\r\n", cmdLen, cmdLen, pakageLen);
        UpdateMsgData(msgBuffer);
        #endif
    }
    if (pakageLen != 0)
    {
        if (packageBufferIndex == pakageLen)
        {
            #if(ENABLE_CHECK_PACKAGE_MSG)
            snprintf(msgBuffer,sizeof(msgBuffer), "-- GET PACKAGE !!! \r\n\r\n");
            UpdateMsgData(msgBuffer);
            #endif

            MRParserData(packageBuffer, packageBufferIndex);

            packageBufferIndex = 0;
            resetPackageValue();
        }
        else if (packageBufferIndex > pakageLen)
        {
            #if(ENABLE_CHECK_PACKAGE_MSG)
            snprintf(msgBuffer,sizeof(msgBuffer), "-- ERROR: packageBufferIndex = %d (pakageLen = %d) !!! \r\n", packageBufferIndex, pakageLen);
            UpdateMsgData(msgBuffer);
            #endif

            packageBufferIndex = 0;
            resetPackageValue();
        }
        else if (packageBufferIndex < pakageLen)
        {
            #if(ENABLE_CHECK_PACKAGE_MSG)
            snprintf(msgBuffer,sizeof(msgBuffer), "-- WAIT: packageBufferIndex = %d (pakageLen = %d) !!! \r\n", packageBufferIndex, pakageLen);
            UpdateMsgData(msgBuffer);
            #endif
        }
        
    }
    return 0;
}

int MRAESEncrypt(uint8_t* pData, int dataLen, uint8_t** resultData, int* resultDataLen)
{
    struct AES_ctx ctx;
    *resultDataLen = 0;
    if (dataLen > sizeof(aesBuffer))
        return 0;

    int batchNum = dataLen/AES_BATCH_LEN;
    if ((dataLen % AES_BATCH_LEN) != 0)
        batchNum++;
    memset(aesBuffer, 0x00, sizeof(aesBuffer));
    memcpy(aesBuffer, pData, dataLen);

    AES_init_ctx(&ctx, combineAeskey);

    for (int i = 0; i < batchNum; i++)
    {
        AES_ECB_encrypt(&ctx, aesBuffer + (i * AES_BATCH_LEN));
    }
    *resultData = aesBuffer;
    *resultDataLen = batchNum * AES_BATCH_LEN;
    return *resultDataLen;
}

static uint8_t vechicleAlgorithmPara[40] = { 0x0A, 0x32, 0x12, 0x00, 0xC8, 0x01, 0x90, 0x28, 0x64, 0x15,
                                            0x23, 0x00, 0x78, 0x00, 0x37, 0x00, 0x96, 0x14, 0x2D, 0x04, 
                                            0x15, 0x2D, 0x11, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
int MRAESDecrypt(uint8_t* pData, int dataLen, uint8_t** resultData, int* resultDataLen)
{
    struct AES_ctx ctx;
    *resultDataLen = 0;

    if (dataLen > sizeof(aesBuffer))
        return 0;

    if ((dataLen % AES_BATCH_LEN) != 0)
        return 0;

    int batchNum = dataLen / AES_BATCH_LEN;

    memset(aesBuffer, 0x00, sizeof(aesBuffer));
    memcpy(aesBuffer, pData, dataLen);

    AES_init_ctx(&ctx, combineAeskey);

    for (int i = 0; i < batchNum; i++)
    {
        AES_ECB_decrypt(&ctx, aesBuffer + (i * AES_BATCH_LEN));
    }
    *resultData = aesBuffer;
    *resultDataLen = batchNum * AES_BATCH_LEN;
    return *resultDataLen;
}

int MRSendRegCmdAck(uint8_t* destData, int destDataLen)
{
    int totalCmdLen = 0;
    packageHeader header;
    s2bRegCmdAck regCmdAck;
    s2bParaDownloadCmd downloadCmd;

    if (destDataLen < (sizeof(header) + sizeof(s2bRegCmdAck) + sizeof(downloadCmd)))
    {
        return 0;
    }

    memset(&header, 0x00, sizeof(header));
    memset(&regCmdAck, 0x00, sizeof(s2bRegCmdAck));
    memset(&downloadCmd, 0x00, sizeof(downloadCmd));
    //header
    header.type = MR_ACK_HEADER_TYPE;
    memcpy(header.aesKey, aesKey, sizeof(header.aesKey)); //3 bytes
    header.ackId = MR_S2B_ACK_WORD;// ackId;
    header.cmdLen = reverseU16(sizeof(s2bRegCmdAck) + sizeof(downloadCmd));
    //reg ack
    regCmdAck.cmdId = MR_S2B_ACK_REG;               //1#  0   命令字 B1  0x0d
    regCmdAck.cmdLen = sizeof(s2bRegCmdAck) - 2;// 0x61 //97;               //    1   命令長度    B1  後續命令長度
    regCmdAck.regSuccessMark = 0x01;            //2#  2   基站註冊成功標識    B1  Bit1~Bit0=00註冊失敗，Bit1~Bit0=01註冊成功;
                                        //                               Bit2~Bit7預留
    regCmdAck.baseId = reverseU16(baseId);// reverseU16(0x000A);                  //3#  3   基站ID    B2  高位元組在前（範圍1~65535）
    regCmdAck.baseHeartbeatTime = 01;       //    5   基站/伺服器脈搏時間  B1  Bit3~bit0 標識基站伺服器脈搏時間
                                        //                                Bit7~bit4預留
                                        //                                  0x00,6秒
                                        //                                  0x01，1分鐘            
                                        //                                  0x02，2分鐘
                                        //                                  0x03，3分鐘
                                        //                                  0x04，4分鐘
                                        //                                  0x05，5分鐘
                                        //                                  ....
                                        //                                  0x0a，10分鐘
                                        //                                  默認1分鐘
    regCmdAck.vechicleHeartbeatTime = 01;   //    6   基站/車檢器脈搏時間  B1  Bit4~bit0 標識基站車檢器脈搏時間
                                        //                            Bit7~bit5預留
                                        //                              0x00,6秒
                                        //                              0x01，1分鐘
                                        //                              0x02，2分鐘
                                        //                              0x03，3分鐘
                                        //                              0x04，4分鐘
                                        //                              0x05，5分鐘
                                        //                              ....
                                        //                              0x0f，15分鐘
                                        //                              默認2分鐘
    #ifdef _WINDOWS
    regCmdAck.timeStamp = reverseU32((uint32_t)getUTCTime());;              //    7   時間戳記    B4  系統當前時鐘
    #else
    regCmdAck.timeStamp = reverseU32((uint32_t)GetCurrentUTCTime());;              //    7   時間戳記    B4  系統當前時鐘
    #endif
    //regCmdAck.timeStandbyTime[4] = 0x00000000;     //    11  基站靜默時間  B4   位置6位元組表示起始小時，預設0（範圍0~23）
                                        //                      位置7位元組表示起始分鐘，預設0（範圍0~59）
                                        //                      位置8位元組表示停止小時，預設0（範圍0~23）
                                        //                      位置9位元組表示停止分鐘，預設0（範圍0~59）
                                        //                      起始停止時間為全0表示基站
                                        //                      不休眠。預設值全0x00000000
    regCmdAck.initBordcastChannel = mrLocalBroadcastCH;   //4#  15  基站/車檢器初始化廣播頻道   B1  LoRa子模組1和車檢器初始化頻道（參考頻率編排表格）
                                        //                      注：0表示未分配初始化頻道
    regCmdAck.workChannel = mrLocalWorkCH;               //    16  基站/車檢器工作頻道  B1  LoRa子模組1和車檢器工作頻道（參考頻率編排表格）
                                        //                      注：0表示未分配初始化頻道
    regCmdAck.resverChannel[0] = 0x00;      //    17  預留  B2  預留
    regCmdAck.resverChannel[1] = 0x00;                                    //                        注：0表示未分配工作頻道
    regCmdAck.baseManageNum = reverseU16(MANAGE_VECHICLE_NUM); //reverseU16(0x0005);          //    19  基站管理車位個數    B2  高位元組在前，每單位標識管理一個車檢器（注：系統管理基站關聯車檢器個數）
                                        //                      例如：0x0100，代表管理256個車檢器
    regCmdAck.baseReceiveAck = 0x07;         //    20  基站接收應答擴頻因數  B1  Bit3~bit0有效，bit4~bit7預留
                                        //                              0x06對應擴頻因數6
                                        //                              0x07對應擴頻因數7
                                        //                              0x08對應擴頻因數8
                                        //                              0x09對應擴頻因數9
                                        //                              0x0a對應擴頻因數10
                                        //                              0x0b對應擴頻因數11
                                        //                              0x0c對應擴頻因數12
                                        //                              默認SF=7
    regCmdAck.loraErrorCorrectionRate = 0x01;//    21  LoRa子模組糾錯率  B1  Bit3~bit0有效，bit4~bit7預留
                                        //                                  0x01對應4/5
                                        //                                  0x02對應4/6
                                        //                                  0x03對應4/7
                                        //                                  0x04對應4/8
                                        //                                  默認0x01
    regCmdAck.loraBandWidth = 0x0A;          //    22  LoRa通訊頻寬    B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x01對應7.8KHZ
                                        //                          0x02對應10.4 KHZ
                                        //                          0x03對應15.6KHZ
                                        //                          0x04對應20.8KHZ
                                        //                          0x05對應31.2KHZ
                                        //                          0x06對應41.7KHZ
                                        //                          0x07對應62.5KHZ
                                        //                          0x08對應125KHZ
                                        //                          0x09對應250KHZ
                                        //                          0x0a對應500 KHZ
                                        //                          默認0x0a
    regCmdAck.loraPower = 0x80;             //    23  LoRa通訊功率    B1    BIT7=0關閉PA
                                        //                              Bit7=1開啟PA
                                        //                              BIT6=0開啟LNA
                                        //                              Bit6=1開啟LNA
                                        //                              BIT5~BIT0 對應發射功率
                                        //                              預設值128
    regCmdAck.resverLora[0] = 0x00;         //    24  預留  B2  預留
    regCmdAck.resverLora[1] = 0x00;
    regCmdAck.vechicleParaCmd = 0x03;       //5#  26  車檢器參數命令 B1  0x03 車檢器參數命令字
    regCmdAck.vechicleParaLen = 0x2A;       //    27  車檢器參數命令長度   B1  命令內容長度
    regCmdAck.vechicleBoradecastPower = 0x01;//    28  車檢器標籤廣播功率   B1  Bit3~bit0有效，bit4~bit7預留
                                        //                              0x00：0dbm，
                                        //                              0x01：-6dbm，
                                        //                              0x02：-12dbm，
                                        //                              0x03：-18dbm
                                        //                              車檢器標籤廣播功率(默認0x01)
    regCmdAck.vechicleSampleTime = 0x02;    //    29  地磁採集週期  B1  Bit3~bit0有效，bit4~bit7預留
                                        //                              0x01-62.5ms
                                        //                              0x02-125ms
                                        //                              0x03-187.5ms
                                        //                              0x04-250ms
                                        //                              0x05-312.5ms
                                        //                              0x06-375ms
                                        //                              0x07-437.5ms
                                        //                              0x08-500ms
                                        //                              ...
                                        //                              0x010-1250ms
                                        //                          62.5*n每單位標識62.5ms 預設2即125ms
    memcpy(regCmdAck.vechicleAlgorithmPara, vechicleAlgorithmPara, sizeof(vechicleAlgorithmPara));//  30  車檢演算法參數 B40 見車檢演算法參數表格
    regCmdAck.tagParaCmd = 0x04;                //6#  70  標籤參數命令  B1  0x04
    regCmdAck.tagParaCmdLen = 0x19; //25        //    71  標籤參數命令長度    B1  根據命令內容長度
    regCmdAck.tagParaStopRange = 0x28;      //    72  標籤靜止幅度  B1  5~100預設值(0x28)
    regCmdAck.tagParaStopTimes = 0x08;      //    73  標籤靜止次數  B1  2~30預設值(0x08)
    regCmdAck.tagParaStopTime = 0x23;       //    74  標籤靜止時間  B1  1~160預設值(0x23)
    regCmdAck.tagParaMoveRange = 0x20;      //    75  標籤移動幅度  B1  5~100預設值(0x20)
    regCmdAck.tagParaMoveTimes = 0x06;      //    76  標籤移動次數  B1   2~30預設值(0x06)
    regCmdAck.tagParaMoveTime = 0x0c;      //    77  標籤移動時間  B1  1~160預設值(0x0c)
    //regCmdAck.resverTag[6];           //    78  標籤磁場參數  B6  預留
    regCmdAck.directedBoardcastTimes = 0x03;    //    84  定向廣播次數  B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x01，6次
                                        //                          0x02，12次
                                        //                          0x03，18次
                                        //                          0x04，24次
                                        //                          默認18次
    regCmdAck.directedBoardcastTime = 0x01; //    85  定向廣播時間間隔    B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x00，關閉定向廣播
                                        //                          0x01，10秒
                                        //                          0x02，20秒
                                        //                          0x03，30秒
                                        //                          0x04，40秒
                                        //                          默認10秒
    regCmdAck.taggleBoardcastTimes = 0x03;  //    86  觸發廣播次數  B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x01，6次
                                        //                          0x02，12次
                                        //                          0x03，18次
                                        //                          0x04，24次
                                        //                          默認18次
    regCmdAck.taggleBoardcastTime = 0x01;   //    87  觸發廣播時間間隔    B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x00，關閉觸發廣播
                                        //                          0x01，10秒
                                        //                          0x02，20秒
                                        //                          0x03，30秒
                                        //                          0x04，40秒
                                        //                          默認10秒
    regCmdAck.scheduledBroadcastTimes = 0x03;//    88  定時廣播次數  B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x01，6次
                                        //                          0x02，12次
                                        //                          0x03，18次
                                        //                          0x04，24次
                                        //                          默認18次
    regCmdAck.scheduledBroadcastTime = 0x05;    //    89  定時廣播時間間隔    B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x00，關閉定時廣播
                                        //                          0x01，1分鐘
                                        //                          0x02，2分鐘
                                        //                          0x03，3分鐘
                                        //                          ...
                                        //                          0x0a，10分鐘
                                        //                          默認5分鐘
    regCmdAck.tagRegTimes = 0x01;           //    90  標籤首次註冊次數    B1  標籤註冊，和車檢器連續多少次通訊成功後上報
                                        //                          0x01，1次
                                        //                          0x02，2次
                                        //                          0x03，3次
                                        //                          0x04，4次
                                        //                          0x05，5次
                                        //                          默認1次
    regCmdAck.bkpBaseId = reverseU16(mrBkpBaseID);   //    91	备份基站ID                  B2	高字节在前（范围1~65535）, 0没有备份基站
    regCmdAck.bkpInitBordcastChannel = mrBkpBroadcastCH;    //    93	备份基站 / 车检器初始化广播频道	B1	LoRa子模块1和车检器初始化频道（参考频率编排表格）
                                                //          注：0表示未分配初始化频道
    regCmdAck.bkpWorkChannel = mrBkpWorkCH;            //    94	备份基站 / 车检器工作频道	   B1	LoRa子模块1和车检器工作频道（参考频率编排表格）
                                                //          注：0表示未分配初始化频道
    //regCmdAck.reserve[2];             //    95  預留  B2  預留
    regCmdAck.checksum = 0x00;              //7#  97  校驗字 B1  資料內容校驗和

    //downloadCmd
    downloadCmd.cmdId = MR_S2B_CMD_PARA_DOWNLOAD;                //1#  0 命令字 B1  0x8d
    downloadCmd.cmdLen = reverseU16(0x0004);                //    1 命令長度    B2  命令內容長度
    downloadCmd.baseId = reverseU16(baseId);// reverseU16(0x000A);;           //2#  2 基站ID    B2  基站ID
    downloadCmd.paraNum = 0x00;         //    4 參數代碼個數  B1  代表後面需要下載的參數個數
    #if(0)
    downloadCmd.code = 0x00;                //    5 參數代碼1   B1  見參數代碼表    
    #endif
    downloadCmd.checksum = 0x00;            //3#    校驗字 B1  命令內容累加（不包括命令長度命令字）


    memcpy(destData,                                      &header,       sizeof(header));
    memcpy(destData + sizeof(header),                     &regCmdAck,    sizeof(regCmdAck));
    memcpy(destData + sizeof(header) + sizeof(regCmdAck), &downloadCmd,  sizeof(downloadCmd));
    totalCmdLen = sizeof(header) + sizeof(s2bRegCmdAck) + sizeof(downloadCmd);
    dumpDataMsg("RegCmdAck (raw)", destData, totalCmdLen);
    {
        uint8_t* destData2;
        int     destDataLen2;
        MRAESEncrypt(destData + sizeof(header), totalCmdLen - sizeof(header), &destData2, &destDataLen2);

        dumpDataMsg("RegCmdAck (Encrypt)", destData2, destDataLen2);

        header.cmdLen = reverseU16(destDataLen2);
        memcpy(destData, &header, sizeof(header));
        memcpy(destData + sizeof(header), destData2, destDataLen2);
        totalCmdLen = sizeof(header) + destDataLen2;
    }
    //dumpDataMsg("RegCmdAck", destData, totalCmdLen);

    return totalCmdLen;
}

int MRGetBaseStatusAck(uint8_t* destData, int destDataLen)
{
    /*
    //1.5.3.14  基站狀態彙報應答（0x6d）
    typedef struct
    {
        uint8_t cmdId;      //1#    0   命令字     B1  0x6d
        uint8_t cmdLen;     //      1   命令長度    B1  命令內容長度
        uint32_t timeStamp; //2#    2   時間戳記    B4  年月日時分秒
        uint8_t reserve[2]; //3#    6   基站狀態字   B2  預留
        uint8_t checksum;   //4#    8   校驗字 B1  命令內容累加（不包括命令長度命令字）
    }s2bBaseStatusAck; //MR_S2B_ACK_BASE_STATUS

    6D                                                  //1Byte         基站狀態應答命令字
    07                                                  //1Byte         命令長度
    64D1E699                                            //4Byte         Unix時間戳(當前系統時鐘)
    0000                                                //2Byte         預留(默認0)
    00                                                  //1Byte
    */
    s2bBaseStatusAck baseStatusAck;
    if (destDataLen < sizeof(baseStatusAck))
    {
        return 0;
    }
    memset(&baseStatusAck, 0x00, sizeof(baseStatusAck));

    baseStatusAck.cmdId = MR_S2B_ACK_BASE_STATUS;                               //1#    0   命令字     B1  0x6d
    baseStatusAck.cmdLen = sizeof(baseStatusAck) - 2;                           //      1   命令長度    B1  命令內容長度

    #ifdef _WINDOWS
    baseStatusAck.timeStamp = reverseU32((uint32_t)getUTCTime());;              //2#    2   時間戳記    B4  年月日時分秒
    #else
    baseStatusAck.timeStamp = reverseU32((uint32_t)GetCurrentUTCTime());;       //2#    2   時間戳記    B4  年月日時分秒
    #endif

    baseStatusAck.reserve[0] = 0x00;                                            //3#    6   基站狀態字   B2  預留
    baseStatusAck.reserve[1] = 0x00;
    baseStatusAck.checksum = 0x00;                                              //4#    8   校驗字 B1  命令內容累加（不包括命令長度命令字）
    
    memcpy(destData, &baseStatusAck, sizeof(baseStatusAck));

    int ackDatalen = sizeof(s2bBaseStatusAck);
    destData[ackDatalen-1] = genAckCheckSum(destData, 2, ackDatalen);

    dumpDataMsg("MRGetBaseStatusAck 0x6D", destData, sizeof(baseStatusAck));

    return sizeof(baseStatusAck);
}

int MRGetVehicleStatusAck(uint8_t* destData, int destDataLen)
{
    /*
    //1.5.3.8   車位元狀態應答（0x4d）
    typedef struct
    {
        uint8_t cmdId;              //1#  0   命令字      B1  0x4d
        uint16_t cmdLen;            //    1   命令長度    B2  命令內容長度
        uint8_t reserve;            //    3   預留      預留
        uint8_t baseNum;            //        基站個數    B1  基站個數包括基站本身和臨近基站ID
        uint16_t baseId;            //        第N個基站ID B2  自身基站ID
        uint16_t vehicleNum;        //2#  4   安裝車檢器個數 B2  系統記錄基站啟動車檢器個數
        uint8_t vehicleAckNum;      //    5   車檢器狀態應答個數   B1  代表有多少條車檢器應答資訊（例如：本次有5條車檢器應答資訊，車檢器個數欄位對應5）
        uint8_t vehicleStatusStart; //vehicleStatusAckData
                                    //... ... ...
                                    //車位元狀態彙報校驗字  B1  命令內容累加（不包括命令長度命令字）
    }s2bVehicleStatusAck; //MR_S2B_ACK_VEHICLE_STATUS

    4D                                                  //1Byte         車檢器狀態應答命令字
    000B                                                //2Byte         命令長度
    00                                                  //1Byte         車檢器值狀態(0代表初值，1代表實時值)
    01                                                  //1Byte         車檢器個數
    000A                                                //2Byte         基站ID
    0005                                                //2Byte         基站管理車檢器個數
    01                                                  //1Byte         車檢器狀態應答個數

    0001                                                //2Byte         車檢器組ID
    00                                                  //1Byte         車檢器狀態控制
    00                                                  //1Byte
    */
    s2bVehicleStatusAck vehicleStatusAck;
    if (destDataLen < sizeof(vehicleStatusAck))
    {
        return 0;
    }
    memset(&vehicleStatusAck, 0x00, sizeof(vehicleStatusAck));

    vehicleStatusAck.cmdId = MR_S2B_ACK_VEHICLE_STATUS;             //1#  0   命令字      B1  0x4d
    vehicleStatusAck.cmdLen = 0x00; //temp value                    //    1   命令長度    B2  命令內容長度
    vehicleStatusAck.reserve = 0x00;                                //    3   預留      預留   //車檢器值狀態(0代表初值，1代表實時值)???
    vehicleStatusAck.baseNum = b2sVehicleStatusCmdItem.baseNum;     //        基站個數    B1  基站個數包括基站本身和臨近基站ID
    vehicleStatusAck.baseId = vehicleStatusBaseDataItem.baseId;       //        第N個基站ID B2  自身基站ID
    vehicleStatusAck.vehicleNum = reverseU16(MANAGE_VECHICLE_NUM);// b2sVehicleStatusCmdItem.vehicleNum;        //2#  4   安裝車檢器個數 B2  系統記錄基站啟動車檢器個數
    vehicleStatusAck.vehicleAckNum = vehicleStatusBaseDataItem.vehicleNum;      //    5   車檢器狀態應答個數   B1  代表有多少條車檢器應答資訊（例如：本次有5條車檢器應答資訊，車檢器個數欄位對應5）
    //vehicleStatusAck.vehicleStatusStart = 0x00; //vehicleStatusAckData
    
    int index = (uint8_t*)&(vehicleStatusAck.vehicleStatusStart) - (uint8_t*)(&vehicleStatusAck);
    /*
    typedef struct
    {
        uint16_t vehicleId;         //   6   車檢器組ID  B2  車檢器組ID
        uint8_t  vehicleStatus;     //   7   車檢器狀態   B1  Bit7：預留
                                    //            Bit6~Bit4 車檢器參數命令
                                    //              000：無任何操作
                                    //              001 車檢器磁場值初值採集
                                    //              010車位元狀態遠端校正(預留)
                                    //              111~011預留
                                    //              預留
                                    //              bit0:表示車檢器相鄰車位元左邊車位元狀態(bit0=1代表左邊車位有車，bit0=0代表左邊車位無車)
                                    //              bit1:表示車檢器相鄰車位元右邊車位元狀態(bit1=1代表右邊車位有車，bit1=0代表右邊車位無車)
                                    //              bit3~bit2：預留
    }vehicleStatusAckData;
    */
    #if(0) //TEST DATA
    vehicleStatusAck.vehicleAckNum = 1;
    #endif
    for(int i = 0; i < vehicleStatusAck.vehicleAckNum; i++)
    {        
        vehicleStatusAckData* pVehicleStatusAckData = (vehicleStatusAckData * )(destData + index);
        pVehicleStatusAckData->vehicleId = vehicleStatusDataItem[i].vehicleId;// reverseU16(i + 1);
        pVehicleStatusAckData->vehicleStatus = vehicleStatusDataItem[i].vehicleStatus;// 0x00;

        index = index + sizeof(vehicleStatusAckData);
    }

    // Bkp Vehicle status part
    //if (vehicleStatusAck.vehicleAckNum > 0)
    {
        vehicleStatusBaseAckData* pBkpVehicleStatusAckData = (vehicleStatusBaseAckData*)(destData + index);
        pBkpVehicleStatusAckData->baseId = vehicleStatusBkpBaseDataItem.baseId;       // 10+3N  第N個基站ID B2  备份基站ID
        pBkpVehicleStatusAckData->vehicleNum = reverseU16(MANAGE_VECHICLE_NUM);       // 12+3N  备份基站管理车检器个数
        pBkpVehicleStatusAckData->vehicleAckNum = vehicleStatusBkpBaseDataItem.vehicleNum;   // 14+3N  备份基站车检器状态应答个数
        index = index + sizeof(vehicleStatusBaseAckData);

        for (int i = 0; i < pBkpVehicleStatusAckData->vehicleAckNum; i++)
        {
            vehicleStatusAckData* pBkpVehicleStatusAckData = (vehicleStatusAckData*)(destData + index);
            pBkpVehicleStatusAckData->vehicleId = bkpVehicleStatusDataItem[i].vehicleId;// reverseU16(i + 1);
            pBkpVehicleStatusAckData->vehicleStatus = bkpVehicleStatusDataItem[i].vehicleStatus;// 0x00;
            index = index + sizeof(vehicleStatusAckData);
        }

    }

    destData[index] = 0x00; //CRC
    index++;

    vehicleStatusAck.cmdLen = reverseU16(index - 3);

    memcpy(destData, &vehicleStatusAck, sizeof(vehicleStatusAck)-1);

    // Generate checksum
    destData[index - 1] = genAckCheckSum(destData, 4, index);

    dumpDataMsg("MRGetVehicleStatusAck 0x4D", destData, index);

    return index;
}

int MRGetVehicleReportAck(uint8_t* destData, int destDataLen)
{
    /*
    //1.5.3.6   車檢器值應答（0xad）
    typedef struct
    {
        uint8_t cmdId;          //1#    0   命令字 B1  0xad
        uint16_t cmdLen;        //      1   命令長度    B2  命令內容長度
        uint8_t dataStatus_;    //2#    3   內容狀態字   B1  Bit7~bit1 預留
                                //                            Bit0=0：地磁內容為車檢器最新初值
        uint8_t vehicleReportNum_;//3#  4   車檢器應答值個數    B1  車檢器應答值個數
        //----
        uint8_t dataStatus;     //4#    3   內容狀態字   B1  Bit7~bit1 預留
                                //                            Bit0=1：地磁內容為車檢器狀態變化值
        uint8_t vehicleReportNum; //5#  4   車檢器應答值個數    B1  車檢器應答值個數
        uint8_t vehicleReportStart;//   5   車檢器組ID   B2  車檢器組ID  vehicleReportAck
                                //       ... ... ...
                                //車位元狀態彙報校驗字  B1  命令內容累加（不包括命令長度命令字）

    }s2bVehicleReportAck; //    MR_S2B_ACK_VEHICLE_REPORT

    AD                                                  //1Byte         車檢器值應答命令字
    0008                                                //2Byte         命令長度
    00                                                  //1Byte         車檢器值狀態(0代表初值，1代表實時值)
    00                                                  //1Byte         車檢器個數
    01                                                  //1Byte         車檢器值狀態(0代表初值，1代表實時值)
    01                                                  //1Byte         車檢器個數
    0001                                                //2Byte         車檢器組ID
    01                                                  //1Bye          車檢器狀態
    00                                                  //1Byte
    */
    s2bVehicleReportAck vehicleReportAck;
    if (destDataLen < sizeof(vehicleReportAck))
    {
        return 0;
    }
    memset(&vehicleReportAck, 0x00, sizeof(vehicleReportAck));
    vehicleReportAck.cmdId = MR_S2B_ACK_VEHICLE_REPORT;          //1#    0   命令字 B1  0xad
    vehicleReportAck.cmdLen = 0x00; //temp value                 //      1   命令長度    B2  命令內容長度
    vehicleReportAck.dataStatus_ = b2sVehicleReportCmdItem.dataStatus_;    //2#    3   內容狀態字   B1  Bit7~bit1 預留
                            //                            Bit0=0：地磁內容為車檢器最新初值
    vehicleReportAck.vehicleReportNum_ = b2sVehicleReportCmdItem.vehicleReportNum_;//3#  4   車檢器應答值個數    B1  車檢器應答值個數
    //----
    vehicleReportAck.dataStatus = b2sVehicleReportCmdItem.dataStatus;     //4#    3   內容狀態字   B1  Bit7~bit1 預留
                            //                            Bit0=1：地磁內容為車檢器狀態變化值
    vehicleReportAck.vehicleReportNum = b2sVehicleReportCmdItem.vehicleReportNum; //5#  4   車檢器應答值個數    B1  車檢器應答值個數
    //vehicleReportAck.vehicleReportStart = 0;//   5   車檢器組ID   B2  車檢器組ID  vehicleReportAck
                            //       ... ... ...
                            //車位元狀態彙報校驗字  B1  命令內容累加（不包括命令長度命令字）

    int index = (uint8_t*)&(vehicleReportAck.vehicleReportStart) - (uint8_t*)(&vehicleReportAck);
    /*
    typedef struct
    {
        uint16_t vehicleId;         //5 車檢器組ID  B2  車檢器組ID
        uint8_t vehicleStatus; //7  車檢器狀態1  B1  Bit7：預留
                                    //                  Bit6~Bit4 車檢器參數命令
                                    //                      000：無任何操作
                                    //                      001 車檢器磁場初值鎖存成功
                                    //                  bit 3~Bit0 預留
    }vehicleReportAckData;
    */
    #if(0) //TEST DATA
    vehicleReportAck.vehicleReportNum = 1;
    #endif
    for (int i = 0; i < vehicleReportAck.vehicleReportNum; i++)
    {
        vehicleReportAckData* pVehicleReportAckData = (vehicleReportAckData*)(destData + index);
        pVehicleReportAckData->vehicleId = vehicleReportDataItem[i].vehicleId;// reverseU16(i + 1);
        pVehicleReportAckData->vehicleStatus = vehicleReportDataItem[i].vehicleStatus;// 0x00;

        index = index + sizeof(vehicleReportAckData);
    }
    destData[index] = 0x00; //CRC
    index++;

    vehicleReportAck.cmdLen = reverseU16(index - 3);
    memcpy(destData, &vehicleReportAck, sizeof(vehicleReportAck) - 1);

    destData[index - 1] = genAckCheckSum(destData, 4, index);

    dumpDataMsg("MRGetVehicleReportAck 0xAD", destData, index);

    return index;
}

int MRGetVehicleVerAck(uint8_t* destData, int destDataLen)
{
    /*
    //1.5.3.10  車檢器版本號彙報應答（0x5d）
    typedef struct
    {
        uint8_t cmdId;      //1#    0   命令字 B1  0x5d
        uint16_t cmdLen;    //      1   命令長度    B2  命令內容長度
        uint8_t vehicleNum; //    2#    3   車檢器版本號個數    B1  應答車檢器版本號個數
                            //3#    4   第一個車檢器組ID   B2  車檢器組ID
                            //...   ... ...
                            //第N個車檢器組ID B2  車檢器組ID
                            //...       ... ... ...
        uint8_t checksum;   //車位元狀態彙報校驗字    B1  命令內容累加（不包括命令長度命令字）
    }s2bVehicleVerAck; //    MR_S2B_ACK_VEHICLE_VER

    5D                                                  //1Byte         車檢器版本應答命令字
    0002                                                //2Byte         命令長度
    00                                                  //1Byte         車檢器個數
    00                                                  //1Byte
    */
    s2bVehicleVerAck vehicleVerAck;
    if (destDataLen < sizeof(vehicleVerAck))
    {
        return 0;
    }
    memset(&vehicleVerAck, 0x00, sizeof(vehicleVerAck));
    vehicleVerAck.cmdId = MR_S2B_ACK_VEHICLE_VER;      //1#    0   命令字 B1  0x5d
    vehicleVerAck.cmdLen = reverseU16(sizeof(vehicleVerAck)-3);    //      1   命令長度    B2  命令內容長度
    vehicleVerAck.vehicleNum = b2sVehicleVerCmdItem.vehicleNum; //    2#    3   車檢器版本號個數    B1  應答車檢器版本號個數
                        //3#    4   第一個車檢器組ID   B2  車檢器組ID
                        //...   ... ...
                        //第N個車檢器組ID B2  車檢器組ID
                        //...       ... ... ...
    int index = (uint8_t*)&(vehicleVerAck.vehicleVerStart) - (uint8_t*)(&vehicleVerAck);
    /*
    typedef struct
    {
        uint16_t vehicleId;         //5 車檢器組ID  B2  車檢器組ID

    }VehicleVerAckData;
    */
    for (int i = 0; i < vehicleVerAck.vehicleNum; i++)
    {
        VehicleVerAckData* pVehicleVerAckData = (VehicleVerAckData*)(destData + index);
        pVehicleVerAckData->vehicleId = vehicleVerDataItem[i].vehicleId;// reverseU16(i + 1);

        index = index + sizeof(VehicleVerAckData);
    }
    destData[index] = 0x00; //CRC
    index++;

    vehicleVerAck.cmdLen = reverseU16(index - 3);
    memcpy(destData, &vehicleVerAck, sizeof(vehicleVerAck) - 1);

    // Generate checksum
    destData[index - 1] = genAckCheckSum(destData, 3, index);

    /*vehicleVerAck.checksum = 0;   //車位元狀態彙報校驗字    B1  命令內容累加（不包括命令長度命令字）

    memcpy(destData, &vehicleVerAck, sizeof(vehicleVerAck));

    dumpDataMsg("MRGetVehicleVerAck 0x5D", destData, sizeof(vehicleVerAck));

    return sizeof(vehicleVerAck);*/

    dumpDataMsg("MRGetVehicleVerAck 0x5D", destData, index);

    return index;
}

int MRGetParaDownloadCmd(uint8_t* destData, int destDataLen)
{
    /*
    //1.5.3.15  參數下載（0x8d）
    typedef struct
    {
        uint8_t cmdId;              //1#  0 命令字      B1  0x8d
        uint16_t cmdLen;            //    1 命令長度    B2  命令內容長度
        uint16_t baseId;            //2#  2 基站ID      B2  基站ID
        uint8_t paraNum;            //    4 參數代碼個數  B1  代表後面需要下載的參數個數
        //uint8_t code;               //    5 參數代碼1   B1  見參數代碼表
        //uint8_t paraLen;            //    6 參數長度1   B1  參數內容長度
        uint8_t checksum;           //3#    校驗字      B1  命令內容累加（不包括命令長度命令字）
    }s2bParaDownloadCmd; //MR_S2B_CMD_PARA_DOWNLOAD

    8D                                                  //1Byte         參數下載命令字
    0004                                                //1Byte         命令長度
    000A                                                //2Byte         基站ID
    00                                                  //1Byte         參數個數
    00                                                  //1Byte
    */
    s2bParaDownloadCmd paraDownloadCmd;
    if (destDataLen < sizeof(paraDownloadCmd))
    {
        return 0;
    }
    memset(&paraDownloadCmd, 0x00, sizeof(paraDownloadCmd));
    paraDownloadCmd.cmdId = MR_S2B_CMD_PARA_DOWNLOAD;                //1#  0 命令字 B1  0x8d
    paraDownloadCmd.cmdLen = reverseU16(sizeof(s2bParaDownloadCmd)-3);                //    1 命令長度    B2  命令內容長度
    paraDownloadCmd.baseId = b2sParaDownloadAckItem.baseId;// reverseU16(baseId);// reverseU16(0x000A);;           //2#  2 基站ID    B2  基站ID
    paraDownloadCmd.paraNum = 0x00;         //    4 參數代碼個數  B1  代表後面需要下載的參數個數
    /*paraDownloadCmd.paraReinit.paraId = MR_S2B_CMD_PARA_DL_REINIT;
    paraDownloadCmd.paraReinit.paraLen = 9;
    paraDownloadCmd.paraReinit.vehicleNum = 1;
    paraDownloadCmd.paraReinit.vehicleId = 0x0100;
    paraDownloadCmd.paraReinit.vehicleGeomagValX = 0x0000;
    paraDownloadCmd.paraReinit.vehicleGeomagValY = 0x0000;
    paraDownloadCmd.paraReinit.vehicleGeomagValZ = 0x0000;*/

    //paraDownloadCmd.code = 0x00;                //    5 參數代碼1   B1  見參數代碼表    
    paraDownloadCmd.checksum = 0x00;            //3#    校驗字 B1  命令內容累加（不包括命令長度命令字）

    memcpy(destData, &paraDownloadCmd, sizeof(paraDownloadCmd));

    int ackDatalen = sizeof(s2bParaDownloadCmd);
    destData[ackDatalen - 1] = genAckCheckSum(destData, 3, ackDatalen);

    dumpDataMsg("MRGetParaDownloadCmd 0x8D", destData, sizeof(paraDownloadCmd));

    return sizeof(paraDownloadCmd);
}

int MRGetParaDLCmdReinit(uint8_t* destData, int destDataLen)
{
    /*
    //1.5.3.15  參數下載（0x8d）
    typedef struct
    {
        uint8_t cmdId;              //1#  0 命令字      B1  0x8d
        uint16_t cmdLen;            //    1 命令長度    B2  命令內容長度
        uint16_t baseId;            //2#  2 基站ID      B2  基站ID
        uint8_t paraNum;            //    4 參數代碼個數  B1  代表後面需要下載的參數個數
        //uint8_t code;               //    5 參數代碼1   B1  見參數代碼表
        //uint8_t paraLen;            //    6 參數長度1   B1  參數內容長度
        uint8_t checksum;           //3#    校驗字      B1  命令內容累加（不包括命令長度命令字）
    }s2bParaDownloadCmd; //MR_S2B_CMD_PARA_DOWNLOAD

    8D                                                  //1Byte         參數下載命令字
    0004                                                //1Byte         命令長度
    000A                                                //2Byte         基站ID
    00                                                  //1Byte         參數個數
    00                                                  //1Byte
    */
    s2bParaDLCmdReinit paraDownloadCmd;
    if (destDataLen < sizeof(paraDownloadCmd))
    {
        return 0;
    }
    memset(&paraDownloadCmd, 0x00, sizeof(paraDownloadCmd));
    paraDownloadCmd.cmdId = MR_S2B_CMD_PARA_DOWNLOAD;                //1#  0 命令字 B1  0x8d
    paraDownloadCmd.cmdLen = reverseU16(sizeof(s2bParaDLCmdReinit) - 3);                //    1 命令長度    B2  命令內容長度
    paraDownloadCmd.baseId = b2sParaDownloadAckItem.baseId;// reverseU16(baseId);// reverseU16(0x000A);;           //2#  2 基站ID    B2  基站ID
    paraDownloadCmd.paraNum = 0x01;         //    4 參數代碼個數  B1  代表後面需要下載的參數個數
    paraDownloadCmd.paraReinit.paraId = MR_S2B_CMD_PARA_DL_REINIT;
    paraDownloadCmd.paraReinit.paraLen = 9;
    paraDownloadCmd.paraReinit.vehicleNum = 1;
    paraDownloadCmd.paraReinit.vehicleId = 0x0100;
    paraDownloadCmd.paraReinit.vehicleGeomagValX = 0x0000;
    paraDownloadCmd.paraReinit.vehicleGeomagValY = 0x0000;
    paraDownloadCmd.paraReinit.vehicleGeomagValZ = 0x0000;

    //paraDownloadCmd.code = 0x00;                //    5 參數代碼1   B1  見參數代碼表    
    paraDownloadCmd.checksum = 0x00;            //3#    校驗字 B1  命令內容累加（不包括命令長度命令字）

    memcpy(destData, &paraDownloadCmd, sizeof(paraDownloadCmd));

    dumpDataMsg("MRGetParaDLCmdReinit 0x8D-0x23", destData, sizeof(paraDownloadCmd));

    return sizeof(paraDownloadCmd);
}

int MRSendBaseStatusAck(uint8_t* destData, int destDataLen)
{
    int totalCmdLen = 0;
    packageHeader header;
    uint8_t destContain[256];
    int destContainLen;

    destContainLen = MRGetBaseStatusAck(destContain, sizeof(destContain));

    if (destDataLen < (sizeof(header) + sizeof(s2bBaseStatusAck)))
    {
        return 0;
    }

    memset(&header, 0x00, sizeof(header));
    //header
    header.type = MR_ACK_HEADER_TYPE;
    memcpy(header.aesKey, aesKey, sizeof(header.aesKey)); //3 bytes
    header.ackId = MR_B2S_ACK_BASE_STATUS_WORD;// ackId;
    header.cmdLen = reverseU16(destContainLen);

    memcpy(destData, &header, sizeof(header));
    memcpy(destData + sizeof(header), destContain, destContainLen);

    totalCmdLen = sizeof(header) + destContainLen;
    dumpDataMsg("BaseStatusAck (raw)", destData, totalCmdLen);
    {
        uint8_t* destData2;
        int     destDataLen2;
        MRAESEncrypt(destData + sizeof(header), totalCmdLen - sizeof(header), &destData2, &destDataLen2);

        dumpDataMsg("BaseStatusAck (Encrypt)", destData2, destDataLen2);

        header.cmdLen = reverseU16(destDataLen2);
        memcpy(destData, &header, sizeof(header));
        memcpy(destData + sizeof(header), destData2, destDataLen2);
        totalCmdLen = sizeof(header) + destDataLen2;
    }
    dumpDataMsg("BaseStatusAck (Total)", destData, totalCmdLen);

    return totalCmdLen;
}

int MRSendAck(uint8_t* destData, int destDataLen, uint8_t* destContain, int destContainLen, char* title)
{
    int totalCmdLen = 0;
    packageHeader header;
    char titleStr[32];

    if (destDataLen < (sizeof(header) + destContainLen))
    {
        return 0;
    }

    memset(&header, 0x00, sizeof(header));
    //header
    header.type = MR_ACK_HEADER_TYPE;
    memcpy(header.aesKey, aesKey, sizeof(header.aesKey)); //3 bytes
    header.ackId = MR_B2S_ACK_BASE_STATUS_WORD;// ackId;
    header.cmdLen = reverseU16(destContainLen);

    memcpy(destData, &header, sizeof(header));
    memcpy(destData + sizeof(header), destContain, destContainLen);

    totalCmdLen = sizeof(header) + destContainLen;

    snprintf(titleStr, sizeof(titleStr), "%s (raw)", title);
    dumpDataMsg(titleStr, destData, totalCmdLen);

    {
        uint8_t* destData2;
        int     destDataLen2;
        MRAESEncrypt(destData + sizeof(header), totalCmdLen - sizeof(header), &destData2, &destDataLen2);

        #if(0)
        snprintf(titleStr, sizeof(titleStr), "%s (Encrypt)", title);
        dumpDataMsg(titleStr, destData2, destDataLen2);
        #endif

        header.cmdLen = reverseU16(destDataLen2);
        memcpy(destData, &header, sizeof(header));
        memcpy(destData + sizeof(header), destData2, destDataLen2);
        totalCmdLen = sizeof(header) + destDataLen2;
    }
    #if(0)
    snprintf(titleStr, sizeof(titleStr), "%s (Total)", title);
    dumpDataMsg(titleStr, destData, totalCmdLen);
    #endif

    return totalCmdLen;
}

/*** * Copyright (C) 2023 Far Easy Pass LTD. All rights reserved. ***/

