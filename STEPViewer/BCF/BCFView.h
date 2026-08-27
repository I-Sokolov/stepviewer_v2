#pragma once

#include "bcfAPI.h"
#include "BCFTopicDlg.h"

#include <map>

class CMySTEPViewerDoc;
class _model;
class CBCFView;

class CBCFEdit : public CEdit
{
protected:
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* oldWnd);
	afx_msg void OnKillFocus(CWnd* newWnd);
	DECLARE_MESSAGE_MAP()
};

class CBCFPaneMenuBar : public CMFCMenuBar
{
public:
	void SetMessageWnd(CWnd* messageWnd)
	{
		for (int i = 0; i < GetCount(); ++i) {
			if (auto button = dynamic_cast<CMFCToolBarMenuButton*>(GetButton(i))) {
				button->SetMessageWnd(messageWnd);
			}
		}
	}

	virtual void OnUpdateCmdUI(CFrameWnd*, BOOL disableIfNoHandler) override
	{
		CMFCMenuBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), disableIfNoHandler);
	}
	virtual BOOL LoadState(LPCTSTR = nullptr, int = -1, UINT = static_cast<UINT>(-1)) override { return TRUE; }
	virtual BOOL SaveState(LPCTSTR = nullptr, int = -1, UINT = static_cast<UINT>(-1)) override { return TRUE; }
	virtual BOOL AllowShowOnList() const override { return FALSE; }
};

class CBCFProjectForm : public CWnd
{
public:
	BOOL Create(CBCFView* pane);
	void Load(BCFTopic* selectTopic = nullptr);
	bool Commit();
	BCFTopic* GetSelectedTopic() const;

protected:
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnTopicChanged(NMHDR* header, LRESULT* result);
	afx_msg void OnTopicDoubleClick(NMHDR* header, LRESULT* result);
	DECLARE_MESSAGE_MAP()

private:
	CBCFView* m_pane = nullptr;
	CStatic m_topicsLabel;
	CListCtrl m_topics;
};

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
	afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd* window, UINT controlColor);
	DECLARE_MESSAGE_MAP()

private:
	void AdjustLayout();
	void ShowTab(int tab);
	void LoadExtension(CComboBox& combo, BCFEnumeration enumeration);
	void UpdateMetadata();

	CBCFView* m_pane = nullptr;
	BCFTopic* m_topic = nullptr;
	CStatic m_topicInfo;
	CTabCtrl m_tabs;
	CStatic m_titleLabel;
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
	CComboBox m_snippet;
	CBCFEdit m_reference;
	CBCFEdit m_schema;
	CBCFEdit m_index;
	CBCFEdit m_serverId;
	CBCFCommentsListBox m_comments;
	CStatic m_documentsPlaceholder;
	CStatic m_linksPlaceholder;
};

class CBCFCommentForm : public CWnd
{
public:
	BOOL Create(CBCFView* pane);
	void Load(BCFComment* comment);
	bool Commit();
	BCFComment* GetComment() const { return m_comment; }

protected:
	afx_msg void OnSize(UINT type, int cx, int cy);
	DECLARE_MESSAGE_MAP()

private:
	CBCFView* m_pane = nullptr;
	BCFComment* m_comment = nullptr;
	CStatic m_createdInfo;
	CStatic m_modifiedInfo;
	CStatic m_textLabel;
	CBCFEdit m_text;
};

class CBCFView : public CDockablePane
{
public:
	CBCFView();
	virtual ~CBCFView();

	void SetDocument(CMySTEPViewerDoc* document) { m_document = document; }
	void Activate();
	void NewProject();
	void OpenProject();
	bool AskAndSaveModified();
	void CloseProject(bool prompt);
	void OnCloseMainDocument();
	void ShowProject();
	void ShowTopic(BCFTopic* topic);
	void ShowComment(BCFComment* comment);
	void RefreshCommandUI();

	BCFProject* GetProject() const { return m_project; }
	CMySTEPViewerDoc* GetDocument() const { return m_document; }
	_model* GetBimModel(BCFBimFile& file);
	void LoadBimFiles(BCFTopic& topic);
	void ShowLog(bool knownError);
	void LoadProjectInfo();
	bool CommitProjectInfo();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT createStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* dc);
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* oldWnd);
	afx_msg void OnNewFile();
	afx_msg void OnOpenFile();
	afx_msg void OnSaveFile();
	afx_msg void OnAddTopic();
	afx_msg void OnDeleteTopic();
	afx_msg void OnTopicDetails();
	afx_msg void OnViewProject();
	afx_msg void OnViewTopic();
	afx_msg void OnViewComment();
	afx_msg void OnSaveComment();
	afx_msg void OnDeleteComment();
	afx_msg void OnUpdateProjectCommand(CCmdUI* commandUI);
	afx_msg void OnUpdateTopicCommand(CCmdUI* commandUI);
	afx_msg void OnUpdateCommentCommand(CCmdUI* commandUI);
	afx_msg void OnUpdateViewProject(CCmdUI* commandUI);
	afx_msg void OnUpdateViewTopic(CCmdUI* commandUI);
	afx_msg void OnUpdateViewComment(CCmdUI* commandUI);
	DECLARE_MESSAGE_MAP()

private:
	enum Form { ProjectForm, TopicForm, CommentForm };
	bool SaveProject();
	bool CommitCurrent();
	void ReleaseProject();
	void ShowForm(Form form);
	void AdjustLayout();
	void UpdateCaption();

	CMySTEPViewerDoc* m_document;
	BCFProject* m_project;
	CString m_filePath;
	CString m_email;
	std::map<BCFBimFile*, _model*> m_bimModels;
	Form m_activeForm;
	CMenu m_menu;
	CBCFPaneMenuBar m_menuBar;
	CFont m_dialogFont;
	CStatic m_projectIdLabel;
	CStatic m_projectNameLabel;
	CBCFEdit m_projectId;
	CBCFEdit m_projectName;
	CBCFProjectForm m_projectForm;
	CBCFTopicForm m_topicForm;
	CBCFCommentForm m_commentForm;
};
