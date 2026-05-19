# NetDisk 前端项目迁移说明

这份文档用于把 Qt/CMake/QML 前端项目迁移到另一台电脑。

推荐单独创建一个 GitHub 仓库：

```text
NetDisk-Frontend
```

## 一、项目类型

当前前端是 Qt + CMake + QML 项目。

核心文件和目录：

```text
CMakeLists.txt
src/
FluentUI/
```

其中：

- `src/` 是前端应用源码、QML、资源文件、控制器和网络层。
- `FluentUI/` 是当前项目依赖的 UI 组件库，而且本项目里对它有改动，所以需要随仓库一起迁移。
- `CMakeLists.txt` 是根 CMake 配置。

## 二、不要迁移到 GitHub 的内容

这些是本机生成文件或构建产物，不应该提交到 GitHub：

```text
build/
bin/
dist/
CMakeLists.txt.user
CMakeLists.txt.user.*
*.exe
*.dll
*.pdb
*.ilk
.vs/
.vscode/
.idea/
```

说明：

- `build/` 是 CMake/Qt Creator 生成的构建目录。
- `bin/` 是编译出的可执行文件和调试文件。
- `CMakeLists.txt.user` 保存 Qt Creator 本机 Kit、路径、调试配置，新电脑应该重新生成。

## 三、旧电脑：提交并推送代码

查看状态：

```powershell
git status
```

提交源码：

```powershell
git add .
git commit -m "Prepare frontend project migration"
```

推送到 GitHub：

```powershell
git push -u origin master
```

如果你的分支不是 `master`，先查看：

```powershell
git branch --show-current
```

然后把命令里的 `master` 改成当前分支名。

## 四、新电脑：安装环境

新电脑建议安装：

- Git
- Qt Creator
- Qt 6.6.x
- CMake
- MSVC 2019 或 MSVC 2022 C++ 构建工具

当前旧电脑曾使用过的 Kit 是：

```text
Desktop_Qt_6_6_2_MSVC2019_64bit-Debug
```

所以新电脑优先使用 Qt 6.6.x + MSVC 64-bit Kit。

## 五、新电脑：克隆项目

使用 SSH：

```powershell
git clone git@github.com:Su2uka/NetDisk-Frontend.git
cd NetDisk-Frontend
```

如果新电脑还没有配置 SSH key，也可以先用 HTTPS：

```powershell
git clone https://github.com/Su2uka/NetDisk-Frontend.git
cd NetDisk-Frontend
```

## 六、新电脑：用 Qt Creator 打开

1. 打开 Qt Creator。
2. 选择 `Open Project`。
3. 打开项目根目录的 `CMakeLists.txt`。
4. 选择 Qt 6.6.x MSVC 64-bit Kit。
5. 等待 CMake Configure 完成。
6. 点击 Build。
7. 点击 Run。

Qt Creator 会在新电脑自动生成新的：

```text
CMakeLists.txt.user
build/
bin/
```

这些文件不需要从旧电脑复制。

## 七、后端地址配置

前端请求后端的基础地址在：

```text
src/global/AppConfig.h
```

开发环境当前配置：

```cpp
const QString API_BASE = "http://localhost:8000/api/v1";
```

所以新电脑本地开发时，需要先启动后端，并确保后端运行在：

```text
http://localhost:8000
```

如果后端在另一台机器或服务器上，需要把这里改成对应地址，例如：

```cpp
const QString API_BASE = "http://192.168.1.10:8000/api/v1";
```

## 八、迁移后检查

### 检查 Git 状态

```powershell
git status
```

正常情况下不应该出现 `build/`、`bin/`、`CMakeLists.txt.user` 之类的待提交文件。

### 检查构建

在 Qt Creator 中执行：

```text
Configure -> Build -> Run
```

或者使用命令行构建，具体命令取决于 Qt 安装路径和 Kit 配置。

### 检查接口连接

1. 先启动后端。
2. 再启动前端。
3. 登录或注册账号。
4. 检查文件列表、上传、下载、收藏、分享等页面是否能正常请求后端。

## 九、推荐迁移顺序

1. 旧电脑提交并推送后端。
2. 旧电脑备份后端 `.env` 和 `data/`。
3. 旧电脑提交并推送前端。
4. 新电脑恢复后端，启动 Docker 和 FastAPI。
5. 新电脑克隆前端。
6. 新电脑安装 Qt、CMake、MSVC Kit。
7. Qt Creator 打开前端 `CMakeLists.txt`。
8. 确认 `src/global/AppConfig.h` 的后端地址正确。
9. 构建并运行前端。
