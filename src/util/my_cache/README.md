# MyCache 模块使用手册

## 1. 概述

`my_cache` 是一个 **本地文件缓存管理模块**，提供：

- **文件存取**：保存、删除、查询文件
- **O(1) 存在性检查**：后台线程定期扫描目录，维护内存索引
- **路径穿越防护**：所有操作自动校验请求路径是否在缓存根目录范围内
- **单例包装器**：`MyCacheProvider` 提供线程安全的全局访问

---

## 2. 核心类

### 2.1 错误码 `CacheErrorCode`

| 错误码 | 含义 |
|--------|------|
| `Ok` | 操作成功 |
| `NotInitialized` | 模块未初始化 |
| `PathTraversal` | 路径穿越攻击（请求路径超出根目录范围） |
| `FileNotFound` | 文件不存在 |
| `FileAlreadyExists` | 文件已存在 |
| `IoError` | 文件读写错误 |
| `InvalidArgument` | 参数非法（如文件名为空） |
| `CreateDirFailed` | 创建目录失败 |

### 2.2 返回值 `CacheResult<T>`

所有接口统一返回 `CacheResult<T>`，包含错误码和可选数据：

```cpp
template <typename T>
struct CacheResult {
    CacheErrorCode code;  // 错误码
    T value;              // 返回值（仅 code == Ok 时有意义）

    bool Ok() const;                                // 判断是否成功
    static CacheResult Success(T val);              // 构造成功结果
    static CacheResult Fail(CacheErrorCode err, T val = T{});  // 构造失败结果
};

// void 特化版本（无返回值）
template <>
struct CacheResult<void> {
    CacheErrorCode code;
    bool Ok() const;
    static CacheResult Success();
    static CacheResult Fail(CacheErrorCode err);
};
```

### 2.3 `MyCache` 类

核心缓存管理器，管理一个本地目录下的文件。

#### 构造函数

```cpp
explicit MyCache(const std::string& root_path);
```

- 接收缓存根目录路径
- 目录不存在时自动创建
- 构造完成后自动启动后台扫描线程（每 5 秒扫描一次）
- 使用 `Status()` 检查初始化是否成功

#### 接口列表

| 方法 | 签名 | 说明 |
|------|------|------|
| `Status` | `bool Status() const` | 查询模块是否初始化成功 |
| `SaveFile` | `CacheResult<void> SaveFile(name, data)` | 保存文件（支持子目录，自动创建父目录） |
| `DeleteFile` | `CacheResult<void> DeleteFile(name)` | 删除文件 |
| `GetFullPath` | `CacheResult<std::string> GetFullPath(name)` | 获取文件的完整绝对路径 |
| `Exists` | `CacheResult<bool> Exists(name)` | O(1) 查询文件是否存在（基于内存索引） |
| `GetRootPath` | `std::string GetRootPath() const` | 获取缓存根目录路径 |

### 2.4 `MyCacheProvider` 单例包装器

提供全局访问入口，适合在整个程序生命周期中使用同一个缓存实例。

| 方法 | 签名 | 说明 |
|------|------|------|
| `Init` | `static CacheResult<void> Init(root_path)` | 线程安全的显式初始化（仅首次生效） |
| `Get` | `static CacheResult<MyCache*> Get()` | 获取 MyCache 实例指针 |
| `Destroy` | `static void Destroy()` | 销毁实例（主要用于测试） |

---

## 3. 使用示例

### 3.1 直接使用 MyCache

```cpp
#include "MyCache.h"

using namespace my_cache;

void example_direct() {
    // 创建缓存实例
    MyCache cache("/data/file_cache");

    // 检查初始化状态
    if (!cache.Status()) {
        // 初始化失败，处理错误
        return;
    }

    // 保存文件
    std::vector<uint8_t> data = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
    auto save_result = cache.SaveFile("images/photo.jpg", data);
    if (!save_result.Ok()) {
        // 处理错误：save_result.code 包含具体错误码
        return;
    }

    // 查询文件是否存在（O(1) 复杂度）
    auto exists_result = cache.Exists("images/photo.jpg");
    if (exists_result.Ok() && exists_result.value) {
        // 文件存在
    }

    // 获取完整路径
    auto path_result = cache.GetFullPath("images/photo.jpg");
    if (path_result.Ok()) {
        std::string full_path = path_result.value;
        // 使用完整路径...
    }

    // 删除文件
    auto del_result = cache.DeleteFile("images/photo.jpg");
    if (!del_result.Ok()) {
        // 处理删除错误
    }
}
```

