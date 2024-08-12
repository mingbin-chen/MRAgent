
// MRAgentDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "MRAgent.h"
#include "MRAgentDlg.h"
#include "afxdialogex.h"

#include "mrlib.h"
#include "cmdlib.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMRAgentDlg dialog
static HANDLE hComm = NULL;
static CMRAgentDlg* pCMRAgentDlg;
static uint8_t buffer[1024 * 1024];
static CString msgStr;

time_t getUTCTime(void)
{
    return time(NULL);
}

void dumpDataMsg(char* title, uint8_t* data, int dataLen)
{
    CString str, strTmp, strTitle(title);

    str = "";
    if (dataLen != 0)
    {
        strTmp.Format(_T(" ============  [%s](len = %d) start ============\r\n    "), strTitle.GetBuffer(), dataLen);
        str = str + strTmp;
        for (int i = 0; i < dataLen; i++)
        {
            strTmp.Format(_T("%02X "), data[i]);
            str = str + strTmp;
            if (((i % 16) == 15) && (i != (dataLen - 1)))
            {
                str = str + _T("\r\n    ");
            }
        }
        strTmp.Format(_T("\r\n ============  [%s](len = %d) end ============\r\n"), strTitle.GetBuffer(), dataLen);
        str = str + strTmp;
    }
    pCMRAgentDlg->UpdateMsgData(str.GetBuffer());
}
void UpdateMsgData(char* msg)
{
    CString str(msg);
    pCMRAgentDlg->UpdateMsgData(str.GetBuffer());
}
void UpdateMsgData2(char* msg)
{
    CString str(msg);
    pCMRAgentDlg->UpdateMsgData2(str.GetBuffer());
}

static int32_t uartRead(uint8_t* buf, const int32_t len)
{
    if (pCMRAgentDlg->btnCloseCtrl.IsWindowEnabled())
    {
        return pCMRAgentDlg->ReceiveData((char*)buf, len);
    }
    else
    {
        return 0;
    }
    //return pCMRAgentDlg->ReceiveData((char*)buf, len);
}

static int32_t uartWrite(const uint8_t* buf, const int32_t len)
{
    return pCMRAgentDlg->SendData((char*)buf, len);
}

int32_t MRUartWrite(const uint8_t* buf, const int32_t len)
{
    return uartWrite(buf, len);
}

UINT MyThreadProc(LPVOID pParam)
{
    while (1)
    {
        int readLen = uartRead(buffer, sizeof(buffer));
        if (readLen > 0)
        {
            MRProcessData(buffer, readLen);
            //CMDProcessData(buffer, readLen);
        }
    }

    return 0;   // thread completed successfully
}


CMRAgentDlg::CMRAgentDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_MRAGENT_DIALOG, pParent)
    , msgEditStr(_T(""))
    , msgEditStr2(_T(""))
    , CheckBoxGeomangEn(FALSE)
    , m_localBroadcastCH(0)
    , m_localBaseID(0)
    , m_localWorkCH(0)
    , m_bkpBroadcastCH(0)
    , m_bkpBaseID(0)
    , m_bkpWorkCH(0)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMRAgentDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_COMLIST, comListCtrl);
    DDX_Control(pDX, IDC_COMBO_BAUDRATE, comBaudrateCtrl);
    DDX_Control(pDX, IDC_BUTTON_OPEN, btnOpenCtrl);
    DDX_Control(pDX, IDC_BUTTON_CLOSE, btnCloseCtrl);
    DDX_Text(pDX, IDC_EDIT_MSG, msgEditStr);
    DDX_Control(pDX, IDC_EDIT_MSG, msgEditCtrl);
    DDX_Text(pDX, IDC_EDIT_MSG2, msgEditStr2);
    DDX_Control(pDX, IDC_EDIT_MSG2, msgEditCtrl2);
    DDX_Control(pDX, IDC_GeomangReinitEn, CheckBoxGeomangReinit);
    DDX_Check(pDX, IDC_GeomangReinitEn, CheckBoxGeomangEn);
    DDX_Text(pDX, IDC_EDIT_LocalBroadcastCH, m_localBroadcastCH);
    DDX_Text(pDX, IDC_EDIT_LocalBaseID, m_localBaseID);
    DDX_Text(pDX, IDC_EDIT_LocalWorkCH, m_localWorkCH);
    DDX_Text(pDX, IDC_EDIT_BkpBroadcastCH, m_bkpBroadcastCH);
    DDX_Text(pDX, IDC_EDIT_BkpBaseID, m_bkpBaseID);
    DDX_Text(pDX, IDC_EDIT_BkpWorkCH, m_bkpWorkCH);
}

