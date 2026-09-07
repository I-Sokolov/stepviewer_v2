#include "stdafx.h"

#include "BCFCommentForm.h"
#include "BCFView.h"

BEGIN_MESSAGE_MAP(CBCFCommentForm, CWnd)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_PANE_COMMENT_VIEW_TOPIC, &CBCFCommentForm::OnViewTopic)
END_MESSAGE_MAP()

BOOL CBCFCommentForm::Create(CBCFView* pane)
{
	m_pane = pane;
	if (!CreateEx(0, RegisterBCFPaneClass(), L"", WS_CHILD | WS_CLIPCHILDREN, CRect(), pane, 0)) {
		return FALSE;
	}
	m_viewTopic.Create(L"<<", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_COMMENT_VIEW_TOPIC);
	SetBCFControlFont(m_viewTopic, this);
	CreateBCFStaticLabel(m_headerInfo, L"Comment to topic", this);
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
	UpdateHeader();
	m_text.SetWindowText(comment ? FromUTF8(comment->GetText()) : CString());
}

void CBCFCommentForm::UpdateHeader()
{
	if (!m_comment) {
		m_headerInfo.SetWindowText(CString());
		return;
	}

	CString created = FormatBCFCommentCreated(*m_comment);
	created.SetAt(0, L'c');
	CString modified = FormatBCFCommentModified(*m_comment);
	if (!modified.IsEmpty()) {
		modified.SetAt(0, L'm');
	}

	CString header;
	header.Format(L"Comment to topic '%s' %s", FromUTF8(m_comment->GetTopic().GetTitle()).GetString(),
		created.GetString());
	if (!modified.IsEmpty()) {
		header += L", ";
		header += modified;
	}
	m_headerInfo.SetWindowText(header);
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
		UpdateHeader();
	}
	return ok;
}

void CBCFCommentForm::OnViewTopic()
{
	if (m_comment && Commit()) {
		m_pane->ShowTopic(&m_comment->GetTopic());
	}
}

void CBCFCommentForm::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);
	if (!m_text.GetSafeHwnd()) {
		return;
	}

	CClientDC dc(this);
	CFont* font = m_headerInfo.GetFont();
	CFont* oldFont = font ? dc.SelectObject(font) : nullptr;
	TEXTMETRIC metrics = {};
	dc.GetTextMetrics(&metrics);
	const int textHeight = metrics.tmAscent + metrics.tmDescent + metrics.tmExternalLeading;
	const int rowHeight = textHeight + textHeight / 5;
	const int margin = rowHeight / 3;
	const int rowSpacing = margin;
	const int labelOffset = (rowHeight - textHeight) / 2;
	CString buttonText;
	m_viewTopic.GetWindowText(buttonText);
	const int buttonWidth = static_cast<int>(dc.GetTextExtent(buttonText).cx) + 2 * margin;
	if (oldFont) {
		dc.SelectObject(oldFont);
	}

	m_viewTopic.MoveWindow(0, 0, buttonWidth, rowHeight);
	m_headerInfo.MoveWindow(buttonWidth + margin, labelOffset,
		max(rowHeight, cx - buttonWidth - 2 * margin), textHeight);
	const int textLabelTop = rowHeight + rowSpacing;
	m_textLabel.MoveWindow(margin, textLabelTop + labelOffset,
		max(rowHeight, cx - 2 * margin), textHeight);
	const int textTop = textLabelTop + rowHeight;
	m_text.MoveWindow(margin, textTop + labelOffset,
		max(rowHeight, cx - 2 * margin),
		max(rowHeight, cy - textTop - labelOffset - margin));
}
