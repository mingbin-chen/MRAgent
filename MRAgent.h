
// MRAgent.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CMRAgentApp:
// See MRAgent.cpp for the implementation of this class
//

class CMRAgentApp : public CWinApp
{
public:
	CMRAgentApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CMRAgentApp theApp;
