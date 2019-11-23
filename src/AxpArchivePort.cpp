#include "AxpArchivePort.hpp"

#include <AxpArchive.hpp>
#include <QThread>

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
  AXP::IStream* fileStream = m_pPakFile->openFile(c_fileName);
  if (!fileStream) {
    return nullptr;
  }

  const unsigned int fileSize = fileStream->size();

  char* fileBuf = new char[fileSize];
  if (!fileBuf) {
    AXP::setLastError(AXP::AXP_ERRORS::AXP_ERR_MEMORY, "WinErr=%d", ::GetLastError());
    return nullptr;
  }
  const unsigned int readSize = fileStream->read(fileBuf, fileSize);
  if (readSize != fileSize) {
    delete[] fileBuf;
    return nullptr;
  }
  QByteArray retData = QByteArray::fromRawData(fileBuf, static_cast<int>(fileSize));
  delete[] fileBuf;

  return retData;
}

bool AxpArchivePort::extractToDisk(const AxpArchivePort::FileName& fileName, const AxpArchivePort::FileName& toDiskFileName)
{
  return true;
}

bool AxpArchivePort::removeFile(const AxpArchivePort::FileName& fileName)
{
  return m_pPakFile->removeFile(fileName.data(), m_editable);
}

void AxpArchivePort::close()
{
  m_pPakFile->closePakFile();
}

void AxpArchivePort::setFileName(const QString &fileName)
{
  m_fileName = fileName;
}

void AxpArchivePort::setFileEditable(const bool editable)
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

    AXP::IStream* listStream = m_pPakFile->openFile(AXP::PackFile::LIST_FILENAME);
    if (!listStream)
    {
      openAxpArchiveThread->quit();
      return;
    }

    // Skip hex line
    listStream->skipLine();

    char szTempLine[MAX_PATH*4] = {0};
    listStream->readLine(szTempLine, sizeof(szTempLine));
    uint32_t nFileCount = static_cast<uint32_t>(atoi(szTempLine));

//    while (!listStream->eof())
    for (uint32_t i = 0; !listStream->eof(); ++i)
    {
      listStream->readLine(szTempLine, sizeof(szTempLine));

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

      AXP::IStream* pFileStream = m_pPakFile->openFile(strFileName.c_str());	//´ò¿ªÎÄ¼þ
      if(!pFileStream)
      {
        continue;
      }

      unsigned int nStreamSize = pFileStream->size();
      if(nStreamSize != nFileSize)
      {
        pFileStream->close();
//        setLastError(AXP_ERR_FILE_READ, "file=%s", strFileName.c_str());
        continue;
      }

      char* pTempBuf = new char[nStreamSize];
      if(nStreamSize != pFileStream->read(pTempBuf, nStreamSize))
      {
        pFileStream->close();
//        setLastError(AXP_ERR_FILE_READ, "file=%s", strFileName.c_str());
        continue;
      }

      pFileStream->close();

      if (m_progressUpdateCallback)
      {
        m_progressUpdateCallback(strFileName, i, nFileCount);
      }
    }
    listStream->close();

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
