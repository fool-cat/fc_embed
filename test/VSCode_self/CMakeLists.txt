

# 使用C++11标准
set(CMAKE_CXX_STANDARD 11)

# 设置项目名称
project(VSCode_self)

# 当前路径下所有源文件存入变量 DIR_SRCS
aux_source_directory(. DIR_SRCS)

# 指定生成目标
add_executable(VSCode_self ${DIR_SRCS})

