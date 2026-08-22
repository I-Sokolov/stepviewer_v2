#pragma once

class CBCFTopicView;

class CBCFAddLabel : public CDialogEx
{
	DECLARE_DYNAMIC(CBCFAddLabel)

public:
	CBCFAddLabel(CBCFTopicView& bcfView);
	virtual ~CBCFAddLabel();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BCF_ADDLABEL };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

private:
	CBCFTopicView& m_view;
	CComboBox m_wndAddLabel;
	CString	  m_strAddLabel;
};