BEGIN_MESSAGE_MAP(CMRAgentDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_OPEN, &CMRAgentDlg::OnBnClickedButtonOpen)
    ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CMRAgentDlg::OnBnClickedButtonClose)
    ON_BN_CLICKED(IDC_BUTTON_CLEAN, &CMRAgentDlg::OnBnClickedButtonClean)
    ON_BN_CLICKED(IDC_BUTTON_AES_ENCRYPT, &CMRAgentDlg::OnBnClickedButtonAesEncrypt)
    ON_BN_CLICKED(IDC_BUTTON_AES_DECRYPT, &CMRAgentDlg::OnBnClickedButtonAesDecrypt)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_BUTTON_REG_ACK, &CMRAgentDlg::OnBnClickedButtonRegAck)
    ON_BN_CLICKED(IDC_BUTTON_PRINT_CMD_REPORT, &CMRAgentDlg::OnBnClickedButtonPrintCmdReport)
    ON_BN_CLICKED(IDC_BUTTON_CMD_PROCESS, &CMRAgentDlg::OnBnClickedButtonCmdProcess)
    ON_BN_CLICKED(IDC_BUTTON_CMD_PREPARE_ACK_DATA, &CMRAgentDlg::OnBnClickedButtonCmdPrepareAckData)
    ON_BN_CLICKED(IDC_BUTTON_BASE_STATUS_ACK, &CMRAgentDlg::OnBnClickedButtonBaseStatusAck)
    ON_BN_CLICKED(IDC_BUTTON_VEHICLE_STATUS_ACK, &CMRAgentDlg::OnBnClickedButtonVehicleStatusAck)
    ON_BN_CLICKED(IDC_BUTTON_VEHICLE_REPORT_ACK, &CMRAgentDlg::OnBnClickedButtonVehicleReportAck)
    ON_BN_CLICKED(IDC_BUTTON_VEHICLE_VER_ACK, &CMRAgentDlg::OnBnClickedButtonVehicleVerAck)
    ON_BN_CLICKED(IDC_BUTTON_PARA_DOWNLOAD_CMD, &CMRAgentDlg::OnBnClickedButtonParaDownloadCmd)
    ON_BN_CLICKED(IDC_BUTTON_GeomagReInit, &CMRAgentDlg::OnBnClickedButtonGeomagreinit)
    ON_BN_CLICKED(IDC_GeomangReinitEn, &CMRAgentDlg::OnBnClickedGeomangreiniten)
    ON_BN_CLICKED(IDC_BUTTON_UpdateBaseData, &CMRAgentDlg::OnBnClickedButtonUpdatebasedata)
END_MESSAGE_MAP()


// CMRAgentDlg message handlers

