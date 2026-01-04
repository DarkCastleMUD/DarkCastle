#ifndef DC_DATASOURCE_H_
#define DC_DATASOURCE_H_
#include <QObject>
#include <QMap>
#include <QFile>
#include <QPointer>

namespace DCNS
{
    class DataSource : public QObject
    {
        Q_OBJECT
    public:
        enum Directory
        {
            archive,
            familiar,
            follower,
            lib,
            MOBProgs,
            lib_playershops,
            save_qdata,
            lib_shops,
            vaults,
            save
        };
        Q_ENUM(Directory);
        enum File
        {
            banned,
            playerhints,
            forbidden_names,
            hints,
            skill_quests
        };
        Q_ENUM(File);

        DataSource(QObject *parent)
            : QObject(parent)
        {
        }
        inline QString source(Directory type)
        {
            if (directory_source_uri_.contains(type))
                return directory_source_uri_[type];

            QMetaEnum metaEnum = QMetaEnum::fromType<Directory>();
            return metaEnum.valueToKey(type);
        }
        inline QString source(File type)
        {
            if (file_source_uri_.contains(type))
                return file_source_uri_[type];

            QMetaEnum metaEnum = QMetaEnum::fromType<File>();
            return metaEnum.valueToKey(type);
        }
        inline QPointer<QFile> ReadQFile(auto type)
        {
            QPointer<QFile> file = new QFile(source(type));
            assert(file);
            auto is_open = file->open(QIODeviceBase::ReadOnly | QIODeviceBase::Text);
            return file;
        }
        inline QPointer<QFile> WriteQFile(auto type)
        {
            QPointer<QFile> file = new QFile(source(type));
            assert(file);
            auto is_open = file->open(QIODeviceBase::WriteOnly);
            return file;
        }

    private:
        QMap<Directory, QString> directory_source_uri_;
        QMap<File, QString> file_source_uri_;
    };
};
#endif