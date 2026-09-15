#pragma once

#include "bcfAPI.h"

#include <map>

class CMySTEPViewerDoc;
class _model;
class CBCFCommentForm;
class CBCFEdit;
class CBCFProjectForm;
class CBCFTopicForm;

class CBCFView : public CDockablePane
{
public:
	CBCFView();
	virtual ~CBCFView();

	void SetDocument(CMySTEPViewerDoc* document) { m_stepViewerDoc = document; }
	void Activate();
	void NewProject();
	void OpenProject();
	bool SaveProject();
	void AddTopic();
	void DeleteTopic();
	void ShowTopicDetails();
	bool AskAndSaveModified();
	void CloseProject(bool prompt);
	void OnCloseMainDocument();
	void ShowProject();
	void ShowTopic(BCFTopic* topic);
	void ShowComment(BCFComment* comment);

	BCFProject* GetProject() const { return m_project; }
	CMySTEPViewerDoc* GetDocument() const { return m_stepViewerDoc; }
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
	afx_msg void OnProjectSettings();
	afx_msg void OnUpdateProjectSettings(CCmdUI* commandUI);
	DECLARE_MESSAGE_MAP()

private:
	enum Form { ProjectForm, TopicForm, CommentForm };
	bool CommitCurrent();
	void ReleaseProject();
	void ShowForm(Form form);
	void AdjustLayout();
	void UpdateCaption();

	CMySTEPViewerDoc* m_stepViewerDoc;
	BCFProject* m_project;
	CString m_filePath;
	CString m_email;
	std::map<BCFBimFile*, _model*> m_bimModels;
	Form m_activeForm;
	CFont m_dialogFont;
	CStatic m_projectIdLabel;
	CStatic m_projectNameLabel;
	CButton m_projectSettings;
	CBCFEdit* m_projectId;
	CBCFEdit* m_projectName;
	CBCFProjectForm* m_projectForm;
	CBCFTopicForm* m_topicForm;
	CBCFCommentForm* m_commentForm;
};
