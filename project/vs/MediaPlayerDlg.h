
// MediaPlayerDlg.h: 头文件
//

#pragma once

#include <string>
#include "../src/MediaPlayerImpl.h"

#define BACK_COLOR RGB(16, 16, 16)     // 背景颜色
#define FONT_COLOR RGB(255, 255, 255)  // 字体颜色
#define GAP_SIZE   10                  // 图标缝隙

// CMediaPlayerDlg 对话框
class CMediaPlayerDlg : public CDialogEx
{
// 构造
public:
    CMediaPlayerDlg(CWnd * pParent = nullptr);   // 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DLG_PLAYER };
#endif

protected:
    virtual void DoDataExchange(CDataExchange * pDX);    // DDX/DDV 支持


// 实现
protected:
    HICON m_hIcon;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
public:
    afx_msg HBRUSH OnCtlColor(CDC * pDC, CWnd * pWnd, UINT nCtlColor);
private:
    CBrush m_brush;
    CFont m_fontSize;     // 字体大小
    CListCtrl m_lstRecord;
    CButton m_btnCtrl;
    CButton m_btnStop;
    CButton m_btnForward;
    CButton m_btnBackward;
    CButton m_btnOpen;
    CButton m_btnLink;

    HICON m_icoStart;      // 开始
    HICON m_icoPause;      // 暂停
    HICON m_icoStop;
    HICON m_icoForward;
    HICON m_icoBackward;
    HICON m_icoLast;
    HICON m_icoNext;
    HICON m_icoVoice;
    HICON m_icoSilence;
    HICON m_icoOpen;
    HICON m_icoLink;
    CToolTipCtrl m_toolTip;
public:
    afx_msg void OnBnClickedBtnCtrl();
    afx_msg void OnBnClickedBtnStop();
    afx_msg void OnBnClickedBtnForward();
    afx_msg void OnBnClickedBtnBackward();
    afx_msg void OnBnClickedBtnOpen();
    afx_msg void OnBnClickedBtnLast();
    afx_msg void OnBnClickedBtnNext();
    afx_msg void OnBnClickedBtnVoice();
    virtual BOOL PreTranslateMessage(MSG * pMsg);
private:
    bool m_blPlaying = false;
    CStatic m_stcDuration;
    CButton m_btnLast;
    CButton m_btnNext;
    CButton m_btnVoice;
    CMediaPlayerImpl * m_pMediaPtr = nullptr;
    // 文件路径
    std::string m_strFilePath = "";
    CSliderCtrl m_sldProgress;
    CSliderCtrl m_sldVoice;
public:
    afx_msg void OnNMCustomdrawSldVoice(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnNMReleasedcaptureSldProgress(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    void strTime(int64_t duration, int64_t ts, CString & str);
    afx_msg void OnSize(UINT nType, int cx, int cy);
};
