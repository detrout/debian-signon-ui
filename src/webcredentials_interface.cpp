/*
 * This file is part of signon-ui
 *
 * Copyright (C) 2012 Canonical Ltd.
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

#include "webcredentials_interface.h"

#include "indicator-service.h"

/*
 * We don't use the code generated by qdbusxml2cpp, because the
 * IndicatorService is provided by the signon-ui process itself, and it would
 * be silly to send requests to it via D-Bus.
 * So, this class wraps the real IndicatorService methods, so that they can be
 * called directly, while still leaving the external API as if it were
 * generated by qdbusxml2cpp (in case we later decide to move this service
 * somewhere else).
 */

using namespace SignOnUi;

ComCanonicalIndicatorsWebcredentialsInterface::
ComCanonicalIndicatorsWebcredentialsInterface(const QString &service,
                                              const QString &path,
                                              const QDBusConnection &connection,
                                              QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(service);
    Q_UNUSED(path);
    Q_UNUSED(connection);
}

ComCanonicalIndicatorsWebcredentialsInterface::~ComCanonicalIndicatorsWebcredentialsInterface()
{
}

QDBusPendingReply<>
ComCanonicalIndicatorsWebcredentialsInterface::ReportFailure(uint account_id,
                                                             const QVariantMap &notification)
{
    Q_ASSERT(IndicatorService::instance() != 0);
    IndicatorService::instance()->reportFailure(account_id, notification);
    return QDBusPendingReply<>();
}
