/**************************************************************************//**
* @file     mrlib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2023 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __MR_LIB_H__
#define __MR_LIB_H__

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
/*
1.5.3.1   基站註冊（0xd0）
1.5.3.2   基站註冊應答（0x0d）
1.5.3.15  參數下載（0x8d）
1.5.3.13  基站狀態彙報（0xd6）
1.5.3.7   車位元狀態彙報（0xd4）
1.5.3.5   車檢器值彙報（0xda）
1.5.3.9   車檢器版本號彙報(0xd5)
1.5.3.16  參數下載應答（0xd8）
1.5.3.14  基站狀態彙報應答（0x6d）
1.5.3.8   車位元狀態應答（0x4d）
1.5.3.6   車檢器值應答（0xad）
1.5.3.10  車檢器版本號彙報應答（0x5d）
*/
#define MR_HEADER_TYPE              0xAC
#define MR_ACK_HEADER_TYPE          0xA0

//* 1.5.2 *//
#define MR_B2S_CMD_REG              0xd0        //基站註冊          基站註冊                基站→管理系統
#define MR_B2S_CMD_TAG_STATUS       0xd3        //標籤狀態彙報        彙報車檢器狀態     基站→管理系統
#define MR_B2S_CMD_VEHICLE_STATUS   0xd4        //車位元狀態彙報       彙報標籤狀態          基站→管理系統
#define MR_B2S_CMD_VEHICLE_VER      0xd5        //車檢器版本號彙報  車檢器版本號彙報        基站→管理系統
#define MR_B2S_CMD_BASE_STATUS      0xd6        //基站狀態彙報        基站狀態彙報          基站→管理系統
#define MR_B2S_ACK_PARA_DOWNLOAD    0xd8        //參數下載應答              設備參數下載應答        基站→管理系統
#define MR_B2S_CMD_VEHICLE_REPORT   0xda        //車檢器值彙報        上報車檢器最新初值       基站→管理系統
#define MR_B2S_CMD_TAG_REG          0xdb        //標籤註冊彙報        彙報標籤資訊          基站→管理系統
            
#define MR_S2B_ACK_REG              0x0d        //基站註冊應答        基站註冊應答          管理系統→基站
#define MR_S2B_ACK_TAG_STATUS       0x3d        //標籤狀態彙報應答  標籤狀態彙報應答        管理系統→基站
#define MR_S2B_ACK_VEHICLE_STATUS   0x4d        //車位元狀態彙報應答     車位元狀態彙報應答       管理系統→基站
#define MR_S2B_ACK_VEHICLE_VER      0x5d        //車檢器版本號彙報應答 車檢器版本號彙報應答     管理系統→基站
#define MR_S2B_ACK_BASE_STATUS      0x6d        //基站狀態彙報  基站狀態彙報應答        管理系統→基站
#define MR_S2B_CMD_PARA_DOWNLOAD    0x8d        //參數下載          數據下載                管理系統→基站
#define MR_S2B_CMD_PARA_DL_REINIT   0x23        //參數下載          車檢器初值更新          管理系統→基站
#define MR_S2B_ACK_VEHICLE_REPORT   0xad        //車檢器初值彙報應答     告知車檢器初值接收到  管理系統→基站
#define MR_S2B_ACK_TAG_REG          0xbd        //標籤註冊彙報應答  標籤註冊彙報應答        管理系統→基站

#define MR_B2S_CMD_OTA              0xdf        //無線升級          無線升級                管理系統→基站
#define MR_B2S_CMD_RESVER           0xdd        //預留                預留                  管理系統→基站



//
#define MR_B2S_CMD_REG_WORD         0xF0
#define MR_S2B_ACK_WORD             0xF2
#define MR_B2S_CMD_BASE_STATUS_WORD 0xF1
#define MR_B2S_ACK_BASE_STATUS_WORD 0xF3

