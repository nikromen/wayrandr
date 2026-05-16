#include "utils/config_file.hpp"

#include <qfiledevice.h>
#include <qtypes.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

#include <QFile>
#include <QLockFile>
#include <QSaveFile>
#include <QString>
#include <cerrno>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

constexpr auto ACL_ATTRIBUTE = "system.posix_acl_access";
using FileStat = struct stat;

[[noreturn]] void fail(const std::filesystem::path & path, const std::string & reason) {
    throw std::runtime_error("Configuration not saved: " + path.string() + ": " + reason);
}

auto target_path(const std::filesystem::path & path) -> std::filesystem::path {
    if (path.empty() || path.string().find('\0') != std::string::npos) {
        fail(path, "invalid path");
    }
    if (std::filesystem::is_symlink(path) && !std::filesystem::exists(path)) {
        fail(path, "dangling symlink; restore its target before saving");
    }
    const auto target = std::filesystem::weakly_canonical(std::filesystem::absolute(path));
    if (QString::fromStdString(target.string()).toStdString() != target.string()) {
        fail(path, "configuration target cannot be represented as UTF-8");
    }
    return target;
}

auto file_stat(int descriptor, const std::filesystem::path & path) -> FileStat {
    struct stat info{};
    if (fstat(descriptor, &info) != 0 || !S_ISREG(info.st_mode)) {
        fail(path, "cannot inspect a regular configuration file");
    }
    return info;
}

auto read_acl(int descriptor, const std::filesystem::path & path) -> std::string {
    const auto size = fgetxattr(descriptor, ACL_ATTRIBUTE, nullptr, 0);
    if (size < 0) {
        if (errno == ENODATA || errno == ENOTSUP) {
            return {};
        }
        fail(path, "cannot read file ACL");
    }
    std::string acl(static_cast<size_t>(size), '\0');
    if (fgetxattr(descriptor, ACL_ATTRIBUTE, acl.data(), acl.size()) != size) {
        fail(path, "cannot read file ACL");
    }
    return acl;
}

auto identity(const struct stat & info, const std::string & acl) -> std::string {
    // Inode detects replacement/recreation, bytes detect in-place edits. Permissions and
    // ownership/ACL changes must also be treated as external changes.
    std::ostringstream value;
    value << info.st_dev << ' ' << info.st_ino << ' ' << info.st_mode << ' ' << info.st_uid << ' '
          << info.st_gid << ' ' << acl;
    return value.str();
}

auto same_snapshot(const ConfigFileSnapshot & left, const ConfigFileSnapshot & right) -> bool {
    return left.target == right.target && left.content == right.content &&
        left.identity == right.identity;
}

void check_version(const std::filesystem::path & path, const ConfigFileSnapshot & expected) {
    const auto actual = read_config_file(path);
    if (expected.target.empty()) {
        if (!actual.content.has_value()) {
            return;
        }
    } else if (same_snapshot(actual, expected)) {
        return;
    }
    fail(path, "file changed externally; reload it and reconcile your edits before saving");
}

}  // namespace

auto read_config_file(const std::filesystem::path & path) -> ConfigFileSnapshot {
    ConfigFileSnapshot snapshot;
    snapshot.target = target_path(path);
    struct stat path_info{};
    if (lstat(snapshot.target.c_str(), &path_info) != 0) {
        if (errno == ENOENT) {
            return snapshot;
        }
        fail(path, "cannot inspect configuration path");
    }
    if (!S_ISREG(path_info.st_mode)) {
        fail(path, "configuration target is not a regular file");
    }
    QFile input(QString::fromStdString(snapshot.target.string()));
    if (!input.open(QIODevice::ReadOnly)) {
        fail(path, input.errorString().toStdString());
    }
    const auto before = file_stat(input.handle(), path);
    const auto acl = read_acl(input.handle(), path);
    snapshot.content = input.readAll().toStdString();
    if (input.error() != QFileDevice::NoError) {
        fail(path, input.errorString().toStdString());
    }
    const auto after = file_stat(input.handle(), path);
    snapshot.identity = identity(after, acl);
    if (identity(before, acl) != snapshot.identity ||
        before.st_mtim.tv_sec != after.st_mtim.tv_sec ||
        before.st_mtim.tv_nsec != after.st_mtim.tv_nsec ||
        before.st_ctim.tv_sec != after.st_ctim.tv_sec ||
        before.st_ctim.tv_nsec != after.st_ctim.tv_nsec ||
        lstat(snapshot.target.c_str(), &path_info) != 0 ||
        identity(path_info, acl) != snapshot.identity || target_path(path) != snapshot.target) {
        fail(path, "file changed while reading; retry loading it");
    }
    return snapshot;
}