BOOL CMRAgentDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    // TODO: Add extra initialization here
    pCMRAgentDlg = this;
    comBaudrateCtrl.SetCurSel(0);
    btnOpenCtrl.EnableWindow(TRUE);
    btnCloseCtrl.EnableWindow(FALSE);
    //btnStartCtrl.EnableWindow(FALSE);
    updateComList();
    SetTimer(0, 50, NULL);
    AfxBeginThread(MyThreadProc, NULL);

    //msgStr.Format(L"sizeof(s2bRegCmdAck) = %d, time = %d\r\n", sizeof(s2bRegCmdAck), time(NULL));
    //UpdateMsgData(msgStr.GetBuffer());
    
    // Setup Local Base Parameter
    CString BaseInitValue = _T("0000");
    CEdit* pEditCtrl = (CEdit*)this->GetDlgItem(IDC_EDIT_LocalBaseID);
    pEditCtrl->SetLimitText(4);
    pEditCtrl->SetWindowText(BaseInitValue);

    BaseInitValue = _T("1");
    pEditCtrl = (CEdit*)this->GetDlgItem(IDC_EDIT_LocalBroadcastCH);
    pEditCtrl->SetLimitText(2);
    pEditCtrl->SetWindowText(BaseInitValue);

    BaseInitValue = _T("11");
    pEditCtrl = (CEdit*)this->GetDlgItem(IDC_EDIT_LocalWorkCH);
    pEditCtrl->SetLimitText(2);
    pEditCtrl->SetWindowText(BaseInitValue);

    // Setup Backup Base Parameter
    BaseInitValue = _T("0005");
    pEditCtrl = (CEdit*)this->GetDlgItem(IDC_EDIT_BkpBaseID);
    pEditCtrl->SetLimitText(4);
    pEditCtrl->SetWindowText(BaseInitValue);

    BaseInitValue = _T("5");
    pEditCtrl = (CEdit*)this->GetDlgItem(IDC_EDIT_BkpBroadcastCH);
    pEditCtrl->SetLimitText(2);
    pEditCtrl->SetWindowText(BaseInitValue);

    BaseInitValue = _T("15");
    pEditCtrl = (CEdit*)this->GetDlgItem(IDC_EDIT_BkpWorkCH);
    pEditCtrl->SetLimitText(2);
    pEditCtrl->SetWindowText(BaseInitValue);

    UpdateData(true);

    mrLocalBroadcastCH = m_localBroadcastCH;
    mrLocalWorkCH = m_localWorkCH;
    mrBkpBaseID = m_bkpBaseID;
    mrBkpBroadcastCH = m_bkpBroadcastCH;
    mrBkpWorkCH = m_bkpWorkCH;

    msgStr.Format(L"### Default Parameter ... Base1 - ID:0001 BCCh:01 WorkCH:11 ; Base2 - ID:0005 BCCh:05 WorkCH:15\r\n");
    UpdateMsgData(msgStr.GetBuffer());

    msgStr.Format(L"### Base Data Update - Local ### BaseID = %d, BroadcastChannel = %d, WorkChannel = %d \r\n", m_localBaseID, m_localBroadcastCH, m_localWorkCH);
    UpdateMsgData(msgStr.GetBuffer());
    msgStr.Format(L"### Base Data Update - BKP   ### BaseID = %d, BroadcastChannel = %d, WorkChannel = %d \r\n", m_bkpBaseID, m_bkpBroadcastCH, m_bkpWorkCH);
    UpdateMsgData(msgStr.GetBuffer());

    UpdateData(false);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMRAgentDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMRAgentDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}



void CMRAgentDlg::updateComList()
{
    // TODO: Add your implementation code here.
    DWORD nValues, nMaxValueNameLen, nMaxValueLen;
    HKEY hKey = NULL;
    LPTSTR szDeviceName = NULL;
    LPTSTR szFriendlyName = NULL;
    DWORD dwType = 0;
    DWORD nValueNameLen = 0;
    DWORD nValueLen = 0;
    DWORD dwIndex = 0;

    BOOL getComportFlag = FALSE;

    //::SetWindowText(::GetDlgItem(dlg->m_hWnd, IDC_STATIC_TITLE), _T("Finding VT300A...\r\n"));

    LSTATUS lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS != lResult)
    {
        TRACE("Failed to open key \'HARDWARE\\DEVICEMAP\\SERIALCOMM\' \n");
        return;
    }

    lResult = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                              &nValues, &nMaxValueNameLen, &nMaxValueLen, NULL, NULL);

    if (ERROR_SUCCESS != lResult)
    {
        TRACE(_T("Failed to RegQueryInfoKey()\n"));
        RegCloseKey(hKey);
        return;
    }

    szDeviceName = (LPTSTR)malloc(nMaxValueNameLen * sizeof(TCHAR) + sizeof(TCHAR));
    if (!szDeviceName)
    {
        TRACE(_T("malloc() fail\n"));
        RegCloseKey(hKey);
        return;
    }

    szFriendlyName = (LPTSTR)malloc(nMaxValueLen * sizeof(TCHAR) + sizeof(TCHAR));
    if (!szFriendlyName)
    {
        free(szDeviceName);
        TRACE(_T("malloc() fail\n"));
        RegCloseKey(hKey);
        return;
    }

    comListCtrl.Clear();
    TRACE(_T("Found %d serial device(s) registered with PnP and active or available at the moment.\n"), nValues);
    for (DWORD dwIndex = 0; dwIndex < nValues; ++dwIndex)
    {
        dwType = 0;
        nValueNameLen = nMaxValueNameLen + sizeof(TCHAR);
        nValueLen = nMaxValueLen + sizeof(TCHAR);

        lResult = RegEnumValueW(hKey, dwIndex,
                                szDeviceName, &nValueNameLen,
                                NULL, &dwType,
                                (LPBYTE)szFriendlyName, &nValueLen);

        if (ERROR_SUCCESS != lResult || REG_SZ != dwType)
        {
            TRACE(_T("SerialPortEnumerator::Init() : can't process registry value, index: %d\n"), dwIndex);
            continue;
        }
        TRACE(_T("Found port \'%s\': Device name for CreateFile(): \'\\.%s\'\n"), szFriendlyName, szDeviceName);
        comListCtrl.AddString(szFriendlyName);
    }

    if (comListCtrl.GetCount() > 0)
        comListCtrl.SetCurSel(0);

    if (szDeviceName != NULL)
        free(szDeviceName);

    if (szFriendlyName != NULL)
        free(szFriendlyName);

    RegCloseKey(hKey);
}