//// ***  Local BaseID = 0x0001 ***
//#define LOCAL_InitBordcastChannel     0x01
//#define LOCAL_WorkChannel             0x0B
//// *** Backup BaseID = 0x0005 ***
//#define BKP_BaseID                  0x0005
//#define BKP_InitBordcastChannel     0x05
//#define BKP_WorkChannel             0x0F

//// ***  Local BaseID = 0x0005 ***
//#define LOCAL_InitBordcastChannel     0x05
//#define LOCAL_WorkChannel             0x0F
//// ***  Backup BaseID = 0x0001 ***
//#define BKP_BaseID                  0x0001
//#define BKP_InitBordcastChannel     0x01
//#define BKP_WorkChannel             0x0B

extern int paraCmdGeomangReinitFlag;
extern int mrLocalBaseID;
extern uint8_t mrLocalBroadcastCH;
extern uint8_t mrLocalWorkCH;
extern int mrBkpBaseID;
extern uint8_t mrBkpBroadcastCH;
extern uint8_t mrBkpWorkCH;

#pragma pack(1)
//1.3.1 通信格式
typedef struct
{
    uint8_t     opCode;           //設備類型
    uint16_t    baseId;        //數據長度
}comboId;
typedef struct
{
    uint8_t     type;           //設備類型
    union  {
        uint8_t     aesKey[3];          //設備ID
        comboId     id;
    };    
    uint8_t     ackId;         //回話字
    uint16_t    cmdLen;        //數據長度
}packageHeader;

//1.5.3.1   基站註冊（0xd0）
typedef struct
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

