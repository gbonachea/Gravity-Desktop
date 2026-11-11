#pragma once

#include <QString>

namespace Stellar
{

/*!
 * Utilidades para abrir carpetas con el gestor de archivos predeterminado del
 * sistema. Se evita depender de xdg-open para las carpetas porque en sesiones
 * Wayland mínimas no siempre hay una asociación MIME para inode/directory.
 */
QString defaultFileManagerExecutable();
QString defaultFileManagerName();

// location puede ser una ruta local o una URL (p. ej. trash:///).
bool openWithFileManager(const QString &location);

// Abre un archivo con la aplicación predeterminada; si no hay asociación,
// recurre al gestor de archivos para mostrar su carpeta.
bool openFileOrFolder(const QString &path);

} // namespace Stellar
