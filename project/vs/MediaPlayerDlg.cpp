﻿
// MediaPlayerDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "MediaPlayer.h"
#include "MediaPlayerDlg.h"
#include "MediaPlayerLinkDlg.h"
#include "log.h"
#include "utils/MediaPlayerUtils.h"
#include "utils/MediaPlayerRecord.h"
#include "afxdialogex.h"
#include <io.h>

#include "../src/MediaPlayerImpl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

protected:
    virtual void DoDataExchange(CDataExchange * pDX);    // DDX/DDV 支持

// 实现
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange * pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMediaPlayerDlg 对话框



CMediaPlayerDlg::CMediaPlayerDlg(CWnd * pParent /*=nullptr*/)
    : CDialogEx(IDD_DLG_PLAYER, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMediaPlayerDlg::DoDataExchange(CDataExchange * pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LST_PLAY, m_lstRecord);
    DDX_Control(pDX, IDC_BTN_CTRL, m_btnCtrl);
    DDX_Control(pDX, IDC_BTN_STOP, m_btnStop);
    DDX_Control(pDX, IDC_BTN_FORWARD, m_btnForward);
    DDX_Control(pDX, IDC_BTN_BACKWARD, m_btnBackward);
    DDX_Control(pDX, IDC_BTN_OPEN, m_btnOpen);
    DDX_Control(pDX, IDC_STC_DURATION, m_stcDuration);
    DDX_Control(pDX, IDC_BTN_LAST, m_btnLast);
    DDX_Control(pDX, IDC_BTN_NEXT, m_btnNext);
    DDX_Control(pDX, IDC_BTN_VOICE, m_btnVoice);
    DDX_Control(pDX, IDC_SLD_PROGRESS, m_sldProgress);
    DDX_Control(pDX, IDC_SLD_VOICE, m_sldVoice);
    DDX_Control(pDX, IDC_BTN_LINK, m_btnLink);
}

BEGIN_MESSAGE_MAP(CMediaPlayerDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_CTRL, &CMediaPlayerDlg::OnBnClickedBtnCtrl)
    ON_BN_CLICKED(IDC_BTN_STOP, &CMediaPlayerDlg::OnBnClickedBtnStop)
    ON_BN_CLICKED(IDC_BTN_FORWARD, &CMediaPlayerDlg::OnBnClickedBtnForward)
    ON_BN_CLICKED(IDC_BTN_BACKWARD, &CMediaPlayerDlg::OnBnClickedBtnBackward)
    ON_BN_CLICKED(IDC_BTN_OPEN, &CMediaPlayerDlg::OnBnClickedBtnOpen)
    ON_BN_CLICKED(IDC_BTN_LAST, &CMediaPlayerDlg::OnBnClickedBtnLast)
    ON_BN_CLICKED(IDC_BTN_NEXT, &CMediaPlayerDlg::OnBnClickedBtnNext)
    ON_BN_CLICKED(IDC_BTN_VOICE, &CMediaPlayerDlg::OnBnClickedBtnVoice)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLD_VOICE, &CMediaPlayerDlg::OnNMCustomdrawSldVoice)
    ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLD_PROGRESS, &CMediaPlayerDlg::OnNMReleasedcaptureSldProgress)
    ON_WM_TIMER()
    ON_NOTIFY(NM_DBLCLK, IDC_LST_PLAY, &CMediaPlayerDlg::OnNMDblclkLstPlay)
    ON_NOTIFY(NM_RCLICK, IDC_LST_PLAY, &CMediaPlayerDlg::OnNMRClickLstPlay)
    ON_COMMAND(IDC_BTN_RECORD_OPEN, &CMediaPlayerDlg::OnBtnRecordOpen)
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_BTN_LINK, &CMediaPlayerDlg::OnBnClickedBtnLink)
    ON_WM_CLOSE()
END_MESSAGE_MAP()


// CMediaPlayerDlg 消息处理程序

