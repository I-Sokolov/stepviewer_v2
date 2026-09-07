#pragma once

#include "bcfAPI.h"

enum BCFViewControlId
{
	IDC_PANE_PROJECT_ID = 2100,
	IDC_PANE_PROJECT_NAME,
	IDC_PANE_TOPICS,
	IDC_PANE_TABS,
	IDC_PANE_VIEW_PROJECT,
	IDC_PANE_TOPIC_TITLE,
	IDC_PANE_TOPIC_DESCRIPTION,
	IDC_PANE_TOPIC_TYPE,
	IDC_PANE_TOPIC_STAGE,
	IDC_PANE_TOPIC_STATUS,
	IDC_PANE_TOPIC_ASSIGNED,
	IDC_PANE_TOPIC_PRIORITY,
	IDC_PANE_TOPIC_DUE,
	IDC_PANE_TOPIC_SNIPPET,
	IDC_PANE_TOPIC_REFERENCE,
	IDC_PANE_TOPIC_SCHEMA,
	IDC_PANE_TOPIC_INDEX,
	IDC_PANE_TOPIC_SERVER_ID,
	IDC_PANE_SELECT_SNIPPET_FILE,
	IDC_PANE_SELECT_TOPIC_LABELS,
	IDC_PANE_BIM_FILES,
	IDC_PANE_ADD_BIM_FILES,
	IDC_PANE_COMMENTS,
	IDC_PANE_COMMENT_TEXT
};

LPCTSTR RegisterBCFPaneClass();
void SetBCFControlFont(CWnd& control, CWnd* owner);
void SetBCFComboValue(CComboBox& combo, const CString& value);
BOOL CreateBCFStaticLabel(CStatic& label, LPCTSTR text, CWnd* parent);
CString FormatBCFCommentCreated(BCFComment& comment);
CString FormatBCFCommentModified(BCFComment& comment);

class CBCFSelectFileDlg : public CFileDialog
{
public:
	CBCFSelectFileDlg(LPCTSTR filePath, bool external, CWnd* parent);

	bool IsExternal() const { return m_external; }

protected:
	virtual BOOL OnFileNameOK() override;

private:
	enum
	{
		ModeControl = 1,
		ExternalFileMode,
		EmbedFileMode
	};

	bool m_external;
};

class CBCFEdit : public CEdit
{
protected:
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* oldWnd);
	afx_msg void OnKillFocus(CWnd* newWnd);
	DECLARE_MESSAGE_MAP()
};