//1.5.3.2   基站註冊應答（0x0d）
typedef struct
{   
    uint8_t     cmdId;                  //1#  0   命令字 B1  0x0d
    uint8_t     cmdLen;                 //    1   命令長度    B1  後續命令長度
    uint8_t     regSuccessMark;         //2#  2   基站註冊成功標識    B1  Bit1~Bit0=00註冊失敗，Bit1~Bit0=01註冊成功;
                                        //                               Bit2~Bit7預留
    uint16_t    baseId;                 //3#  3   基站ID    B2  高位元組在前（範圍1~65535）
    uint8_t     baseHeartbeatTime;      //    5   基站/伺服器脈搏時間  B1  Bit3~bit0 標識基站伺服器脈搏時間
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
    uint8_t     vechicleHeartbeatTime;  //    6   基站/車檢器脈搏時間  B1  Bit4~bit0 標識基站車檢器脈搏時間
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
    uint32_t     timeStamp;             //    7   時間戳記    B4  系統當前時鐘
    uint8_t     timeStandbyTime[4];     //    11  基站靜默時間  B4   位置6位元組表示起始小時，預設0（範圍0~23）
                                        //                      位置7位元組表示起始分鐘，預設0（範圍0~59）
                                        //                      位置8位元組表示停止小時，預設0（範圍0~23）
                                        //                      位置9位元組表示停止分鐘，預設0（範圍0~59）
                                        //                      起始停止時間為全0表示基站
                                        //                      不休眠。預設值全0x00000000
    uint8_t     initBordcastChannel;    //4#  15  基站/車檢器初始化廣播頻道   B1  LoRa子模組1和車檢器初始化頻道（參考頻率編排表格）
                                        //                      注：0表示未分配初始化頻道
    uint8_t     workChannel;            //    16  基站/車檢器工作頻道  B1  LoRa子模組1和車檢器工作頻道（參考頻率編排表格）
                                        //                      注：0表示未分配初始化頻道
    uint8_t     resverChannel[2];       //    17  預留  B2  預留
                                        //                      注：0表示未分配工作頻道
    uint16_t    baseManageNum;          //    19  基站管理車位個數    B2  高位元組在前，每單位標識管理一個車檢器（注：系統管理基站關聯車檢器個數）
                                        //                      例如：0x0100，代表管理256個車檢器
    uint8_t     baseReceiveAck;         //    20  基站接收應答擴頻因數  B1  Bit3~bit0有效，bit4~bit7預留
                                        //                              0x06對應擴頻因數6
                                        //                              0x07對應擴頻因數7
                                        //                              0x08對應擴頻因數8
                                        //                              0x09對應擴頻因數9
                                        //                              0x0a對應擴頻因數10
                                        //                              0x0b對應擴頻因數11
                                        //                              0x0c對應擴頻因數12
                                        //                              默認SF=7
    uint8_t     loraErrorCorrectionRate;//    21  LoRa子模組糾錯率  B1  Bit3~bit0有效，bit4~bit7預留
                                        //                                  0x01對應4/5
                                        //                                  0x02對應4/6
                                        //                                  0x03對應4/7
                                        //                                  0x04對應4/8
                                        //                                  默認0x01
    uint8_t     loraBandWidth;          //    22  LoRa通訊頻寬    B1  Bit3~bit0有效，bit4~bit7預留
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
    uint8_t     loraPower;              //    23  LoRa通訊功率    B1    BIT7=0關閉PA
                                        //                              Bit7=1開啟PA
                                        //                              BIT6=0開啟LNA
                                        //                              Bit6=1開啟LNA
                                        //                              BIT5~BIT0 對應發射功率
                                        //                              預設值128
    uint8_t     resverLora[2];          //    24  預留  B2  預留
    uint8_t     vechicleParaCmd;        //5#  26  車檢器參數命令 B1  0x03 車檢器參數命令字
    uint8_t     vechicleParaLen;        //    27  車檢器參數命令長度   B1  命令內容長度
    uint8_t     vechicleBoradecastPower;//    28  車檢器標籤廣播功率   B1  Bit3~bit0有效，bit4~bit7預留
                                        //                              0x00：0dbm，
                                        //                              0x01：-6dbm，
                                        //                              0x02：-12dbm，
                                        //                              0x03：-18dbm
                                        //                              車檢器標籤廣播功率(默認0x01)
    uint8_t     vechicleSampleTime;     //    29  地磁採集週期  B1  Bit3~bit0有效，bit4~bit7預留
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
    uint8_t     vechicleAlgorithmPara[40];//  30  車檢演算法參數 B40 見車檢演算法參數表格
    uint8_t     tagParaCmd;             //6#  70  標籤參數命令  B1  0x04
    uint8_t     tagParaCmdLen;          //    71  標籤參數命令長度    B1  根據命令內容長度
    uint8_t     tagParaStopRange;       //    72  標籤靜止幅度  B1  5~100預設值(0x28)
    uint8_t     tagParaStopTimes;       //    73  標籤靜止次數  B1  2~30預設值(0x08)
    uint8_t     tagParaStopTime;        //    74  標籤靜止時間  B1  1~160預設值(0x23)
    uint8_t     tagParaMoveRange;       //    75  標籤移動幅度  B1  5~100預設值(0x20)
    uint8_t     tagParaMoveTimes;       //    76  標籤移動次數  B1  2~30預設值(0x06)
    uint8_t     tagParaMoveTime;        //    77  標籤移動時間  B1  1~160預設值(0x0c)
    uint8_t     resverTag[6];           //    78  標籤磁場參數  B6  預留
    uint8_t     directedBoardcastTimes; //    84  定向廣播次數  B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x01，6次
                                        //                          0x02，12次
                                        //                          0x03，18次
                                        //                          0x04，24次
                                        //                          默認18次
    uint8_t     directedBoardcastTime;  //    85  定向廣播時間間隔    B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x00，關閉定向廣播
                                        //                          0x01，10秒
                                        //                          0x02，20秒
                                        //                          0x03，30秒
                                        //                          0x04，40秒
                                        //                          默認10秒
    uint8_t     taggleBoardcastTimes;   //    86  觸發廣播次數  B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x01，6次
                                        //                          0x02，12次
                                        //                          0x03，18次
                                        //                          0x04，24次
                                        //                          默認18次
    uint8_t     taggleBoardcastTime;    //    87  觸發廣播時間間隔    B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x00，關閉觸發廣播
                                        //                          0x01，10秒
                                        //                          0x02，20秒
                                        //                          0x03，30秒
                                        //                          0x04，40秒
                                        //                          默認10秒
    uint8_t     scheduledBroadcastTimes;//    88  定時廣播次數  B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x01，6次
                                        //                          0x02，12次
                                        //                          0x03，18次
                                        //                          0x04，24次
                                        //                          默認18次
    uint8_t     scheduledBroadcastTime; //    89  定時廣播時間間隔    B1  Bit3~bit0有效，bit4~bit7預留
                                        //                          0x00，關閉定時廣播
                                        //                          0x01，1分鐘
                                        //                          0x02，2分鐘
                                        //                          0x03，3分鐘
                                        //                          ...
                                        //                          0x0a，10分鐘
                                        //                          默認5分鐘
    uint8_t     tagRegTimes;            //    90  標籤首次註冊次數    B1  標籤註冊，和車檢器連續多少次通訊成功後上報
                                        //                          0x01，1次
                                        //                          0x02，2次
                                        //                          0x03，3次
                                        //                          0x04，4次
                                        //                          0x05，5次
                                        //                          默認1次
    uint16_t    bkpBaseId;              //    91	备份基站ID                  B2	高字节在前（范围1~65535）, 0没有备份基站
    uint8_t     bkpInitBordcastChannel; //    93	备份基站 / 车检器初始化广播频道	B1	LoRa子模块1和车检器初始化频道（参考频率编排表格）
                                        //          注：0表示未分配初始化频道
    uint8_t     bkpWorkChannel;         //    94	备份基站 / 车检器工作频道	   B1	LoRa子模块1和车检器工作频道（参考频率编排表格）
                                        //          注：0表示未分配初始化频道
    uint8_t     reserve[2];             //    95  預留  B2  預留
    uint8_t     checksum;               //7#  97  校驗字 B1  資料內容校驗和
}s2bRegCmdAck;

