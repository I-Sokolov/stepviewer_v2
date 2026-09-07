#pragma once

#include "bcfAPI.h"
#include "BCFViewControls.h"

class CBCFView;

class CBCFCommentForm : public CWnd
{
public:
	BOOL Create(CBCFView* pane);
	void Load(BCFComment* comment);
	bool Commit();
	BCFComment* GetComment() const { return m_comment; }

protected:
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnViewTopic();
	DECLARE_MESSAGE_MAP()

private:
	void UpdateHeader();

	CBCFView* m_pane = nullptr;
	BCFComment* m_comment = nullptr;
	CButton m_viewTopic;
	CStatic m_headerInfo;
	CStatic m_textLabel;
	CBCFEdit m_text;
};
