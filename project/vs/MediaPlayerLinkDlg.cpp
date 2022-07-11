// MediaPlayerLinkDlg.cpp: 实现文件
//

#include "pch.h"
#include "MediaPlayer.h"
#include "MediaPlayerLinkDlg.h"
#include "afxdialogex.h"


// CMediaPlayerLinkDlg 对话框

IMPLEMENT_DYNAMIC(CMediaPlayerLinkDlg, CDialog)

CMediaPlayerLinkDlg::CMediaPlayerLinkDlg(CWnd * pParent /*=nullptr*/)
    : CDialog(IDD_DLG_LINK, pParent)
{

}

CMediaPlayerLinkDlg::~CMediaPlayerLinkDlg()
{
}

void CMediaPlayerLinkDlg::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EET_NET_LINK, m_edtLinkName);
}


BEGIN_MESSAGE_MAP(CMediaPlayerLinkDlg, CDialog)
    ON_BN_CLICKED(IDC_BTN_LINK_CONFIRM, &CMediaPlayerLinkDlg::OnBnClickedBtnLinkConfirm)
    ON_BN_CLICKED(IDC_BTN_LINK_CANCEL, &CMediaPlayerLinkDlg::OnBnClickedBtnLinkCancel)
END_MESSAGE_MAP()


// CMediaPlayerLinkDlg 消息处理程序


BOOL CMediaPlayerLinkDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // TODO:  在此添加额外的初始化
    m_fontSize.CreatePointFont(120, _T("微软雅黑"), NULL);
    GetDlgItem(IDC_STC_NET_LINK)->SetFont(&m_fontSize);
    GetDlgItem(IDC_BTN_LINK_CONFIRM)->SetFont(&m_fontSize);
    GetDlgItem(IDC_BTN_LINK_CANCEL)->SetFont(&m_fontSize);
    //m_edtLinkName.SetFont(&m_fontSize);
    m_edtLinkName.SetLimitText(MAX_PATH);

    return TRUE;
}


void CMediaPlayerLinkDlg::OnBnClickedBtnLinkConfirm()
{
    m_edtLinkName.GetWindowText(m_strLinkName);
    CDialog::OnOK();
}


void CMediaPlayerLinkDlg::OnBnClickedBtnLinkCancel()
{
    m_strLinkName.SetString(_T(""));
    CDialog::OnCancel();
}

CString CMediaPlayerLinkDlg::getLinkName()
{
    return m_strLinkName;
}
