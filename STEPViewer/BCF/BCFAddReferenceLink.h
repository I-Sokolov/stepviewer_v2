#pragma once

class CBCFTopicView;

class CBCFAddReferenceLink : public CDialogEx
{
	DECLARE_DYNAMIC(CBCFAddReferenceLink)

public:
	CBCFAddReferenceLink(CBCFTopicView& view);
	virtual ~CBCFAddReferenceLink();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BCF_ADDREFERENCELINK };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnChangeEdit();

public:
	CBCFTopicView& m_view;
	CEdit m_wndEdit;
	CButton m_wndOK;
};