BOOL CMediaPlayerDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 将“关于...”菜单项添加到系统菜单中。

    // IDM_ABOUTBOX 必须在系统命令范围内。
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu * pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);         // 设置大图标
    SetIcon(m_hIcon, FALSE);        // 设置小图标

    // TODO: 在此添加额外的初始化代码
    std::string work_path = media_utils_get_work_path();
    std::string log_path = work_path + "MediaPlayer.log";
    if (0 == _access(log_path.c_str(), 0))
        remove(log_path.c_str());
    log_msg_init(log_path.c_str(), LOG_LEVEL_INFO);
    atexit(log_msg_uninit);

    m_toolTip.Create(this);
    m_toolTip.SetDelayTime(TTDT_INITIAL, 10);
    m_toolTip.SetMaxTipWidth(200);
    m_toolTip.SetTipBkColor(BACK_COLOR);
    m_toolTip.SetTipTextColor(FONT_COLOR);

    m_toolTip.AddTool(GetDlgItem(IDC_BTN_CTRL), _T("播放/暂停"));
    m_toolTip.AddTool(GetDlgItem(IDC_BTN_STOP), _T("停止"));
    m_toolTip.AddTool(GetDlgItem(IDC_BTN_FORWARD), _T("快进"));
    m_toolTip.AddTool(GetDlgItem(IDC_BTN_BACKWARD), _T("快退"));
    m_toolTip.AddTool(GetDlgItem(IDC_BTN_LAST), _T("上一个"));
    m_toolTip.AddTool(GetDlgItem(IDC_BTN_NEXT), _T("下一个"));
    m_toolTip.AddTool(GetDlgItem(IDC_BTN_VOICE), _T("静音 开/关"));
    m_toolTip.AddTool(GetDlgItem(IDC_BTN_LINK), _T("链接"));
    m_toolTip.AddTool(GetDlgItem(IDC_BTN_OPEN), _T("打开"));

    m_fontSize.CreatePointFont(120, _T("微软雅黑"), nullptr);
    m_lstRecord.SetFont(&m_fontSize);
    m_lstRecord.SetBkColor(BACK_COLOR);
    m_lstRecord.SetTextBkColor(BACK_COLOR);
    m_lstRecord.SetTextColor(FONT_COLOR);
    m_lstRecord.InsertItem(0, _T("          播放记录"));
    m_lstRecord.InsertItem(1, _T("张韶涵-阿刁"));
    m_brush.CreateSolidBrush(BACK_COLOR);
    m_stcDuration.SetFont(&m_fontSize);
    m_sldProgress.SetPageSize(1);
    m_sldProgress.SetTic(1);
    m_sldProgress.SetTicFreq(1);

    CRect btnRect;
    m_btnOpen.GetWindowRect(&btnRect);
    m_icoPause = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICO_PAUSE), IMAGE_ICON,
                                  btnRect.Width() - GAP_SIZE, btnRect.Height() - GAP_SIZE, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    m_icoStart = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICO_PLAY), IMAGE_ICON,
                                  btnRect.Width() - GAP_SIZE, btnRect.Height() - GAP_SIZE, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    m_icoStop = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICO_STOP), IMAGE_ICON,
                                 btnRect.Width() - GAP_SIZE, btnRect.Height() - GAP_SIZE, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    m_icoForward = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICO_FORWARD), IMAGE_ICON,
                                    btnRect.Width() - GAP_SIZE, btnRect.Height() - GAP_SIZE, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    m_icoBackward = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICO_BACKWARD), IMAGE_ICON,
                                     btnRect.Width() - GAP_SIZE, btnRect.Height() - GAP_SIZE, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    m_icoLast = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICO_LAST), IMAGE_ICON,
                                 btnRect.Width() - GAP_SIZE, btnRect.Height() - GAP_SIZE, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    m_icoNext = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICO_NEXT), IMAGE_ICON,
                                 btnRect.Width() - GAP_SIZE, btnRect.Height() - GAP_SIZE, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    m_icoVoice = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICO_VOICE), IMAGE_ICON,
                                  btnRect.Width() - GAP_SIZE, btnRect.Height() - GAP_SIZE, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    m_icoSilence = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICO_SILENCE), IMAGE_ICON,
                                    btnRect.Width() - GAP_SIZE, btnRect.Height() - GAP_SIZE, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    m_icoOpen = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICO_FOLDER), IMAGE_ICON,
                                 btnRect.Width() - GAP_SIZE, btnRect.Height() - GAP_SIZE, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    m_icoLink = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICO_LINK), IMAGE_ICON,
                                 btnRect.Width() - GAP_SIZE, btnRect.Height() - GAP_SIZE, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION);
    m_btnCtrl.SetIcon(m_icoPause);
    m_btnStop.SetIcon(m_icoStop);
    m_btnForward.SetIcon(m_icoForward);
    m_btnBackward.SetIcon(m_icoBackward);
    m_btnLast.SetIcon(m_icoLast);
    m_btnNext.SetIcon(m_icoNext);
    m_btnVoice.SetIcon(m_icoVoice);
    m_btnOpen.SetIcon(m_icoOpen);
    m_btnLink.SetIcon(m_icoLink);
    m_sldVoice.SetRange(0, 128);
    m_sldVoice.SetTicFreq(1);
    m_sldVoice.SetPos(80);

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMediaPlayerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMediaPlayerDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 用于绘制的设备上下文

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 使图标在工作区矩形中居中
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 绘制图标
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CRect rect;
        CPaintDC dc(this);
        GetClientRect(rect);
        dc.FillSolidRect(rect, BACK_COLOR);  //设置背景颜色
    }
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMediaPlayerDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}



