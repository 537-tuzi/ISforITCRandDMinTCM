#include "winmanager.h"
#include "view/LoginView.h"
#include "view/MainWindow.h"
#include "Toast.h"
#include "UiScale.h"
#include <QApplication>
#include <QtGlobal>

WinManager* WinManager::instance()
{
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

void WinManager::onLoginSucceeded(int role, const QString& workNo, const QString& name)
{
    if (!m_mainBuilt) buildMain(role, workNo, name);
    m_login->hide();
    m_main->show();
    UiScale::apply(qMax<qreal>(1.0, m_main->width() / 800.0));
}

void WinManager::buildMain(int role, const QString& workNo, const QString& name)
{
    m_mainBuilt = true;
    m_main = new MainWindow(role, workNo, name);
    connect(m_main, &MainWindow::logoutRequested, this, &WinManager::logout);
}

void WinManager::logout()
{
    m_main->hide();
    UiScale::apply(1.0);
    m_login->show();
}
