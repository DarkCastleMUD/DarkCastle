#ifndef LEGACY_FILE_H_
#define LEGACY_FILE_H_

#include <QString>
#include <QFile>

class LegacyFile
{
public:
    explicit LegacyFile(QString directory_name, QString filename, QString error_message, FILE *stream = nullptr)
        : directory_name_(directory_name), filename_(filename), error_message_(error_message), stream_(stream)
    {
        full_filename_ = QString(directory_name + "/%1").arg(filename);
        full_new_filename_ = QString(directory_name + "/%1.new").arg(filename);
        full_last_filename_ = QString(directory_name + "/%1.last").arg(filename);
        open();
    }
    inline FILE *getStream(void) { return stream_; }

    ~LegacyFile(void)
    {
        fprintf(stream_, "$~\n");
        fclose(stream_);
        QFile file(full_filename_);
        file.copy(full_last_filename_);

        QFile new_file(full_new_filename_);
        new_file.rename(full_filename_);
    }

private:
    void open(void);
    QString directory_name_ = {};
    QString filename_ = {};
    QString error_message_ = {};

    FILE *stream_ = {};
    QString full_filename_ = {};
    QString full_new_filename_ = {};
    QString full_last_filename_ = {};
};

#endif