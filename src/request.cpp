/*
 * This file is part of signon-ui
 *
 * Copyright (C) 2011 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define HAS_XEMBED (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#define HAS_FOREIGN_QWINDOW (QT_VERSION >= QT_VERSION_CHECK(5, 1, 0) || \
                             defined(FORCE_FOREIGN_QWINDOW))
#include "request.h"

#ifdef USE_UBUNTU_WEB_VIEW
#include "ubuntu-browser-request.h"
#endif
#include "browser-request.h"
#include "debug.h"
#include "dialog-request.h"
#if HAS_XEMBED
#include "embed-manager.h"
#endif
#include "errors.h"
#include "indicator-service.h"
#ifndef UNIT_TESTS
#include "webcredentials_interface.h"
#else
#include "fake-webcredentials-interface.h"
#endif

#include <Accounts/Account>
#include <Accounts/Manager>
#include <QApplication>
#include <QDBusArgument>
#include <QVBoxLayout>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QX11Info>
#endif
#if HAS_FOREIGN_QWINDOW
#include <QWindow>
#endif
#include <SignOn/uisessiondata.h>
#include <SignOn/uisessiondata_priv.h>
#include <X11/Xlib.h>

using namespace SignOnUi;
using namespace com::canonical;

namespace SignOnUi {

class RequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Request)

public:
    RequestPrivate(const QDBusConnection &connection,
                   const QDBusMessage &message,
                   const QVariantMap &parameters,
                   Request *request);
    ~RequestPrivate();

    WId windowId() const {
        return m_clientData[SSOUI_KEY_WINDOWID].toUInt();
    }

    bool embeddedUi() const {
        return m_clientData[SSOUI_KEY_EMBEDDED].toBool();
    }

private Q_SLOTS:
#if HAS_XEMBED
    void onEmbedError();
#endif
    void onIndicatorCallFinished(QDBusPendingCallWatcher *watcher);

private:
    void setWidget(QWidget *widget);
    Accounts::Account *findAccount();
    bool dispatchToIndicator();
    void onIndicatorCallSucceeded();

private:
    mutable Request *q_ptr;
    QDBusConnection m_connection;
    QDBusMessage m_message;
    QVariantMap m_parameters;
    QVariantMap m_clientData;
    bool m_inProgress;
    Accounts::Manager *m_accountManager;
    QPointer<QWidget> m_widget;
};

} // namespace

RequestPrivate::RequestPrivate(const QDBusConnection &connection,
                               const QDBusMessage &message,
                               const QVariantMap &parameters,
                               Request *request):
    QObject(request),
    q_ptr(request),
    m_connection(connection),
    m_message(message),
    m_parameters(parameters),
    m_inProgress(false),
    m_accountManager(0),
    m_widget(0)
{
    if (parameters.contains(SSOUI_KEY_CLIENT_DATA)) {
        QVariant variant = parameters[SSOUI_KEY_CLIENT_DATA];
        m_clientData = (variant.type() == QVariant::Map) ?
            variant.toMap() :
            qdbus_cast<QVariantMap>(variant.value<QDBusArgument>());
    }
}

RequestPrivate::~RequestPrivate()
{
}

void RequestPrivate::setWidget(QWidget *widget)
{
    if (m_widget != 0) {
        BLAME() << "Widget already set";
        return;
    }

    m_widget = widget;

#if HAS_XEMBED
    if (embeddedUi() && windowId() != 0) {
        TRACE() << "Requesting widget embedding";
        QX11EmbedWidget *embed =
            EmbedManager::instance()->widgetFor(windowId());
        QObject::connect(embed, SIGNAL(error(QX11EmbedWidget::Error)),
                         this, SLOT(onEmbedError()),
                         Qt::UniqueConnection);
        QObject::connect(embed, SIGNAL(containerClosed()),
                         widget, SLOT(close()));
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(widget);
        widget->show();
        /* Delete any previous layout */
        delete embed->layout();
        embed->setLayout(layout);
        embed->show();
        return;
    }
