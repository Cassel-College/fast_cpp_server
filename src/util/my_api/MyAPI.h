#pragma once
#ifndef MY_API_H
#define MY_API_H

#include <thread>
#include <atomic>
#include <memory>
#include "MyLog.h"
#include <csignal>

namespace my_api {

class MyAPI {
public:
    // 获取单例实例
    static MyAPI& GetInstance() {
        static MyAPI instance;
        return instance;
    }

    // 禁用拷贝构造和赋值
    MyAPI(const MyAPI&) = delete;
    MyAPI& operator=(const MyAPI&) = delete;

    // 在独立线程中启动 API 服务
    void Start(int port = 8000);
    
    // 停止服务
    void Stop();
    bool IsRunning() const { return is_running_.load(); }

private:
    MyAPI() : is_running_(false) {}
    ~MyAPI() { Stop(); }

    void ServerThread_old(int port);
    void ServerThread(int port);

    std::atomic<bool> is_running_;
    std::thread server_thread_;
};

void RunRestServer(int port);

} // namespace my_api

#endif