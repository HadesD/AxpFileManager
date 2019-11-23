#include "AxpFile.hpp"

#include "Log.hpp"

AxpFile::AxpFile(QObject *parent) : QObject(parent)
{

}

AxpFile::AxpFile(const QString& fileName) :
  m_sFileName(fileName)
{
}

AxpFile::~AxpFile()
{
  LOG_DEBUG("Closing file" << m_sFileName);
//  if (m_hFile.isOpen())
//  {
//    m_hFile.close();
//  }
}

bool AxpFile::open(const QString& fileName, const QIODevice::OpenMode mode)
{
  m_sFileName = fileName;
  return this->open(mode);
}

bool AxpFile::open(const QIODevice::OpenMode mode)
{
  LOG("Openning File:" << m_sFileName);
//  m_hFile.setFileName(m_sFileName);
//  if (!m_hFile.open(mode))
//  {
//    LOG("Open file handle failed");
//    return false;
//  }

  this->prepareCryptTable();

  m_hFile = CreateFileA(
        m_sFileName.toStdString().data(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
        );
  if (INVALID_HANDLE_VALUE == m_hFile)
  {
    LOG("INVALID_HANDLE_VALUE");
    return false;
  }

  m_uFileSize = GetFileSize(m_hFile, nullptr);

  if (!m_uFileSize)
  {
    CloseHandle(m_hFile);
    m_hFile = INVALID_HANDLE_VALUE;
    LOG("File size error");
    return false;
  }
  m_hFileMap = CreateFileMapping(m_hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);

  if (!m_hFileMap)
  {
    LOG("File map error");
    return false;
  }

  m_hMapView = MapViewOfFile(m_hFileMap, FILE_MAP_READ, 0, 0, 0);

  if (!m_hMapView)
  {
    LOG("Map view error");
    return false;
  }

  this->GetFileList();

  return true;
}

void AxpFile::prepareCryptTable()
{
  LOG(__FUNCTION__ << "called");

  /*
DWORD var_C, var_8, var_4;
LPVOID lpCryptTable = (LPVOID)&m_CryptTable;
__asm
{
    mov		eax,lpCryptTable
    mov		var_C, eax
    add		eax,400h
    mov     var_8, eax
    add		eax,400h
    mov     var_4, eax
    push    ebx
    push    esi
    mov     ebx, ecx
    push    edi
    mov     ecx, 400h
    xor     eax, eax
    mov     edi, var_C
    rep		stosd
    mov     edx, 100001h
    mov     ecx, var_8
    lea     ecx, [ecx]
$iter:
    mov     eax, edx
    imul    eax, 7Dh
    add     eax, 3
    xor     edx, edx
    mov     esi, 2AAAABh
    div     esi
    mov     edi, 2AAAABh
    add     ecx, 4
    mov     eax, edx
    mov     esi, eax
    imul    eax, 7Dh
    add     eax, 3
    xor     edx, edx
    div     edi
    shl     esi, 10h
    mov     eax, edx
    imul    eax, 7Dh
    add     eax, 3
    and     edx, 0FFFFh
    or      edx, esi
    mov     [ecx-404h], edx
    xor     edx, edx
    mov     esi, edi
    div     esi
    mov     eax, edx
    mov     esi, eax
    imul    eax, 7Dh
    add     eax, 3
    xor     edx, edx
    div     edi
    shl     esi, 10h
    mov     eax, edx
    imul    eax, 7Dh
    and     edx, 0FFFFh
    or      edx, esi
    add     eax, 3
    mov     [ecx-4], edx
    xor     edx, edx
    mov     esi, edi
    div     esi
    mov     eax, edx
    mov     esi, eax
    imul    eax, 7Dh
    add     eax, 3
    xor     edx, edx
    div     edi
    shl     esi, 10h
    mov     eax, edx
    imul    eax, 7Dh
    and     edx, 0FFFFh
    or      edx, esi
    add     eax, 3
    mov     [ecx+3FCh], edx
    xor     edx, edx
    mov     esi, edi
    div     esi
    mov     eax, edx
    mov     esi, eax
    imul    eax, 7Dh
    add     eax, 3
    xor     edx, edx
    div     edi
    shl     esi, 10h
    mov     eax, edx
    and     eax, 0FFFFh
    or      eax, esi
    cmp     ecx, var_4
    mov     [ecx+7FCh], eax
    jl      $iter
    mov		eax,lpCryptTable
}
    */

//  DWORD seed = 0x00100001;
//  std::memset(m_CryptTable, 0, 0x500 * sizeof(DWORD));

//  DWORD temp1, temp2;
//  for (DWORD i = 0; i < 0x100 ; i++)
//  {
//    LOG_DEBUG(i);
//    seed = (125 * seed + 3) % 0x2AAAAB;
//    temp1 = seed << 0x10;
//    seed = (125 * seed + 3) % 0x2AAAAB;
//    temp2 = 0xFFFF & seed;
//    m_CryptTable[i] = temp1 | temp2;

//    seed = (125 * seed + 3) % 0x2AAAAB;
//    temp1 = seed << 0x10;
//    seed = (125 * seed + 3) % 0x2AAAAB;
//    temp2 = 0xFFFF & seed;
//    m_CryptTable[i + 0x100] = temp1 | temp2;

//    seed = (125 * seed + 3) % 0x2AAAAB;
//    temp1 = seed << 0x10;
//    seed = (125 * seed + 3) % 0x2AAAAB;
//    temp2 = 0xFFFF & seed;
//    m_CryptTable[i + 0x200] = temp1 | temp2;

//    seed = (125 * seed + 3) % 0x2AAAAB;
//    temp1 = seed << 0x10;
//    seed = (125 * seed + 3) % 0x2AAAAB;
//    temp2 = 0xFFFF & seed;
//    m_CryptTable[i + 0x300] = temp1 | temp2;

//    /*
//        for (DWORD j = 0; j < 5 ; j++)
//        {
//            seed = (125 * seed + 3) % 0x2AAAAB;
//            temp1 = seed << 0x10;
//            seed = (125 * seed + 3) % 0x2AAAAB;
//            temp2 = 0xFFFF & seed;
//            m_CryptTable[(j * 0x100 + i] = (temp1 | temp2);
//        }
//        */
//  }

    DWORD dwHih, dwLow,seed = 0x00100001,index1 = 0,index2 = 0, i;
    for(index1 = 0; index1 < 0x100; index1++)
    {
        for(index2 = index1, i = 0; i < 5; i++, index2 += 0x100)
        {
            seed = (seed * 125 + 3) % 0x2AAAAB;
            //dwHih = (seed & 0xFFFF) << 0x10;
            dwHih = seed << 0x10;
            seed = (seed * 125 + 3) % 0x2AAAAB;
            dwLow = (seed & 0xFFFF);
            m_CryptTable[index2] = (dwHih | dwLow);
        }
    }
  LOG(__FUNCTION__ << "completed");
}

DWORD AxpFile::HashString(const char *lpszFileName, DWORD uCryptIndex)
{
  LOG(__FUNCTION__ << "called");
/*
    __asm
    {
        mov     esi, lpszFileName
        mov     dl, [esi]
        test    dl, dl
        mov     eax, 7FED7FEDh
        mov     ecx, 0EEEEEEEEh
        jz      short $ret
        mov     edi, dwCryptIndex
        shl     edi, 8

    $iter:
        add		eax, ecx
        imul    ecx, 21h
        movsx   edx, dl
        lea     ebx, [edi+edx]
        mov     ebp, m_CryptTable[ebx*4]
        inc     esi
        add     ecx, edx
        mov     dl, [esi]
        xor     eax, ebp
        test    dl, dl
        lea     ecx, [ecx+eax+3]
        jnz     short $iter
    $ret:
    }
*/

    signed char *key = (signed char *)(lpszFileName);
    DWORD seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;
    signed int ch;

    while(*key != 0)
    {
        ch = *key++;
        seed1 = m_CryptTable[(uCryptIndex<< 8) + ch] ^ (seed1 + seed2);
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
    }

    return seed1;
}

bool AxpFile::GetHashTablePos(const char *lpszString, int *lpFileOffset, int *lpFileSize)
{
  LOG(__FUNCTION__ << "called");
  constexpr int HASH_A = 1, HASH_B = 2, HASH_OFFSET = 3;

  int nHashA = HashString(lpszString, HASH_A);
  int nHashB = HashString(lpszString, HASH_B);

  int nHash = HashString(lpszString, HASH_OFFSET);

  nHash &= 0x7FFF;
  int nHashStart = nHash, nHashPos;

  try
  {
    while (!((nHashPos = ((int*)m_lpAxpHashTable)[nHashStart * 3 + 2]) & 0x80000000) ||\
           nHashA != ((int*)m_lpAxpHashTable)[nHashStart * 3] ||\
           nHashB != ((int*)m_lpAxpHashTable)[nHashStart * 3 + 1])
    {
      nHashStart++;
      nHashStart &= 0x7FFF;
      if (nHashStart == nHash)
      {
        return false;
      }
    }
  }
  catch (...)
  {
    return false;
  }

  nHashPos &= 0x3FFFFFFF;
  *lpFileOffset = ((int*)m_lpAxpFileInfo)[nHashPos * 3];
  *lpFileSize = ((int*)m_lpAxpFileInfo)[nHashPos * 3 + 1];

  return true;
}

bool AxpFile::GetFileList()
{
  LOG(__FUNCTION__ << "called");
  bool bResult = false;
  int nFileOffset, nFileSize;
  try
  {
    LOG(__FUNCTION__ << "trying");
    if (this->GetHashTablePos("(list)", &nFileOffset, &nFileSize))
    {
      char* lpBuffer = (char*)((DWORD)m_hMapView + nFileOffset);

      std::string liststr;
      std::vector<std::string> v;
      LOG(__FUNCTION__ << "condition");

      for(int i = 0; i < nFileSize; i++)
      {
        if (lpBuffer[i] != 0xD && lpBuffer[i + 1] != 0xA)
        {
          liststr += tolower(lpBuffer[i]);
        }
        else
        {
          std::string::size_type loc = liststr.find("|");
          if (loc != std::string::npos)
          {
//            m_FileList.push_back(liststr.substr(0, loc));
            LOG_DEBUG(QString::fromStdString(liststr.substr(0, loc)));
          }
          liststr = "" ;
          i++;
        }
      }

      bResult = true;
    }
  }
  catch (...)
  {
    bResult = false;
  }

  return bResult;
}
