#include "d_custom_logger.hpp"

#include "utiltime.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif
#include <QtCore/QFile>
#include <QtCore/QTextStream>

DCustomLogger d_cl;

void DCustomLogger::go(audlog::Level level, const char * file, int line,
                       const char * func, const char * message)
{
    if (d_cl.filter(level, file, line, func, message))
    {
        d_cl.output(level, file, line, func, message);
    }
}
DCustomLogger::DCustomLogger()
    : log_fn_(
          ("d_audlog" + msw::helpers::format_current_time("%d.%m.%y") + ".txt")
              .c_str()),
      filt_allowed_files_({"plugin.cc", "plugin.h", "gui.cc"}),
      filt_forbidden_files_({"probe-buffer.cc", "ffaudio-core.cc"})
{
}
bool DCustomLogger::filter(audlog::Level level, const char * file, int line,
                           const char * func, const char * message)
{
    if (std::find(std::begin(d_cl.filt_forbidden_files_),
                  std::end(d_cl.filt_forbidden_files_),
                  func) != std::end(d_cl.filt_forbidden_files_))
    {
        return false;
    }
    if (level >= audlog::Info)
    {
        return true;
    }
    if (std::find(std::begin(d_cl.filt_allowed_files_),
                  std::end(d_cl.filt_allowed_files_),
                  func) == std::end(d_cl.filt_allowed_files_))
    {
        return false;
    }
    return true;
}
void DCustomLogger::output(audlog::Level level, const char * file, int line,
                           const char * func, const char * message)
{
    QString str;
    QTextStream oss(&str);
    oss << msw::helpers::current_time_with_ms().c_str() << " "
        << get_level_name(level) << ' ' << file << ':' << line << " [" << func
        << "]: " << message;
    // QString str{  QString::fromUtf8(result.c_str()) };
#ifdef _WIN32
    OutputDebugStringW(str.toStdWString().c_str());
#endif
    QFile qfile(d_cl.log_fn_);
    if (qfile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        QTextStream outStream(&qfile);
        outStream << str;
    }
}
void DCustomLogger::add_file_filt(const char * const file)
{
    d_cl.filt_allowed_files_.emplace_back(file);
}
DCustomLogger::~DCustomLogger() { audlog::unsubscribe(&DCustomLogger::go); }
bool DCustomLogger::CompareFile::operator==(
    const DCustomLogger::CompareFile & rhs) const
{
    return (rhs.fn_.find(fn_) != std::string::npos);
}
bool DCustomLogger::CompareFile::operator!=(
    const DCustomLogger::CompareFile & rhs) const
{
    return !(rhs == *this);
}
DCustomLogger::CompareFile::CompareFile(std::string fn) : fn_(std::move(fn)) {}