//2.2.3	車檢器初值更新
typedef struct
{
    uint8_t paraId;             //1#  0 參數代碼            B1  0x23
    uint8_t paraLen;            //    1 內容長度            B1  命令內容長度
    uint8_t vehicleNum;         //    2 參數代碼個數        B1  代表後面需要下載的參數個數
    uint16_t vehicleId;         //2#  3 第一個更新初值ID    B2  車檢器組ID
    uint16_t vehicleGeomagValX; //    5 第一個車檢器X軸值	   B2  地磁值(帶入0000)
    uint16_t vehicleGeomagValY; //    5 第一個車檢器Y軸值	   B2  地磁值(帶入0000)
    uint16_t vehicleGeomagValZ; //    5 第一個車檢器Z軸值	   B2  地磁值(帶入0000)
}s2bPDCReInit;      //MR_S2B_CMD_PARA_DOWNLOAD

//1.5.3.15  參數下載（0x8d） - "參數代碼個數 = 0"
typedef struct
{
    uint8_t cmdId;              //1#  0 命令字      B1  0x8d
    uint16_t cmdLen;            //    1 命令長度    B2  命令內容長度
    uint16_t baseId;            //2#  3 基站ID      B2  基站ID
    uint8_t paraNum;            //    5 參數代碼個數  B1  代表後面需要下載的參數個數
    //uint8_t code;               //    5 參數代碼1   B1  見參數代碼表
    //uint8_t paraLen;            //    6 參數長度1   B1  參數內容長度
    uint8_t checksum;           //4#    校驗字      B1  命令內容累加（不包括命令長度命令字）
}s2bParaDownloadCmd; //MR_S2B_CMD_PARA_DOWNLOAD

