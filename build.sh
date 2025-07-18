#!/bin/bash


# 创建build目录
if [ -d "build" ]; then
  echo "📁 Directory 'build' already exists. ✅"
elif mkdir build; then
  echo "📁 Successfully created build directory. ✅"
else
  echo "📁 Failed to create build directory. ❌"
fi

# 创建external目录
if [ -d "external" ]; then
  echo "📁 Directory 'external' already exists. ✅"
elif mkdir external; then
  echo "📁 Successfully created external directory. ✅"
else
  echo "📁 Failed to create external directory. ❌"
fi

# # 创建include目录
# if [ -d "src/include" ]; then
#   echo "📁 Directory 'src/include' already exists. ✅"
# elif mkdir src/include; then
#   echo "📁 Successfully created src/include directory. ✅"
# else
#   echo "📁 Failed to create src/include directory. ❌"
# fi

# 创建src目录
if [ -d "src" ]; then
  echo "📁 Directory 'src' already exists. ✅"
elif mkdir src; then
  echo "📁 Successfully created src directory. ✅"
else
  echo "📁 Failed to create src directory. ❌"
fi

# 创建test目录
if [ -d "test" ]; then
  echo "📁 Directory 'test' already exists. ✅"
elif mkdir test; then
  echo "📁 Successfully created test directory. ✅"
else
  echo "📁 Failed to create test directory. ❌"
fi

# 创建tools目录
if [ -d "src/tools" ]; then
  echo "📁 Directory 'src/tools' already exists. ✅"
elif mkdir src/tools; then
  echo "📁 Successfully created src/tools directory. ✅"
else
  echo "📁 Failed to create src/tools directory. ❌"
fi

# 创建util目录
if [ -d "src/util" ]; then
  echo "📁 Directory 'src/util' already exists. ✅"
elif mkdir src/util; then
  echo "📁 Successfully created src/util directory. ✅"
else
  echo "📁 Failed to create src/util directory. ❌"
fi

# 创建scripts目录
if [ -d "scripts" ]; then
  echo "📁 Directory 'scripts' already exists. ✅"
elif mkdir scripts; then
  echo "📁 Successfully created scripts directory. ✅"
else
  echo "📁 Failed to create scripts directory. ❌"
fi

# 创建scripts目录
if [ -d "docs" ]; then
  echo "📁 Directory 'docs' already exists. ✅"
elif mkdir docs; then
  echo "📁 Successfully created docs directory. ✅"
else
  echo "📁 Failed to create docs directory. ❌"
fi

# 创建CMakeLists.txt文件
if [ -f "CMakeLists.txt" ]; then
  echo "📄 File 'CMakeLists.txt' already exists. ✅"
elif touch CMakeLists.txt; then
  echo "📄 Successfully created CMakeLists.txt file. ✅"
else
  echo "📄 Failed to create CMakeLists.txt file. ❌"
fi

# 下载spdlog
if [ -d "external/spdlog" ]; then
  echo "⬇️ Directory 'external/spdlog' already exists. ✅"
elif git clone https://github.com/gabime/spdlog.git external/spdlog; then
  echo "⬇️ Successfully downloaded spdlog. ✅"
else
  echo "⬇️ Failed to download spdlog. ❌"
fi

# 下载easyloggingpp
if [ -d "external/easyloggingpp" ]; then
  echo "⬇️ Directory 'external/easyloggingpp' already exists. ✅"
elif git clone https://github.com/amrayn/easyloggingpp.git external/easyloggingpp; then
  echo "⬇️ Successfully downloaded easyloggingpp. ✅"
else
  echo "⬇️ Failed to download easyloggingpp. ❌"
fi

# 下载Google Test
if [ -d "external/googletest" ]; then
  echo "⬇️ Directory 'external/googletest' already exists. ✅"
elif git clone --recurse-submodules https://github.com/google/googletest.git external/googletest; then
  echo "⬇️ Successfully downloaded Google Test. ✅"
else
  echo "⬇️ Failed to download Google Test. ❌"
fi


# 下载jsoncpp
if [ -d "external/jsoncpp" ]; then
  echo "⬇️ Directory 'external/jsoncpp' already exists. ✅"
elif git clone https://github.com/open-source-parsers/jsoncpp.git external/jsoncpp; then
  echo "⬇️ Successfully downloaded JSONCPP. ✅"
else
  echo "⬇️ Failed to download JSONCPP. ❌"
fi

# 下载nlohmann/json
if [ -d "external/json" ]; then
  echo "⬇️ Directory 'external/json' already exists. ✅"
elif git clone https://github.com/nlohmann/json.git external/json; then
  echo "⬇️ Successfully downloaded nlohmann/json. ✅"
else
  echo "⬇️ Failed to download nlohmann/json. ❌"
fi

# 下载nlohmann/cpr
if [ -d "external/cpr" ]; then
  echo "⬇️ Directory 'external/cpr' already exists. ✅"
elif git clone https://github.com/libcpr/cpr.git external/cpr; then
  echo "⬇️ Successfully downloaded libcpr/cpr. ✅"
else
  echo "⬇️ Failed to download libcpr/cpr. ❌"
fi

# 下载mosquitto
if [ -d "external/mosquitto" ]; then
  echo "⬇️ Directory 'external/mosquitto' already exists. ✅"
elif git clone --branch v2.0.18 https://github.com/eclipse/mosquitto.git external/mosquitto; then
  echo "⬇️ Successfully downloaded mosquitto. ✅"
  cd external/mosquitto
  git submodule update --init --recursive
  cd -
else
  echo "⬇️ Failed to download mosquitto. ❌"
fi

