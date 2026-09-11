#include "stdafx.h"

#include "BCFSnapshotCtrl.h"

BEGIN_MESSAGE_MAP(CBCFSnapshotCtrl, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()

bool CBCFSnapshotCtrl::Load(const CString& path)
{
	Clear();
	if (path.IsEmpty()) {
		return true;
	}

	const HRESULT result = m_image.Load(path);
	if (FAILED(result)) {
		m_image.Destroy();
	}
	Invalidate(FALSE);
	return SUCCEEDED(result);
}

void CBCFSnapshotCtrl::Clear()
{
	m_image.Destroy();
	if (GetSafeHwnd()) {
		Invalidate(FALSE);
	}
}

void CBCFSnapshotCtrl::OnPaint()
{
	CPaintDC dc(this);
	CRect client;
	GetClientRect(client);
	dc.FillSolidRect(client, ::GetSysColor(COLOR_WINDOW));
	dc.DrawEdge(client, EDGE_SUNKEN, BF_RECT);
	client.DeflateRect(2, 2);

	if (m_image.IsNull() || client.IsRectEmpty()) {
		return;
	}

	const int imageWidth = m_image.GetWidth();
	const int imageHeight = m_image.GetHeight();
	if (imageWidth <= 0 || imageHeight <= 0) {
		return;
	}

	int width = client.Width();
	int height = client.Height();
	if (static_cast<__int64>(imageWidth) * height >
		static_cast<__int64>(imageHeight) * width) {
		height = MulDiv(width, imageHeight, imageWidth);
	}
	else {
		width = MulDiv(height, imageWidth, imageHeight);
	}

	CRect destination(
		client.left + (client.Width() - width) / 2,
		client.top + (client.Height() - height) / 2,
		client.left + (client.Width() - width) / 2 + width,
		client.top + (client.Height() - height) / 2 + height);
	const int oldMode = dc.SetStretchBltMode(HALFTONE);
	m_image.Draw(dc.GetSafeHdc(), destination);
	dc.SetStretchBltMode(oldMode);
}
