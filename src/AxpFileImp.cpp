#include "AxpFile.hpp"

#include "Log.hpp"

template<typename T>
T AxpFile::read(const uint offset, const uint size)
{
  m_mutex.lock();
  m_hFile.seek(offset);
  T ret;
  m_hFile.read(reinterpret_cast<char*>(&ret), size);
  m_mutex.unlock();
  return ret;
}

template<>
QByteArray AxpFile::read(const uint offset, const uint size)
{
  m_mutex.lock();
  m_hFile.seek(offset);
  auto ret = m_hFile.read(size);
  m_mutex.unlock();
  return ret;
}