//1.5.3.15  參數下載 - 2.2.3	車檢器初值更新（0x8d - 0x23） "參數代碼個數 > 0"
typedef struct
{
    uint8_t cmdId;              //1#  0 命令字      B1  0x8d
    uint16_t cmdLen;            //    1 命令長度    B2  命令內容長度
    uint16_t baseId;            //2#  3 基站ID      B2  基站ID
    uint8_t paraNum;            //    5 參數代碼個數  B1  代表後面需要下載的參數個數
    s2bPDCReInit paraReinit;    //3#    6          B9
    uint8_t checksum;           //4#    校驗字      B1  命令內容累加（不包括命令長度命令字）
}s2bParaDLCmdReinit;             //MR_S2B_CMD_PARA_DOWNLOAD

//1.5.3.13  基站狀態彙報（0xd6）
typedef struct
{
    uint8_t cmdId;              //1#  0   命令字        B1  0xd6
    uint8_t cmdLen;             //    1   命令長度      B1  命令內容長度
    uint8_t baseStatus;         //2#  2   基站狀態字    B1  BIT3=1:基站電池為鋰電池
                                //                          BIT3=0:基站電池為鉛酸電池
                                //                          Bit1~bit0 基站電源電量（對應鋰電池）
                                //                              00： 電壓大於12.5V
                                //                              01：電壓大於12V
                                //                              10：電壓大於11.5V
                                //                              11：電壓大於11V
                                //                          Bit1~bit0 基站電源電量（對應鉛酸電池）
                                //                              00： 電壓大於13V
                                //                              01：電壓大於12.5V
                                //                              10：電壓大於12V
                                //                              11：電壓大於11.5V
                                //                          Bit7~Bit4 太陽能電池狀態：
                                //                          BIT2: 預留

    uint8_t FourGRSSIValue;     //    3   4g模組RSSI值   B1 RSSI值

    uint8_t Sub1ghzRSSIValue;   //    4   Sub1ghz模組RSSI值     B1  RSSI值
    uint8_t baseDataType;       //3#  5   基站資料上傳類型      B1  Bit7~bit2預留
                                //                                  Bit1~bit0
                                //                                      00：4G模組方上傳
                                //                                      01：有線網口上傳
                                //                                      10：WIFI上傳
                                //                                      11： 其它方式上傳
    uint8_t reserve[4];         //4#  6   預留          B4  預留
    uint8_t checksum;           //5#  10  校驗字 B1  命令內容累加（不包括命令長度命令字）
}b2sBaseStatusCmd; //MR_B2S_CMD_BASE_STATUS


//1.5.3.7   車位元狀態彙報（0xd4）
typedef struct
{
    uint8_t cmdId;              //1#  0 命令字      B1  0xd4
    uint16_t cmdLen;            //    1 命令長度    B2  命令內容長度
    uint8_t reserve;            //2#  3 預留        B1  預留
    uint8_t baseNum;            //      基站個數    B1  基站個數包括基站本身和臨近基站ID
    //uint16_t baseId;            //      第N個基站ID B2  自身基站ID
    //uint8_t vehicleNum;         //3#  4 彙報車檢器狀態個數   B1  彙報車檢器狀態個數
    uint8_t vehicleDataStart;   //    5 第1個車檢器狀態內容  B7
                                //   ...
                                //車位元狀態彙報校驗字  B1  命令內容累加（不包括命令長度命令字）
}b2sVehicleStatusCmd; //MR_B2S_CMD_VEHICLE_STATUS

typedef struct
{
    uint16_t baseId;         //    6 备份机基站ID	    B2	备份基站ID
    uint8_t vehicleNum;      //    7 汇报备份车检器状态个数	B1	汇报备份车检器状态个数(无备份车检器状态时填为0), 备份车检器个数用M表示
}vehicleStatusBaseData; //MR_B2S_CMD_VEHICLE_STATUS

