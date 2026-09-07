#pragma once

#include "bcfAPI.h"
#include "BCFTopicDlg.h"
#include "BCFViewControls.h"

#include <map>

class CBCFView;
class _model;

class CBCFTopicForm : public CWnd
{
public:
	BOOL Create(CBCFView* pane);
	void Load(BCFTopic* topic);
	bool Commit();
	BCFTopic* GetTopic() const { return m_topic; }
	BCFComment* GetSelectedComment() const;
	void ReloadComments(BCFComment* selectComment = nullptr);

protected:
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnTabChanged(NMHDR* header, LRESULT* result);
	afx_msg void OnCommentChanged();
	afx_msg void OnCommentDoubleClick();
	afx_msg void OnViewProject();
	afx_msg void OnSelectSnippetFile();
	afx_msg void OnSelectTopicLabels();
	afx_msg void OnAddBimFiles();
	afx_msg void OnCheckBimFiles();
	afx_msg void OnAddDocument();
	afx_msg void OnRemoveDocument();
	afx_msg void OnDocumentChanged();
	afx_msg void OnAddLink();
	afx_msg void OnRemoveLink();
	afx_msg void OnLinkChanged();
	afx_msg void OnAddRelatedTopic();
	afx_msg void OnRemoveRelatedTopic();
	afx_msg void OnRelatedTopicChanged();
	afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd* window, UINT controlColor);
	DECLARE_MESSAGE_MAP()

private:
	void AdjustLayout();
	void ShowTab(int tab);
	void LoadExtension(CComboBox& combo, BCFEnumeration enumeration);
	void FormatTopicInfo();
	void UpdateLabels();
	void ReloadBimFiles();
	bool AddBimFile(const CString& path);
	void ReloadDocuments(BCFDocumentReference* selectDocument = nullptr);
	BCFDocumentReference* GetSelectedDocument() const;
	void ReloadLinks(int selection = LB_ERR);
	void ReloadRelatedTopics(BCFTopic* selectTopic = nullptr);
	BCFTopic* GetSelectedRelatedTopic() const;

	CBCFView* m_pane = nullptr;
	BCFTopic* m_topic = nullptr;
	CButton m_viewProject;
	CStatic m_topicInfo;
	CStatic m_separator;
	CTabCtrl m_tabs;
	CBCFEdit m_title;
	CStatic m_descriptionLabel;
	CBCFEdit m_description;
	CStatic m_attributeLabels[12];
	CComboBox m_type;
	CComboBox m_stage;
	CComboBox m_status;
	CComboBox m_assigned;
	CComboBox m_priority;
	CBCFEdit m_due;
	CStatic m_labelsLabel;
	CBCFEdit m_labels;
	CButton m_selectTopicLabels;
	CComboBox m_snippetType;
	CButton m_snippetExternal;
	CButton m_selectSnippetFile;
	CBCFEdit m_snippetReference;
	CBCFEdit m_snippetSchema;
	CBCFEdit m_index;
	CBCFEdit m_serverId;
	CCheckListBox m_bimFiles;
	CButton m_addBimFiles;
	std::map<_model*, BCFBimFile*> m_usedBimModels;
	CBCFCommentsListBox m_comments;
	CListBox m_documents;
	CButton m_addDocument;
	CButton m_removeDocument;
	CListBox m_links;
	CButton m_addLink;
	CButton m_removeLink;
	CListBox m_relatedTopics;
	CButton m_addRelatedTopic;
	CButton m_removeRelatedTopic;
};
