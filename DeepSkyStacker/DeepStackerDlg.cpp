// DeepStackerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DeepSkyStacker.h"
#include "DeepStackerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* ------------------------------------------------------------------- */

static BOOL	GetDefaultSettingsFileName(CString & strFile)
{
	CString			strBase;
	TCHAR			szFileName[1+_MAX_PATH];
	TCHAR			szDrive[1+_MAX_DRIVE];
	TCHAR			szDir[1+_MAX_DIR];

	GetModuleFileName(nullptr, szFileName, sizeof(szFileName));
	strBase = szFileName;
	_tsplitpath(strBase, szDrive, szDir, nullptr, nullptr);

	strFile = szDrive;
	strFile += szDir;
	strFile += "DSSSettings.DSSSettings";

	return TRUE;
};

/* ------------------------------------------------------------------- */

#pragma pack(push, HDSETTINGS, 2)

const DWORD			HDSSETTINGS_MAGIC = 0x7ABC6F10L;

typedef struct tagHDSETTINGSHEADER
{
	DWORD			dwMagic;		// Magic number (always HDSSETTINGS_MAGIC)
	DWORD			dwHeaderSize;	// Always sizeof(HDSETTINGSHEADER);
	LONG			lNrSettings;	// Number of settings
	DWORD			dwFlags;		// Flags
	char			Reserved[32];	// Reserved (set to 0)
}HDSETTINGSHEADER;

#pragma pack(pop, HDSETTINGS)

/* ------------------------------------------------------------------- */

BOOL	CDSSSettings::Load(LPCTSTR szFile)
{
	BOOL			bResult = FALSE;
	CString			strFile = szFile;
	FILE *			hFile = nullptr;

	if (!strFile.GetLength())
		GetDefaultSettingsFileName(strFile);

	hFile = _tfopen(strFile, _T("rb"));
	if (hFile)
	{
		HDSETTINGSHEADER		Header;
		LONG					i;

		fread(&Header, sizeof(Header), 1, hFile);
		if ((Header.dwMagic == HDSSETTINGS_MAGIC) &&
			(Header.dwHeaderSize == sizeof(Header)))
		{
			m_lSettings.clear();
			for (i = 0;i<Header.lNrSettings;i++)
			{
				CDSSSetting		cds;

				cds.Load(hFile);
				m_lSettings.push_back(cds);
			};

			bResult = TRUE;
			m_lSettings.sort();
		};

		fclose(hFile);
	};

	m_bLoaded = TRUE;

	return bResult;
};

/* ------------------------------------------------------------------- */

BOOL	CDSSSettings::Save(LPCTSTR szFile)
{
	BOOL			bResult = FALSE;
	CString			strFile = szFile;
	FILE *			hFile = nullptr;

	if (!strFile.GetLength())
		GetDefaultSettingsFileName(strFile);

	hFile = _tfopen(strFile, _T("wb"));
	if (hFile)
	{
		m_lSettings.sort();

		HDSETTINGSHEADER		Header;
		DSSSETTINGITERATOR		it;

		memset(&Header, 0, sizeof(Header));

		Header.dwMagic = HDSSETTINGS_MAGIC;
		Header.dwHeaderSize = sizeof(Header);
		Header.lNrSettings  = (LONG)m_lSettings.size();

		fwrite(&Header, sizeof(Header), 1, hFile);
		for (it = m_lSettings.begin(); it != m_lSettings.end();it++)
			(*it).Save(hFile);

		fclose(hFile);
		bResult = TRUE;
	};

	return bResult;
};

/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */

/////////////////////////////////////////////////////////////////////////////
// CDeepStackerDlg dialog

UINT WM_TASKBAR_BUTTON_CREATED = ::RegisterWindowMessage("TaskbarButtonCreated");

CDeepStackerDlg::CDeepStackerDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CDeepStackerDlg::IDD, pParent),
	m_dlgStacking(this),
	m_dlgProcessing(this),
	m_dlgLibrary(this),
	m_ExplorerBar(this)
{
	//{{AFX_DATA_INIT(CDeepStackerDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_dwCurrentTab = 0;
    m_taskbarList = nullptr;
    m_progress = false;
}