typedef struct
{
    uint16_t vehicleId;     //2Byte         車檢器組ID
    uint8_t  vehicleStatus; //1Bye          車檢器狀態  
                            //    5   第1個車檢器狀態內容  B7  位置8 ,9車位組ID
                            //                                 位置10
                            //                              Bit1~Bit0 有車/無車狀態
                            //                                  00 無車
                            //                                  01 有車
                            //                                  10分割
                            //                                  11 分割
                            //                                  之所以高2個分割是為了區分2次不同的分割
                            //                              Bit3~Bit2 地磁狀態
                            //                                  00 正常
                            //                                  01 地磁值溢出
                            //                                  10磁場不穩定
                            //                                  11 預留
                            //                              Bit4~bit5 車檢器電源電量
                            //                                  00： 電壓大於3.6
                            //                                  01：電壓大於3.3
                            //                                  10：電壓大於3.0
                            //                                  11：電壓大於2.7
                            //                              Bit7~Bit6：預留
    uint32_t timeStamp;     //4Bye          車檢器狀態時間戳
}vehicleStatusData;

//1.5.3.5   車檢器值彙報（0xda）
typedef struct
{
    uint8_t cmdId;          //1#  0   命令字 B1  0xda
    uint16_t cmdLen;        //    1   命令長度    B2  命令內容長度,高位元組在前
    uint8_t dataStatus_;     //2#  3   內容狀態字   B1   Bit7~bit1 預留
                            //                          Bit0=0：地磁內容為車檢器最新初值
    uint8_t vehicleReportNum_;//3#  4  車檢器彙報值個數    B1  車檢器彙報值個數
                            //                              （無車檢器值上報個數為0）
    uint8_t dataStatus;     //4#      內容狀態字   B1   Bit7~bit1 預留
                            //                          Bit0=0：地磁內容為車檢器最新初值
    uint8_t vehicleReportNum;//       車檢器彙報值個數    B1  車檢器彙報值個數
                            //                              （無車檢器值上報個數為0）
    uint8_t vehicleReportStart;// 車檢器狀態               第一個車檢器組ID   B2  車檢器組ID
                            //... ... ...
                            //車位元狀態彙報校驗字  B1  命令內容累加（不包括命令長度命令字）
}b2sVehicleReportCmd; //MR_B2S_CMD_VEHICLE_REPORT

typedef struct
{
    uint16_t vehicleId;     //2Byte         車檢器組ID
    uint8_t  vehicleStatus; //1Bye          車檢器狀態  Bit7~Bit2 預留
                            //                  Bit1~Bit0 有車 / 無車狀態
                            //                      00 無車
                            //                      01 有車
                            //                      10分割（後臺前一狀態是無車分割後變成有車，後臺前一狀態是有車更新來車時間）
                            //                      11 分割
                            //                      之所以高2個分割是為了區分2次不同的分割                            
    uint16_t xValue;        //6Bye          車檢器X,Y,Z軸磁場值(每2字節代表一個軸)
    uint16_t yValue;
    uint16_t zValue;
}vehicleReportData;

//1.5.3.9   車檢器版本號彙報(0xd5)
typedef struct
{
    uint8_t cmdId;      //0 命令字     B1  0xd5
    uint16_t cmdLen;    //1 命令長度        B2  後續數據長度
    uint8_t vehicleNum; //  車檢器個數   B1  上報版本號的車檢器個數 
    uint8_t vehicleVerStart;
                        //... ... ...
                        //校驗字   B1  累加和
}b2sVehicleVerCmd; //MR_B2S_CMD_VEHICLE_VER

typedef struct
{
    uint16_t vehicleId;    //3     第一個車檢器組ID    B2  車位號
    uint8_t  verStr[12];    //4     第一個分咪表的版本   B12 形如：HV3.0 - SV1.41
                            //                      表示硬體3.0版本；軟體1.41版本
}vehicleVerData;

