#pragma once

#include <atlimage.h>

class CBCFSnapshotCtrl : public CStatic
{
public:
	bool Load(const CString& path);
	void Clear();

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

private:
	CImage m_image;
};