#endif
#if HAS_FOREIGN_QWINDOW
    if (embeddedUi() && windowId() != 0) {
        TRACE() << "Requesting window embedding";
        QWindow *host = QWindow::fromWinId(windowId());
        widget->show();
        widget->windowHandle()->setParent(host);
        return;
    }
#endif

    /* If the window has no parent and the webcredentials indicator service is
     * up, dispatch the request to it. */
    if (windowId() == 0 && dispatchToIndicator()) {
        return;
    }

    widget->setWindowModality(Qt::WindowModal);
    widget->show();
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    if (windowId() != 0) {
        TRACE() << "Setting" << widget->effectiveWinId() << "transient for" << windowId();
        XSetTransientForHint(QX11Info::display(),
                             widget->effectiveWinId(),
                             windowId());
    }
#endif
#if HAS_FOREIGN_QWINDOW
    if (windowId() != 0) {
        TRACE() << "Requesting window reparenting";
        QWindow *parent = QWindow::fromWinId(windowId());
        widget->windowHandle()->setTransientParent(parent);
    }
#endif
}

#if HAS_XEMBED
void RequestPrivate::onEmbedError()
{
    Q_Q(Request);

    QX11EmbedWidget *embed = qobject_cast<QX11EmbedWidget*>(sender());
    TRACE() << "Embed error:" << embed->error();

    q->fail(SIGNON_UI_ERROR_EMBEDDING_FAILED,
            QString("Embedding signon UI failed: %1").arg(embed->error()));
}
#endif

Accounts::Account *RequestPrivate::findAccount()
{
    if (!m_parameters.contains(SSOUI_KEY_IDENTITY))
        return 0;

    uint identity = m_parameters.value(SSOUI_KEY_IDENTITY).toUInt();
    if (identity == 0)
        return 0;

    /* Find the account using this identity.
     * FIXME: there might be more than one!
     */
    if (m_accountManager == 0) {
        m_accountManager = new Accounts::Manager(this);
    }
    foreach (Accounts::AccountId accountId, m_accountManager->accountList()) {
        Accounts::Account *account = m_accountManager->account(accountId);
        if (account == 0) continue;

        QVariant value(QVariant::UInt);
        if (account->value("CredentialsId", value) != Accounts::NONE &&
            value.toUInt() == identity) {
            return account;
        }
    }

    // Not found
    return 0;
}

bool RequestPrivate::dispatchToIndicator()
{
    Q_Q(Request);

    Accounts::Account *account = findAccount();
    if (account == 0) {
        return false;
    }

    QVariantMap notification;
    notification["DisplayName"] = account->displayName();
    notification["ClientData"] = m_clientData;
    notification["Identity"] = q->identity();
    notification["Method"] = q->method();
    notification["Mechanism"] = q->mechanism();

    indicators::webcredentials *webcredentialsIf =
        new indicators::webcredentials(WEBCREDENTIALS_BUS_NAME,
                                       WEBCREDENTIALS_OBJECT_PATH,
                                       m_connection, this);
    QDBusPendingReply<> reply =
        webcredentialsIf->ReportFailure(account->id(), notification);
    if (reply.isFinished()) {
        if (reply.isError() &&
            /* if this is a fake D-Bus interface, we get the
             * "Disconnected" error. */
            reply.error().type() != QDBusError::Disconnected) {
            BLAME() << "Error dispatching to indicator:" <<
                reply.error().message();
            return false;
        } else {
            onIndicatorCallSucceeded();
            return true;
        }
    }
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                     this,
                     SLOT(onIndicatorCallFinished(QDBusPendingCallWatcher*)));

    return true;
}

void RequestPrivate::onIndicatorCallFinished(QDBusPendingCallWatcher *watcher)
{
    if (watcher->isError()) {
        /* if the notification could not be delivered to the indicator, show
         * the widget. */
        if (m_widget != 0)
            m_widget->show();
    } else {
        onIndicatorCallSucceeded();
    }
}

