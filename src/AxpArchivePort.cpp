#include "AxpArchivePort.hpp"

#include <AxpArchive.hpp>
#include <QThread>
#include <QTemporaryDir>
#include <QDir>

#include <memory>

#include "Log.hpp"

AxpArchivePort::AxpArchivePort(QObject *parent) : QObject(parent)
{
  m_pPakFile = AXP::createPakFile();
}

AxpArchivePort::~AxpArchivePort()
{
  AXP::destroyPakFile(m_pPakFile);
}

bool AxpArchivePort::exists(const AxpArchivePort::FileName& fileName) const
{
  return m_pPakFile->isFileExists(fileName.data());
}

QByteArray AxpArchivePort::read(const AxpArchivePort::FileName& fileName) const
{
  const char* c_fileName = fileName.data();
  std::unique_ptr<
      AXP::IStream, std::function<void(AXP::IStream*)>
      > hFileStream(m_pPakFile->openFile(c_fileName), [](AXP::IStream* s){
    s->close();
  });
  if (!hFileStream)
  {
    AXP::setLastError(AXP::AXP_ERRORS::AXP_ERR_FILE_FORMAT, "WinErr=%d FileName=%s", ::GetLastError(), c_fileName);
    return nullptr;
  }

  const uint32_t fileSize = hFileStream->size();

  std::unique_ptr<uchar[]> fileData{new uchar[fileSize]};
  if (!fileData)
  {
    AXP::setLastError(AXP::AXP_ERRORS::AXP_ERR_MEMORY, "WinErr=%d FileName=%s", ::GetLastError(), c_fileName);
    return nullptr;
  }

  const uint32_t readSize = hFileStream->read(fileData.get(), fileSize);
  if (fileSize != readSize)
  {
    AXP::setLastError(AXP::AXP_ERRORS::AXP_ERR_FILE_READ, "WinErr=%d FileName=%s", ::GetLastError(), c_fileName);
    return nullptr;
  }

  return QByteArray().append(reinterpret_cast<char*>(fileData.get()), static_cast<int>(readSize));
}

bool AxpArchivePort::extractToDisk(const AxpArchivePort::FileName& fileName, const AxpArchivePort::FileName& toDiskFileName)
{
  QString q_fileName = QString::fromLocal8Bit(toDiskFileName.c_str());
  if (!QDir(q_fileName).mkpath(".."))
  {
    LOG_DEBUG(__FUNCTION__ << "Error:" << "mkpath");
    return false;
  }

  QFile hFile(q_fileName);
  if (!hFile.open(QIODevice::WriteOnly))
  {
    LOG_DEBUG(__FUNCTION__ << "Error:" << "open WriteOnly");
    return false;
  }

  QByteArray fileData = this->read(fileName);
  if (fileData == nullptr)
  {
    LOG_DEBUG(__FUNCTION__ << "Error:" << "read fileData");
    hFile.close();
    return false;
  }

  if (!hFile.write(fileData))
  {
    LOG_DEBUG(__FUNCTION__ << "Error:" << "write fileData");
    hFile.close();
    return false;
  }

  hFile.close();

  return true;
}

bool AxpArchivePort::insertDiskFile(const AxpArchivePort::FileName& diskFileName, const AxpArchivePort::FileName& fileName, const bool saveAtOnce)
{
  return m_pPakFile->insertContents(diskFileName.data(), 0, fileName.data(), AXP::AXP_CONTENTS::AC_DISKFILE, saveAtOnce);
}

bool AxpArchivePort::removeFile(const AxpArchivePort::FileName& fileName, const bool saveAtOnce)
{
  return m_pPakFile->removeFile(fileName.data(), saveAtOnce);
}

bool AxpArchivePort::saveToDiskFile(const AxpArchivePort::FileName& toDiskFileName)
{
  LOG_DEBUG(__FUNCTION__ << "called");
  const char* c_toDiskFileName = toDiskFileName.data();
  QString q_toDiskFileName = QString::fromLocal8Bit(c_toDiskFileName);
  if (!QDir(q_toDiskFileName).mkpath(".."))
  {
    LOG(__FUNCTION__ << "error make path");
    return false;
  }

  std::unique_ptr<
      AXP::IPakMaker, std::function<void(AXP::IPakMaker*)>
      > pakMaker(AXP::createPakMaker(), [](AXP::IPakMaker* s){
    AXP::destroyPakMaker(s);
  });
  if (!pakMaker)
  {
    return false;
  }

  QTemporaryDir tempDir;
  if (!tempDir.isValid())
  {
    return false;
  }

  // Extract temp
  QString q_tempDirPath = tempDir.path();
  QDir qDir(q_tempDirPath);
  std::shared_ptr<std::size_t> i{new std::size_t};*i = 0;
  const std::size_t listSize = m_fileList.size();
  for (const auto& fileListPair : m_fileList)
  {
    const auto& fileName = fileListPair.first;
    const auto& fileListItemData = fileListPair.second;
    ++(*i); if (m_progressUpdateCallback) m_progressUpdateCallback(fileName, *i, listSize);
    switch (fileListItemData.status)
    {
      case FileListData::FileStatus::NEW:
        if (!fileListItemData.nameFromDisk.empty())
        {
          pakMaker->addDiskFile(fileListItemData.nameFromDisk.data(), fileName.data());
        }
        continue;
      case FileListData::FileStatus::UNKNOWN:
      case FileListData::FileStatus::DELETED:
        continue;

      default:
        break;
    }
    if (!this->extractToDisk(fileName, qDir.filePath(QString::fromLocal8Bit(fileName.c_str())).toLocal8Bit().data()))
    {
      LOG_DEBUG(__FUNCTION__ << "error extract" << fileName.c_str() << getLastErrorMessage());
      continue;
    }
  }

  // Save on working file
  if (q_toDiskFileName == m_fileName)
  {
    this->close();
  }

  pakMaker->addDiskFold(q_tempDirPath.toLocal8Bit().data(), "", "", true);

  *i = 0;

  bool saveRet = pakMaker->savePakFile(toDiskFileName.data(), [=](auto _1,auto _2,auto _3,auto _4)->bool{
    Q_UNUSED(_1);
    Q_UNUSED(_3);
    Q_UNUSED(_4);
    strncpy(_3, _1, _4);

    ++(*i); if (m_progressUpdateCallback) m_progressUpdateCallback(_2, *i, listSize);

    return true;
  });

  return saveRet;
}

