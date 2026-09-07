#include "stdafx.h"

#include "BCFCommentForm.h"
#include "BCFView.h"

BEGIN_MESSAGE_MAP(CBCFCommentForm, CWnd)
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CBCFCommentForm::Create(CBCFView* pane)
{
	m_pane = pane;
	if (!CreateEx(0, RegisterBCFPaneClass(), L"", WS_CHILD | WS_CLIPCHILDREN, CRect(), pane, 0)) {
		return FALSE;
	}
	CreateBCFStaticLabel(m_createdInfo, L"Created by", this);
	CreateBCFStaticLabel(m_modifiedInfo, L"Modified by", this);
	CreateBCFStaticLabel(m_textLabel, L"Comment:", this);
	m_text.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL |
		ES_WANTRETURN | WS_VSCROLL, CRect(), this, IDC_PANE_COMMENT_TEXT);
	SetBCFControlFont(m_text, this);
	return TRUE;
}

void CBCFCommentForm::Load(BCFComment* comment)
{
	m_comment = comment;
	if (!GetSafeHwnd()) {
		return;
	}
	m_createdInfo.SetWindowText(comment ? FormatBCFCommentCreated(*comment) : CString());
	m_modifiedInfo.SetWindowText(comment ? FormatBCFCommentModified(*comment) : CString());
	m_text.SetWindowText(comment ? FromUTF8(comment->GetText()) : CString());
}

bool CBCFCommentForm::Commit()
{
	if (!m_comment) {
		return true;
	}
	CString text;
	m_text.GetWindowText(text);
	bool ok = m_comment->SetText(ToUTF8(text).c_str());
	m_pane->ShowLog(!ok);
	if (ok) {
		m_createdInfo.SetWindowText(FormatBCFCommentCreated(*m_comment));
		m_modifiedInfo.SetWindowText(FormatBCFCommentModified(*m_comment));
	}
	return ok;
}

void CBCFCommentForm::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);
	if (!m_text.GetSafeHwnd()) {
		return;
	}
	const int margin = 8;
	m_createdInfo.MoveWindow(margin, margin, max(20, cx - 2 * margin), 18);
	m_modifiedInfo.MoveWindow(margin, margin + 20, max(20, cx - 2 * margin), 18);
	m_textLabel.MoveWindow(margin, margin + 45, 80, 18);
	m_text.MoveWindow(margin, margin + 65, max(20, cx - 2 * margin), max(25, cy - 3 * margin - 65));
}