HBRUSH CMediaPlayerDlg::OnCtlColor(CDC * pDC, CWnd * pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    if (nCtlColor == CTLCOLOR_STATIC)
    {
        pDC->SetTextColor(RGB(255, 255, 255));
        pDC->SetBkColor(BACK_COLOR);
        return (HBRUSH)m_brush.GetSafeHandle();
    }

    return hbr;
}


void CMediaPlayerDlg::OnBnClickedBtnCtrl()
{
    auto & media = CMediaPlayerImpl::getInstance();
    if (!media.opened())
    {
        MessageBox(_T("没有可播放的文件"), _T("提示"), MB_OK | MB_ICONWARNING);
        return;
    }

    if (!m_blPlaying)
    {
        CRect rect;
        CWnd * pScreen = GetDlgItem(IDC_STC_SCREEN);
        pScreen->GetClientRect(&rect);

        if (!media.start(pScreen->GetSafeHwnd(), rect.Width(), rect.Height()))
        {
            MessageBox(_T("播放文件失败！"), _T("提示"), MB_OK | MB_ICONWARNING);
            return;
        }
        m_btnCtrl.SetIcon(m_icoStart);
    }
    else
    {
        media.pause();
        m_btnCtrl.SetIcon(m_icoPause);
    }
    m_blPlaying = !m_blPlaying;
}


void CMediaPlayerDlg::OnBnClickedBtnStop()
{
    KillTimer(1);
    auto & media = CMediaPlayerImpl::getInstance();
    if (media.opened())
    {
        media.setVolume(0);
        media.close();
    }
    m_blPlaying = false;
    m_btnCtrl.SetIcon(m_icoPause);
    m_stcDuration.SetWindowText(_T("00:00:00/00:00:00"));
    GetDlgItem(IDC_STC_SCREEN)->ShowWindow(TRUE);
    m_sldProgress.SetPos(0);
}


void CMediaPlayerDlg::OnBnClickedBtnForward()
{
    CMediaPlayerImpl::getInstance().forward();
}


void CMediaPlayerDlg::OnBnClickedBtnBackward()
{
    CMediaPlayerImpl::getInstance().backward();
}


