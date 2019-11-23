#ifndef AXPARCHIVEPORT_HPP
#define AXPARCHIVEPORT_HPP

#include <QObject>
#include <QFile>
#include <functional>

namespace AXP {
  class IPakFile;
  enum AXP_ERRORS : int;
}

class AxpArchivePort : public QObject
{
    Q_OBJECT

  public:
    using FileName = std::string;
    using ProgressUpdateCallback = std::function<void(FileName fileName, uint32_t value, uint32_t total)>;
    struct FileListData {

    };
    using FileList = std::map<FileName, int>;

  public:
    explicit AxpArchivePort(QObject *parent = nullptr);
    ~AxpArchivePort();

  public:
    bool exists(const FileName& fileName) const;
    QByteArray read(const FileName& fileName) const;
    bool extractToDisk(const FileName& fileName, const FileName& toDiskFileName);
    bool insertDiskFile(const FileName& diskFileName, const FileName& fileName);
    bool removeFile(const FileName& fileName);
    bool saveToDiskFile(const FileName& toDiskFileName);
    void close();

  public:
    void setFileName(const QString& fileName);
    void setFileEditable(const bool editable);
    void startOpenAxpArchive(std::function<void()> onStarted = nullptr, std::function<void()> onFinished = nullptr);
    void setProgressCallback(ProgressUpdateCallback callback = nullptr) {m_progressUpdateCallback = callback;}
    AXP::AXP_ERRORS getLastError() const;
    QString getLastErrorMessage() const;

  public:
    QString getArchiveFileName() const;
    FileList& getFileList() {return m_fileList;}

  private:
    AXP::IPakFile* m_pPakFile = nullptr;
    ProgressUpdateCallback m_progressUpdateCallback = nullptr;
    QString m_fileName;
    bool m_editable = false;
    FileList m_fileList;

  signals:

  public slots:
};

#endif // AXPARCHIVEPORT_HPP
