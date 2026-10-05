#include <qtestcase.h>
#include <qtmetamacros.h>
#include <signal.h>  // NOLINT: SIGXFSZ is a POSIX extension, not a C++ signal.
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/xattr.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QObject>
#include <QString>
#include <QTest>
#include <csignal>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "backend/auto_wlr_randr/config_repository.hpp"
#include "backend/kanshi/config_repository.hpp"
#include "support/environment.hpp"
#include "support/fixture.hpp"
#include "utils/config_file.hpp"
#include "utils/helpers.hpp"

namespace {

template <typename Work>
void with_repository(int mode, const QString & path, Work work) {
    if (mode == 1) {
        work(KanshiConfigRepository(path.toStdString()));
        return;
    }
    work(AutoWlrRandrConfigRepository(path.toStdString()));
}

auto initial_config(int mode) -> std::string {
    if (mode == 1) {
        return "profile desk {\n exec true\n}\n";
    }
    return "[profile.desk]\nexec = ['true']\n";
}

// Simulate a short/failed write without consuming disk space. This test process
// has no outstanding asynchronous jobs while the process-wide limit is active.
class FileSizeLimit {
public:
    FileSizeLimit()
        : previous_signal_(std::signal(SIGXFSZ, SIG_IGN)) {
        require(getrlimit(RLIMIT_FSIZE, &previous_) == 0, "Read file limit");
        require(previous_signal_ != SIG_ERR, "Ignore file limit signal");
        auto limit = previous_;
        limit.rlim_cur = 1024;
        require(setrlimit(RLIMIT_FSIZE, &limit) == 0, "Set file limit");
    }

    ~FileSizeLimit() {
        setrlimit(RLIMIT_FSIZE, &previous_);
        std::signal(SIGXFSZ, previous_signal_);
    }

    FileSizeLimit(const FileSizeLimit &) = delete;
    auto operator=(const FileSizeLimit &) -> FileSizeLimit & = delete;

private:
    struct rlimit previous_{};
    using SignalHandler = void (*)(int);
    SignalHandler previous_signal_ = SIG_DFL;
};

}  // namespace

class ConfigStorageTests : public QObject {
    Q_OBJECT
private slots:

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.

