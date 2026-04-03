/**
 * @file TestFileApiController.cpp
 * @brief FileApiController 单元测试
 *
 * 测试覆盖：
 *   - ValidateFilename 入参校验（合法/非法场景）
 *   - CacheErrorToHttpCode 错误码→HTTP 状态码映射
 *   - 与 MyCache 集成的完整上传/查询/删除/列表流程
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <string>

// 辅助函数（可独立测试，无需 oatpp 运行时）
#include "controller/file_cache/FileApiHelpers.h"

// 供集成测试使用
#include "MyCache.h"
#include "MyCacheProvider.h"
#include "CacheTypes.h"
#include <nlohmann/json.hpp>

using namespace my_api::file_cache_api;
using namespace my_cache;
using json = nlohmann::json;

// ============================================================================
// 测试辅助
// ============================================================================

namespace {

std::string MakeTestDir(const std::string& suffix) {
    return "/tmp/file_api_test_" + suffix + "_" +
           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
}

void CleanupDir(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
}

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
// ValidateFilename 测试
// ============================================================================

/// 合法文件名应通过校验
TEST(FileApi_ValidateFilename, ValidNames) {
    std::string err;

    EXPECT_TRUE(ValidateFilename("test.txt", err));
    EXPECT_TRUE(ValidateFilename("images/photo.jpg", err));
    EXPECT_TRUE(ValidateFilename("a/b/c/d.bin", err));
    EXPECT_TRUE(ValidateFilename("file_with-dash.tar.gz", err));
    EXPECT_TRUE(ValidateFilename("a", err));
    EXPECT_TRUE(ValidateFilename(".hidden", err));
    EXPECT_TRUE(ValidateFilename("dir/.hidden_file", err));
}

/// 空文件名应被拒绝
TEST(FileApi_ValidateFilename, EmptyName) {
    std::string err;
    EXPECT_FALSE(ValidateFilename("", err));
    EXPECT_NE(err.find("不能为空"), std::string::npos);
}

/// 超长文件名应被拒绝
TEST(FileApi_ValidateFilename, TooLongName) {
    std::string err;
    std::string long_name(1025, 'a');
    EXPECT_FALSE(ValidateFilename(long_name, err));
    EXPECT_NE(err.find("1024"), std::string::npos);
}

/// 包含空字符应被拒绝
TEST(FileApi_ValidateFilename, NullCharacter) {
    std::string err;
    std::string name = "test";
    name += '\0';
    name += ".txt";
    EXPECT_FALSE(ValidateFilename(name, err));
    EXPECT_NE(err.find("空字符"), std::string::npos);
}

/// 包含反斜杠应被拒绝
TEST(FileApi_ValidateFilename, BackslashRejected) {
    std::string err;
    EXPECT_FALSE(ValidateFilename("path\\file.txt", err));
    EXPECT_NE(err.find("反斜杠"), std::string::npos);
}

/// 以 "/" 开头的绝对路径应被拒绝
TEST(FileApi_ValidateFilename, AbsolutePathRejected) {
    std::string err;
    EXPECT_FALSE(ValidateFilename("/etc/passwd", err));
    EXPECT_NE(err.find("'/'"), std::string::npos);
}

/// 包含 ".." 的路径穿越应被拒绝
TEST(FileApi_ValidateFilename, PathTraversalRejected) {
    std::string err;

    EXPECT_FALSE(ValidateFilename("../secret", err));
    EXPECT_NE(err.find(".."), std::string::npos);

    err.clear();
    EXPECT_FALSE(ValidateFilename("foo/../../etc/passwd", err));
    EXPECT_NE(err.find(".."), std::string::npos);

    err.clear();
    EXPECT_FALSE(ValidateFilename("a/b/../../../c", err));
    EXPECT_NE(err.find(".."), std::string::npos);
}

/// "..." 三个点(不是 "..")应该被允许
TEST(FileApi_ValidateFilename, TripleDotAllowed) {
    std::string err;
    EXPECT_TRUE(ValidateFilename("...", err));
    EXPECT_TRUE(ValidateFilename("dir/.../file", err));
}

/// 单个 "." 应该被允许（当前目录引用）
TEST(FileApi_ValidateFilename, SingleDotAllowed) {
    std::string err;
    EXPECT_TRUE(ValidateFilename("./file.txt", err));
    EXPECT_TRUE(ValidateFilename("dir/./file.txt", err));
}

// ============================================================================
// CacheErrorToHttpCode 映射测试
// ============================================================================

TEST(FileApi_ErrorMapping, AllCodesMapCorrectly) {
    EXPECT_EQ(CacheErrorToHttpCode(CacheErrorCode::Ok), 200);
    EXPECT_EQ(CacheErrorToHttpCode(CacheErrorCode::NotInitialized), 500);
    EXPECT_EQ(CacheErrorToHttpCode(CacheErrorCode::PathTraversal), 403);
    EXPECT_EQ(CacheErrorToHttpCode(CacheErrorCode::FileNotFound), 404);
    EXPECT_EQ(CacheErrorToHttpCode(CacheErrorCode::FileAlreadyExists), 409);
    EXPECT_EQ(CacheErrorToHttpCode(CacheErrorCode::IoError), 500);
    EXPECT_EQ(CacheErrorToHttpCode(CacheErrorCode::InvalidArgument), 400);
    EXPECT_EQ(CacheErrorToHttpCode(CacheErrorCode::CreateDirFailed), 500);
    EXPECT_EQ(CacheErrorToHttpCode(CacheErrorCode::FileTooLarge), 413);
}

// ============================================================================
// 与 MyCache 集成测试（不经过 HTTP，直接验证业务逻辑）
// ============================================================================

/// 测试完整流程：校验文件名 → 保存 → 查询 → 删除
TEST(FileApi_Integration, UploadQueryDeleteFlow) {
    auto dir = MakeTestDir("integration");
    {
        // 初始化 MyCache
        MyCache cache;
        auto init = cache.Init(MakeConfig(dir));
        ASSERT_TRUE(init.Ok());
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        // 模拟 Controller 的上传逻辑
        std::string filename = "test/data.bin";
        std::string validate_err;
        ASSERT_TRUE(ValidateFilename(filename, validate_err));

        std::vector<uint8_t> data = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
        auto save_result = cache.SaveFile(filename, data);
        EXPECT_TRUE(save_result.Ok());
        EXPECT_EQ(CacheErrorToHttpCode(save_result.code), 200);

        // 模拟 Controller 的查询逻辑
        auto exists_result = cache.Exists(filename);
        EXPECT_TRUE(exists_result.Ok());
        EXPECT_TRUE(exists_result.value);

        auto path_result = cache.GetFullPath(filename);
        EXPECT_TRUE(path_result.Ok());
        EXPECT_FALSE(path_result.value.empty());

        // 模拟 Controller 的删除逻辑
        auto del_result = cache.DeleteFile(filename);
        EXPECT_TRUE(del_result.Ok());
        EXPECT_EQ(CacheErrorToHttpCode(del_result.code), 200);

        // 删除后查询
        auto exists_after = cache.Exists(filename);
        EXPECT_TRUE(exists_after.Ok());
        EXPECT_FALSE(exists_after.value);
    }
    CleanupDir(dir);
}

/// 测试非法文件名被拒绝后不会到达 MyCache 层
TEST(FileApi_Integration, InvalidFilenameBlockedBeforeCache) {
    auto dir = MakeTestDir("blocked");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        std::vector<std::string> bad_names = {
            "", "../secret", "/etc/passwd", "path\\file",
            "a/../../etc/shadow"
        };

        for (const auto& name : bad_names) {
            std::string err;
            // Controller 层校验拦截
            EXPECT_FALSE(ValidateFilename(name, err))
                << "应被拒绝的文件名未被拦截: " << name;
        }
    }
    CleanupDir(dir);
}

/// 测试删除不存在的文件返回正确映射
TEST(FileApi_Integration, DeleteNonExistentMapsTo404) {
    auto dir = MakeTestDir("del404");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto result = cache.DeleteFile("nonexistent.txt");
        EXPECT_FALSE(result.Ok());
        EXPECT_EQ(result.code, CacheErrorCode::FileNotFound);
        EXPECT_EQ(CacheErrorToHttpCode(result.code), 404);
    }
    CleanupDir(dir);
}

/// 测试 GetAllFileList 集成
TEST(FileApi_Integration, ListFilesReturnsCorrectMetadata) {
    auto dir = MakeTestDir("list");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        // 保存几个文件
        std::vector<uint8_t> data1 = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
        cache.SaveFile("readme.txt", data1);
        cache.SaveFile("images/photo.jpg", {0xFF, 0xD8, 0xFF});

        auto list_result = cache.GetAllFileList();
        EXPECT_TRUE(list_result.Ok());
        EXPECT_EQ(list_result.value.size(), 2u);

        // 验证元信息
        for (const auto& fi : list_result.value) {
            EXPECT_FALSE(fi.name.empty());
            EXPECT_GT(fi.size, 0u);
            EXPECT_FALSE(fi.type.empty());
            EXPECT_FALSE(fi.modified_at.empty());
        }
    }
    CleanupDir(dir);
}

/// 测试 FileTooLarge 映射到 413
TEST(FileApi_Integration, FileTooLargeMapsTo413) {
    auto dir = MakeTestDir("toolarge413");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir, 5));  // max_file_size = 5 bytes
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        auto result = cache.SaveFile("big.bin", {1, 2, 3, 4, 5, 6});
        EXPECT_FALSE(result.Ok());
        EXPECT_EQ(result.code, CacheErrorCode::FileTooLarge);
        EXPECT_EQ(CacheErrorToHttpCode(result.code), 413);
    }
    CleanupDir(dir);
}

/// 测试 GetConfig（对应 /v1/cache/info 逻辑）
TEST(FileApi_Integration, CacheInfoReturnsCorrectConfig) {
    auto dir = MakeTestDir("cacheinfo");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir, 2048, -1));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        const auto& config = cache.GetConfig();
        EXPECT_EQ(config.root_path, dir);
        EXPECT_EQ(config.max_file_size, 2048u);
        EXPECT_EQ(config.max_retention_seconds, -1);
    }
    CleanupDir(dir);
}

/// 测试 Status（对应 /v1/cache/status 逻辑）
TEST(FileApi_Integration, CacheStatusReturnsCorrectState) {
    auto dir = MakeTestDir("cachestatus");
    {
        // 未初始化
        MyCache cache;
        EXPECT_EQ(cache.Status(), CacheStatus::NotInitialized);
        EXPECT_STREQ(CacheStatusToString(cache.Status()), "NotInitialized");

        // 初始化后
        cache.Init(MakeConfig(dir));
        EXPECT_EQ(cache.Status(), CacheStatus::Running);
        EXPECT_STREQ(CacheStatusToString(cache.Status()), "Running");
    }
    CleanupDir(dir);
}

/// 测试 max_retention_seconds = -1 时的配置
TEST(FileApi_Integration, NegativeRetentionInConfig) {
    auto dir = MakeTestDir("negretention");
    {
        MyCache cache;
        cache.Init(MakeConfig(dir, 0, -1));
        ASSERT_EQ(cache.Status(), CacheStatus::Running);

        const auto& config = cache.GetConfig();
        EXPECT_EQ(config.max_retention_seconds, -1);

        // 文件不应被过期清理
        cache.SaveFile("keep.txt", std::vector<uint8_t>{'H', 'i'});
        auto exists = cache.Exists("keep.txt");
        EXPECT_TRUE(exists.Ok());
        EXPECT_TRUE(exists.value);
    }
    CleanupDir(dir);
}
