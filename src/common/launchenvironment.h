#pragma once

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

namespace Stellar
{

/*!
 * Environment used when launching user applications.
 *
 * The shell configures layer-shell per window through LayerShellQt, so it never
 * needs QT_WAYLAND_SHELL_INTEGRATION=layer-shell. If that variable leaks into a
 * normal application, Qt creates every top-level window as a layer surface and
 * the compositor shows it full-screen.
 */
QProcessEnvironment launchEnvironment();

/*!
 * Detached launch with launchEnvironment() applied. Returns true when the
 * helper process was started.
 */
bool startDetached(const QString &program,
                   const QStringList &arguments = {},
                   const QString &workingDirectory = {});

} // namespace Stellar
