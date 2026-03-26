#include "gtest/gtest.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "MyFileTools.h"

namespace {

class TempTestDir {
public:
    TempTestDir() {
        char path_template[] = "/tmp/my_file_tools_test_XXXXXX";
        char* result = ::mkdtemp(path_template);
        if (result != nullptr) {
            dir_path_ = result;
        }
    }

    ~TempTestDir() {
        if (!dir_path_.empty()) {
            ::rmdir(dir_path_.c_str());
        }
    }

    const std::string& path() const {
        return dir_path_;
    }

    std::string filePath(const std::string& file_name) const {
        return dir_path_ + "/" + file_name;
    }

private:
    std::string dir_path_;
};

std::string ReadFileAll(const std::string& file_path) {
    FILE* fp = ::fopen(file_path.c_str(), "rb");
    if (fp == nullptr) {
        return std::string();
    }

    std::string content;
    char buffer[256];
    size_t read_size = 0;
    while ((read_size = ::fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        content.append(buffer, read_size);
    }
    ::fclose(fp);
    return content;
}

} // namespace

TEST(MyFileToolsTest, ExistsReturnsFalseForEmptyPath) {
    EXPECT_FALSE(my_tools::MyFileTools::Exists(""));
}

TEST(MyFileToolsTest, CreateFileCreatesEmptyFile) {
    TempTestDir temp_dir;
    ASSERT_FALSE(temp_dir.path().empty());

    const std::string file_path = temp_dir.filePath("create.txt");
    EXPECT_TRUE(my_tools::MyFileTools::CreateFile(file_path));
    EXPECT_TRUE(my_tools::MyFileTools::Exists(file_path));
    EXPECT_TRUE(ReadFileAll(file_path).empty());

    ::unlink(file_path.c_str());
}

TEST(MyFileToolsTest, CreateFileWithTruncateClearsOldContent) {
    TempTestDir temp_dir;
    ASSERT_FALSE(temp_dir.path().empty());

    const std::string file_path = temp_dir.filePath("truncate.txt");
    ASSERT_TRUE(my_tools::MyFileTools::WriteText(file_path, "old-content"));
    ASSERT_TRUE(my_tools::MyFileTools::CreateFile(file_path, true));
    EXPECT_EQ(ReadFileAll(file_path), "");

    ::unlink(file_path.c_str());
}

TEST(MyFileToolsTest, WriteTextOverwritesFileContent) {
    TempTestDir temp_dir;
    ASSERT_FALSE(temp_dir.path().empty());

    const std::string file_path = temp_dir.filePath("overwrite.txt");
    ASSERT_TRUE(my_tools::MyFileTools::WriteText(file_path, "hello"));
    ASSERT_TRUE(my_tools::MyFileTools::WriteText(file_path, "world"));
    EXPECT_EQ(ReadFileAll(file_path), "world");

    ::unlink(file_path.c_str());
}

TEST(MyFileToolsTest, AppendTextAppendsFileContent) {
    TempTestDir temp_dir;
    ASSERT_FALSE(temp_dir.path().empty());

    const std::string file_path = temp_dir.filePath("append.txt");
    ASSERT_TRUE(my_tools::MyFileTools::WriteText(file_path, "hello"));
    ASSERT_TRUE(my_tools::MyFileTools::AppendText(file_path, "-world"));
    EXPECT_EQ(ReadFileAll(file_path), "hello-world");

    ::unlink(file_path.c_str());
}

TEST(MyFileToolsTest, DeleteFileRemovesExistingFile) {
    TempTestDir temp_dir;
    ASSERT_FALSE(temp_dir.path().empty());

    const std::string file_path = temp_dir.filePath("delete.txt");
    ASSERT_TRUE(my_tools::MyFileTools::WriteText(file_path, "data"));
    EXPECT_TRUE(my_tools::MyFileTools::DeleteFile(file_path));
    EXPECT_FALSE(my_tools::MyFileTools::Exists(file_path));
}

TEST(MyFileToolsTest, DeleteFileReturnsTrueWhenFileMissing) {
    TempTestDir temp_dir;
    ASSERT_FALSE(temp_dir.path().empty());

    const std::string file_path = temp_dir.filePath("missing.txt");
    EXPECT_TRUE(my_tools::MyFileTools::DeleteFile(file_path));
}

TEST(MyFileToolsTest, CreateFileFailsWhenParentDirectoryMissing) {
    const std::string file_path = "/tmp/my_file_tools_nonexistent_parent_12345/file.txt";
    EXPECT_FALSE(my_tools::MyFileTools::CreateFile(file_path));
}

TEST(MyFileToolsTest, DeleteFileRejectsDirectoryPath) {
    TempTestDir temp_dir;
    ASSERT_FALSE(temp_dir.path().empty());
    EXPECT_FALSE(my_tools::MyFileTools::DeleteFile(temp_dir.path()));
}