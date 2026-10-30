#include "winmanager.h"
#include "view/LoginView.h"
#include "view/MainWindow.h"
#include "view/HomeView.h"
#include "view/ProfileView.h"
#include "view/DetectView.h"
#include "view/RecordView.h"
#include "Toast.h"
#include "UiScale.h"
#include "AppDefs.h"
#include <QTimer>
#include <QApplication>
#include <QtGlobal>

WinManager* WinManager::instance()
{
    // 父对象 qApp：随 QApplication 析构，避免静态对象在应用销毁后残留
    static WinManager* s_inst = new WinManager(qApp);
    return s_inst;
}

WinManager::WinManager(QObject* parent) : QObject(parent)
{
    m_login = new LoginView;
    connect(m_login, &LoginView::loginSucceeded, this, &WinManager::onLoginSucceeded);
}

void WinManager::start()
{
    m_login->show();
}

void WinManager::onLoginSucceeded()
{
    if (!m_mainBuilt) buildMain();

    // 手册 3.3.1：未建档账号登录，提示并强制引导初次建档
    const bool needJd = !m_jdDone;
    if (needJd)
        Toast::show(m_login, QStringLiteral("该医保账号未建立档案，请先进入初次建档"));

    m_login->hide();
    m_main->show();
    // 主窗可能处于放大几何（上次最大化后登出），恢复对应缩放因子
    UiScale::apply(def::FONT_COMP * qMax<qreal>(1.0, m_main->width() / 800.0));
    m_main->setCurrent(needJd ? pd::Page::Jd : pd::Page::Home);
}

void WinManager::buildMain()
{
    m_mainBuilt = true;
    m_main = new MainWindow;

    // ---- 导航（带建档守卫） ----
    connect(m_main, &MainWindow::navRequested, this, &WinManager::navTo);

    HomeView*    home    = m_main->home();
    ProfileView* profile = m_main->profile();
    DetectView*  detect  = m_main->detect();
    RecordView*  record  = m_main->record();

    // ---- 首页 ----
    connect(home, &HomeView::cellClicked,     this, &WinManager::navTo);
    connect(home, &HomeView::logoutRequested, this, &WinManager::logout);

    // ---- 建档页 ----
    connect(profile, &ProfileView::profileSaved, this, &WinManager::onProfileSaved);

    // ---- 检测页 ----
    connect(detect, &DetectView::logoutClicked, this, &WinManager::logout);
    connect(detect, &DetectView::appointmentAdded, this, [home]
            (const pd::Appointment& appt, bool autoBook) {
        home->addAppointment(appt, autoBook); // 检测页挂号 -> 首页待办同步
    });

    // ---- 记录页 ----
    connect(record, &RecordView::logoutClicked, this, &WinManager::logout);
}

void WinManager::navTo(pd::Page page)
{
    // 手册 3.3.1：未完成建档，切换检测/记录须提示
    if ((page == pd::Page::Detect || page == pd::Page::Record) && !m_jdDone) {
        Toast::show(m_main, QStringLiteral("请先完成初次建档业务"));
        return;
    }
    m_main->setCurrent(page);
}

// 建档保存（MD §13.2）：全局状态切换 + 入口转"我的档案" + 1.2s 后回首页
// 2026-10-30：信息栏只剩静态四项，过敏史/既往病史/建档状态不再向面板同步（值留给 service 层入库）
void WinManager::onProfileSaved(const QString& gm, const QString& jw)
{
    Q_UNUSED(gm);
    Q_UNUSED(jw);
    m_jdDone = true;                               // 全局状态：已建档
    m_main->home()->setJdDone(true);               // 首页入口转"我的档案"
    m_main->setJdTitle(QStringLiteral("我的档案")); // 导航改名
    QTimer::singleShot(def::PROFILE_BACK_MS, this, [this] { navTo(pd::Page::Home); });
}

void WinManager::logout()
{
    m_main->hide();
    UiScale::apply(def::FONT_COMP);   // 登录窗固定尺寸，回到补偿基线防大字号溢出
    m_login->show();
}
