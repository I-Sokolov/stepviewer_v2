#pragma once

class CBCFTopicDlg;
class CBCFView;
struct BCFTopic;

class CBCFAddReferenceLink : public CDialogEx
{
	DECLARE_DYNAMIC(CBCFAddReferenceLink)

public:
	CBCFAddReferenceLink(CBCFTopicDlg& view);
	CBCFAddReferenceLink(CBCFView& view, BCFTopic& topic);
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
	CEdit m_wndEdit;
	CButton m_wndOK;

private:
	BCFTopic* m_topic;
	CBCFTopicDlg* m_topicView;
	CBCFView* m_paneView;
};