//1.5.3.16  參數下載應答（0xd8）
typedef struct
{
    uint8_t cmdId;      //1#  0   命令字 B1  0xd8
    uint8_t cmdLen;     //    1   命令長度    B1  後續命令長度
    uint16_t baseId;  //2#  2   設備ID    B2  基站ID
    uint8_t paraNum;    //3#  3   參數代碼個數  B1  參數代碼個數
    uint8_t paraDataStart; //paraDownloadData
                        //4#  6   校驗字 B1  命令內容累加（不包括命令長度命令字）
}b2sParaDownloadAck; //MR_B2S_ACK_PARA_DOWNLOAD

typedef struct
{
    uint8_t code;       //    4   參數代碼1   B1  見參數代碼表
    uint8_t successFlag;//    5   成功標誌1   B1  Bit7~bit1 預留
                        //                          Bit0：
                        //                          Bit0=0 默認失敗
                        //                          Bit0=1 下載成功
                        //                          ... ... ...
}paraDownloadData;

//1.5.3.14  基站狀態彙報應答（0x6d）
typedef struct
{
    uint8_t cmdId;      //1#    0   命令字     B1  0x6d
    uint8_t cmdLen;     //      1   命令長度    B1  命令內容長度
    uint32_t timeStamp; //2#    2   時間戳記    B4  年月日時分秒
    uint8_t reserve[2]; //3#    6   基站狀態字   B2  預留
    uint8_t checksum;   //4#    8   校驗字 B1  命令內容累加（不包括命令長度命令字）
}s2bBaseStatusAck; //MR_S2B_ACK_BASE_STATUS

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

typedef struct
{
    uint16_t baseId;            //        第N個基站ID B2  自身基站ID
    uint16_t vehicleNum;        //2#  4   安裝車檢器個數 B2  系統記錄基站啟動車檢器個數
    uint8_t vehicleAckNum;      //    5   車檢器狀態應答個數   B1  代表有多少條車檢器應答資訊（例如：本次有5條車檢器應答資訊，車檢器個數欄位對應5）
}vehicleStatusBaseAckData; //MR_B2S_CMD_VEHICLE_STATUS

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

typedef struct
{
    uint16_t vehicleId;         //5 車檢器組ID  B2  車檢器組ID
    uint8_t vehicleStatus; //7  車檢器狀態1  B1  Bit7：預留
                                //                  Bit6~Bit4 車檢器參數命令
                                //                      000：無任何操作
                                //                      001 車檢器磁場初值鎖存成功
                                //                  bit 3~Bit0 預留
}vehicleReportAckData;

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
    uint8_t vehicleVerStart;//   5   車檢器組ID   B2  車檢器組ID  vehicleReportAck
}s2bVehicleVerAck; //    MR_S2B_ACK_VEHICLE_VER

typedef struct
{
    uint16_t vehicleId;         //5 車檢器組ID  B2  車檢器組ID

}VehicleVerAckData;

#pragma pack()
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
int MRTimerCallback(void);
int MRProcessData(uint8_t* pData, int dataLen);
int MRPrintCmd(uint8_t* pData, int dataLen);
int MRAESEncrypt(uint8_t* pData, int dataLen, uint8_t** resultData, int* resultDataLen);
int MRAESDecrypt(uint8_t* pData, int dataLen, uint8_t** resultData, int* resultDataLen);
//ACK command
int MRSendRegCmdAck(uint8_t* destData, int destDataLen);
int MRSendBaseStatusAck(uint8_t* destData, int destDataLen);
int MRSendAck(uint8_t* destData, int destDataLen, uint8_t* destContain, int destContainLen, char* title);
//prepare ACK command
int MRGetBaseStatusAck(uint8_t* destData, int destDataLen);
int MRGetVehicleStatusAck(uint8_t* destData, int destDataLen);
int MRGetVehicleReportAck(uint8_t* destData, int destDataLen);
int MRGetVehicleVerAck(uint8_t* destData, int destDataLen);
int MRGetParaDownloadCmd(uint8_t* destData, int destDataLen);
int MRGetParaDLCmdReinit(uint8_t* destData, int destDataLen);

#ifdef __cplusplus
}
#endif

#endif //__MR_LIB_H__