void CMRAgentDlg::OnBnClickedButtonOpen()
{
    // TODO: Add your control notification handler code here
    CString comstr;
    CString baudratestr;
    comListCtrl.GetLBText(comListCtrl.GetCurSel(), comstr); 
    comBaudrateCtrl.GetLBText(comBaudrateCtrl.GetCurSel(), baudratestr);
    TRACE(_T("OnBnClickedOpen %d \'%s\' \r\n"), comListCtrl.GetCurSel(), comstr.GetBuffer());
    if (openComPort(comstr.GetBuffer(), _ttoi(baudratestr)) == TRUE)
    {
        btnOpenCtrl.EnableWindow(FALSE);
        btnCloseCtrl.EnableWindow(TRUE);
        UpdateMsgData(_T("OPEN SUCCESS !!!\r\n"));
    }
    else
    {
        hComm = INVALID_HANDLE_VALUE;
        btnOpenCtrl.EnableWindow(TRUE);
        btnCloseCtrl.EnableWindow(FALSE);
        UpdateMsgData(_T("OPEN ERR !!!\r\n"));
    }
}


void CMRAgentDlg::OnBnClickedButtonClose()
{
    // TODO: Add your control notification handler code here
    if (hComm != INVALID_HANDLE_VALUE)
    {
        if (CloseHandle(hComm) == FALSE)
        {
            btnOpenCtrl.EnableWindow(FALSE);
            btnCloseCtrl.EnableWindow(TRUE);

            UpdateMsgData(_T("CLOSE ERR !!!\r\n"));
        }
        else
        {
            hComm = INVALID_HANDLE_VALUE;
            btnOpenCtrl.EnableWindow(TRUE);
            btnCloseCtrl.EnableWindow(FALSE);

            UpdateMsgData(_T("CLOSE SUCCESS !!!\r\n"));
        }
    }
}


int CMRAgentDlg::openComPort(TCHAR* comString, int baudrate)
{
    // TODO: Add your implementation code here.
    TCHAR portString[64];
    DCB dcb;
    BOOL fSuccess;
    BOOL reVal = FALSE;
    swprintf_s(portString, _T("\\\\.\\%s"), comString);

    hComm = CreateFile(portString,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       0,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL/* | FILE_FLAG_OVERLAPPED */,
                       0);
    if (hComm == INVALID_HANDLE_VALUE)
    {
        // error opening port; abort
        TRACE(_T("TryOpenComPort [%s] CreateFile Fail\r\n"), comString);
        return FALSE;
    }
    //  Initialize the DCB structure.
    SecureZeroMemory(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);

    //  Build on the current configuration by first retrieving all current
    //  settings.
    fSuccess = GetCommState(hComm, &dcb);

    if (!fSuccess)
    {
        //  Handle the error.
        TRACE("GetCommState failed with error %d.\n", GetLastError());
        CloseHandle(hComm);
        return FALSE;
    }

    //PrintCommState(dcb);       //  Output to console

    //  Fill in some DCB values and set the com state: 
    //  57,600 bps, 8 data bits, no parity, and 1 stop bit.
    dcb.BaudRate = baudrate;     //  baud rate
    dcb.ByteSize = 8;             //  data size, xmit and rcv
    dcb.Parity = NOPARITY;      //  parity bit
    dcb.StopBits = ONESTOPBIT;    //  stop bit
    dcb.fBinary = true;
    dcb.fDtrControl = true;

    fSuccess = SetCommState(hComm, &dcb);

    if (!fSuccess)
    {
        //  Handle the error.
        TRACE("SetCommState failed with error %d.\n", GetLastError());
        CloseHandle(hComm);
        return FALSE;
    }

    //  Get the comm config again.
    fSuccess = GetCommState(hComm, &dcb);

    if (!fSuccess)
    {
        //  Handle the error.
        TRACE("GetCommState failed with error %d.\n", GetLastError());
        CloseHandle(hComm);
        return FALSE;
    }

    //PrintCommState(dcb);       //  Output to console

    TRACE(TEXT("Serial port %s successfully reconfigured.\n"), comString);

    COMMTIMEOUTS CommTimeouts;
    GetCommTimeouts(hComm, &CommTimeouts);

    // Change the COMMTIMEOUTS structure settings.
    #if(0)
    CommTimeouts.ReadIntervalTimeout = 0;
    CommTimeouts.ReadTotalTimeoutMultiplier = 0;
    CommTimeouts.ReadTotalTimeoutConstant = 1;
    CommTimeouts.WriteTotalTimeoutMultiplier = 0;
    CommTimeouts.WriteTotalTimeoutConstant = 0;
    #else
    CommTimeouts.ReadIntervalTimeout = MAXDWORD;
    CommTimeouts.ReadTotalTimeoutMultiplier = 0;
    CommTimeouts.ReadTotalTimeoutConstant = 0;
    CommTimeouts.WriteTotalTimeoutMultiplier = 2000;
    CommTimeouts.WriteTotalTimeoutConstant = 5000;
    #endif

    // Set the timeout parameters for all read and write operations
    // on the port.
    if (SetCommTimeouts(hComm, &CommTimeouts) == 0)
    {
        printf("SetCommTimeOuts ERROR\n");
        CloseHandle(hComm);
        return FALSE;
    }


    SetupComm(hComm, 1024, 1024);
    PurgeComm(hComm, PURGE_TXCLEAR | PURGE_RXCLEAR);


    EscapeCommFunction(hComm, SETDTR);
    EscapeCommFunction(hComm, SETRTS);

    SetCommMask(hComm, EV_RXCHAR);

    if (SetCommMask(hComm, EV_TXEMPTY) == 0)
    {
        printf("SetCommMask ERROR\n");
        CloseHandle(hComm);
        return FALSE;
    }
    return TRUE;
}