### 3.2 通过 MyCacheProvider 全局使用

```cpp
#include "MyCache.h"

using namespace my_cache;

// 程序启动时初始化（通常在 main 或初始化函数中调用一次）
void init() {
    auto result = MyCacheProvider::Init("/data/file_cache");
    if (!result.Ok()) {
        // 初始化失败
        MYLOG_ERROR("MyCache 初始化失败：{}", CacheErrorCodeToString(result.code));
        return;
    }
}

// 在任何地方使用
void process_file(const std::string& filename) {
    auto result = MyCacheProvider::Get();
    if (!result.Ok()) {
        MYLOG_ERROR("MyCache 未初始化");
        return;
    }

    MyCache* cache = result.value;

    if (cache->Exists(filename).value) {
        auto path = cache->GetFullPath(filename);
        // 使用文件...
    }
}
```

### 3.3 标准错误处理模式

```cpp
auto result = cache.SaveFile("data.bin", bytes);
switch (result.code) {
    case CacheErrorCode::Ok:
        // 保存成功
        break;
    case CacheErrorCode::PathTraversal:
        // 路径穿越攻击被阻止
        MYLOG_ERROR("非法路径操作！");
        break;
    case CacheErrorCode::IoError:
        // IO 错误
        MYLOG_ERROR("文件写入失败");
        break;
    default:
        MYLOG_ERROR("未知错误：{}", CacheErrorCodeToString(result.code));
        break;
}
```

---

## 4. 安全特性

### 路径穿越防护

所有文件操作接口在执行前会：

1. 将用户传入的相对路径与 `root_path` 拼接
2. 使用 `std::filesystem::weakly_canonical` 规范化路径
3. 验证规范化后的路径以 `root_path` 为前缀

以下攻击方式均会被拦截并返回 `CacheErrorCode::PathTraversal`：

```cpp
cache.SaveFile("../etc/passwd", data);           // ❌ 拒绝
cache.SaveFile("foo/../../etc/passwd", data);     // ❌ 拒绝
cache.DeleteFile("../../sensitive_file", data);   // ❌ 拒绝
```

---

## 5. 内部机制

### 后台扫描线程

- 每 **5 秒** 扫描一次缓存根目录（递归）
- 使用 `std::filesystem::recursive_directory_iterator` 遍历
- 发现的所有常规文件的相对路径存入 `std::unordered_set<std::string>`
- 使用 `std::shared_mutex` 实现读写分离：
  - `Exists()` 使用读锁 → 多线程可并发查询
  - 索引更新使用写锁 → 保证一致性
- `SaveFile` / `DeleteFile` 在操作成功后立即更新索引，无需等待扫描

### 原子写入

`SaveFile` 采用 **写临时文件 + 重命名** 的策略，避免写入过程中崩溃导致文件损坏。

---

## 6. CMake 集成

模块已通过 `BUILD_MY_CACHE` 开关集成到项目构建系统中。

在你的 CMakeLists.txt 中链接：

```cmake
target_link_libraries(your_target PUBLIC my_cache)
```

依赖：`pthread`、`mylog`、`myconfig`

---

## 7. 单元测试

测试文件位于 `test/util/my_cache/TestMyCache.cpp`，运行：

```bash
cd build && cmake .. && make fast_cpp_server_Test
./bin/fast_cpp_server_Test --gtest_filter="MyCache*"
```

测试覆盖面：

| 测试类别 | 测试数量 | 覆盖内容 |
|----------|----------|----------|
| 基础功能 | 3 | 初始化、状态查询、路径返回 |
| SaveFile | 4 | 正常保存、空数据、覆盖写、子目录 |
| Exists | 2 | 保存后查询、删除后查询 |
| DeleteFile | 2 | 正常删除、删除不存在文件 |
| GetFullPath | 1 | 路径正确性 |
| 路径穿越 | 4 | `../`、绝对路径、隐蔽穿越、纯 `..` |
| 非法输入 | 1 | 空文件名 |
| 后台扫描 | 2 | 检测外部创建/删除的文件 |
| MyCacheProvider | 3 | Init+Get、未初始化 Get、Destroy 后 Get |
| CacheResult | 2 | Success/Fail 工厂方法、默认值 |
| 错误码 | 1 | 所有错误码字符串转换 |
| 子目录 | 1 | 深层嵌套目录操作 |
| 批量操作 | 1 | 50 个文件批量保存查询 |
| 预存文件 | 1 | 在已有文件目录上初始化 |
