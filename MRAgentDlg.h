
// MRAgentDlg.h : header file
//

#pragma once


// CMRAgentDlg dialog
class CMRAgentDlg : public CDialogEx
{
// Construction
public:
	CMRAgentDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MRAGENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	void updateComList();
	CComboBox comListCtrl;
	CComboBox comBaudrateCtrl;
	CButton btnOpenCtrl;
	CButton btnCloseCtrl;
	afx_msg void OnBnClickedButtonOpen();
	afx_msg void OnBnClickedButtonClose();
	int openComPort(TCHAR* comString, int baudrate);
	int UpdateMsgData(TCHAR* msg);
	CString msgEditStr;
	CEdit msgEditCtrl;
	int ReceiveData(char* buff, int buffLen);
	int SendData(char* data, int dataLen);
	afx_msg void OnBnClickedOk2();
	afx_msg void OnBnClickedButtonClean();
	afx_msg void OnBnClickedButtonAesEncrypt();
	afx_msg void OnBnClickedButtonAesDecrypt();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonRegAck();
	afx_msg void OnBnClickedButtonPrintCmdReport();
	afx_msg void OnBnClickedButtonCmdProcess();
	afx_msg void OnBnClickedButtonCmdPrepareAckData();
	afx_msg void OnBnClickedButtonBaseStatusAck();
	afx_msg void OnBnClickedButtonVehicleStatusAck();
	afx_msg void OnBnClickedButtonVehicleReportAck();
	afx_msg void OnBnClickedButtonVehicleVerAck();
	afx_msg void OnBnClickedButtonParaDownloadCmd();
	int UpdateMsgData2(TCHAR* msg);
	CString msgEditStr2;
	CEdit msgEditCtrl2;
	afx_msg void OnBnClickedButtonGeomagreinit();
	afx_msg void OnBnClickedGeomangreiniten();
	CButton CheckBoxGeomangReinit;
	BOOL CheckBoxGeomangEn;

	afx_msg void OnBnClickedButtonUpdatebasedata();
	int m_localBaseID;
	uint8_t m_localBroadcastCH;
	uint8_t m_localWorkCH;
	int m_bkpBaseID;
	uint8_t m_bkpBroadcastCH;
	uint8_t m_bkpWorkCH;
};