void AxpArchivePort::close()
{
  m_fileList.clear();
  m_pPakFile->closePakFile();
}

void AxpArchivePort::setAxpArchiveFileName(const QString &fileName)
{
  m_fileName = fileName;
}

void AxpArchivePort::setAxpArchiveFileEditable(const bool editable)
{
  m_editable = editable;
}

void AxpArchivePort::startOpenAxpArchive(std::function<void ()> onStarted, std::function<void ()> onFinished)
{
  QThread* openAxpArchiveThread = new QThread();
  connect(openAxpArchiveThread, &QThread::started, [=](){
    onStarted();

    if (!m_pPakFile->openPakFile(m_fileName.toLocal8Bit().data(), !m_editable))
    {
      openAxpArchiveThread->quit();
      return;
    }

    std::unique_ptr<
        AXP::IStream, std::function<void(AXP::IStream*)>
        > hListStream(m_pPakFile->openFile(AXP::PackFile::LIST_FILENAME), [](AXP::IStream* s){
      s->close();
    });
    if (!hListStream)
    {
      openAxpArchiveThread->quit();
      return;
    }

    // Skip hex line
    hListStream->skipLine();

    char szTempLine[MAX_PATH*4] = {0};
    hListStream->readLine(szTempLine, sizeof(szTempLine));
    uint32_t nFileCount = static_cast<uint32_t>(atoi(szTempLine));

    for (uint32_t i = 0; !hListStream->eof(); ++i)
    {
      hListStream->readLine(szTempLine, sizeof(szTempLine));

      std::vector< std::string > vStringVec;
      AXP::convertStringToVector(szTempLine, vStringVec, "|", true, false);
      if(vStringVec.size() != 3)
      {
        LOG_DEBUG("AxpArchivePort::startOpenAxpArchive::Thread line split count !=3" << QString::fromLocal8Bit(szTempLine));
        continue;
      }

      const std::string& strFileName = vStringVec[0];
      unsigned int nFileSize, nFileCRC;
      sscanf(vStringVec[1].c_str(), "%08X", &(nFileSize));
      sscanf(vStringVec[2].c_str(), "%08X", &(nFileCRC));

      const char* c_strFileName = strFileName.c_str();

      if (!m_pPakFile->isFileExists(c_strFileName))
      {
        LOG("AxpArchivePort::startOpenAxpArchive::Thread" << AXP::getLastErrorDesc());
        continue;
      }

      std::unique_ptr<
          AXP::IStream, std::function<void(AXP::IStream*)>
          > hFileStream(m_pPakFile->openFile(c_strFileName), [](AXP::IStream* s){
        s->close();
      });
      if(!hFileStream)
      {
        continue;
      }

      const unsigned int nStreamSize = hFileStream->size();
      if(nStreamSize != nFileSize)
      {
        LOG_DEBUG("AxpArchivePort::startOpenAxpArchive::Thread nStreamSize != nFileSize");
        continue;
      }

      // Set arr
      auto& fileListItem = m_fileList[strFileName];
      fileListItem.size = nStreamSize;
      fileListItem.status = FileListData::FileStatus::ORIGIN;

      if (m_progressUpdateCallback)
      {
        m_progressUpdateCallback(strFileName, i+1, nFileCount);
      }
    }

    // Quit at the end
    openAxpArchiveThread->quit();
  });
  connect(openAxpArchiveThread, &QThread::finished, [=](){
    LOG_DEBUG("AxpArchivePort::startOpenAxpArchive::Thread finished");
    onFinished();

    openAxpArchiveThread->deleteLater();
  });

  openAxpArchiveThread->start();
}

AXP::AXP_ERRORS AxpArchivePort::getLastError() const
{
  return AXP::getLastError();
}

QString AxpArchivePort::getLastErrorMessage() const
{
  return AXP::getLastErrorDesc();
}

bool AxpArchivePort::isModified() const
{
  for (const auto& fileListItem : m_fileList)
  {
    if (fileListItem.second.status != AxpArchivePort::FileListData::FileStatus::ORIGIN)
    {
      return true;
    }
  }
  return false;
}
