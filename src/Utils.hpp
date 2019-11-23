#ifndef UTILS_HPP
#define UTILS_HPP

#include <QIcon>

namespace Utils {
  void AssignIconAxpFile();

  enum class IconSize {
    FILE_ICON_SMALL,
    FILE_ICON_MEDIUM,
    FILE_ICON_LARGE,
    FILE_ICON_EXTRALARGE,
  };

  QIcon getExtIcon(QString const& ext, const uint32_t flag);
  QIcon iconFromExtensionSmall(QString const &ext);
  std::string normaliseName(const std::string& fileName, bool toLowCase, bool norSlash);
}

#endif // UTILS_HPP