    void repositories_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("kanshi") << 1;
        QTest::newRow("auto-wlr-randr") << 2;
    }

    void repositories() {
        QFETCH(int, mode);
        Fixture f;
        const auto original = initial_config(mode);
        f.write("config", original);
        const auto path = f.dir.filePath("config");
        QVERIFY(
            QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup)
        );
        QFile old_inode(path);
        QVERIFY(old_inode.open(QIODevice::ReadOnly));
        with_repository(mode, path, [&](const auto & repository) {
            auto config = repository.load();
            config.profiles[0].exec.push_back("notify-send saved");
            config.file_snapshot = repository.save(config);
            QCOMPARE(repository.load().profiles[0].exec, config.profiles[0].exec);
            // The old open inode still contains the complete original file.
            QCOMPARE(old_inode.readAll().toStdString(), original);
            QVERIFY(QFile::permissions(path).testFlag(QFile::ReadGroup));
            QVERIFY(!QFile::permissions(path).testFlag(QFile::ReadOther));
            config.file_snapshot = repository.save(config);
            QCOMPARE(repository.load().profiles[0].exec, config.profiles[0].exec);
        });
        QVERIFY(!QFile::exists(path + ".wayrandr.lock"));
        QCOMPARE(QDir(f.dir.path()).entryList({ "config*" }, QDir::Files), QStringList{ "config" });
    }

    void failed_write_data() { repositories_data(); }

    void failed_write() {
        QFETCH(int, mode);
        Fixture f;
        const auto original = initial_config(mode);
        f.write("config", original);
        with_repository(mode, f.dir.filePath("config"), [&](const auto & repository) {
            auto config = repository.load();
            const std::string edit = "notify-send " + std::string(65536, 'x');
            config.profiles[0].exec.push_back(edit);
            {
                const FileSizeLimit limit;
                QVERIFY_THROWS_EXCEPTION(
                    std::runtime_error, static_cast<void>(repository.save(config))
                );
            }
            QCOMPARE(f.read("config"), original);
            QCOMPARE(config.profiles[0].exec.back(), edit);
            QCOMPARE(
                QDir(f.dir.path()).entryList({ "config*" }, QDir::Files), QStringList{ "config" }
            );
            // A failed write does not consume the loaded version or the edits.
            config.file_snapshot = repository.save(config);
            QCOMPARE(repository.load().profiles[0].exec.back(), edit);
        });
    }

    void external_changes_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<QString>("change");
        for (const int mode : { 1, 2 }) {
            for (const auto * change : { "edit", "replace", "delete", "create", "permissions" }) {
                QTest::newRow(qPrintable(QString("%1-%2").arg(mode).arg(change)))
                    << mode << QString(change);
            }
        }
    }

    void external_changes() {
        QFETCH(int, mode);
        QFETCH(QString, change);
        Fixture f;
        const auto path = f.dir.filePath("config");
        const auto original = initial_config(mode);
        if (change != "create") {
            f.write("config", original);
        }
        with_repository(mode, path, [&](const auto & repository) {
            auto config = repository.load();
            if (change == "create") {
                f.write("config", original);
            } else if (change == "delete") {
                QVERIFY(QFile::remove(path));
            } else if (change == "replace") {
                // Retain the old inode so replacement cannot reuse its number.
                QVERIFY(QFile::rename(path, f.dir.filePath("old")));
                f.write("config", original);
            } else if (change == "permissions") {
                QVERIFY(QFile::setPermissions(path, QFile::ReadOwner));
            } else {
                f.write("config", original + "# external edit\n");
            }
            const auto before = read_config_file(path.toStdString());
            QVERIFY_THROWS_EXCEPTION(
                std::runtime_error, static_cast<void>(repository.save(config))
            );
            QCOMPARE(read_config_file(path.toStdString()).content, before.content);
            QCOMPARE(QFile::exists(path), change != "delete");
        });
    }

    void symlinks_data() { repositories_data(); }

    void symlinks() {
        QFETCH(int, mode);
        Fixture f;
        const auto original = initial_config(mode);
        f.write("target", original);
        const auto path = f.dir.filePath("config");
        std::filesystem::create_symlink("target", path.toStdString());
        with_repository(mode, path, [&](const auto & repository) {
            auto config = repository.load();
            config.profiles[0].exec.push_back("notify-send saved");
            config.file_snapshot = repository.save(config);
            QVERIFY(QFileInfo(path).isSymLink());
            QCOMPARE(repository.load().profiles[0].exec, config.profiles[0].exec);
            QVERIFY(f.read("target") != original);
            f.write("other", original);
            QVERIFY(QFile::remove(path));
            std::filesystem::create_symlink("other", path.toStdString());
            QVERIFY_THROWS_EXCEPTION(
                std::runtime_error, static_cast<void>(repository.save(config))
            );
            QCOMPARE(f.read("other"), original);
            QVERIFY(QFileInfo(path).isSymLink());
        });
    }

    void invalid_paths_data() { repositories_data(); }

    void invalid_encoding() {
        Fixture f;
        const std::filesystem::path path = f.dir.path().toStdString() + "/config-\xff";
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, static_cast<void>(read_config_file(path)));
        QVERIFY_THROWS_EXCEPTION(
            std::runtime_error, static_cast<void>(save_config_file(path, "data", {}))
        );
        QVERIFY(QDir(f.dir.path()).entryList({ "config*" }, QDir::Files).isEmpty());
    }

    void invalid_paths() {
        QFETCH(int, mode);
        Fixture f;
        const auto path = f.dir.filePath("new/config");
        with_repository(mode, path, [&](const auto & repository) {
            auto config = repository.load();
            f.write("new", "parent is a file");
            QVERIFY_THROWS_EXCEPTION(std::exception, static_cast<void>(repository.save(config)));
            QCOMPARE(f.read("new"), std::string("parent is a file"));
            QVERIFY(QFile::remove(f.dir.filePath("new")));
            config.file_snapshot = repository.save(config);
            QCOMPARE(
                QFile::permissions(path) &
                    (QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther),
                QFile::Permissions{}
            );
        });
    }

    void read_only_data() { repositories_data(); }

    void read_only() {
        QFETCH(int, mode);
        Fixture f;
        const auto original = initial_config(mode);
        f.write("config", original);
        const auto path = f.dir.filePath("config");
        QVERIFY(QFile::setPermissions(path, QFile::ReadOwner));
        with_repository(mode, path, [&](const auto & repository) {
            auto config = repository.load();
            config.profiles[0].exec.push_back("notify-send unsaved");
            QVERIFY_THROWS_EXCEPTION(
                std::runtime_error, static_cast<void>(repository.save(config))
            );
            QCOMPARE(f.read("config"), original);
            QCOMPARE(config.profiles[0].exec.back(), std::string("notify-send unsaved"));
            QVERIFY(!QFile::exists(path + ".wayrandr.lock"));
        });
    }

    void acl_permissions_data() { repositories_data(); }

    void acl_permissions() {
        QFETCH(int, mode);
        Fixture f;
        f.write("config", initial_config(mode));
        const auto path = f.dir.filePath("config").toStdString();
        // Linux POSIX ACL: owner rw, uid 65534 read, owning group none,
        // mask read, others none. Mode bits alone would grant group access.
        const auto acl = QByteArray::fromHex(
            "0200000001000600ffffffff02000400feff000004000000ffffffff"
            "10000400ffffffff20000000ffffffff"
        );
        QVERIFY(
            setxattr(
                path.c_str(),
                "system.posix_acl_access",
                acl.constData(),
                static_cast<size_t>(acl.size()),
                0
            ) == 0
        );
        with_repository(mode, QString::fromStdString(path), [&](const auto & repository) {
            auto config = repository.load();
            config.profiles[0].exec.push_back("notify-send saved");
            config.file_snapshot = repository.save(config);
            QByteArray actual(acl.size(), '\0');
            QCOMPARE(
                getxattr(
                    path.c_str(),
                    "system.posix_acl_access",
                    actual.data(),
                    static_cast<size_t>(actual.size())
                ),
                static_cast<ssize_t>(acl.size())
            );
            QCOMPARE(actual, acl);
            config.file_snapshot = repository.save(config);
            QCOMPARE(repository.load().profiles[0].exec, config.profiles[0].exec);
        });
    }

    void locked_writer_data() { repositories_data(); }

    void locked_writer() {
        QFETCH(int, mode);
        Fixture f;
        const auto original = initial_config(mode);
        f.write("config", original);
        const auto path = f.dir.filePath("config");
        with_repository(mode, path, [&](const auto & repository) {
            auto config = repository.load();
            QLockFile lock(path + ".wayrandr.lock");
            QVERIFY(lock.tryLock());
            QVERIFY_THROWS_EXCEPTION(
                std::runtime_error, static_cast<void>(repository.save(config))
            );
            QCOMPARE(f.read("config"), original);
            lock.unlock();
            config.file_snapshot = repository.save(config);
        });
    }
};
QTEST_MAIN(ConfigStorageTests)
#include "config_storage.moc"
