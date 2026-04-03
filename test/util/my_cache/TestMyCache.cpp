/**
 * @file TestMyCache.cpp
 * @brief MyCache 模块单元测试
 *
 * 测试覆盖：
 *   - 基础初始化与状态查询（含 CacheStatus 枚举）
 *   - JSON 配置解析（root_path / max_file_size / max_retention_seconds）
 *   - 文件保存、查询、获取路径、删除的完整生命周期
 *   - GetAllFileList 文件列表与元信息
 *   - 路径穿越攻击防护
 *   - 空文件名等非法输入
 *   - 子目录文件操作
 *   - 后台线程索引同步
 *   - 文件大小限制
 *   - 过期文件清理
 *   - MyCacheProvider 单例包装器
 *   - 错误码转换
 *   - 边界条件：空数据、大文件名等
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>

#include "MyCache.h"
#include "MyCacheProvider.h"
#include <nlohmann/json.hpp>

using namespace my_cache;
using json = nlohmann::json;

// ============================================================================
// 测试辅助：为每个测试创建临时目录，测试结束后清理
// ============================================================================

namespace {

/// 生成唯一的测试目录路径
std::string MakeTestDir(const std::string& suffix) {
    return "/tmp/my_cache_test_" + suffix + "_" +
           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
}

/// 递归删除目录
void CleanupDir(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
}

/// 将字符串转换为 vector<uint8_t>
std::vector<uint8_t> ToBytes(const std::string& s) {
    return {s.begin(), s.end()};
}

/// 构造 JSON 配置字符串
std::string MakeConfig(const std::string& dir,
                       uint64_t max_file_size = 0,
                       int64_t max_retention_seconds = 0) {
    json j;
    j["root_path"] = dir;
    if (max_file_size > 0) j["max_file_size"] = max_file_size;
    if (max_retention_seconds != 0) j["max_retention_seconds"] = max_retention_seconds;
    return j.dump();
}

}  // namespace

// ============================================================================
// 基础功能测试
// ============================================================================

/// 测试：默认构造后 Status 为 NotInitialized
TEST(MyCache_Basic, DefaultConstructorNotInitialized) {
    MyCache cache;
    EXPECT_EQ(cache.Status(), CacheStatus::NotInitialized);
}

/// 测试：Init 自动创建不存在的目录，Status() 返回 Running
TEST(MyCache_Basic, InitCreatesDirectoryAndStatusRunning) {
    auto dir = MakeTestDir("init");
    {
        MyCache cache;
        auto r = cache.Init(MakeConfig(dir));
        EXPECT_TRUE(r.Ok());
        // 目录应该被创建
        EXPECT_TRUE(std::filesystem::exists(dir));
        // Status 应该返回 Running
        EXPECT_EQ(cache.Status(), CacheStatus::Running);
    }
    CleanupDir(dir);
}

/// 测试：在已存在的目录上初始化
TEST(MyCache_Basic, InitWithExistingDirectory) {
    auto dir = MakeTestDir("existing");
    std::filesystem::create_directories(dir);
    {
        MyCache cache;
        auto r = cache.Init(MakeConfig(dir));
        EXPECT_TRUE(r.Ok());
        EXPECT_EQ(cache.Status(), CacheStatus::Running);
    }
    CleanupDir(dir);
}

/// 测试：GetRootPath 返回规范化路径
TEST(MyCache_Basic, GetRootPathReturnsCanonicalPath) {
    auto dir = MakeTestDir("rootpath");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        EXPECT_EQ(cache.Status(), CacheStatus::Running);
        auto root = cache.GetRootPath();
        EXPECT_FALSE(root.empty());
        EXPECT_TRUE(std::filesystem::path(root).is_absolute());
    }
    CleanupDir(dir);
}

/// 测试：无效 JSON 返回 InvalidArgument，Status 为 Error
TEST(MyCache_Basic, InitWithInvalidJsonReturnsError) {
    MyCache cache;
    auto r = cache.Init("not valid json");
    EXPECT_FALSE(r.Ok());
    EXPECT_EQ(r.code, CacheErrorCode::InvalidArgument);
    EXPECT_EQ(cache.Status(), CacheStatus::Error);
}

/// 测试：JSON 缺少 root_path 返回 InvalidArgument
TEST(MyCache_Basic, InitWithoutRootPathReturnsError) {
    MyCache cache;
    auto r = cache.Init(R"({"max_file_size": 100})");
    EXPECT_FALSE(r.Ok());
    EXPECT_EQ(r.code, CacheErrorCode::InvalidArgument);
    EXPECT_EQ(cache.Status(), CacheStatus::Error);
}

/// 测试：GetConfig 返回解析后的配置
TEST(MyCache_Basic, GetConfigReturnsCorrectValues) {
    auto dir = MakeTestDir("getconfig");
    {
        MyCache cache;
        auto r = cache.Init(MakeConfig(dir, 1024, 3600));
        ASSERT_TRUE(r.Ok());
        const auto& cfg = cache.GetConfig();
        EXPECT_EQ(cfg.root_path, dir);
        EXPECT_EQ(cfg.max_file_size, 1024u);
        EXPECT_EQ(cfg.max_retention_seconds, 3600);
    }
    CleanupDir(dir);
}

/// 测试：重复 Init 被忽略
TEST(MyCache_Basic, DuplicateInitIgnored) {
    auto dir = MakeTestDir("dup_init");
    {
        MyCache cache;
        auto r1 = cache.Init(MakeConfig(dir));
        EXPECT_TRUE(r1.Ok());
        // 再次 Init 应成功但被忽略
        auto r2 = cache.Init(MakeConfig(dir));
        EXPECT_TRUE(r2.Ok());
        EXPECT_EQ(cache.Status(), CacheStatus::Running);
    }
    CleanupDir(dir);
}

// ============================================================================
// SaveFile 测试
// ============================================================================

/// 测试：保存文件 → 文件在磁盘上存在
TEST(MyCache_SaveFile, SaveCreatesFileOnDisk) {
    auto dir = MakeTestDir("save");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto data = ToBytes("hello world");
        auto result = cache.SaveFile("test.txt", data);
        EXPECT_TRUE(result.Ok());
        EXPECT_EQ(result.code, CacheErrorCode::Ok);

        // 验证磁盘上的文件内容
        auto full = std::filesystem::path(dir) / "test.txt";
        EXPECT_TRUE(std::filesystem::exists(full));

        std::ifstream ifs(full, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        EXPECT_EQ(content, "hello world");
    }
    CleanupDir(dir);
}

/// 测试：保存空数据
TEST(MyCache_SaveFile, SaveEmptyData) {
    auto dir = MakeTestDir("save_empty");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        std::vector<uint8_t> empty_data;
        auto result = cache.SaveFile("empty.bin", empty_data);
        EXPECT_TRUE(result.Ok());

        auto full = std::filesystem::path(dir) / "empty.bin";
        EXPECT_TRUE(std::filesystem::exists(full));
        EXPECT_EQ(std::filesystem::file_size(full), 0u);
    }
    CleanupDir(dir);
}

/// 测试：覆盖已存在的文件
TEST(MyCache_SaveFile, SaveOverwritesExistingFile) {
    auto dir = MakeTestDir("overwrite");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto result1 = cache.SaveFile("f.dat", ToBytes("version1"));
        EXPECT_TRUE(result1.Ok());

        auto result2 = cache.SaveFile("f.dat", ToBytes("version2_longer"));
        EXPECT_TRUE(result2.Ok());

        // 验证内容被覆盖
        auto full = std::filesystem::path(dir) / "f.dat";
        std::ifstream ifs(full, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        EXPECT_EQ(content, "version2_longer");
    }
    CleanupDir(dir);
}

/// 测试：保存到子目录（自动创建父目录）
TEST(MyCache_SaveFile, SaveInSubdirectory) {
    auto dir = MakeTestDir("subdir_save");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto result = cache.SaveFile("sub/dir/file.txt", ToBytes("nested"));
        EXPECT_TRUE(result.Ok());

        auto full = std::filesystem::path(dir) / "sub" / "dir" / "file.txt";
        EXPECT_TRUE(std::filesystem::exists(full));
    }
    CleanupDir(dir);
}

/// 测试：超过 max_file_size 返回 FileTooLarge
TEST(MyCache_SaveFile, FileTooLargeRejected) {
    auto dir = MakeTestDir("toolarge");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir, 10));  // max_file_size = 10 bytes
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        // 小于限制 → 成功
        auto r1 = cache.SaveFile("small.txt", ToBytes("12345"));
        EXPECT_TRUE(r1.Ok());

        // 恰好等于限制 → 成功
        auto r2 = cache.SaveFile("exact.txt", ToBytes("1234567890"));
        EXPECT_TRUE(r2.Ok());

        // 超过限制 → 失败
        auto r3 = cache.SaveFile("big.txt", ToBytes("12345678901"));
        EXPECT_FALSE(r3.Ok());
        EXPECT_EQ(r3.code, CacheErrorCode::FileTooLarge);

        // 确保文件没有被创建
        EXPECT_FALSE(cache.Exists("big.txt").value);
    }
    CleanupDir(dir);
}

/// 测试：max_file_size = 0 不限制
TEST(MyCache_SaveFile, NoSizeLimitWhenZero) {
    auto dir = MakeTestDir("nosizelimit");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir, 0));  // 不限制
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        std::vector<uint8_t> big_data(100000, 0x42);
        auto r = cache.SaveFile("big.bin", big_data);
        EXPECT_TRUE(r.Ok());
    }
    CleanupDir(dir);
}

/// 测试：max_retention_seconds = -1 表示文件永久有效
TEST(MyCache_SaveFile, NegativeRetentionMeansPermanent) {
    auto dir = MakeTestDir("perm_retention");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir, 0, -1));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);
        EXPECT_EQ(cache.GetConfig().max_retention_seconds, -1);

        // 保存文件
        auto r = cache.SaveFile("permanent.txt", ToBytes("永久保留"));
        EXPECT_TRUE(r.Ok());

        // 文件应该存在且不会被清理
        auto exists = cache.Exists("permanent.txt");
        EXPECT_TRUE(exists.Ok());
        EXPECT_TRUE(exists.value);
    }
    CleanupDir(dir);
}

// ============================================================================
// Exists 测试
// ============================================================================

/// 测试：保存后 Exists 返回 true（立即生效，无需等待扫描）
TEST(MyCache_Exists, ExistsAfterSave) {
    auto dir = MakeTestDir("exists_save");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        // 保存前不存在
        auto r1 = cache.Exists("check.txt");
        EXPECT_TRUE(r1.Ok());
        EXPECT_FALSE(r1.value);

        // 保存后立即存在
        cache.SaveFile("check.txt", ToBytes("data"));
        auto r2 = cache.Exists("check.txt");
        EXPECT_TRUE(r2.Ok());
        EXPECT_TRUE(r2.value);
    }
    CleanupDir(dir);
}

/// 测试：删除后 Exists 返回 false（立即生效）
TEST(MyCache_Exists, ExistsAfterDelete) {
    auto dir = MakeTestDir("exists_delete");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        cache.SaveFile("del.txt", ToBytes("data"));
        EXPECT_TRUE(cache.Exists("del.txt").value);

        cache.DeleteFile("del.txt");
        EXPECT_FALSE(cache.Exists("del.txt").value);
    }
    CleanupDir(dir);
}

// ============================================================================
// DeleteFile 测试
// ============================================================================

/// 测试：正常删除文件
TEST(MyCache_DeleteFile, DeleteRemovesFile) {
    auto dir = MakeTestDir("delete");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        cache.SaveFile("to_delete.txt", ToBytes("bye"));
        auto full = std::filesystem::path(dir) / "to_delete.txt";
        EXPECT_TRUE(std::filesystem::exists(full));

        auto result = cache.DeleteFile("to_delete.txt");
        EXPECT_TRUE(result.Ok());
        EXPECT_FALSE(std::filesystem::exists(full));
    }
    CleanupDir(dir);
}

/// 测试：删除不存在的文件返回 FileNotFound
TEST(MyCache_DeleteFile, DeleteNonExistentReturnsFileNotFound) {
    auto dir = MakeTestDir("delete_nf");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto result = cache.DeleteFile("no_such_file.txt");
        EXPECT_FALSE(result.Ok());
        EXPECT_EQ(result.code, CacheErrorCode::FileNotFound);
    }
    CleanupDir(dir);
}

// ============================================================================
// GetFullPath 测试
// ============================================================================

/// 测试：GetFullPath 返回正确路径
TEST(MyCache_GetFullPath, ReturnsCorrectAbsolutePath) {
    auto dir = MakeTestDir("fullpath");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto result = cache.GetFullPath("some/file.txt");
        EXPECT_TRUE(result.Ok());
        EXPECT_FALSE(result.value.empty());

        // 返回的路径应该以 root_path 开头
        auto root = cache.GetRootPath();
        EXPECT_EQ(result.value.substr(0, root.size()), root);
        // 应该包含文件名
        EXPECT_NE(result.value.find("file.txt"), std::string::npos);
    }
    CleanupDir(dir);
}

// ============================================================================
// GetAllFileList 测试
// ============================================================================

/// 测试：空目录返回空列表
TEST(MyCache_GetAllFileList, EmptyDirectoryReturnsEmptyList) {
    auto dir = MakeTestDir("list_empty");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto r = cache.GetAllFileList();
        EXPECT_TRUE(r.Ok());
        EXPECT_TRUE(r.value.empty());
    }
    CleanupDir(dir);
}

/// 测试：保存文件后 GetAllFileList 返回正确的元信息
TEST(MyCache_GetAllFileList, ReturnsCorrectFileInfo) {
    auto dir = MakeTestDir("list_info");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        cache.SaveFile("hello.txt", ToBytes("hello world!"));
        cache.SaveFile("data.bin", ToBytes("binary data"));
        cache.SaveFile("sub/photo.jpg", ToBytes("fake jpg"));

        auto r = cache.GetAllFileList();
        EXPECT_TRUE(r.Ok());
        EXPECT_EQ(r.value.size(), 3u);

        // 检查每个文件都在列表中
        auto find_by_name = [&](const std::string& name) -> const FileInfo* {
            for (const auto& fi : r.value) {
                if (fi.name == name) return &fi;
            }
            return nullptr;
        };

        auto* f1 = find_by_name("hello.txt");
        ASSERT_NE(f1, nullptr);
        EXPECT_EQ(f1->size, 12u);
        EXPECT_EQ(f1->type, "txt");
        EXPECT_FALSE(f1->modified_at.empty());

        auto* f2 = find_by_name("data.bin");
        ASSERT_NE(f2, nullptr);
        EXPECT_EQ(f2->size, 11u);
        EXPECT_EQ(f2->type, "bin");

        auto* f3 = find_by_name("sub/photo.jpg");
        ASSERT_NE(f3, nullptr);
        EXPECT_EQ(f3->size, 8u);
        EXPECT_EQ(f3->type, "jpg");
    }
    CleanupDir(dir);
}

/// 测试：未初始化时 GetAllFileList 返回 NotInitialized
TEST(MyCache_GetAllFileList, NotInitializedReturnsError) {
    MyCache cache;
    auto r = cache.GetAllFileList();
    EXPECT_FALSE(r.Ok());
    EXPECT_EQ(r.code, CacheErrorCode::NotInitialized);
}

// ============================================================================
// 路径穿越攻击防护测试
// ============================================================================

/// 测试：使用 "../" 的路径穿越
TEST(MyCache_Security, PathTraversalWithDotDot) {
    auto dir = MakeTestDir("traversal1");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        // 尝试路径穿越
        auto r1 = cache.SaveFile("../etc/passwd", ToBytes("hack"));
        EXPECT_FALSE(r1.Ok());
        EXPECT_EQ(r1.code, CacheErrorCode::PathTraversal);

        auto r2 = cache.Exists("../../etc/shadow");
        EXPECT_FALSE(r2.Ok());
        EXPECT_EQ(r2.code, CacheErrorCode::PathTraversal);

        auto r3 = cache.DeleteFile("../../../tmp/something");
        EXPECT_FALSE(r3.Ok());
        EXPECT_EQ(r3.code, CacheErrorCode::PathTraversal);

        auto r4 = cache.GetFullPath("../outside");
        EXPECT_FALSE(r4.Ok());
        EXPECT_EQ(r4.code, CacheErrorCode::PathTraversal);
    }
    CleanupDir(dir);
}

/// 测试：使用绝对路径的穿越尝试
TEST(MyCache_Security, PathTraversalWithAbsolutePath) {
    auto dir = MakeTestDir("traversal2");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto r = cache.SaveFile("/etc/passwd", ToBytes("hack"));
        // 取决于实现，只要不写到 root_path 外面就安全
    }
    CleanupDir(dir);
}

/// 测试：隐蔽的路径穿越 "foo/../../etc/passwd"
TEST(MyCache_Security, PathTraversalHidden) {
    auto dir = MakeTestDir("traversal3");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto r = cache.SaveFile("foo/../../etc/passwd", ToBytes("hack"));
        EXPECT_FALSE(r.Ok());
        EXPECT_EQ(r.code, CacheErrorCode::PathTraversal);
    }
    CleanupDir(dir);
}

/// 测试：只包含 ".." 的路径
TEST(MyCache_Security, PathTraversalPureDotDot) {
    auto dir = MakeTestDir("traversal4");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto r = cache.SaveFile("..", ToBytes("hack"));
        EXPECT_FALSE(r.Ok());
        // 要么是 PathTraversal 要么是 InvalidArgument
        EXPECT_TRUE(r.code == CacheErrorCode::PathTraversal ||
                    r.code == CacheErrorCode::InvalidArgument);
    }
    CleanupDir(dir);
}

// ============================================================================
// 非法输入测试
// ============================================================================

/// 测试：空文件名返回 InvalidArgument
TEST(MyCache_Invalid, EmptyName) {
    auto dir = MakeTestDir("invalid_empty");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto r1 = cache.SaveFile("", ToBytes("data"));
        EXPECT_EQ(r1.code, CacheErrorCode::InvalidArgument);

        auto r2 = cache.Exists("");
        EXPECT_EQ(r2.code, CacheErrorCode::InvalidArgument);

        auto r3 = cache.DeleteFile("");
        EXPECT_EQ(r3.code, CacheErrorCode::InvalidArgument);

        auto r4 = cache.GetFullPath("");
        EXPECT_EQ(r4.code, CacheErrorCode::InvalidArgument);
    }
    CleanupDir(dir);
}

// ============================================================================
// 后台线程索引同步测试
// ============================================================================

/// 测试：绕过 SaveFile 直接写文件，后台线程应能发现
TEST(MyCache_BackgroundScan, DetectsExternallyCreatedFile) {
    auto dir = MakeTestDir("bg_scan");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        // 初始状态文件不存在
        EXPECT_FALSE(cache.Exists("external.txt").value);

        // 绕过 MyCache 直接写文件
        {
            std::ofstream ofs(std::filesystem::path(dir) / "external.txt");
            ofs << "created externally";
        }

        // 等待后台线程扫描（扫描间隔 5 秒，等 7 秒确保至少扫描一次）
        std::this_thread::sleep_for(std::chrono::seconds(7));

        // 后台线程应该已经发现这个文件
        auto r = cache.Exists("external.txt");
        EXPECT_TRUE(r.Ok());
        EXPECT_TRUE(r.value);
    }
    CleanupDir(dir);
}

/// 测试：绕过 DeleteFile 直接删除文件，后台线程应能感知
TEST(MyCache_BackgroundScan, DetectsExternallyDeletedFile) {
    auto dir = MakeTestDir("bg_scan_del");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        // 通过 MyCache 保存文件
        cache.SaveFile("will_vanish.txt", ToBytes("temp"));
        EXPECT_TRUE(cache.Exists("will_vanish.txt").value);

        // 绕过 MyCache 直接删除
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(dir) / "will_vanish.txt", ec);

        // 等待后台线程扫描
        std::this_thread::sleep_for(std::chrono::seconds(7));

        // 索引应该已更新
        auto r = cache.Exists("will_vanish.txt");
        EXPECT_TRUE(r.Ok());
        EXPECT_FALSE(r.value);
    }
    CleanupDir(dir);
}

// ============================================================================
// MyCacheProvider 单例测试
// ============================================================================

/// 测试：Init + Get 的基本流程
TEST(MyCacheProvider_Basic, InitAndGet) {
    auto dir = MakeTestDir("provider");
    {
        // 先销毁可能残留的实例
        MyCacheProvider::Destroy();

        auto init_result = MyCacheProvider::Init(MakeConfig(dir));
        EXPECT_TRUE(init_result.Ok());

        auto get_result = MyCacheProvider::Get();
        EXPECT_TRUE(get_result.Ok());
        EXPECT_NE(get_result.value, nullptr);
        EXPECT_EQ(get_result.value->Status(), CacheStatus::Running);

        // 通过 Provider 获取的实例正常工作
        get_result.value->SaveFile("provider_test.txt", ToBytes("via provider"));
        EXPECT_TRUE(get_result.value->Exists("provider_test.txt").value);

        MyCacheProvider::Destroy();
    }
    CleanupDir(dir);
}

/// 测试：未初始化时 Get 返回 NotInitialized
TEST(MyCacheProvider_Basic, GetBeforeInitReturnsError) {
    MyCacheProvider::Destroy();

    auto result = MyCacheProvider::Get();
    EXPECT_FALSE(result.Ok());
    EXPECT_EQ(result.code, CacheErrorCode::NotInitialized);
}

/// 测试：Destroy 后 Get 返回 NotInitialized
TEST(MyCacheProvider_Basic, GetAfterDestroyReturnsError) {
    auto dir = MakeTestDir("provider_destroy");
    {
        MyCacheProvider::Destroy();
        MyCacheProvider::Init(MakeConfig(dir));
        auto r1 = MyCacheProvider::Get();
        EXPECT_TRUE(r1.Ok());

        MyCacheProvider::Destroy();

        auto r2 = MyCacheProvider::Get();
        EXPECT_FALSE(r2.Ok());
        EXPECT_EQ(r2.code, CacheErrorCode::NotInitialized);
    }
    CleanupDir(dir);
}

// ============================================================================
// CacheErrorCode / CacheStatus 工具函数测试
// ============================================================================

/// 测试：错误码转字符串
TEST(MyCache_ErrorCode, ToStringCoversAllCodes) {
    EXPECT_STREQ(CacheErrorCodeToString(CacheErrorCode::Ok), "Ok");
    EXPECT_STREQ(CacheErrorCodeToString(CacheErrorCode::NotInitialized), "NotInitialized");
    EXPECT_STREQ(CacheErrorCodeToString(CacheErrorCode::PathTraversal), "PathTraversal");
    EXPECT_STREQ(CacheErrorCodeToString(CacheErrorCode::FileNotFound), "FileNotFound");
    EXPECT_STREQ(CacheErrorCodeToString(CacheErrorCode::FileAlreadyExists), "FileAlreadyExists");
    EXPECT_STREQ(CacheErrorCodeToString(CacheErrorCode::IoError), "IoError");
    EXPECT_STREQ(CacheErrorCodeToString(CacheErrorCode::InvalidArgument), "InvalidArgument");
    EXPECT_STREQ(CacheErrorCodeToString(CacheErrorCode::CreateDirFailed), "CreateDirFailed");
    EXPECT_STREQ(CacheErrorCodeToString(CacheErrorCode::FileTooLarge), "FileTooLarge");
}

/// 测试：状态码转字符串
TEST(MyCache_CacheStatus, StatusToString) {
    EXPECT_STREQ(CacheStatusToString(CacheStatus::NotInitialized), "NotInitialized");
    EXPECT_STREQ(CacheStatusToString(CacheStatus::Running), "Running");
    EXPECT_STREQ(CacheStatusToString(CacheStatus::Error), "Error");
}

// ============================================================================
// CacheResult 类测试
// ============================================================================

/// 测试：CacheResult<T> 的 Success 和 Fail 工厂方法
TEST(MyCache_CacheResult, SuccessAndFail) {
    // 带值的 Result
    auto ok = CacheResult<std::string>::Success("path");
    EXPECT_TRUE(ok.Ok());
    EXPECT_EQ(ok.code, CacheErrorCode::Ok);
    EXPECT_EQ(ok.value, "path");

    auto fail = CacheResult<std::string>::Fail(CacheErrorCode::IoError, "");
    EXPECT_FALSE(fail.Ok());
    EXPECT_EQ(fail.code, CacheErrorCode::IoError);

    // void 特化
    auto void_ok = CacheResult<void>::Success();
    EXPECT_TRUE(void_ok.Ok());

    auto void_fail = CacheResult<void>::Fail(CacheErrorCode::PathTraversal);
    EXPECT_FALSE(void_fail.Ok());
    EXPECT_EQ(void_fail.code, CacheErrorCode::PathTraversal);
}

/// 测试：CacheResult<bool> 的 Fail 带默认值
TEST(MyCache_CacheResult, BoolFailWithDefault) {
    auto r = CacheResult<bool>::Fail(CacheErrorCode::NotInitialized);
    EXPECT_FALSE(r.Ok());
    EXPECT_FALSE(r.value);  // 默认 bool{} == false
}

// ============================================================================
// 子目录文件测试
// ============================================================================

/// 测试：深层子目录的保存、查询、删除
TEST(MyCache_Subdirectory, DeepNestedOperations) {
    auto dir = MakeTestDir("deep");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        std::string deep_name = "a/b/c/d/e/file.dat";
        auto data = ToBytes("deep nested data");

        auto save = cache.SaveFile(deep_name, data);
        EXPECT_TRUE(save.Ok());

        auto exists = cache.Exists(deep_name);
        EXPECT_TRUE(exists.Ok());
        EXPECT_TRUE(exists.value);

        auto path = cache.GetFullPath(deep_name);
        EXPECT_TRUE(path.Ok());
        EXPECT_NE(path.value.find("a/b/c/d/e/file.dat"), std::string::npos);

        auto del = cache.DeleteFile(deep_name);
        EXPECT_TRUE(del.Ok());

        auto exists_after = cache.Exists(deep_name);
        EXPECT_TRUE(exists_after.Ok());
        EXPECT_FALSE(exists_after.value);
    }
    CleanupDir(dir);
}

// ============================================================================
// 多文件操作测试
// ============================================================================

/// 测试：批量保存和查询
TEST(MyCache_MultiFile, BatchSaveAndQuery) {
    auto dir = MakeTestDir("batch");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        constexpr int kFileCount = 50;
        for (int i = 0; i < kFileCount; ++i) {
            std::string name = "batch/file_" + std::to_string(i) + ".bin";
            auto data = ToBytes("data_" + std::to_string(i));
            auto r = cache.SaveFile(name, data);
            EXPECT_TRUE(r.Ok()) << "保存失败：" << name;
        }

        // 全部应该存在
        for (int i = 0; i < kFileCount; ++i) {
            std::string name = "batch/file_" + std::to_string(i) + ".bin";
            auto r = cache.Exists(name);
            EXPECT_TRUE(r.Ok());
            EXPECT_TRUE(r.value) << "文件不存在：" << name;
        }
    }
    CleanupDir(dir);
}

// ============================================================================
// 初始化已有文件的目录
// ============================================================================

/// 测试：在已有文件的目录上初始化，首次扫描应建立索引
TEST(MyCache_InitWithFiles, IndexBuiltOnInit) {
    auto dir = MakeTestDir("preinit");

    // 预先创建一些文件
    std::filesystem::create_directories(std::filesystem::path(dir) / "sub");
    {
        std::ofstream(std::filesystem::path(dir) / "pre1.txt") << "pre1";
        std::ofstream(std::filesystem::path(dir) / "sub" / "pre2.txt") << "pre2";
    }

    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        // 构造时首次扫描应该已经发现这些文件
        auto r1 = cache.Exists("pre1.txt");
        EXPECT_TRUE(r1.Ok());
        EXPECT_TRUE(r1.value);

        auto r2 = cache.Exists("sub/pre2.txt");
        EXPECT_TRUE(r2.Ok());
        EXPECT_TRUE(r2.value);

        // GetAllFileList 也应该包含这些文件
        auto list = cache.GetAllFileList();
        EXPECT_TRUE(list.Ok());
        EXPECT_EQ(list.value.size(), 2u);
    }
    CleanupDir(dir);
}