auto save_config_file(
    const std::filesystem::path & path,
    const std::string & content,
    const ConfigFileSnapshot & expected
) -> ConfigFileSnapshot {
    check_version(path, expected);
    const auto target = target_path(path);
    std::filesystem::create_directories(target.parent_path());
    // Resolve symlink aliases to the same lock. Qt creates this exclusively and removes it
    // on normal failure/success; disable age-based eviction of a still-running writer.
    QLockFile lock(QString::fromStdString(target.string() + ".wayrandr.lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0)) {
        fail(path, "cannot acquire save lock (another writer or directory permissions)");
    }
    check_version(path, expected);

    std::optional<struct stat> original_info;
    std::string acl;
    QFile original(QString::fromStdString(target.string()));
    if (expected.content.has_value()) {
        if (!original.open(QIODevice::ReadOnly)) {
            fail(path, original.errorString().toStdString());
        }
        original_info = file_stat(original.handle(), path);
        acl = read_acl(original.handle(), path);
        if (identity(*original_info, acl) != expected.identity) {
            fail(path, "file changed externally; reload it before saving");
        }
        if ((original_info->st_mode & 0222) == 0) {
            fail(path, "configuration is read-only");
        }
    }

    QSaveFile output(QString::fromStdString(target.string()));
    output.setDirectWriteFallback(false);
    if (!output.open(QIODevice::WriteOnly)) {
        fail(path, output.errorString().toStdString());
    }
    const auto descriptor = output.handle();
    // Strip inherited ACL and restrict the staging file before putting data in it.
    if (fremovexattr(descriptor, ACL_ATTRIBUTE) != 0 && errno != ENODATA && errno != ENOTSUP) {
        fail(path, "cannot restrict temporary file ACL");
    }
    if (fchmod(descriptor, 0600) != 0) {
        fail(path, "cannot restrict temporary file permissions");
    }
    const auto size = static_cast<qint64>(content.size());
    if (output.write(content.data(), size) != size || !output.flush()) {
        fail(path, output.errorString().toStdString());
    }
    if (original_info.has_value()) {
        const auto temporary_info = file_stat(descriptor, path);
        if ((temporary_info.st_uid != original_info->st_uid ||
             temporary_info.st_gid != original_info->st_gid) &&
            fchown(descriptor, original_info->st_uid, original_info->st_gid) != 0) {
            fail(path, "cannot preserve file ownership");
        }
        if (fchmod(descriptor, original_info->st_mode & 07777) != 0 ||
            (!acl.empty() &&
             fsetxattr(descriptor, ACL_ATTRIBUTE, acl.data(), acl.size(), 0) != 0)) {
            fail(path, "cannot preserve file permissions/ACL");
        }
    }
    ConfigFileSnapshot saved;
    saved.target = target;
    saved.content = content;
    saved.identity = identity(file_stat(descriptor, path), read_acl(descriptor, path));
    check_version(path, expected);
    if (!output.commit()) {
        fail(path, output.errorString().toStdString());
    }
    // Do not re-read after commit: another editor can immediately write again. The next
    // save must compare against our own committed version, not adopt those newer bytes.
    return saved;
}