/* ------------------------------------------------------------------- */

void CDeepStackerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeepStackerDlg)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_BARSTATIC, m_BarStatic);
}

/* ------------------------------------------------------------------- */

BEGIN_MESSAGE_MAP(CDeepStackerDlg, CDialog)
	//{{AFX_MSG_MAP(CDeepStackerDlg)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_WM_CLOSE()
	ON_MESSAGE(WM_HELP, OnHTMLHelp)
	ON_BN_CLICKED(IDCANCEL, &CDeepStackerDlg::OnBnClickedCancel)
	ON_WM_DROPFILES()
	ON_WM_ERASEBKGND()
    ON_REGISTERED_MESSAGE(WM_TASKBAR_BUTTON_CREATED, &CDeepStackerDlg::OnTaskbarButtonCreated)
    ON_MESSAGE(WM_PROGRESS_INIT, &CDeepStackerDlg::OnProgressInit)
    ON_MESSAGE(WM_PROGRESS_UPDATE, &CDeepStackerDlg::OnProgressUpdate)
    ON_MESSAGE(WM_PROGRESS_STOP, &CDeepStackerDlg::OnProgressStop)
END_MESSAGE_MAP()

/* ------------------------------------------------------------------- */

void CDeepStackerDlg::UpdateTab()
{
	switch (m_dwCurrentTab)
	{
	case IDD_REGISTERING :
	case IDD_STACKING :
		m_dlgStacking.ShowWindow(SW_SHOW);
		m_dlgProcessing.ShowWindow(SW_HIDE);
		m_dlgLibrary.ShowWindow(SW_HIDE);
		break;
	case IDD_LIBRARY :
		m_dlgStacking.ShowWindow(SW_HIDE);
		m_dlgProcessing.ShowWindow(SW_HIDE);
		m_dlgLibrary.ShowWindow(SW_SHOW);
		break;
	case IDD_PROCESSING :
		m_dlgStacking.ShowWindow(SW_HIDE);
		m_dlgProcessing.ShowWindow(SW_SHOW);
		m_dlgLibrary.ShowWindow(SW_HIDE);
		break;
	};
	m_ExplorerBar.InvalidateRect(nullptr);
};

/* ------------------------------------------------------------------- */

BOOL CDeepStackerDlg::OnEraseBkgnd(CDC * pDC)
{
	return 1;
};

/* ------------------------------------------------------------------- */

void CDeepStackerDlg::UpdateSizes()
{
	// Resize the tab control
	CRect			rcDlg;
	CRect			rcExplorerBar;

	GetClientRect(&rcDlg);

	if (m_dlgStacking.m_hWnd)
	{
		rcExplorerBar = rcDlg;
		rcDlg.left += 220;
		rcExplorerBar.right = rcDlg.left;

		if (m_dlgStacking.m_hWnd)
			m_dlgStacking.MoveWindow(&rcDlg);
		if (m_dlgProcessing.m_hWnd)
			m_dlgProcessing.MoveWindow(&rcDlg);
		if (m_dlgLibrary.m_hWnd)
			m_dlgLibrary.MoveWindow(&rcDlg);
		if (m_ExplorerBar.m_hWnd)
			m_ExplorerBar.MoveWindow(&rcExplorerBar);
	};
};

/* ------------------------------------------------------------------- */
/////////////////////////////////////////////////////////////////////////////
// CDeepStackerDlg message handlers

void CDeepStackerDlg::ChangeTab(DWORD dwTabID)
{
	if (dwTabID == IDD_REGISTERING)
		dwTabID = IDD_STACKING;
#ifdef DSSBETA
	if (dwTabID == IDD_STACKING && 	(GetAsyncKeyState(VK_CONTROL) & 0x8000))
		dwTabID = IDD_LIBRARY;
#endif
	m_dwCurrentTab = dwTabID;
	UpdateTab();
};

/* ------------------------------------------------------------------- */

