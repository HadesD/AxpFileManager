#ifndef AXPFILE_HPP
#define AXPFILE_HPP

#include <QObject>
#include <QFile>
#include <Windows.h>

class AxpFile : public QObject
{
    Q_OBJECT

  private:
    typedef struct _AXPHeader
    {
        int signature;				// Axp文件标志
        int nUnknow0;
        int nUnknow1;
        int nHashTableOffset;		// 哈希表文件偏移
        int nIndexTableOffset;		// 文件索引信息偏移
        int nFileCount;				// 文件数量
        int nSizeOfIndexTable;		// 文件索引表大小
        int nDataOffset;
        int nUnknow3;
        int nUnknow4;
    } AXPHeader;

    typedef struct _AXPHashTable
    {
        DWORD nHashA;
        DWORD nHashB;
        DWORD bExists;
    } AXPHashTable;

    typedef struct _AXPFileInfo
    {
        int nFileOffset;				// 文件偏移
        int nFileSize;					// 文件大小
        int nFileFlag;					// 文件标志
    } AXPFileInfo;

  public:
    explicit AxpFile(QObject *parent = nullptr);
    AxpFile(const QString& fileName);
    ~AxpFile();

  public:
    bool open(const QIODevice::OpenMode mode);
    bool open(const QString& fileName, const QIODevice::OpenMode mode);

  private:
    void prepareCryptTable();
    DWORD HashString(const char *lpszFileName, DWORD uCryptIndex);
    bool GetHashTablePos(const char *lpszString, int *lpFileOffset, int *lpFileSize);
    bool GetFileList();

  private:
    QString m_sFileName;
//    QFile m_hFile;
    HANDLE m_hFile;
    HANDLE m_hFileMap;
    LPVOID m_hMapView;
    DWORD m_uFileSize;

    DWORD m_CryptTable[0x500];

  private:
    AXPHeader *m_lpAxPHeader = nullptr;
    AXPHashTable **m_lpAxpHashTable = nullptr;
    AXPFileInfo	**m_lpAxpFileInfo = nullptr;

  signals:

  public slots:
};

#endif // AXPFILE_HPP
