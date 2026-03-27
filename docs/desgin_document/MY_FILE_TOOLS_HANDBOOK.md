# MyFileTools 使用手册

## 1. 功能概述

`MyFileTools` 是一个轻量级文件工具类，面向常见文本文件操作场景，提供以下能力：

- 判断文件是否存在
- 创建指定文件
- 删除指定文件
- 以追加方式写入文件
- 以覆盖方式写入文件

实现约束：

- 不依赖 `std::filesystem`
- 基于 Linux/POSIX 文件接口实现
- 所有接口均带日志输出，便于定位失败原因

## 2. 头文件位置

`src/util/my_tools/MyFileTools.h`

## 3. 接口说明

### 3.1 判断文件是否存在

```cpp
bool my_tools::MyFileTools::Exists(const std::string& file_path);
```

说明：

- 仅当路径存在且为普通文件时返回 `true`
- 如果路径是目录、符号链接到目录、或路径不存在，返回 `false`

### 3.2 创建文件

```cpp
bool my_tools::MyFileTools::CreateFile(const std::string& file_path, bool truncate_if_exists = false);
```

说明：

- 当文件不存在时直接创建
- 当文件已存在时：
  - `truncate_if_exists=false`：保持原内容不变
  - `truncate_if_exists=true`：清空原内容
- 不会自动创建父目录

### 3.3 删除文件

```cpp
bool my_tools::MyFileTools::DeleteFile(const std::string& file_path);
```

说明：

- 删除成功返回 `true`
- 文件不存在也返回 `true`
- 如果目标是目录而不是普通文件，返回 `false`

### 3.4 追加写文件

```cpp
bool my_tools::MyFileTools::AppendText(const std::string& file_path, const std::string& content);
```

说明：

- 文件不存在时会自动创建
- 新内容追加到文件末尾

### 3.5 覆盖写文件

```cpp
bool my_tools::MyFileTools::WriteText(const std::string& file_path, const std::string& content);
```

说明：

- 文件不存在时会自动创建
- 文件存在时会清空旧内容，再写入新内容

## 4. 示例代码

```cpp
#include "MyFileTools.h"

void DemoMyFileTools() {
    const std::string path = "/tmp/demo_my_file_tools.txt";

    my_tools::MyFileTools::CreateFile(path);
    my_tools::MyFileTools::WriteText(path, "line-1\n");
    my_tools::MyFileTools::AppendText(path, "line-2\n");

    if (my_tools::MyFileTools::Exists(path)) {
        // do something
    }

    my_tools::MyFileTools::DeleteFile(path);
}
```

## 5. 注意事项

- 本工具只处理“文件”，不处理目录创建
- 若父目录不存在，创建和写入都会失败
- 当前实现主要面向文本写入场景；若需要二进制读写，可后续扩展

## 6. 单元测试

测试文件位置：

`test/util/my_tools/TestMyFileTools.cpp`

已覆盖场景：

- 空路径
- 正常创建文件
- 截断创建
- 覆盖写
- 追加写
- 删除现有文件
- 删除不存在文件
- 父目录不存在
- 误传目录路径