BOOL CDeepStackerDlg::OnInitDialog()
{
	ZFUNCTRACE_RUNTIME();
	ZTRACE_RUNTIME("Initializing Main Dialog");
	CDialog::OnInitDialog();
	ZTRACE_RUNTIME("Initializing Main Dialog - ok");


	CString			strMask;
	CString			strTitle;

	//
	// The call to CWnd::DragAcceptFiles() was moved here from DeepSkyStacker.cpp because it can only be called once
	// the HWND for the dialog is valid (not nullptr).  This is only true once CDialog::OnInitDialog() above has been called.
	//
	this->DragAcceptFiles(TRUE);

	GetWindowText(strMask);
	strTitle.Format(strMask, _T(VERSION_DEEPSKYSTACKER));
	SetWindowText(strTitle);
	m_strBaseTitle = strTitle;

	ZTRACE_RUNTIME("Creating Left Panel");
	m_ExplorerBar.Create(IDD_EXPLORERBAR, this);
	ZTRACE_RUNTIME("Creating Left Panel - ok");

	SetIcon(AfxGetApp()->LoadIcon(IDI_APP), TRUE);
	m_dlgStacking.SetStartingFileList(m_strStartFileList);
	ZTRACE_RUNTIME("Creating Stacking Panel");
	m_dlgStacking.Create(IDD_STACKING, this);
	ZTRACE_RUNTIME("Creating Stacking Panel - ok");
	ZTRACE_RUNTIME("Creating Processing Panel");
	m_dlgProcessing.Create(IDD_PROCESSING, this);
	ZTRACE_RUNTIME("Creating Processing Panel - ok");
	m_dlgLibrary.Create(IDD_LIBRARY, this);

	ZTRACE_RUNTIME("Restoring Window Position");
	RestoreWindowPosition(this, REGENTRY_BASEKEY_DEEPSKYSTACKER_POSITION);
	ZTRACE_RUNTIME("Restoring Window Position - ok");

	m_dwCurrentTab = IDD_REGISTERING;
	ZTRACE_RUNTIME("Updating All Panels");
	UpdateTab();
	ZTRACE_RUNTIME("Updating All Panels - ok");
	ZTRACE_RUNTIME("Updating Sizes");
	UpdateSizes();
	ZTRACE_RUNTIME("Updating Sizes - ok");

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/* ------------------------------------------------------------------- */

void CDeepStackerDlg::SetCurrentFileInTitle(LPCTSTR szFile)
{
	CString					strFileName = szFile;
	if (strFileName.GetLength())
	{
		TCHAR				szFileName[1+_MAX_FNAME];
		TCHAR				szExt[1+_MAX_EXT];

		_tsplitpath(szFile, nullptr, nullptr, szFileName, szExt);

		CString				strTitle;

		strTitle.Format(_T("%s - %s%s"), (LPCTSTR)m_strBaseTitle, szFileName, szExt);
		SetWindowText(strTitle);
	}
	else
		SetWindowText(m_strBaseTitle);
};


/* ------------------------------------------------------------------- */

void CDeepStackerDlg::OnDropFiles(HDROP hDropInfo)
{
	if (hDropInfo && m_dlgStacking.m_hWnd)
	{
		SetForegroundWindow();
		BringWindowToTop();
		SetActiveWindow();
		m_dlgStacking.DropFiles(hDropInfo);
	};
};

LRESULT CDeepStackerDlg::OnTaskbarButtonCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    HRESULT hr = ::CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, reinterpret_cast<void**>(&m_taskbarList));

    if (FAILED(hr))
        return 0;

    hr = m_taskbarList->HrInit();

    m_taskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);

    return 0;
}

LRESULT CDeepStackerDlg::OnProgressInit(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_taskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);

    m_progress = true;

    return 0;
}

LRESULT CDeepStackerDlg::OnProgressUpdate(WPARAM wParam, LPARAM lParam)
{
    // do not update if progress wasn't started manually
    if (m_progress)
        m_taskbarList->SetProgressValue(m_hWnd, wParam, lParam);

    return 0;
}

LRESULT CDeepStackerDlg::OnProgressStop(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_taskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);

    m_progress = false;

    return 0;
}

