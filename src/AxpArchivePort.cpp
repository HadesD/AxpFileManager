#include "AxpArchivePort.hpp"

#include <AxpArchive.hpp>
#include <QThread>
#include <QDir>

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
  const char* c_fileName = fileName.data();

  QString q_fileName = QString::fromLocal8Bit(toDiskFileName.data());
  if (!QDir(q_fileName).mkpath(".."))
  {
    AXP::setLastError(AXP::AXP_ERRORS::AXP_ERR_FILE_ACCESS, "WinErr=%d FileName=%s", ::GetLastError(), c_fileName);
    return false;
  }

  QFile hFile(q_fileName);
  if (!hFile.open(QIODevice::WriteOnly))
  {
    AXP::setLastError(AXP::AXP_ERRORS::AXP_ERR_FILE_READ, "WinErr=%d FileName=%s", ::GetLastError(), c_fileName);
    return false;
  }

  QByteArray fileData = this->read(fileName);
  if (fileData == nullptr)
  {
    AXP::setLastError(AXP::AXP_ERRORS::AXP_ERR_FILE_READ, "WinErr=%d FileName=%s", ::GetLastError(), c_fileName);
    hFile.close();
    return false;
  }

  if (!hFile.write(fileData))
  {
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
  QThread* openAxpArchiveThread = new QThread(this);
  connect(openAxpArchiveThread, &QThread::started, [=](){
    LOG_DEBUG("AxpArchivePort::startOpenAxpArchive::Thread started");
    onStarted();

    if (!m_pPakFile->openPakFile(m_fileName.toLocal8Bit().data(), m_editable))
    {
      openAxpArchiveThread->quit();
      return;
    }
    LOG_DEBUG("AxpArchivePort::startOpenAxpArchive::Thread open success");

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

//    while (!listStream->eof())
    for (uint32_t i = 0; !hListStream->eof(); ++i)
    {
      hListStream->readLine(szTempLine, sizeof(szTempLine));

      std::vector< std::string > vStringVec;
      AXP::convertStringToVector(szTempLine, vStringVec, "|", true, false);
      if(vStringVec.size() != 3)
      {
//        setLastError(AXP::AXP_ERRORS::AXP_ERR_FILE_FORMAT, "list file=%s", szTempLine);
        continue;
      }

      const std::string& strFileName = vStringVec[0];
      unsigned int nFileSize, nFileCRC;
      sscanf(vStringVec[1].c_str(), "%08X", &(nFileSize));
      sscanf(vStringVec[2].c_str(), "%08X", &(nFileCRC));

      const char* c_strFileName = strFileName.c_str();

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
//        setLastError(AXP::AXP_ERRORS::AXP_ERR_FILE_READ, "file=%s", c_strFileName);
        continue;
      }

//      std::unique_ptr<uchar[]> pTempBuf{new uchar[nStreamSize]};
//      if(nStreamSize != hFileStream->read(pTempBuf.get(), nStreamSize))
//      {
//        setLastError(AXP::AXP_ERRORS::AXP_ERR_FILE_READ, "file=%s", c_strFileName);
//        continue;
//      }

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
