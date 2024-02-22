#pragma once


// CMediaPlayerLinkDlg 对话框

class CMediaPlayerLinkDlg : public CDialog
{
    DECLARE_DYNAMIC(CMediaPlayerLinkDlg)

public:
    CMediaPlayerLinkDlg(CWnd * pParent = nullptr);   // 标准构造函数
    virtual ~CMediaPlayerLinkDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DLG_LINK };
#endif

protected:
    virtual void DoDataExchange(CDataExchange * pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    CString GetLinkName();

private:
    CEdit m_edtLinkName;
    CString m_strLinkName;
    CFont m_fontSize;

public:
    afx_msg void OnBnClickedBtnLinkConfirm();
    afx_msg void OnBnClickedBtnLinkCancel();
};