void RequestPrivate::onIndicatorCallSucceeded()
{
    Q_Q(Request);

    /* the account has been reported as failing. We can now close this
     * request, and tell the application that UI interaction is forbidden.
     */
    QVariantMap result;
    result[SSOUI_KEY_ERROR] = SignOn::QUERY_ERROR_FORBIDDEN;
    q->setResult(result);
}

Request *Request::newRequest(const QDBusConnection &connection,
                             const QDBusMessage &message,
                             const QVariantMap &parameters,
                             QObject *parent)
{
    if (parameters.contains(SSOUI_KEY_OPENURL)) {
#ifdef USE_UBUNTU_WEB_VIEW
        TRACE() << "Platform:" << QGuiApplication::platformName();
        /* We need to use the RemoteRequest implementation in UbuntuTouch,
         * because displaying of QtWidgets is not working there. This is a
         * workaround which can be revisited later. */
        if (QGuiApplication::platformName().startsWith("ubuntu") ||
            qgetenv("SSOUI_USE_UBUNTU_WEB_VIEW") == QByteArray("1")) {
            return new UbuntuBrowserRequest(connection, message,
                                            parameters, parent);
        }
#endif
        return new BrowserRequest(connection, message, parameters, parent);
    } else {
        return new DialogRequest(connection, message, parameters, parent);
    }
}

Request::Request(const QDBusConnection &connection,
                 const QDBusMessage &message,
                 const QVariantMap &parameters,
                 QObject *parent):
    QObject(parent),
    d_ptr(new RequestPrivate(connection, message, parameters, this))
{
}

Request::~Request()
{
}

QString Request::id(const QVariantMap &parameters)
{
    return parameters[SSOUI_KEY_REQUESTID].toString();
}

QString Request::id() const
{
    Q_D(const Request);
    return Request::id(d->m_parameters);
}

void Request::setWidget(QWidget *widget)
{
    Q_D(Request);
    d->setWidget(widget);
}

uint Request::identity() const
{
    Q_D(const Request);

    return d->m_parameters.value(SSOUI_KEY_IDENTITY).toUInt();
}

QString Request::method() const
{
    Q_D(const Request);

    return d->m_parameters.value(SSOUI_KEY_METHOD).toString();
}

QString Request::mechanism() const
{
    Q_D(const Request);

    return d->m_parameters.value(SSOUI_KEY_MECHANISM).toString();
}

WId Request::windowId() const
{
    Q_D(const Request);
    return d->windowId();
}

bool Request::embeddedUi() const
{
    Q_D(const Request);
    return d->embeddedUi();
}

bool Request::isInProgress() const
{
    Q_D(const Request);
    return d->m_inProgress;
}

const QVariantMap &Request::parameters() const
{
    Q_D(const Request);
    return d->m_parameters;
}

const QVariantMap &Request::clientData() const
{
    Q_D(const Request);
    return d->m_clientData;
}

void Request::start()
{
    Q_D(Request);
    if (d->m_inProgress) {
        BLAME() << "Request already started!";
        return;
    }
    d->m_inProgress = true;
}

void Request::cancel()
{
    setCanceled();
}

void Request::fail(const QString &name, const QString &message)
{
    Q_D(Request);
    QDBusMessage reply = d->m_message.createErrorReply(name, message);
    d->m_connection.send(reply);

    Q_EMIT completed();
}

void Request::setCanceled()
{
    QVariantMap result;
    result[SSOUI_KEY_ERROR] = SignOn::QUERY_ERROR_CANCELED;

    setResult(result);
}

void Request::setResult(const QVariantMap &result)
{
    Q_D(Request);
    QDBusMessage reply = d->m_message.createReply(result);
    d->m_connection.send(reply);

    Q_EMIT completed();
}

#include "request.moc"
