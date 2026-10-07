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
	bool OpenProject(LPCTSTR filePath= NULL);
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
	CString GetBimModel(BCFBimFile& file, _model** ppLoadedModel = NULL);
	void SetBimFilesToView(BCFTopic& topic);
	void ShowLog(bool knownError);
	void LoadProjectInfo();
	bool CommitProjectInfo();

protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
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
	BCFTopic* CreateTopic();
	bool CloseActiveProjectForOperation(LPCTSTR operation);

	CMySTEPViewerDoc* m_stepViewerDoc;
	BCFProject* m_project;
	CString m_filePath;
	CString m_email;
	std::map<BCFBimFile*, CString> m_bimModels; //maps to loaded path file, we can not keep _model* if may be deleted by controller
	Form m_activeForm;
	CFont m_dialogFont;
	CStatic m_emptyMessage;
	CStatic m_projectIdLabel;
	CStatic m_projectNameLabel;
	CButton m_projectSettings;
	CBCFEdit* m_projectId;
	CBCFEdit* m_projectName;
	CBCFProjectForm* m_projectForm;
	CBCFTopicForm* m_topicForm;
	CBCFCommentForm* m_commentForm;
};
