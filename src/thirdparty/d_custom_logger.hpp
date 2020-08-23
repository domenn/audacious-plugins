#pragma once

#include <QtCore/QString>
#include <libaudcore/runtime.h>
#include <string>
#include <vector>
class DCustomLogger
{

    class CompareFile{
        std::string fn_;

    public:
        bool operator==(const CompareFile & rhs) const;
        bool operator==(const std::string & rhs) const;
        bool operator!=(const std::string & rhs) const;
        bool operator!=(const CompareFile & rhs) const;

        CompareFile(std::string fn); // NOLINT(google-explicit-constructor)
    };


    QString log_fn_;
    std::vector<CompareFile> filt_allowed_files_;
    std::vector<CompareFile> filt_forbidden_files_;

    static std::vector<CompareFile> mkvec(std::initializer_list<std::string> il);

public:
    DCustomLogger();

    static void go(audlog::Level level, const char * file, int line,
                   const char * func, const char * message);
    static void add_file_filt(const char * const file);

private:
    void output(audlog::Level level, const char * file, int line,
                const char * func, const char * message);
    bool filter(audlog::Level level, const char * file, int line,
                const char * func, const char * message);
};

//#define X____2_SECRET_LINE(X) ____2_SECRET_LINE(X)
//#define ____2_SECRET_LINE(X) #X
//#define ADD_TO_FILTER(filename) \
//static bool _____111__add_me_to_##__LINE__##() { \
//     \
//    }
