#include "Global.hpp"

#include "Log.hpp"

namespace Global {
  QTemporaryDir& getTempDir()
  {
    static QTemporaryDir tempDir;

    LOG_DEBUG(__FUNCTION__ << tempDir.path());

    return tempDir;
  }

}