int CMRAgentDlg::UpdateMsgData(TCHAR* msg)
{
    pCMRAgentDlg->msgEditStr = pCMRAgentDlg->msgEditStr + msg;// +_T("\r\n");
    ::SetWindowText(::GetDlgItem(pCMRAgentDlg->m_hWnd, IDC_EDIT_MSG), pCMRAgentDlg->msgEditStr);

    pCMRAgentDlg->msgEditCtrl.LineScroll(pCMRAgentDlg->msgEditCtrl.GetLineCount(), 0);
    pCMRAgentDlg->msgEditCtrl.UpdateWindow();
    return 0;

}


int CMRAgentDlg::ReceiveData(char* buff, int buffLen)
{
    // TODO: Add your implementation code here.
    if (hComm != INVALID_HANDLE_VALUE)
    {
        DWORD dwCommEvent = 0;
        DWORD dwRead = 0;

        //WaitCommEvent(hComm, &dwCommEvent, NULL);
        //if(GetCommMask(hComm, EV_RXCHAR) != 0)
        {
            if (ReadFile(hComm, buff, buffLen, &dwRead, NULL))
            {
                #if(0)
                if (dwRead > 0)
                    TRACE(_T(" - Receive %d data [%c]\r\n"), dwRead, buff);
                #endif
                return dwRead;
            }
        }
    }
    return 0;
}


int CMRAgentDlg::SendData(char* data, int dataLen)
{
    // TODO: Add your implementation code here.
    if (hComm != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten;
        //PurgeComm(hComm, PURGE_TXCLEAR | PURGE_RXCLEAR);
        if (WriteFile(hComm, data, dataLen, &dwWritten, NULL))
        {
            if (dwWritten > 0)
            {
                //TRACE(_T(" - Send %d data \r\n"), dwWritten);
                //TRACE(_T("."));
            }
            return dwWritten;
        }
    }
    return 0;
}


void CMRAgentDlg::OnBnClickedButtonClean()
{
    pCMRAgentDlg->msgEditStr = "";
    ::SetWindowText(::GetDlgItem(pCMRAgentDlg->m_hWnd, IDC_EDIT_MSG), pCMRAgentDlg->msgEditStr);
    pCMRAgentDlg->msgEditStr2 = "";
    ::SetWindowText(::GetDlgItem(pCMRAgentDlg->m_hWnd, IDC_EDIT_MSG2), pCMRAgentDlg->msgEditStr2);
}