# 下载libzmq
if [ -d "external/libzmq" ]; then
  echo "⬇️ Directory 'external/libzmq' already exists. ✅"
elif git clone https://github.com/zeromq/libzmq.git external/libzmq; then
  echo "⬇️ Successfully downloaded libzmq. ✅"
  # cd external/libzmq
  # git submodule update --init --recursive
  # cd -
else
  echo "⬇️ Failed to download libzmq. ❌"
fi

# # 下载simpleini
# if [ -d "external/simpleini" ]; then
#   echo "⬇️ Directory 'external/simpleini' already exists. ✅"
# elif git clone https://github.com/brofield/simpleini.git external/simpleini; then
#   echo "Run simpleini code init over. ✅"
# else
#   echo "⬇️ Failed to download simpleini. ❌"
# fi

# 下载putobuf
if [ -d "external/protobuf" ]; then
  echo "⬇️ Directory 'external/protobuf' already exists. ✅"
elif git clone --branch 21.x https://github.com/protocolbuffers/protobuf.git external/protobuf; then
  echo "⬇️ Successfully downloaded protobuf. ✅"
  echo "Run protobuf code init."
  cd external/protobuf
  git submodule update --init --recursive
  cd -
  echo "Run protobuf code init over. ✅"
else
  echo "⬇️ Failed to download protobuf. ❌"
fi

# 下载 eigen; 它是 header-only，无需编译。
if [ -d "external/eigen" ]; then
  echo "⬇️ Directory 'external/eigen' already exists. ✅"
elif git clone https://gitlab.com/libeigen/eigen.git external/eigen; then
  echo "⬇️ Successfully downloaded eigen. ✅"
else
  echo "⬇️ Failed to download eigen. ❌"
fi

# 下载 OpenCV
if [ -d "external/opencv" ]; then
  echo "⬇️ Directory 'external/opencv' already exists. ✅"
elif git clone --branch 4.9.0 https://github.com/opencv/opencv.git external/opencv; then
  echo "⬇️ Successfully downloaded OpenCV. ✅"
  cd external/opencv
  git submodule update --init --recursive
  cd -
else
  echo "⬇️ Failed to download OpenCV. ❌"
fi

# 下载 OpenCV contrib（可选）
if [ -d "external/opencv_contrib" ]; then
  echo "⬇️ Directory 'external/opencv_contrib' already exists. ✅"
elif git clone --branch 4.9.0 https://github.com/opencv/opencv_contrib.git external/opencv_contrib; then
  echo "⬇️ Successfully downloaded OpenCV contrib. ✅"
  cd external/opencv_contrib
  git submodule update --init --recursive
  cd -
else
  echo "⬇️ Failed to download OpenCV contrib. ❌"
fi

# 下载 SymEngine
if [ -d "external/symengine" ]; then
  echo "⬇️ Directory 'external/symengine' already exists. ✅"
elif git clone https://github.com/symengine/symengine.git external/symengine; then
  echo "⬇️ Successfully downloaded SymEngine. ✅"
  cd external/symengine
  git submodule update --init --recursive
  cd -
else
  echo "⬇️ Failed to download SymEngine. ❌"
fi

# create main.cpp
if [ -f "src/main.cpp" ]; then
  echo "📄 File 'src/main.cpp' already exists. ✅"
elif touch src/main.cpp; then
  echo "📄 Successfully created main.cpp file. ✅"
else
  echo "📄 Failed to create main.cpp file. ❌"
fi

# create build_google_test_framework.sh
if [ -f "scripts/build_google_test_framework.sh" ]; then
  echo "📄 File 'scripts/build_google_test_framework.sh' already exists. ✅"
elif touch scripts/build_google_test_framework.sh; then
  echo "📄 Successfully created build_google_test_framework.sh file. ✅"
else
  echo "📄 Failed to create build_google_test_framework.sh file. ❌"
fi

# create build_spdlog_lib.sh
if [ -f "scripts/build_spdlog_lib.sh" ]; then
  echo "📄 File 'scripts/build_spdlog_lib.sh' already exists. ✅"
elif touch scripts/build_spdlog_lib.sh; then
  echo "📄 Successfully created build_spdlog_lib.sh file. ✅"
else
  echo "📄 Failed to create build_spdlog_lib.sh file. ❌"
fi

# create clear_build_dir.sh
if [ -f "scripts/clear_build_dir.sh" ]; then
  echo "📄 File 'scripts/clear_build_dir.sh' already exists. ✅"
elif touch scripts/clear_build_dir.sh; then
  echo "📄 Successfully created clear_build_dir.sh file. ✅"
else
  echo "📄 Failed to create clear_build_dir.sh file. ❌"
fi


# 创建proto目录
if [ -d "src/proto" ]; then
  echo "📁 Directory 'proto' already exists. ✅"
elif mkdir src/proto; then
  echo "📁 Successfully created proto directory. ✅"
  TEST_PROTO_FILE="src/proto/test.proto"
  touch $TEST_PROTO_FILE
  echo 'syntax = "proto3";' > $TEST_PROTO_FILE
  echo '' >> $TEST_PROTO_FILE
  echo 'message Person {' >> $TEST_PROTO_FILE
  echo '  string name = 1;' >> $TEST_PROTO_FILE
  echo '  int32 id = 2;' >> $TEST_PROTO_FILE
  echo '}' >> $TEST_PROTO_FILE
else
  echo "📁 Failed to create proto directory. ❌"
fi

# 创建 protobuf 目录
if [ -d "src/protobuf" ]; then
  echo "📁 Directory 'protobuf' already exists. ✅"
elif mkdir src/protobuf; then
  echo "📁 Successfully created protobuf directory. ✅"
else
  echo "📁 Failed to create protobuf directory. ❌"
fi