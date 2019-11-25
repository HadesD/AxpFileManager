#include "Utils.hpp"

#include <string>

#include <Windows.h>
#include <ShlObj.h>
#include <shellapi.h>
#include <commoncontrols.h>
#include <QtWinExtras/QtWinExtras>

#include <TlHelp32.h>

#include "Log.hpp"

namespace Utils {
  void AssignIconAxpFile()
  {
    // This code will apply all windows app
    int aElements[2] = {COLOR_WINDOW, COLOR_ACTIVECAPTION};
    DWORD aOldColors[2];
    DWORD aNewColors[2];

    aOldColors[0] = GetSysColor(aElements[0]);
    aOldColors[1] = GetSysColor(aElements[1]);
    aNewColors[0] = RGB(0xFF, 0xFF, 0xFF);  // light gray
    aNewColors[1] = RGB(0xFF, 0xFF, 0xFF);  // dark purple

//    SetSysColors(2, aElements, aNewColors);
//    SetSysColors(2, aElements, aOldColors);
  }

  QIcon getExtIcon(QString const& ext, const uint32_t flag)
  {
    QIcon icon;
    QString name = "*." + ext;
    SHFILEINFOW shinfo;
    if (SHGetFileInfoW(reinterpret_cast<const wchar_t*>(name.utf16()), 0, &shinfo, sizeof(shinfo), flag | SHGFI_ICON | SHGFI_USEFILEATTRIBUTES) != 0) {
      if (shinfo.hIcon) {
        QPixmap pm = QtWin::fromHICON(shinfo.hIcon);
        if (!pm.isNull()) {
          icon = QIcon(pm);
        }
        DestroyIcon(shinfo.hIcon);
      }
    }
    return icon;
  }

  QIcon iconFromExtensionLarge(QString const& ext)
  {
    return getExtIcon(ext, SHGFI_LARGEICON);
  }

  QIcon iconFromExtensionSmall(QString const& ext)
  {
    return getExtIcon(ext, SHGFI_SMALLICON);
  }
}
