#  使用官方的 Ubuntu 作为基础镜像
FROM ubuntu:24.04

# 设置时区为 UTC，避免时区警告
# ENV TZ=UTC
ENV TZ=Asia/Shanghai

# 更新镜像，并安装必要的工具和依赖
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y \
    build-essential \
    gcc \
    g++ \
    cmake \
    git \
    curl \
    wget \
    unzip \
    gcovr \
    nginx \
    doxygen \
    graphviz \
    graphviz-dev \
    libldap2-dev \
    libsasl2-dev \
    python3.14 \
    python3-pip \
    libldap2-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    clangd-14 \
    libgmp-dev \
    && apt-get clean

# # 安装 Python 包 pygraphviz
RUN pip3 install pygraphviz --break-system-packages

# 设置工作目录
WORKDIR /workspace

# 创建文件夹用于存储生成的文档和覆盖率报告
RUN mkdir -p /workspace/code /workspace/doxygen_docs /var/www/html 

# 暴露端口:通过端口映射暴露覆盖率报告和 Doxygen 生成的文档。
EXPOSE 5555 4444

# 定义容器启动时的命令，允许进入交互式 shell，方便开发
CMD ["/bin/bash"]
