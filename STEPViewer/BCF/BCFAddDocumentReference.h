#pragma once

class CBCFView;
struct BCFTopic;

// CBCFAddDocumentReference dialog

class CBCFAddDocumentReference : public CDialogEx
{
	DECLARE_DYNAMIC(CBCFAddDocumentReference)

public:
	CBCFAddDocumentReference(CBCFView& view, BCFTopic& topic);
	virtual ~CBCFAddDocumentReference();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BCF_ADDOCUMENT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnClickedButtonBrowse();

public:
	CString m_strPath;
	CString m_strDescription;
	BOOL m_isExternal;

private:
	BCFTopic* m_topic;
	CBCFView* m_paneView;
};