/* ------------------------------------------------------------------- */

void CDeepStackerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	UpdateSizes();
	// Resize all dialogs
}

/* ------------------------------------------------------------------- */

void CDeepStackerDlg::OnClose()
{
	if (m_dlgStacking.SaveOnClose() &&
		m_dlgProcessing.SaveOnClose())
	{
		SaveWindowPosition(this, REGENTRY_BASEKEY_DEEPSKYSTACKER_POSITION);

		CDialog::OnClose();
	};
}

/* ------------------------------------------------------------------- */

void CDeepStackerDlg::OnBnClickedCancel()
{
	if (!(GetKeyState(VK_ESCAPE) & 0x8000))
		OnCancel();
}

/* ------------------------------------------------------------------- */

void CDeepStackerDlg::OnOK()
{

};

/* ------------------------------------------------------------------- */

LRESULT CDeepStackerDlg::OnHTMLHelp(WPARAM, LPARAM)
{
	OnHelp();
	return 1;
};

/* ------------------------------------------------------------------- */

void CDeepStackerDlg::OnHelp()
{
	if (m_ExplorerBar.m_hWnd)
		m_ExplorerBar.CallHelp();
};

/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */

void	SaveWindowPosition(CWnd * pWnd, LPCTSTR szRegistryPath)
{
	DWORD		dwMaximized = 0;
	CString		strTop = "";
	CString		strLeft = "";
	DWORD		dwWidth = 0;
	DWORD		dwHeight = 0;

	CRegistry	reg;

	WINDOWPLACEMENT		wp;

	memset(&wp, 0, sizeof(wp));
	wp.length  = sizeof(wp);

	pWnd->GetWindowPlacement(&wp);
	dwMaximized = (wp.showCmd == SW_SHOWMAXIMIZED);
	strLeft.Format(_T("%ld"), wp.rcNormalPosition.left);
	strTop.Format(_T("%ld"), wp.rcNormalPosition.top);

	dwWidth  = wp.rcNormalPosition.right-wp.rcNormalPosition.left;
	dwHeight = wp.rcNormalPosition.bottom-wp.rcNormalPosition.top;

	reg.SaveKey(szRegistryPath, _T("Maximized"), dwMaximized);
	reg.SaveKey(szRegistryPath, _T("Top"), strTop);
	reg.SaveKey(szRegistryPath, _T("Left"), strLeft);
	reg.SaveKey(szRegistryPath, _T("Width"), dwWidth);
	reg.SaveKey(szRegistryPath, _T("Height"), dwHeight);
};

/* ------------------------------------------------------------------- */

void	RestoreWindowPosition(CWnd * pWnd, LPCTSTR szRegistryPath, bool bCenter)
{
	DWORD		dwMaximized = 0;
	CString		strTop = "";
	CString		strLeft = "";
	DWORD		dwWidth = 0;
	DWORD		dwHeight = 0;

	CRegistry	reg;

	reg.LoadKey(szRegistryPath, _T("Maximized"), dwMaximized);
	reg.LoadKey(szRegistryPath, _T("Top"), strTop);
	reg.LoadKey(szRegistryPath, _T("Left"), strLeft);
	reg.LoadKey(szRegistryPath, _T("Width"), dwWidth);
	reg.LoadKey(szRegistryPath, _T("Height"), dwHeight);

	if (strTop.GetLength() && strLeft.GetLength() && dwWidth && dwHeight)
	{
		WINDOWPLACEMENT		wp;

		memset(&wp, 0, sizeof(wp));
		wp.length  = sizeof(wp);
		wp.flags   = 0;
		wp.showCmd = dwMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
		wp.rcNormalPosition.left   = _ttol(strLeft);
		wp.rcNormalPosition.top    = _ttol(strTop);
		wp.rcNormalPosition.right  = wp.rcNormalPosition.left+dwWidth;
		wp.rcNormalPosition.bottom = wp.rcNormalPosition.top+dwHeight;

		pWnd->SetWindowPlacement(&wp);
		if (bCenter)
			pWnd->CenterWindow();
	};
};

/* ------------------------------------------------------------------- */
