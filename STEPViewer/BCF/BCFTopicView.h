#pragma once

#include "bcfAPI.h"
#include "BCFViewPointMgr.h"

#include <vector>
using namespace std;

class CBCFProjectView;
class _model;


class CBCFTopicView : public CDialogEx
{
	DECLARE_DYNAMIC(CBCFTopicView)

public:
	CBCFTopicView(CBCFProjectView& projectView, BCFTopic& topic);
	virtual ~CBCFTopicView();

public:
	CBCFProjectView& GetProjectView() { return m_projectView; }
	BCFTopic& GetTopic() { return m_topic; }
	void ShowLog(bool knownError); //false: show log if any, not necessary error

	CString GetTopicDisplayName(BCFTopic& topic);

	_model* GetBimModel(BCFBimFile& file);

public:
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BCF_TOPIC_VIEW };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnCancel();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSelchangeCommentsList();
	afx_msg void OnSelchangeTab(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickedButtonAddMulti();
	afx_msg void OnClickedButtonRemoveMulti();
	afx_msg void OnSelchangeMultiList();
	afx_msg void OnClickedUpdateViewpoint();
	afx_msg void OnClickedButtonBims();

private:
	void LoadView();
	void UpdateTopicInfo();
	void LoadExtensions();
	void LoadExtension(CComboBox& wnd, BCFEnumeration enumeraion);
	void LoadBIMFiles();
	void LoadTopic();
	void LoadComments(int select = 0);
	void SaveTopic();
	void SaveActiveComment();
	void FillMultiList();
	void FillLabels(BCFTopic* topic);
	void FillRelated(BCFTopic* topic);
	void FillLinks(BCFTopic* topic);
	void FillDocuments(BCFTopic* topic);
	void AddLabel(BCFTopic* topic);
	void AddRelated(BCFTopic* topic);
	void AddLink(BCFTopic* topic);
	void AddDocument(BCFTopic* topic);
	void RemoveLabel(BCFTopic* topic);
	void RemoveRelated(BCFTopic* topic);
	void RemoveLink(BCFTopic* topic);
	void RemoveDocument(BCFTopic* topic);

private:
	CBCFProjectView&				m_projectView;
	BCFTopic&						m_topic;

	CBCFViewPointMgr				m_viewPointMgr;

private:
	CStatic m_wndTopicInfo;
	CComboBox m_wndTopicType;
	CString m_strTopicType;
	CComboBox m_wndTopicStage;
	CString m_strTopicStage;
	CComboBox m_wndTopicStatus;
	CString m_strTopicStatus;
	CComboBox m_wndAssigned;
	CString m_strAssigned;
	CComboBox m_wndPriority;
	CString m_strPriority;
	CComboBox m_wndSnippetType;
	CString m_strSnippetType;
	CTabCtrl m_wndTab;
	CEdit m_wndDue;
	CString m_strDue;
	CEdit m_wndDescription;
	CString m_strDescription;
	CEdit m_wndTitle;
	CString m_strTitle;
	CEdit m_wndSnippetReference;
	CString m_strSnippetReference;
	CEdit m_wndSnippetSchema;
	CString m_strSnippetSchema;
	CEdit m_wndIndex;
	CString m_strIndex;
	CEdit m_wndServerIndex;
	CString m_strServerId;
	CListBox m_wndCommentsList;
	CEdit m_wndCommentText;
	CListBox m_wndMultiList;
	CButton m_wndAddMulti;
	CButton m_wndRemoveMulti;
	CButton m_wndUpdateViewPoint;
};
