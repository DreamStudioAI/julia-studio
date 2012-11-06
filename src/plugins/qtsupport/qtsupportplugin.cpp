/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qtsupportplugin.h"

#include "customexecutablerunconfiguration.h"
#include "qtoptionspage.h"
#include "qtkitinformation.h"
#include "qtversionmanager.h"

#include "profilereader.h"

#include "gettingstartedwelcomepage.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kitmanager.h>

#include <QtPlugin>
#include <QMenu>

using namespace QtSupport;
using namespace QtSupport::Internal;

bool QtSupportPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);
    QMakeParser::initialize();
    ProFileEvaluator::initialize();
    new ProFileCacheManager(this);

    QtVersionManager *mgr = new QtVersionManager;
    addAutoReleasedObject(mgr);

    QtFeatureProvider *featureMgr = new QtFeatureProvider;
    addAutoReleasedObject(featureMgr);

    ExamplesWelcomePage *welcomePage;
    welcomePage = new ExamplesWelcomePage;
    addAutoReleasedObject(welcomePage);

    welcomePage = new ExamplesWelcomePage;
    welcomePage->setShowExamples(true);
    addAutoReleasedObject(welcomePage);

    GettingStartedWelcomePage *gettingStartedWelcomePage = new GettingStartedWelcomePage;
    addAutoReleasedObject(gettingStartedWelcomePage);

    addAutoReleasedObject(new CustomExecutableRunConfigurationFactory);

    return true;
}

void QtSupportPlugin::extensionsInitialized()
{
    QtVersionManager::instance()->extensionsInitialized();
    ProjectExplorer::KitManager::instance()->registerKitInformation(new QtKitInformation);
}

bool QtSupportPlugin::delayedInitialize()
{
    return QtVersionManager::instance()->delayedInitialize();
}

Q_EXPORT_PLUGIN(QtSupportPlugin)
