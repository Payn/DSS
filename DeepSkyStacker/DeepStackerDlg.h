#if !defined(AFX_DEEPSTACKERDLG_H__C02E1779_4790_4F52_89BD_FD816B7D1C7D__INCLUDED_)
#define AFX_DEEPSTACKERDLG_H__C02E1779_4790_4F52_89BD_FD816B7D1C7D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DeepStackerDlg.h : header file
//

#include "DeepStack.h"
#include "DeepSkyStacker.h"
#include <list>

class CDSSSetting
{
public :
	CString				m_strName;
	CBezierAdjust		m_BezierAdjust;
	CRGBHistogramAdjust	m_HistoAdjust;

private :
	void	CopyFrom(const CDSSSetting & cds)
	{
		m_strName		= cds.m_strName;
		m_BezierAdjust	= cds.m_BezierAdjust;
		m_HistoAdjust	= cds.m_HistoAdjust;
	};

public :
	CDSSSetting() {};
	virtual ~CDSSSetting() {};

	CDSSSetting(const CDSSSetting & cds)
	{
		CopyFrom(cds);
	};

	CDSSSetting & operator = (const CDSSSetting & cds)
	{
		CopyFrom(cds);
		return (*this);
	};

	bool operator < (const CDSSSetting & cds) const
	{
		int			nCompare;
		nCompare = m_strName.CompareNoCase(cds.m_strName);

		if (nCompare < 0)
			return true;
		else
			return false;
	};

	BOOL	Load(FILE * hFile)
	{
		LONG		lNameSize;
		TCHAR		szName[2000];

		fread(&lNameSize, sizeof(lNameSize), 1, hFile);
		fread(szName, sizeof(TCHAR), lNameSize, hFile);
		m_strName = szName;
		return m_BezierAdjust.Load(hFile) && m_HistoAdjust.Load(hFile);
	};

	BOOL	Save(FILE * hFile)
	{
		LONG		lNameSize = m_strName.GetLength()+1;
		fwrite(&lNameSize, sizeof(lNameSize), 1, hFile);
		fwrite((LPCTSTR)m_strName, sizeof(TCHAR), lNameSize, hFile);

		return m_BezierAdjust.Save(hFile) && m_HistoAdjust.Save(hFile);
	};
};

typedef std::list<CDSSSetting>			DSSSETTINGLIST;
typedef	DSSSETTINGLIST::iterator		DSSSETTINGITERATOR;

class CDSSSettings
{
private :
	std::list<CDSSSetting>	m_lSettings;
	BOOL					m_bLoaded;

public :
	CDSSSettings()
	{
		m_bLoaded = FALSE;
	};
	virtual ~CDSSSettings() {};

	BOOL	IsLoaded()
	{
		return m_bLoaded;
	};
	BOOL	Load(LPCTSTR szFile = nullptr);
	BOOL	Save(LPCTSTR szFile = nullptr);

	LONG	Count()
	{
		return (LONG)m_lSettings.size();
	};

	BOOL	GetItem(LONG lIndice, CDSSSetting & cds)
	{
		BOOL			bResult = FALSE;

		if (lIndice < m_lSettings.size())
		{
			DSSSETTINGITERATOR	it;

			it = m_lSettings.begin();
			while (lIndice)
			{
				it++;
				lIndice--;
			};

			cds = (*it);
			bResult = TRUE;
		};

		return bResult;
	};

	BOOL	Add(const CDSSSetting & cds)
	{
		m_lSettings.push_back(cds);
		return TRUE;
	};

	BOOL	Remove(LONG lIndice)
	{
		BOOL			bResult = FALSE;

		if (lIndice < m_lSettings.size())
		{
			DSSSETTINGITERATOR	it;

			it = m_lSettings.begin();
			while (lIndice)
			{
				it++;
				lIndice--;
			};

			m_lSettings.erase(it);
			bResult = TRUE;
		};

		return bResult;
	};
};

#include "StackingDlg.h"
#include "ProcessingDlg.h"
#include "LibraryDlg.h"
#include "ExplorerBar.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CDeepStackerDlg dialog

enum DeepStackerDlgMessages
{
    WM_PROGRESS_INIT = WM_USER + 10000,
    WM_PROGRESS_UPDATE,
    WM_PROGRESS_STOP,
};

class CDeepStackerDlg : public CDialog
{
private :
	CStackingDlg			m_dlgStacking;
	CProcessingDlg			m_dlgProcessing;
	CLibraryDlg				m_dlgLibrary;

	CDeepStack				m_DeepStack;
	CDSSSettings			m_Settings;
	CExplorerBar			m_ExplorerBar;
	DWORD					m_dwCurrentTab;
	CString					m_strStartFileList;
	CString					m_strBaseTitle;
    ITaskbarList3*          m_taskbarList;
    bool                    m_progress;

// Construction
public:
	CDeepStackerDlg(CWnd* pParent = nullptr);   // standard constructor
	void	ChangeTab(DWORD dwTabID);
	DWORD	GetCurrentTab()
	{
		return m_dwCurrentTab;
	};