void CMRAgentDlg::OnBnClickedButtonAesEncrypt()
{
    // TODO: Add your control notification handler code here
    uint8_t* destData;
    int     destDataLen;
    uint8_t plain_text[] = {    0xD0, 0x27, 0x45, 0x25, 0x56, 0x31, 0x2E, 0x30, 0x5F, 0x53, 0x56, 0x34, 0x2E, 0x30, 0x30, 0x2E, 
                                0x30, 0x31, 0x4A, 0x75, 0x6E, 0x20, 0x31, 0x33, 0x20, 0x32, 0x30, 0x32, 0x33, 0x20, 0x31, 0x30,
                                0x3A, 0x33, 0x30, 0x3A, 0x30, 0x30, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    uint8_t plain_text2[] = { 0xD0, 0x27, 0x45, 0x25, 0x56, 0x31, 0x2E, 0x30, 0x5F, 0x53, 0x56, 0x34, 0x2E, 0x30, 0x30, 0x2E,
                                0x30, 0x31, 0x4A, 0x75, 0x6E, 0x20, 0x31, 0x33, 0x20, 0x32, 0x30, 0x32, 0x33, 0x20, 0x31, 0x30,
                                0x3A, 0x33, 0x30, 0x3A, 0x30, 0x30, 0x06, 0x00, 0x00};

    MRAESEncrypt(plain_text, sizeof(plain_text), &destData, &destDataLen);
    dumpDataMsg("MRAESEncrypt plain_text", destData, destDataLen);

    MRAESEncrypt(plain_text2, sizeof(plain_text2), &destData, &destDataLen);
    dumpDataMsg("MRAESEncrypt plain_text2", destData, destDataLen);
    
}


void CMRAgentDlg::OnBnClickedButtonAesDecrypt()
{
    // TODO: Add your control notification handler code here
    uint8_t* destData;
    int     destDataLen;
    uint8_t plain_text[] = { 0xD4, 0x11, 0xC5, 0x9E, 0x0D, 0x49, 0x34, 0x8C, 0xD2, 0x71, 0x4C, 0x06, 0x02, 0x01, 0xA5, 0x6B, 
                             0xAD, 0x65, 0x8B, 0x42, 0xD0, 0xBE, 0x0C, 0x8C, 0x9A, 0xEF, 0x6E, 0x8E, 0x15, 0xE2, 0xE2, 0xBE, 
                             0x64, 0x1E, 0x06, 0xC3, 0xD5, 0x8C, 0x62, 0xF9, 0x82, 0x23, 0x75, 0x60, 0xC5, 0xA9, 0x1A, 0xF2 };

    MRAESDecrypt(plain_text, sizeof(plain_text), &destData, &destDataLen);
    dumpDataMsg("MRAESDecrypt plain_text src", plain_text, sizeof(plain_text));
    dumpDataMsg("MRAESDecrypt plain_text dest", destData, destDataLen);

}


void CMRAgentDlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: Add your message handler code here and/or call default
    MRTimerCallback();
    CDialogEx::OnTimer(nIDEvent);
}


void CMRAgentDlg::OnBnClickedButtonRegAck()
{
    uint8_t destData[512];
    int cmdLen = MRSendRegCmdAck(destData, sizeof(destData));
    if (cmdLen > 0)
    {
        dumpDataMsg("MRSendRegCmdAck", destData, cmdLen);
        uartWrite(destData, cmdLen);
    }
    // TODO: Add your control notification handler code here
}


void CMRAgentDlg::OnBnClickedButtonPrintCmdReport()
{
    // TODO: Add your control notification handler code here
    uint8_t reportData[] = { 0xD6, 0x09, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
                             0xD4, 0x00, 0x06, 0x00, 0x01, 0x00, 0x0A, 0x00, 0x0B,
                             0xDA, 0x00, 0x0E, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x03, 0xFE, 0x7D, 0xFF, 0xA8, 0xFF, 0x2D, 0x54,
                             0xD5, 0x00, 0x02, 0x00, 0x00,
                             0xD8, 0x04, 0x00, 0x0A, 0x00, 0x00 };

    uint8_t reportData2[] = {   0xD6, 0x09, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
                                0xD4, 0x00, 0x0D, 0x00, 0x01, 0x00, 0x0A, 0x01, 0x00, 0x01, 0x11, 0x64, 0xD1, 0xE6, 0x6C, 0xA5,
                                0xDA, 0x00, 0x0E, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x11, 0x02, 0x73, 0x01, 0xDA, 0x03, 0x38, 0x9F,
                                0xD5, 0x00, 0x02, 0x00, 0x00,
                                0xD8, 0x04, 0x00, 0x0A, 0x00, 0x00 };
    //msgStr.Format(L"sizeof(reportData) = %d\r\n", sizeof(reportData));
    //UpdateMsgData(msgStr.GetBuffer());

    MRPrintCmd(reportData, sizeof(reportData));
    MRPrintCmd(reportData2, sizeof(reportData2));
}


