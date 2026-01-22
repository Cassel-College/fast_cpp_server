#include "MyLog.h"
#include "MFMQTTServer.h"
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace fast_mqtt {

void MFMQTTServer::init(const nlohmann::json& cfg) {
    cfg_ = cfg;
    // 默认路径支持 ${HOME}
    mosquitto_path_ = expand_env(cfg.value("mosquitto_path", "mosquitto"));
    conf_path_ = expand_env(cfg.value("conf_path", "${HOME}/.cache/mosquitto_tmp.conf"));
    log_path_  = expand_env(cfg.value("log_path", "${HOME}/.cache/mosquitto.log"));
    MYLOG_INFO("Server init conf=%s log=%s", conf_path_.c_str(), log_path_.c_str());
}

std::string MFMQTTServer::expand_env(const std::string& s) {
    std::string out = s;
    const char* home = getenv("HOME");
    if (home) {
        // 简单替换 ${HOME}
        size_t pos = out.find("${HOME}");
        if (pos != std::string::npos) out.replace(pos, 7, home);
    }
    return out;
}

bool MFMQTTServer::write_conf() {
    try {
        std::ofstream ofs(conf_path_, std::ios::out | std::ios::trunc);
        if (!ofs) {
            MYLOG_ERROR("open conf failed: %s", conf_path_.c_str());
            return false;
        }
        // 基于 JSON 生成 mosquitto.conf
        // 关键项：listener, allow_anonymous, persistence, log_dest 等
        int port = cfg_.value("port", 1883);
        std::string bind_addr = cfg_.value("bind_addr", "0.0.0.0");
        bool allow_anonymous = cfg_.value("allow_anonymous", true);
        bool persistence = cfg_.value("persistence", false);

        ofs << "listener " << port << " " << bind_addr << "\n";
        ofs << "allow_anonymous " << (allow_anonymous ? "true" : "false") << "\n";
        ofs << "persistence " << (persistence ? "true" : "false") << "\n";
        ofs << "log_dest file " << log_path_ << "\n";
        ofs.close();
        MYLOG_INFO("write_conf ok: %s", conf_path_.c_str());
        return true;
    } catch (const std::exception& e) {
        MYLOG_ERROR("write_conf exception: %s", e.what());
        return false;
    }
}

bool MFMQTTServer::start() {
    if (!write_conf()) return false;
    pid_t cpid = fork();
    if (cpid < 0) {
        MYLOG_ERROR("fork failed");
        return false;
    }
    if (cpid == 0) {
        // 子进程：启动 mosquitto
        MYLOG_WARN("正在启动 mosquitto...");
        MYLOG_WARN("执行命令: mosquitto -c %s", conf_path_.c_str());
        execlp("mosquitto", "mosquitto", "-c", conf_path_.c_str(), (char*)nullptr);
        _exit(127); // execlp 失败
    }
    // 父进程：记录 pid
    pid_ = cpid;
    MYLOG_INFO("mosquitto started pid=%d", (int)cpid);
    return true;
}

bool MFMQTTServer::stop() {
    if (!pid_) return false;
    pid_t p = *pid_;
    int rc = kill(p, SIGTERM);
    if (rc != 0) {
        MYLOG_ERROR("kill(SIGTERM) failed rc=%d", rc);
        return false;
    }
    int status = 0;
    waitpid(p, &status, 0);
    MYLOG_INFO("mosquitto stopped pid=%d status=%d", (int)p, status);
    pid_.reset();
    return true;
}

} // namespace fast_mqtt