	void	SetStartingFileList(LPCTSTR szStartFileList)
	{
		m_strStartFileList = szStartFileList;
	};

	void disableSubDialogs()
	{
		m_dlgStacking.EnableWindow(false);
		m_dlgProcessing.EnableWindow(false);
		m_dlgLibrary.EnableWindow(false);
		m_ExplorerBar.EnableWindow(false);
	};

	void enableSubDialogs()
	{
		m_dlgStacking.EnableWindow(true);
		m_dlgProcessing.EnableWindow(true);
		m_dlgLibrary.EnableWindow(true);
		m_ExplorerBar.EnableWindow(true);
	};

// Dialog Data
	//{{AFX_DATA(CDeepStackerDlg)
	enum { IDD = IDD_DEEPSTACKER };
	CStatic		m_BarStatic;
	//}}AFX_DATA

	CDeepStack & GetDeepStack()
	{
		return m_DeepStack;
	};

	CDSSSettings & GetDSSSettings()
	{
		if (!m_Settings.IsLoaded())
			m_Settings.Load();

		return m_Settings;
	};

	CStackingDlg & GetStackingDlg()
	{
		return m_dlgStacking;
	};

	CProcessingDlg & GetProcessingDlg()
	{
		return m_dlgProcessing;
	};


	CExplorerBar & GetExplorerBar()
	{
		return m_ExplorerBar;
	};

	void	SetCurrentFileInTitle(LPCTSTR szFileName);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeepStackerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDeepStackerDlg)
	virtual BOOL	OnInitDialog();
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg	BOOL	OnEraseBkgnd(CDC * pDC);
	//}}AFX_MSG
	afx_msg LRESULT OnHTMLHelp(WPARAM, LPARAM);
	afx_msg	LRESULT OnOpenStartFileList(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()

private :
	void	UpdateTab();
	void	UpdateSizes();
	afx_msg void OnOK();
	afx_msg void OnClose();
	afx_msg void OnBnClickedCancel();
	afx_msg	void OnHelp();

	afx_msg void OnDropFiles(HDROP hDropInfo);
    afx_msg LRESULT OnTaskbarButtonCreated(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnProgressInit(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnProgressUpdate(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnProgressStop(WPARAM wParam, LPARAM lParam);
};

/* ------------------------------------------------------------------- */

inline CDeepStackerDlg * GetDeepStackerDlg(CWnd * pDialog)
{
	if (pDialog)
	{
		CWnd *				pParent = pDialog->GetParent();
		CDeepStackerDlg *	pDlg = dynamic_cast<CDeepStackerDlg *>(pParent);

		if (!pDlg)
			pDlg = dynamic_cast<CDeepStackerDlg *>(GetDSSApp()->m_pMainDlg);

		return pDlg;
	}
	else
	{
		CWnd *				pWnd = GetDSSApp()->m_pMainDlg;
		CDeepStackerDlg *	pDlg = dynamic_cast<CDeepStackerDlg *>(pWnd);

		return pDlg;
	};
};

/* ------------------------------------------------------------------- */

inline CDeepStack & GetDeepStack(CWnd * pDialog)
{
	CDeepStackerDlg *	pDlg = GetDeepStackerDlg(pDialog);

	return pDlg->GetDeepStack();
};

/* ------------------------------------------------------------------- */

inline CDSSSettings & GetDSSSettings(CWnd * pDialog)
{
	CDeepStackerDlg *	pDlg = GetDeepStackerDlg(pDialog);

	return pDlg->GetDSSSettings();
};

/* ------------------------------------------------------------------- */

inline CStackingDlg & GetStackingDlg(CWnd * pDialog)
{
	CDeepStackerDlg *	pDlg = GetDeepStackerDlg(pDialog);

	return pDlg->GetStackingDlg();
};

/* ------------------------------------------------------------------- */

inline CProcessingDlg & GetProcessingDlg(CWnd * pDialog)
{
	CDeepStackerDlg *	pDlg = GetDeepStackerDlg(pDialog);

	return pDlg->GetProcessingDlg();
};


/* ------------------------------------------------------------------- */

inline void	SetCurrentFileInTitle(LPCTSTR szFileName)
{
	CDeepStackerDlg *		pDlg = GetDeepStackerDlg(nullptr);

	if (pDlg)
		pDlg->SetCurrentFileInTitle(szFileName);
};

/* ------------------------------------------------------------------- */

void	SaveWindowPosition(CWnd * pWnd, LPCTSTR szRegistryPath);
void	RestoreWindowPosition(CWnd * pWnd, LPCTSTR szRegistryPath, bool bCenter = false);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_DEEPSTACKERDLG_H__C02E1779_4790_4F52_89BD_FD816B7D1C7D__INCLUDED_)