void CMRAgentDlg::OnBnClickedButtonCmdProcess()
{
    // TODO: Add your control notification handler code here

    //char* cmd1 = "[M4CMD;LED;100;1000;200;2000;300;3000;400;4000]\r\n";
    //char* cmd2 = "[M4CMD;BUZZER;100;1000;4]\r\n";
    //char* cmd3 = "[M4CMD;TIME;Current Time;End Time 1;End Time 2;End Time 3;...;End Time n]\r\n";
    char* cmd0 = "\r\n[M4CMD;LED;100;10";
    char* cmd1 = "00;200;2000;300;";
    char* cmd2 = "3000;400;4000]\r\n[M4CMD;BUZZ";
    char* cmd2_1 = "ER;100;1000;4]\r\n[M4CMD;";
    char* cmd3 = "TIME;Current Time;";
    char* cmd4 = "End Time 1;End Time ";
    char* cmd5 = "2;End Time 3;...;End Time n]\r\nxxx";
    char* cmd6 = "zzz\r\nAAA";

    CMDProcessData((uint8_t*)cmd0, strlen(cmd0));
    CMDProcessData((uint8_t*)cmd1, strlen(cmd1));
    CMDProcessData((uint8_t*)cmd2, strlen(cmd2));
    CMDProcessData((uint8_t*)cmd2_1, strlen(cmd2_1));
    CMDProcessData((uint8_t*)cmd3, strlen(cmd3));
    CMDProcessData((uint8_t*)cmd4, strlen(cmd4));
    CMDProcessData((uint8_t*)cmd5, strlen(cmd5));
    CMDProcessData((uint8_t*)cmd6, strlen(cmd6));
}


void CMRAgentDlg::OnBnClickedButtonCmdPrepareAckData()
{
    // TODO: Add your control notification handler code here
    uint8_t destContain[512];
    int destContainLen;
    uint8_t destData[512];
    int destDataLen;

    UpdateMsgData(_T("              *** Base Status Ack ***\r\n"));
    destContainLen = MRGetBaseStatusAck(destContain, sizeof(destContain));
    destDataLen = MRSendAck(destData, sizeof(destData), destContain, destContainLen, "BaseStatusAck");

    destContainLen = MRGetVehicleStatusAck(destContain, sizeof(destContain));

    destContainLen = MRGetVehicleReportAck(destContain, sizeof(destContain));

    destContainLen = MRGetVehicleVerAck(destContain, sizeof(destContain));

    destContainLen = MRGetParaDownloadCmd(destContain, sizeof(destContain));


    UpdateMsgData(_T("\r\n"));
    //
    destDataLen = MRSendBaseStatusAck(destData, sizeof(destData));
    
}

void CMRAgentDlg::OnBnClickedButtonBaseStatusAck()
{
    // TODO: Add your control notification handler code here
    uint8_t destContain[64];
    int destContainLen;
    uint8_t destData[128];
    int destDataLen;

    UpdateMsgData(_T("              *** Base Status Ack ***\r\n"));
    destContainLen = MRGetBaseStatusAck(destContain, sizeof(destContain));
    destDataLen = MRSendAck(destData, sizeof(destData), destContain, destContainLen, "BaseStatusAck");

    if (destDataLen > 0)
    {
        dumpDataMsg("MRSendBaseStatusAck", destData, destDataLen);
        uartWrite(destData, destDataLen);
    }
}


void CMRAgentDlg::OnBnClickedButtonVehicleStatusAck()
{
    // TODO: Add your control notification handler code here
    uint8_t destContain[64];
    int destContainLen;
    uint8_t destData[128];
    int destDataLen;

    UpdateMsgData(_T("              *** Vehicle Status Ack ***\r\n"));
    destContainLen = MRGetVehicleStatusAck(destContain, sizeof(destContain));
    destDataLen = MRSendAck(destData, sizeof(destData), destContain, destContainLen, "VehicleStatusAck");

    if (destDataLen > 0)
    {
        dumpDataMsg("MRSendVehicleStatusAck", destData, destDataLen);
        uartWrite(destData, destDataLen);
    }
}