void CMediaPlayerDlg::OnBnClickedBtnOpen()
{
    CString strExtFilter = _T("MP4|*.mp4|MKV|*.mkv|MPEGTS|*.ts|All Files|*.*||");
    CFileDialog dlg(TRUE, _T("MP4"), nullptr, OFN_READONLY | OFN_FILEMUSTEXIST, strExtFilter);
    if (IDOK != dlg.DoModal())
    {
        return;
    }

    CString strFilePath = dlg.GetPathName();
    auto & media = CMediaPlayerImpl::getInstance();

    if (media.opened())
    {
        OnBnClickedBtnStop();
    }

    m_strFilePath = media_utils_wchar_to_utf8(strFilePath.GetBuffer());
    if (!media.open(m_strFilePath.c_str()))
    {
        MessageBox(_T("打开文件失败！"), _T("提示"), MB_OK | MB_ICONWARNING);
        return;
    }

    int64_t duration = media.getDuration();
    if (duration < 0)
    {
        MessageBox(_T("获取文件时长失败！"), _T("提示"), MB_OK | MB_ICONWARNING);
        return;
    }

    CString strVideoDuration;
    strTime(duration, 0, strVideoDuration);
    m_stcDuration.SetWindowText(strVideoDuration);
    m_sldProgress.SetRange(0, static_cast<int>(duration));

    SetTimer(1, 100, nullptr);

    OnBnClickedBtnCtrl();
}


BOOL CMediaPlayerDlg::PreTranslateMessage(MSG * pMsg)
{
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
        return TRUE;

    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
        return TRUE;

    if (m_toolTip.m_hWnd != NULL)
    {
        m_toolTip.RelayEvent(pMsg);
    }

    return CDialogEx::PreTranslateMessage(pMsg);
}


void CMediaPlayerDlg::OnBnClickedBtnLast()
{
    // TODO: 在此添加控件通知处理程序代码
}


void CMediaPlayerDlg::OnBnClickedBtnNext()
{
    // TODO: 在此添加控件通知处理程序代码
}


void CMediaPlayerDlg::OnBnClickedBtnVoice()
{
    int iVolume = 0;
    int iVoice = m_sldVoice.GetPos();
    if (iVoice > 0)
    {
        m_btnVoice.SetIcon(m_icoSilence);
        m_sldVoice.SetPos(0);
        iVolume = 0;
    }
    else
    {
        m_btnVoice.SetIcon(m_icoVoice);
        m_sldVoice.SetPos(64);
        iVolume = 64;
    }

    SendMessage(WM_PAINT, 0, 0);

    CMediaPlayerImpl::getInstance().setVolume(iVolume);
}


void CMediaPlayerDlg::OnNMCustomdrawSldVoice(NMHDR * pNMHDR, LRESULT * pResult)
{
    LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
    int iVoice = m_sldVoice.GetPos();
    if (iVoice <= 0)
        m_btnVoice.SetIcon(m_icoSilence);
    else
        m_btnVoice.SetIcon(m_icoVoice);
    auto & media = CMediaPlayerImpl::getInstance();
    if (media.opened())
        media.setVolume(iVoice);

    *pResult = 0;
}


void CMediaPlayerDlg::OnNMReleasedcaptureSldProgress(NMHDR * pNMHDR, LRESULT * pResult)
{
    // TODO: 在此添加控件通知处理程序代码
    CRect rect;
    m_sldProgress.GetWindowRect(&rect);

    POINT pt;
    GetCursorPos(&pt);

    const auto width = rect.Width();
    const auto tmp = pt.x - rect.left;
    const auto radix = static_cast<double>(tmp) / static_cast<double>(width);
    const auto range = m_sldProgress.GetRangeMax();
    int pos = static_cast<int>(ceil(static_cast<double>(range) * radix));
    if (pos > range)
        pos = range;
    m_sldProgress.SetPos(pos);
    CMediaPlayerImpl::getInstance().setPosition(pos);

    *pResult = 0;
}


void CMediaPlayerDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1)
    {
        auto & media = CMediaPlayerImpl::getInstance();
        if (media.opened())
        {
            int64_t pos = media.getPosition();
            if (pos < 0)
                return;
            int64_t duration = media.getDuration();
            m_sldProgress.SetPos(static_cast<int>(pos));
            CString strVideoDuration;
            strTime(duration, pos, strVideoDuration);
            m_stcDuration.SetWindowText(strVideoDuration);
            if (pos == duration)
            {
                // 播放到最后置为播放结束
                OnBnClickedBtnStop();
            }
        }
    }

    CDialogEx::OnTimer(nIDEvent);
}

void CMediaPlayerDlg::strTime(int64_t duration, int64_t ts, CString & str)
{
    int d_hour = static_cast<int>(duration / 3600);
    int d_minute = static_cast<int>(duration / 60 % 60);
    int d_second = static_cast<int>(duration % 60);
    int t_hour = static_cast<int>(ts / 3600);
    int t_minute = static_cast<int>(ts / 60 % 60);
    int t_second = static_cast<int>(ts % 60);
    str.Format(_T("%02d:%02d:%02d/%02d:%02d:%02d"), t_hour, t_minute, t_second, d_hour, d_minute, d_second);
}

void CMediaPlayerDlg::OnNMDblclkLstPlay(NMHDR * pNMHDR, LRESULT * pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: 在此添加控件通知处理程序代码



    *pResult = 0;
}


void CMediaPlayerDlg::OnNMRClickLstPlay(NMHDR * pNMHDR, LRESULT * pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: 在此添加控件通知处理程序代码
    int iSelectedItem = pNMItemActivate->iItem;
    if (iSelectedItem >= 0)
    {
        CMenu menu;
        menu.LoadMenu(IDR_MNU_RECORD);
        CMenu * pPopup = menu.GetSubMenu(0);
        CPoint m_point;
        GetCursorPos(&m_point);
        pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, m_point.x, m_point.y, this);
    }

    *pResult = 0;
}


void CMediaPlayerDlg::OnBtnRecordOpen()
{
    // TODO: 在此添加命令处理程序代码
    MessageBox(_T("Done Here"), _T("提示"), MB_OK | MB_ICONINFORMATION);
}


void CMediaPlayerDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);

    CRect rect;
    CButton * btnCtrl = (CButton *)GetDlgItem(IDC_BTN_CTRL);
    if (NULL != btnCtrl)
    {
        btnCtrl->GetClientRect(&rect);
        btnCtrl->MoveWindow(0, cy - rect.Height(), rect.Width(), rect.Height());
    }
    const int btnHeight = rect.Height();

    CButton * btnStop = (CButton *)GetDlgItem(IDC_BTN_STOP);
    if (NULL != btnStop)
    {
        btnStop->GetClientRect(&rect);
        btnStop->MoveWindow(rect.Width() + 1, cy - rect.Height(), rect.Width(), rect.Height());
    }

    CButton * btnForward = (CButton *)GetDlgItem(IDC_BTN_FORWARD);
    if (NULL != btnForward)
    {
        btnForward->GetClientRect(&rect);
        btnForward->MoveWindow(rect.Width() * 2 + 2, cy - rect.Height(), rect.Width(), rect.Height());
    }

    CButton * btnBackward = (CButton *)GetDlgItem(IDC_BTN_BACKWARD);
    if (NULL != btnBackward)
    {
        btnBackward->GetClientRect(&rect);
        btnBackward->MoveWindow(rect.Width() * 3 + 3, cy - rect.Height(), rect.Width(), rect.Height());
    }

    CButton * btnLast = (CButton *)GetDlgItem(IDC_BTN_LAST);
    if (NULL != btnLast)
    {
        btnLast->GetClientRect(&rect);
        btnLast->MoveWindow(rect.Width() * 4 + 4, cy - rect.Height(), rect.Width(), rect.Height());
    }

    CButton * btnNext = (CButton *)GetDlgItem(IDC_BTN_NEXT);
    if (NULL != btnNext)
    {
        btnNext->GetClientRect(&rect);
        btnNext->MoveWindow(rect.Width() * 5 + 5, cy - rect.Height(), rect.Width(), rect.Height());
    }

    CListCtrl * lstRecord = (CListCtrl *)GetDlgItem(IDC_LST_PLAY);
    if (NULL != lstRecord)
    {
        lstRecord->GetWindowRect(&rect);
        lstRecord->MoveWindow(cx - rect.Width(), 0, rect.Width(), cy);
    }

    const int lstWidth = rect.Width();

    CSliderCtrl * sldVoice = (CSliderCtrl *)GetDlgItem(IDC_SLD_VOICE);
    if (NULL != sldVoice)
    {
        sldVoice->GetClientRect(&rect);
        sldVoice->MoveWindow(cx - rect.Width() - lstWidth, cy - btnHeight - rect.Height(), rect.Width(), rect.Height());
    }

    const int sldWidth = rect.Width();
    CButton * btnVoice = (CButton *)GetDlgItem(IDC_BTN_VOICE);
    if (NULL != btnVoice)
    {
        btnVoice->GetClientRect(&rect);
        btnVoice->MoveWindow(cx - sldWidth - lstWidth - rect.Width(), cy - btnHeight - rect.Height(), rect.Width(), rect.Height());
    }

    const int voiceWidth = rect.Width();
    const int voiceHeight = rect.Height();
    CSliderCtrl * sldProgress = (CSliderCtrl *)GetDlgItem(IDC_SLD_PROGRESS);
    if (NULL != sldProgress)
    {
        sldProgress->GetClientRect(&rect);
        sldProgress->MoveWindow(0, cy - btnHeight - voiceHeight, cx - lstWidth - sldWidth - voiceWidth - 1, voiceHeight);
    }

    CStatic * stcScreen = (CStatic *)GetDlgItem(IDC_STC_SCREEN);
    if (NULL != stcScreen)
    {
        stcScreen->GetClientRect(&rect);
        stcScreen->MoveWindow(0, 0, cx - lstWidth, cy - btnHeight - voiceHeight);
    }

    CButton * btnLink = (CButton *)GetDlgItem(IDC_BTN_LINK);
    if (NULL != btnLink)
    {
        btnLink->GetClientRect(&rect);
        btnLink->MoveWindow(cx - rect.Width() * 2 - lstWidth - 2, cy - rect.Height(), rect.Width(), rect.Height());
    }

    CButton * btnFile = (CButton *)GetDlgItem(IDC_BTN_OPEN);
    if (NULL != btnFile)
    {
        btnFile->GetClientRect(&rect);
        btnFile->MoveWindow(cx - rect.Width() - lstWidth - 1, cy - rect.Height(), rect.Width(), rect.Height());
    }

    CStatic * stcDuration = (CStatic *)GetDlgItem(IDC_STC_DURATION);
    if (NULL != stcDuration)
    {
        const int width = rect.Width();
        const int height = rect.Height();
        stcDuration->MoveWindow(width * 6 + 6, cy - height, cx - width * 8 - 9 - lstWidth, height);
    }

    Invalidate();
}


void CMediaPlayerDlg::OnBnClickedBtnLink()
{
    CMediaPlayerLinkDlg dlg;
    if (IDOK != dlg.DoModal())
    {
        return;
    }

    CString strLinkName = dlg.GetLinkName();
    if (strLinkName.IsEmpty())
    {
        MessageBox(_T("输入链接为空！"), _T("警告"), MB_ICONWARNING | MB_OK);
        return;
    }

    // TODO:打开链接
}


void CMediaPlayerDlg::OnClose()
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    CMediaPlayerImpl::getInstance().close();
    CDialogEx::OnClose();
}
