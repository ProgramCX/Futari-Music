# Futari Music

自托管音乐服务，包含 Qt 桌面客户端和 Spring Boot 后端。

- 客户端安装包：[GitHub Releases](https://github.com/ProgramCX/Futari-Music/releases)
- 服务端推荐使用 Docker Compose 部署。
- 完整配置与数据迁移说明：[后端部署文档](SpringBoot/doc/后端实现与部署.md)
- 客户端打包与 Release 命名：[客户端发行文档](Qt/doc/客户端自动更新与发行约定.md)

## Docker Compose 部署

### 准备文件

服务器需要安装 Docker Engine 和 Docker Compose 插件。克隆仓库、构建服务端 JAR，并创建运行配置：

    git clone https://github.com/ProgramCX/Futari-Music.git
    cd Futari-Music
    mkdir -p build
    mvn -f SpringBoot/pom.xml clean package
    cp SpringBoot/target/futari-*.jar build/futari-server.jar
    cp .env.compose.example .env

也可以从 GitHub Release 下载后端 JAR，放到 build/futari-server.jar。

### 配置密钥

编辑仓库根目录的 .env，替换数据库、Redis 和管理员密码，并生成 JWT 密钥：

    openssl rand -hex 32

将命令输出填入 JWT_SECRET。至少设置以下值：

    MYSQL_ROOT_PASSWORD=替换为独立随机密码
    DB_USER=futari
    DB_PASSWORD=替换为独立随机密码
    REDIS_PASSWORD=替换为独立随机密码
    JWT_SECRET=填入刚生成的密钥
    BOOTSTRAP_ADMIN_USERNAME=admin
    BOOTSTRAP_ADMIN_PASSWORD=设置一个12至72位的管理员密码

首次登录后可以从 .env 移除引导管理员密码。不要提交 .env，也不要将数据库或 Redis 端口开放到公网。

### 启动和查看状态

在仓库根目录运行：

    docker compose config
    docker compose up -d
    docker compose ps
    docker compose logs -f server

服务默认监听 8080。直连时，桌面客户端服务器地址填写 http://服务器地址:8080；通过 HTTPS 反向代理时填写代理域名。公网部署请在前面配置 HTTPS 反向代理；仓库提供的 Nginx 配置示例和存储目录要求见[后端部署文档](SpringBoot/doc/后端实现与部署.md)。

### 更新和数据

更新后端时替换 JAR，再重建服务容器：

    mvn -f SpringBoot/pom.xml clean package
    cp SpringBoot/target/futari-*.jar build/futari-server.jar
    docker compose up -d --force-recreate server

全新 MySQL 数据卷会自动初始化数据库。已有数据库不会自动执行新 SQL；升级前先备份，再按[迁移文档](SpringBoot/doc/后端实现与部署.md)执行对应脚本。

停止服务并保留数据：

    docker compose down

Compose 使用持久卷保存数据库、Redis、音乐文件和客户端更新包。只有明确要删除这些数据时才执行 docker compose down -v。

## 安装桌面客户端

从 [GitHub Releases](https://github.com/ProgramCX/Futari-Music/releases) 下载与系统匹配的安装包：

- Windows x64：运行 Inno Setup EXE，可选择简体中文或英文。
- Ubuntu amd64：安装 DEB，例如 `sudo apt install ./FutariMusic-版本号-ubuntu-amd64.deb`；系统可能请求管理员授权。

首次启动时将服务器地址设为上文的地址。客户端会从服务端检查版本；发布客户端新版本时，GitHub Release 需要包含相应平台的安装包。具体 tag 和附件命名见[客户端发行文档](Qt/doc/客户端自动更新与发行约定.md)。

## 从源码部署后端

Docker Compose 不适用时，需要 JDK 17、MySQL 8 和 Redis。新数据库从仓库根目录执行：

    mysql -u root -p < SpringBoot/sql/schema.sql

复制 SpringBoot/.env.example 为 SpringBoot/.env，填写数据库、Redis、JWT 和管理员配置，并将 SPRING_PROFILES_ACTIVE 设为 prod。后端默认监听 8080；需要更换本地端口时，将 SERVER_PORT 改成所需端口，例如 18080，然后运行：

    cd SpringBoot
    mvn clean package
    java -jar target/futari-0.1.0.jar

公开部署时使用生产配置，并通过 HTTPS 反向代理对外提供服务。存储目录、Nginx、备份和旧数据库迁移步骤见[后端部署文档](SpringBoot/doc/后端实现与部署.md)。

## 从源码打包 Qt 客户端

需要 Qt 6.8+、CMake，以及对应平台的构建工具。Windows 还需 Qt MinGW 和 Inno Setup；设置 FUTARI_QT_ROOT 与 FUTARI_MINGW_ROOT，分别指向包含 windeployqt.exe 和 g++.exe 的安装目录。Ubuntu DEB 会依赖打包环境中的 Qt ABI，建议在目标 Ubuntu 版本上构建。脚本会把安装包生成到仓库根目录的 build/：

Windows x64（Qt MinGW 和 Inno Setup）：

    .\Qt\FutariMusic\packaging\package-windows.ps1 -ClientVersion 0.1.0 -ServerCompatVersion 1.1

Ubuntu amd64（Qt、CMake 和 CPack）：

    bash ./Qt/FutariMusic/packaging/package-ubuntu.sh 0.1.0 1.1

版本规则和 GitHub Release 发布示例见[客户端发行文档](Qt/doc/客户端自动更新与发行约定.md)。

## 许可证

本项目使用 [MIT License](LICENSE)。
