#include "Pipeline.h"
#include "MyHeartbeatManager.h"
#include "MyEdgeManager.h"
#include "EdgeDevice.h"
#include "IEdge.h"
#include "MyEdges.h"
#include "MyEdge.h"
#include "MyMqttBrokerManager.h"
#include "MqttService.hpp"
#include "SoftHealthMonitorManager.h"
#include "SoftHealthSnapshot.h"
#include "SoftHealthMonitorConfig.h"
#include "MyAPI.h"
#include "MyFlyControlManager.h"
#include "pod_manager.h"
#include "MyLog.h"
#include "MyTools.h"

#include <cstring>
#include <system_error>
#include <thread>
#include <chrono>
#include <string>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>


namespace tools {
namespace pipeline {

using namespace my_tools;
using namespace my_edge;
using namespace my_heartbeat;

Pipeline::~Pipeline() {
    Stop();
}

void Pipeline::LogRecursive(const std::string& prefix, const nlohmann::json& j) {
    if (j.is_object()) {
        for (auto it = j.begin(); it != j.end(); ++it) {
            LogRecursive(prefix + (prefix.empty() ? "" : ".") + it.key(), it.value());
        }
    } else {
        // 使用你要求的日志格式
        MYLOG_INFO("* Arg: {}, Value: {}", prefix, j.dump());
    }
}

void Pipeline::LogArg(const std::string& name, const std::string& value) {
    MYLOG_INFO("* 参数名: {}, 数值: {}", name, value);
}

void Pipeline::Init(const nlohmann::json& config) {
    config_data_ = config;

    MYLOG_INFO("开始初始化 Pipeline 配置...");
    MYLOG_INFO("* Arg: {}, Value: {}", "Pipeline", "Starting Initialization");

    // 如果包含节点数量，先记一下
    if (config_data_.contains("execute_node_number")) {
        MYLOG_INFO("* Arg: {}, Value: {}", "execute_node_number", config_data_["execute_node_number"].dump());
    }

    // 递归记录所有参数，方便调试
    MYLOG_INFO("------------------------------------------------------------");
    LogRecursive("Config", config_data_);
    MYLOG_INFO("------------------------------------------------------------");
}

void Pipeline::LaunchRoBot() {
    MYLOG_INFO("Launching RoBot modules...");
    // 这里可以添加具体的启动逻辑
    try {
        // 配置项存在性校验
        if (!config_data_.contains("executes") || !config_data_["executes"].is_object()) {
            MYLOG_INFO("* Arg: {}, Value: {}", "配置错误", "未找到有效的 'executes' 节点列表，启动终止");
            is_running_ = false;
            return;
        }

        const auto& executes    = config_data_["executes"];     // 执行节点列表
        int total_nodes         = 0;                            // 总节点计数
        int success_count       = 0;                            // 成功启动的节点计数
        int step_time_interval  = 3;                            // 默认间隔时间

        MYLOG_INFO("* Arg: {}, Value: {}", "流程分发", "准备遍历执行节点，节点总数预测: " + std::to_string(executes.size()));
        
        // 3. 遍历执行节点
        for (auto it = executes.begin(); it != executes.end(); ++it) {
            MYLOG_INFO("-----------------------------------正在启动节点 {} -------", it.key());
            std::string node_index = it.key();
            total_nodes++;

            // 为每个节点的启动逻辑添加独立的 try-catch 保护，确保节点间互不影响
            try {
                const auto& node_body = it.value();
                
                // 参数安全提取
                if (!node_body.contains("model_name")) {
                    MYLOG_INFO("* Arg: {}, Value: {}", "节点[" + node_index + "]错误", "缺失 'model_name' 字段，跳过此节点");
                    MYLOG_INFO("------------------------------------------------------------");
                    continue;
                }

                std::string model_name = node_body.at("model_name").get<std::string>();
                bool enable = node_body.value("enable", true);
                const auto& model_args = node_body.value("model_args", nlohmann::json::object());
                int temp_step_time_interval = node_body.value("step_time_interval", step_time_interval);
                if (temp_step_time_interval > 0) {
                    step_time_interval = temp_step_time_interval;
                }

                if (!enable) {
                    MYLOG_WARN("* Arg: {}, Value: {}", "节点[" + node_index + ": " + model_name + "]已禁用", "跳过此节点的启动");
                    MYLOG_INFO("------------------------------------------------------------");
                    continue;
                }

                MYLOG_WARN("* Arg: {}, Value: {}", "节点分发开始", "正在启动节点[" + node_index + "] 模块名称 >>> " + model_name + " <<<");

                // --- 业务逻辑分发 ---
                if (model_name == "heartbeat") { LaunchHeartbeat(model_args); success_count++;}
                else if (model_name == "mqtt_comm") { LaunchMQTTComm(model_args); success_count++;}
                else if (model_name == "comm") { LaunchComm(model_args); success_count++;}
                else if (model_name == "system_healthy") { LaunchSystemHealthy(model_args); success_count++;}
                else if (model_name == "edge_monitor") { LaunchEdgeMonitor(model_args); success_count++;}
                else if (model_name == "rest_api") { LaunchRestAPI(model_args); success_count++;}
                else if (model_name == "edge") { LaunchEdge(model_args); success_count++;}
                else if (model_name == "MQTTBroker") { LaunchMyMqttBroker(model_args); success_count++;}
                else if (model_name == "soft_healthy_monitor") { LaunchSoftHealthyMonitor(model_args); success_count++;}
                else if (model_name == "fly_control") { LaunchFlyControl(model_args); success_count++;}
                else if (model_name == "pod") { LaunchPodManager(model_args); success_count++;}
                else { MYLOG_INFO("* Arg: {}, Value: {}", "节点[" + node_index + "]警告", "未知的模型名称: " + model_name);}
                
                MYLOG_INFO("* Arg: {}, Value: {}", "节点分发完成", "节点[" + node_index + "] 已成功加入监听列表");

                // 节点间等待，避免资源争抢
                MYLOG_INFO("* Arg: {}, Value: {}", "节点间隔等待", "等待 " + std::to_string(step_time_interval) + " 秒后启动下一个节点...");
                for (int i = 0; i < step_time_interval; ++i) {
                    MYLOG_INFO("  - 等待中... {}/{} 秒", i + 1, step_time_interval);
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                MYLOG_INFO("------------------------------------------------------------");

            } catch (const nlohmann::json::exception& e) {
                MYLOG_INFO("* Arg: {}, Value: {}", "节点[" + node_index + "]配置异常", std::string("JSON解析失败: ") + e.what());
            } catch (const std::exception& e) {
                MYLOG_INFO("* Arg: {}, Value: {}", "节点[" + node_index + "]运行异常", std::string("系统错误: ") + e.what());
            } catch (...) {
                MYLOG_INFO("* Arg: {}, Value: {}", "节点[" + node_index + "]未知异常", "捕获到未分类的严重错误");
            }
        }

        // 4. 启动总结日志
        MYLOG_INFO("* Arg: {}, Value: {}", "启动流程总结", 
            "全部节点处理完成。总计: " + std::to_string(total_nodes) + 
            ", 成功启动: " + std::to_string(success_count) + 
            ", 失败/跳过: " + std::to_string(total_nodes - success_count));

    } catch (const std::exception& e) {
        // 捕获可能导致的循环中断的顶层异常
        is_running_ = false;
        MYLOG_INFO("* Arg: {}, Value: {}", "Pipeline核心崩溃", std::string("致命错误: ") + e.what());
    }
    MYLOG_INFO("RoBot launched successfully.");
}


void Pipeline::Start() {
    // 1. 状态检查与原子锁保护
    MYLOG_INFO("* Arg: {}, Value: {}", "启动状态", "开始尝试启动 Pipeline 模块...");
    if (is_running_.exchange(true)) {
        MYLOG_INFO("* Arg: {}, Value: {}", "启动跳过", "Pipeline 已经在运行中，无需重复启动");
        return;
    }

    try {
        // 2. 配置项存在性校验
        if (!config_data_.contains("executes") || !config_data_["executes"].is_object()) {
            MYLOG_INFO("* Arg: {}, Value: {}", "配置错误", "未找到有效的 'executes' 节点列表，启动终止");
            is_running_ = false;
            return;
        }

        const auto& executes    = config_data_["executes"];     // 执行节点列表
        int total_nodes         = 0;                            // 总节点计数
        int success_count       = 0;                            // 成功启动的节点计数
        int step_time_interval  = 3;                            // 默认间隔时间

        int v = 0x676;
        MYLOG_INFO("* Arg: {}, Value: {}", "流程分发", "准备遍历执行节点，节点总数预测: " + std::to_string(executes.size()));
        // 3. 显示启动信息
        MYLOG_INFO("------------------------------------------------------------(启动节点列表)");
        for (auto it = executes.begin(); it != executes.end(); ++it) {
            std::string node_index = it.key();
            total_nodes++;
            // MYLOG_INFO("* Arg: {}, Value: {}", item.key(), item.value().dump());
            MYLOG_INFO("* Arg: {}, Value: {}", "节点序号", node_index);
            MYLOG_INFO("* Arg: {}, Value: {}", "节点名称", it.value().value("model_name", "未知模型"));
            bool enable = it.value().value("enable", true);
            if (!enable) {
                MYLOG_INFO("* Arg: {}, Value: {}", "节点状态", "已禁用");
            } else {
                MYLOG_INFO("* Arg: {}, Value: {}", "节点状态", "已启用");
            }
            MYLOG_INFO("===================================================");
        }
        MYLOG_INFO("------------------------------------------------------------(启动节点列表结束)");
        // 4. 启动机器人
        LaunchRoBot();
    } catch (const std::exception& e) {
        // 捕获可能导致的循环中断的顶层异常
        is_running_ = false;
        MYLOG_INFO("* Arg: {}, Value: {}", "Pipeline核心崩溃", std::string("致命错误: ") + e.what());
    }
    MYLOG_INFO("* Arg: {}, Value: {}", "启动完成", "Pipeline 模块已成功启动。");
}

// --- 模块逻辑实现区 ---

// --- 心跳模块启动函数 ---
void Pipeline::LaunchHeartbeat(const nlohmann::json& args) {
    const std::string module_name = "心跳模块(Heartbeat)";
    MYLOG_INFO("===== 开始启动模块: {} =====", module_name);

    try {
        // 1. 获取业务单例
        auto& hb = my_heartbeat::HeartbeatManager::GetInstance();

        // 2. 初始化配置 (带保护)
        if (args.is_null() || args.empty()) {
            MYLOG_ERROR("* 模块: {}, 错误: {}", module_name, "配置参数为空, 执行默认设置");
        }
        
        LogArg(module_name + " - 初始参数", args.dump());
        hb.Init(args);

        // 3. 启动线程 (不使用匿名函数/Lambda)
        // 使用成员函数指针：&类名::函数名, 实例地址
        workers_.emplace_back(&my_heartbeat::HeartbeatManager::Start, &hb);

        MYLOG_INFO("* 模块: {}, 状态: {}", module_name, "线程已成功创建并加入管理列表");
        // sleep 10;
        std::this_thread::sleep_for(std::chrono::seconds(15));
    } catch (const std::exception& e) {
        MYLOG_ERROR("* 模块: {}, 捕获异常: {}", module_name, e.what());
    } catch (...) {
        MYLOG_ERROR("* 模块: {}, 捕获未知严重异常", module_name);
    }
}

// --- 边缘监控模块启动函数 ---
void Pipeline::LaunchEdgeMonitor(const nlohmann::json& args) {
    const std::string module_name = "边缘监控模块(EdgeMonitor)";
    MYLOG_INFO("===== 开始启动模块: {} =====", module_name);

    try {
        // 1. 获取业务单例
        auto& em = edge_manager::MyEdgeManager::GetInstance();

        // 2. 初始化配置 (带保护)
        if (args.is_null() || args.empty()) {
            MYLOG_ERROR("* 模块: {}, 错误: {}", module_name, "配置参数为空, 执行默认设置");
        }
        
        LogArg(module_name + " - 初始参数", args.dump());
        em.Init(args);

        // 3. 启动线程 (不使用匿名函数/Lambda)
        // 使用成员函数指针：&类名::函数名, 实例地址
        workers_.emplace_back(&edge_manager::MyEdgeManager::StartAll, &em);

        MYLOG_INFO("* 模块: {}, 状态: {}", module_name, "线程已成功创建并加入管理列表");

    } catch (const std::exception& e) {
        MYLOG_ERROR("* 模块: {}, 捕获异常: {}", module_name, e.what());
    } catch (...) {
        MYLOG_ERROR("* 模块: {}, 捕获未知严重异常", module_name);
    }

}


// 在 Pipeline.cpp 的模块逻辑实现区添加
void Pipeline::LaunchRestAPI(const nlohmann::json& args) {
    const std::string module_name = "REST-API模块";
    MYLOG_INFO("===== 开始启动模块: {} =====", module_name);

    try {
        // 1. 提取参数
        int port = args.value("port", 8000); // 默认 8000 端口
        
        // 2. 等待端口可用（检查 + 重试）
        const std::chrono::seconds retry_interval(5);
        while (true) {
            if (my_tools::isPortAvailable(port)) {
                MYLOG_INFO("* 模块: {}, 端口 {} 当前可用，准备启动", module_name, port);
                // 尝试启动（Start 内部可能会 bind 并启动线程）
                try {
                    my_api::MyAPI::GetInstance().Start(port);
                    MYLOG_INFO("* 模块: {}, 监听端口: {}, 状态: {}", module_name, port, "启动成功");
                    break; // 启动成功，跳出重试循环
                } catch (const std::system_error& se) {
                    // 如果是地址被占用的系统错误，等待后重试
                    if (se.code().value() == EADDRINUSE) {
                        MYLOG_WARN("* 模块: {}, 启动时端口被占用（Start 抛出 EADDRINUSE），等待 {} 秒后重试...", module_name, retry_interval.count());
                        std::this_thread::sleep_for(retry_interval);
                        continue;
                    } else {
                        // 非端口相关的 system_error，记录并重新抛出或中止
                        MYLOG_ERROR("* 模块: {}, 启动失败（system_error）：{}，code={}，中止启动", module_name, se.what(), se.code().value());
                        throw;
                    }
                } catch (const std::exception& e) {
                    // Start 抛出了其它 std::exception，记录并中止（如果希望也可选择重试）
                    MYLOG_ERROR("* 模块: {}, 启动失败（异常）：{}，中止启动", module_name, e.what());
                    throw;
                }
            } else {
                // 端口被占用，等待后重试
                MYLOG_WARN("* 模块: {}, 端口 {} 被占用，{} 秒后重试检查...", module_name, port, retry_interval.count());
                std::this_thread::sleep_for(retry_interval);
            }
        }
        // 2. 获取 API 单例并启动
        // 注意：MyAPI::Start 内部已经启动了 std::thread，
        // 所以我们不需要再把 MyAPI::Start 丢进 workers_
        my_api::MyAPI::GetInstance().Start(port);

        // 3. 为了让 Pipeline::Stop 能关闭 API，
        // 我们可以把 API 的运行状态监测挂载到一个监控线程里（可选）
        // 或者直接让 Pipeline::Stop 显式调用 MyAPI::Stop
        
        MYLOG_INFO("* 模块: {}, 监听端口: {}, 状态: {}", module_name, port, "启动成功");

    } catch (const std::exception& e) {
        MYLOG_ERROR("* 模块: {}, 捕获异常: {}", module_name, e.what());
    }
}

void Pipeline::LaunchMQTTComm(const nlohmann::json& args) {
    int interval = args.value("interval_sec_", 3);

    my_mqtt::MqttService& mqtt_service = my_mqtt::MqttService::GetInstance();
    if (!mqtt_service.Init(args)) {
        MYLOG_ERROR("MQTTComm 模块初始化失败，跳过启动");
        return;
    } else {
        MYLOG_INFO("MQTTComm 模块初始化成功");
    }

    // 注册消息回调等（如果需要）
    // // mqtt_service.SetMessageCallback(...);
    mqtt_service.AddRoute("/system/heartbeats/do_operation", [](const std::string& topic, const std::string& payload) {
        MYLOG_INFO("MQTTComm 收到操作请求，Topic: {}, Payload: {}", topic, payload);
        // 处理操作请求的逻辑
    });
    
    // MyProto::Person ph;
    // std::string pb_data = "";
    // ph.SerializeToString(&pb_data);
    // while (true) {
    //     mqtt_service.Publish("/test/ph_data", pb_data, false, 1);
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }
    // 注入 publisher 适配器（heartbeat 只看到 IMqttPublisher）
    // my_heartbeat::HeartbeatManager::GetInstance().SetPublisher(mqtt_service.GetPublisher());
    // my_heartbeat::HeartbeatManager::GetInstance().SetPublisher(mqtt_service.GetPublisher());

    // 启动线程
    workers_.emplace_back(&my_mqtt::MqttService::Start, &mqtt_service);
    MYLOG_INFO("MQTTComm 模块线程已成功创建并加入管理列表");

}
    


void Pipeline::LaunchComm(const nlohmann::json& args) {
    int interval = args.value("interval_sec_", 3);

    workers_.emplace_back([this, interval]() {
        MYLOG_INFO("* Arg: {}, Value: {}", "Comm", "Thread Running");
        while (is_running_) {
            // TODO: 通信逻辑
            std::this_thread::sleep_for(std::chrono::seconds(interval));
        }
    });
}

void Pipeline::LaunchSystemHealthy(const nlohmann::json& args) {
    int interval = args.value("interval_sec_", 3);

    // 针对有 edges 的复杂参数，直接在这里读取
    if (args.contains("edges")) {
        MYLOG_INFO("* Arg: {}, Value: {}", "SystemHealthy", "Found Edges, processing connections...");
    }

    workers_.emplace_back([this, interval, args]() {
        MYLOG_INFO("* Arg: {}, Value: {}", "SystemHealthy", "Thread Running");
        while (is_running_) {
            // TODO: 健康检查逻辑
            std::this_thread::sleep_for(std::chrono::seconds(interval));
        }
    });
}

void Pipeline::LaunchEdge(const nlohmann::json& args) {
    int interval = args.value("interval_sec_", 10);

    MYLOG_INFO("开始启动 Edge 模块，间隔: {} 秒", interval);
    MYLOG_INFO("Edge 模块参数: {}", args.dump(4));
    // 这里可以根据 args 创建和管理多个 Edge 设备实例
    // "model_args": {
    //     "edge_create_args": [
    //       {
    //         "edge_type": "UUV",
    //         "name": "uuv_001"
    //       }
    //     ],
    //     "edge_number": 1,
    //     "interval_sec_": 10
    //   },
    try {
        int edge_number = args.value("edge_number", 1);
        nlohmann::json edge_create_args = args.value("edge_create_args", nlohmann::json::array({}));
        MYLOG_INFO("准备启动 {} 个 Edge 设备", edge_number);
        std::string error_msg = "";
        MYLOG_INFO("-------------------------------------------");
        for (int i = 0; i < edge_number; ++i) {
            MYLOG_INFO("----- 正在启动 Edge 设备 ID: {} -----", i);
            nlohmann::json single_edge_args = nlohmann::json::object();
            if (i < edge_create_args.size()) {
                single_edge_args = edge_create_args[i];
            }
            std::string edge_type = single_edge_args.value("edge_type", "UnknownType");
            // 如果上层配置缺失 devices 字段，自动构造一个最小 devices 列表，满足 Edge.Init 的要求
            if (!single_edge_args.contains("devices") || !single_edge_args["devices"].is_array()) {
                std::string device_id = single_edge_args.value("name", "device_" + std::to_string(i));
                std::string dev_type = single_edge_args.value("edge_type", edge_type);
                nlohmann::json dev = nlohmann::json::object();
                dev["device_id"] = device_id;
                dev["type"] = dev_type;
                single_edge_args["devices"] = nlohmann::json::array();
                single_edge_args["devices"].push_back(dev);
                // 补上 edge_id 方便日志/内部使用
                if (!single_edge_args.contains("edge_id")) {
                    single_edge_args["edge_id"] = single_edge_args.value("name", "edge_" + std::to_string(i));
                }
                MYLOG_INFO("Edge 设备 ID: {} 自动补全 devices 字段: {}", i, single_edge_args["devices"].dump());
            }
            MYLOG_INFO("Edge 设备 ID: {} 类型: {}", i, edge_type);
            MYLOG_INFO("Edge 设备 ID: {} 参数: {}", i, single_edge_args.dump(4));
            MYLOG_INFO("-------------------------------------------");

            // 创建 Edge 设备实例 + 初始化 Edge 设备
            std::unique_ptr<my_edge::IEdge> edge_device = my_edge::MyEdge::GetInstance().Create(edge_type, single_edge_args, &error_msg);
            // edge_device->ShowAnalyzeInitArgs(single_edge_args);
            if (!edge_device) {
                MYLOG_ERROR("Edge 设备 ID: {} 创建失败，类型未知: {}", i, edge_type);
                continue;
            } else {
                MYLOG_INFO("Edge 设备 ID: {} 创建成功", i);
            }
            
            MYLOG_INFO("成功启动 Edge 设备 ID: {}", i);
            MYLOG_INFO("获取数据快照: {}", edge_device->GetStatusSnapshot().toString());
            MYLOG_INFO("内部信息: {}", edge_device->DumpInternalInfo().dump(4));
            my_edge::MyEdges::GetInstance().appendEdge(std::move(edge_device));
        }
    } catch (const std::exception& e) {
        MYLOG_ERROR("启动 Edge 模块时捕获异常: {}", e.what());
    }
    MYLOG_INFO("Edge 模块设备创建流程完成，正在启动所有 Edge 设备...");
    ::my_edge::MyEdges::GetInstance().startAllEdges();
    MYLOG_INFO("所有 Edge 设备已启动");
}

void Pipeline::LaunchMyMqttBroker(const nlohmann::json& args) {
    MYLOG_INFO("启动 MQTT Broker 模块");
    MYLOG_INFO("MQTT Broker 模块参数: {}", args.dump(4));
    // 这里可以根据 args 创建和管理 MQTT Broker 实例
    try {
        my_mqtt_broker_manager::MyMqttBrokerManager::GetInstance().Init(args);
        my_mqtt_broker_manager::MyMqttBrokerManager::GetInstance().Start();
        MYLOG_INFO("成功启动 MQTT Broker 模块");
    } catch (const std::exception& e) {
        MYLOG_ERROR("启动 MQTT Broker 模块时捕获异常: {}", e.what());
    } 
    return;
}


void Pipeline::LaunchSoftHealthyMonitor(const nlohmann::json& args) {
    MYLOG_INFO("启动 SoftHealthyMonitor 模块");
    MYLOG_INFO("SoftHealthyMonitor 模块参数: {}", args.dump(4));
    
    // 这里可以根据 args 创建和管理 SoftHealthyMonitor 实例
    try {
        MySoftHealthy::SoftHealthMonitorConfig config;
        MySoftHealthy::generateSampleConfigByJson(args, config);
        MySoftHealthy::ShowSoftHealthMonitorConfigAsJson(config);
        // 根据 args 初始化 config
        auto& monitor = MySoftHealthy::SoftHealthMonitorManager::getInstance();
        monitor.init(config);
        monitor.start();
        MYLOG_INFO("成功启动 SoftHealthyMonitor 模块");
    } catch (const std::exception& e) {
        MYLOG_ERROR("启动 SoftHealthyMonitor 模块时捕获异常: {}", e.what());
    }
}

void Pipeline::LaunchFlyControl(const nlohmann::json& args) {
    MYLOG_INFO("启动 FlyControl 模块");
    MYLOG_INFO("FlyControl 模块参数: {}", args.dump(4));

    constexpr int kMaxRetries = 10;
    constexpr int kRetryIntervalSeconds = 3;

    std::string err;
    try {
        auto& manager = fly_control::MyFlyControlManager::GetInstance();

        for (int attempt = 1; attempt <= kMaxRetries; ++attempt) {
            err.clear();
            if (manager.Init(args, &err)) {
                break;
            }
            if (attempt == kMaxRetries) {
                MYLOG_ERROR("FlyControl 模块初始化失败（已重试 {} 次）: {}", kMaxRetries, err.empty() ? std::string("unknown error") : err);
                return;
            }
            MYLOG_WARN("FlyControl 模块初始化失败（第 {}/{} 次），{}秒后重试: {}", attempt, kMaxRetries, kRetryIntervalSeconds, err);
            std::this_thread::sleep_for(std::chrono::seconds(kRetryIntervalSeconds));
        }

        if (!manager.Start(&err)) {
            MYLOG_ERROR("FlyControl 模块启动失败: {}", err.empty() ? std::string("unknown error") : err);
            return;
        }
        MYLOG_INFO("成功启动 FlyControl 模块");
    } catch (const std::exception& e) {
        MYLOG_ERROR("启动 FlyControl 模块时捕获异常: {}", e.what());
    }
}

void Pipeline::LaunchPodManager(const nlohmann::json& args) {
    const std::string module_name = "吊舱管理模块(PodManager)";
    MYLOG_INFO("===== 开始启动模块: {} =====", module_name);
    MYLOG_INFO("PodManager 模块参数: {}", args.dump(4));

    try {
        auto& manager = PodModule::PodManager::GetInstance();
        if (!manager.Init(args)) {
            MYLOG_ERROR("* 模块: {}, 状态: {}", module_name, "初始化失败");
            return;
        }
        MYLOG_INFO("* 模块: {}, 状态: {}, 吊舱数量: {}", module_name, "初始化成功", manager.size());
        // manager.Start();
    } catch (const std::exception& e) {
        MYLOG_ERROR("* 模块: {}, 捕获异常: {}", module_name, e.what());
    }
}

void Pipeline::Stop() {
    is_running_ = false;
    // 显式停止 API 服务
    my_api::MyAPI::GetInstance().Stop();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    workers_.clear();
    MYLOG_INFO("* Arg: {}, Value: {}", "Pipeline", "System Stopped Cleanly");
}

} // namespace pipeline
} // namespace tools