void CMRAgentDlg::OnBnClickedButtonVehicleReportAck()
{
    // TODO: Add your control notification handler code here
    uint8_t destContain[64];
    int destContainLen;
    uint8_t destData[128];
    int destDataLen;

    UpdateMsgData(_T("              *** Vehicle Report Ack ***\r\n"));
    destContainLen = MRGetVehicleReportAck(destContain, sizeof(destContain));
    destDataLen = MRSendAck(destData, sizeof(destData), destContain, destContainLen, "VehicleReportAck");

    if (destDataLen > 0)
    {
        dumpDataMsg("MRSendVehicleReportAck", destData, destDataLen);
        uartWrite(destData, destDataLen);
    }
}


void CMRAgentDlg::OnBnClickedButtonVehicleVerAck()
{
    // TODO: Add your control notification handler code here
    uint8_t destContain[64];
    int destContainLen;
    uint8_t destData[128];
    int destDataLen;

    UpdateMsgData(_T("              *** Vehicle Ver Ack ***\r\n"));
    destContainLen = MRGetVehicleVerAck(destContain, sizeof(destContain));
    destDataLen = MRSendAck(destData, sizeof(destData), destContain, destContainLen, "VehicleVerAck");

    if (destDataLen > 0)
    {
        dumpDataMsg("MRSendVehicleVerAck", destData, destDataLen);
        uartWrite(destData, destDataLen);
    }
}


void CMRAgentDlg::OnBnClickedButtonParaDownloadCmd()
{
    // TODO: Add your control notification handler code here
    uint8_t destContain[64];
    int destContainLen;
    uint8_t destData[128];
    int destDataLen;

    UpdateMsgData(_T("              *** Para Download Cmd ***\r\n"));
    destContainLen = MRGetParaDLCmdReinit(destContain, sizeof(destContain));
    destDataLen = MRSendAck(destData, sizeof(destData), destContain, destContainLen, "ParaDownloadCmd");

    if (destDataLen > 0)
    {
        dumpDataMsg("MRSendParaDownloadCmd", destData, destDataLen);
        uartWrite(destData, destDataLen);
    }
}


int CMRAgentDlg::UpdateMsgData2(TCHAR* msg)
{
    // TODO: Add your implementation code here.
    pCMRAgentDlg->msgEditStr2 = pCMRAgentDlg->msgEditStr2 + msg;// +_T("\r\n");
    ::SetWindowText(::GetDlgItem(pCMRAgentDlg->m_hWnd, IDC_EDIT_MSG2), pCMRAgentDlg->msgEditStr2);

    pCMRAgentDlg->msgEditCtrl2.LineScroll(pCMRAgentDlg->msgEditCtrl2.GetLineCount(), 0);
    pCMRAgentDlg->msgEditCtrl2.UpdateWindow();
    return 0;
}



void CMRAgentDlg::OnBnClickedButtonGeomagreinit()
{
    uint8_t destData[512];
    int cmdLen = MRSendRegCmdAck(destData, sizeof(destData));
    if (cmdLen > 0)
    {
        dumpDataMsg("MRSendRegCmdAck", destData, cmdLen);
        uartWrite(destData, cmdLen);
    }
    // TODO: 在此加入控制項告知處理常式程式碼
}


void CMRAgentDlg::OnBnClickedGeomangreiniten()
{
    CString str;
    str = L"Geomang is ";
    UpdateData(true);
    if (CheckBoxGeomangEn)
    {
        str += L"Enable";
        paraCmdGeomangReinitFlag = 1;
    }
    else
    {
        str += L"Disable";
        paraCmdGeomangReinitFlag = 0;
    }
    MessageBox(str);
        // TODO: 在此加入控制項告知處理常式程式碼
}





void CMRAgentDlg::OnBnClickedButtonUpdatebasedata()
{
    // TODO: 在此加入控制項告知處理常式程式碼
    UpdateData(true);

    m_localBaseID = mrLocalBaseID;
    msgStr.Format(L"### Base Data Update - Local ### BaseID = %d, BroadcastChannel = %d, WorkChannel = %d \r\n", m_localBaseID, m_localBroadcastCH, m_localWorkCH);
    UpdateMsgData(msgStr.GetBuffer());
    msgStr.Format(L"### Base Data Update - BKP   ### BaseID = %d, BroadcastChannel = %d, WorkChannel = %d \r\n", m_bkpBaseID, m_bkpBroadcastCH, m_bkpWorkCH);
    UpdateMsgData(msgStr.GetBuffer());

    mrLocalBroadcastCH = m_localBroadcastCH;
    mrLocalWorkCH = m_localWorkCH;
    mrBkpBaseID = m_bkpBaseID;
    mrBkpBroadcastCH = m_bkpBroadcastCH;
    mrBkpWorkCH = m_bkpWorkCH;
    UpdateData(false);
}
