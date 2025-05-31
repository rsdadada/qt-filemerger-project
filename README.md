# FileMergerApp

这是一个用于合并文件的 Qt 应用程序。

## 构建指南

### 前提条件

1.  **Qt 开发环境**: 确保你已经安装了 Qt (>= 5.x 版本，包含 `core`, `gui`, `widgets` 模块)。你可以从 [Qt 官方网站](https://www.qt.io/download) 下载。
2.  **C++ 编译器**: 需要一个支持 C++11 或更高版本的编译器 (例如 GCC, Clang, MSVC)。

### 构建步骤

1.  **打开 Qt Creator**:
    *   打开 Qt Creator IDE。
    *   选择 "File" > "Open File or Project..."。
    *   导航到项目根目录，选择 `filemerger.pro` 文件。
    *   配置项目，选择合适的 Qt 版本和构建套件 (Kit)。

2.  **或者使用命令行**:
    *   打开终端或命令行提示符。
    *   导航到项目根目录。
    *   运行以下命令来生成 Makefile (具体命令可能因你的 Qt 版本和操作系统而略有不同):
        ```bash
        qmake
        ```
    *   然后编译项目:
        ```bash
        make  # 或者在 Windows 上使用 mingw32-make 或 nmake
        ```

## 运行指南

编译成功后，可执行文件通常位于构建目录下的 `release` 或 `debug` 子目录中 (例如 `build-release/release/FileMergerApp.exe` 或者在 Linux/macOS 上的类似路径)。

双击可执行文件，或者在终端中运行它。

### Windows 部署

为了让应用程序能够在没有安装 Qt 开发环境的 Windows 机器上运行，你需要将 Qt 的运行时库和插件与你的可执行文件一起打包。Qt 提供了 `windeployqt` 工具来简化这个过程。

1.  **打开 Qt 命令行提示符**: 从开始菜单找到并打开对应你 Qt 版本的命令行提示符 (例如 "Qt 5.15.2 (MSVC 2019 64-bit)" 或类似的 MinGW 版本名称)。这个环境会自动配置好 Qt 工具的路径。
2.  **导航到项目根目录**: 在 Qt 命令行提示符中，使用 `cd` 命令进入你的项目根目录 (例如 `cd D:\qt_projext`)。
3.  **运行 `windeployqt`**:
    ```powershell
    windeployqt.exe .\build-release\release\FileMergerApp.exe
    ```
    (请确保 `.\build-release\release\FileMergerApp.exe` 是你 Release 版本可执行文件的正确路径)。
    该命令会将所有必需的 Qt DLLs、插件和翻译文件复制到你的 `build-release\release` 目录下。
4.  **分发**: 现在，你可以将整个 `build-release\release` 目录复制到目标 Windows 机器上，然后运行 `FileMergerApp.exe`。

    **注意**:
    *   如果你的应用程序使用了网络功能 (特别是 HTTPS)，你可能需要额外处理 OpenSSL 库的部署。
    *   根据你的编译器 (MSVC 或 MinGW)，目标机器可能需要相应的 C++ 运行时库。MinGW 的库通常由 `windeployqt` 自动包含。对于 MSVC，目标机器可能需要安装对应版本的 "Microsoft Visual C++ Redistributable"。

## 测试

项目的测试用例位于 `tests/` 目录下，使用 Qt Test 框架。

### 编译测试

1.  **打开 Qt Creator (推荐)**:
    *   如果你已经在 Qt Creator 中打开了主项目 (`filemerger.pro`)，通常 Qt Creator 会自动识别 `tests/MyFileModelTest.pro` 子项目。
    *   你可以在 Qt Creator 的项目面板中选择 `MyFileModelTest` 项目作为活动项目进行构建。

2.  **或者使用命令行**:
    *   打开终端或命令行提示符。
    *   导航到 `tests/` 目录: `cd tests`
    *   运行 qmake (如果需要，指定主项目源码路径，不过通常 Qt Test 的 .pro 文件会通过相对路径正确找到依赖):
        ```bash
        qmake MyFileModelTest.pro 
        ```
        或者在项目根目录运行，并指定 .pro 文件：
        ```bash
        qmake tests/MyFileModelTest.pro
        ```
    *   然后编译测试:
        ```bash
        make # 或者在 Windows 上使用 mingw32-make 或 nmake
        ```

### 运行测试

编译成功后，测试可执行文件 `tst_customfilemodel` (或 `tst_customfilemodel.exe` 在 Windows 上) 会生成在测试项目的构建目录中。

*   **通过 Qt Creator**: 你可以直接在 Qt Creator 的测试界面运行测试。
*   **通过命令行**: 导航到测试可执行文件所在的目录，然后运行它：
    ```bash
    ./tst_customfilemodel 
    ```
    测试结果会输出到控制台。

## Using Docker for a Consistent Build & Test Environment

To ensure that this project can be compiled and tested reliably on any machine, regardless of its pre-installed software (beyond Docker itself), we provide a `Dockerfile`.

### Prerequisites

1.  **Install Docker**: You need to have Docker installed on your system. Follow the official installation guide for your operating system:
    *   [Install Docker Engine](https://docs.docker.com/engine/install/)

### Building the Docker Image

1.  **Open your terminal or command prompt**.
2.  **Navigate to the root directory of this project** (where the `Dockerfile` is located).
3.  **Build the Docker image** by running the following command. Replace `filemerger-app-env` with your preferred image name and tag:
    ```bash
    docker build -t filemerger-app-env:latest .
    ```
    This process might take a few minutes the first time as it downloads the base image and installs all the dependencies.

### Compiling and Testing Inside Docker

Once the image is built, you can run a container and perform compilation and testing tasks within it.

1.  **Run a Docker container from the image**:
    This command starts a new container, mounts your current project directory (on your host machine) into the `/app` directory inside the container, and gives you an interactive bash shell:
    ```bash
    docker run -it --rm -v "${PWD}:/app" filemerger-app-env:latest bash
    ```
    *   `-it`: Runs the container in interactive mode with a pseudo-TTY.
    *   `--rm`: Automatically removes the container when it exits.
    *   `-v "${PWD}:/app"`: Mounts the current working directory (your project) on the host to `/app` in the container. On Windows PowerShell, you might need to use `%(cd)%` or an absolute path instead of `"${PWD}"`. For Command Prompt (cmd.exe), use `"%cd%:/app"`.
    *   `filemerger-app-env:latest`: The name and tag of the image you built.
    *   `bash`: Starts a bash shell in the container.

2.  **Inside the Docker container's bash shell**:
    You are now inside the `/app` directory in the container, which mirrors your project files.

    *   **Compile the main application**:
        ```bash
        qmake filemerger.pro # Or just 'qmake' if it defaults to the .pro file
        make
        ```
        The compiled application (e.g., `FileMergerApp`) will be in the `/app` directory (and thus also in your project directory on your host machine).

    *   **Compile the tests**:
        Navigate to the tests directory:
        ```bash
        cd tests
        qmake MyFileModelTest.pro
        make
        cd .. # Go back to the project root
        ```
        The compiled test executable (e.g., `tst_customfilemodel`) will be in `/app/tests/` (and thus also in your project's `tests` directory on your host machine).

    *   **Run the tests**:
        Navigate to where the test executable was built (if not already in the test directory):
        ```bash
        ./tests/tst_customfilemodel
        ```
        Test results will be printed to the console.

3.  **Exiting the container**: Once you're done, type `exit` in the container's bash shell to stop and remove the container (due to `--rm`).

This Docker setup provides a self-contained environment for building and testing your Qt application and its tests, ensuring consistency across different machines.

### Running GUI Applications from Docker (Advanced)

While the Docker setup (above) is primarily for compilation and running console-based tests, it is possible to run GUI applications from within the Linux Docker container and display them on your host operating system. This is an advanced topic and requires specific configuration on your host machine.

**General Approach (X11 Forwarding):**

1.  **Install an X Server on your Host OS:**
    *   **Windows:** Install an X server like [VcXsrv](https://sourceforge.net/projects/vcxsrv/) or [Xming](https://sourceforge.net/projects/xming/). Configure it to accept connections (e.g., disable access control).
    *   **macOS:** Install [XQuartz](https://www.xquartz.org/). In XQuartz preferences, ensure "Allow connections from network clients" is enabled.
    *   **Linux:** You likely already have an X server running.

2.  **Determine your Host IP Address for Docker:**
    *   On Windows/macOS, Docker often uses a special hostname like `host.docker.internal` to refer to the host machine from within a container.
    *   Alternatively, find your machine's local IP address that is accessible from the Docker container.

3.  **Set the `DISPLAY` environment variable when running the container:**
    When running `docker run`, you'll need to pass the `DISPLAY` variable.
    *   Example for Linux hosts (assuming X11 socket is available):
        ```bash
        docker run -it --rm -v "${PWD}:/app" -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=$DISPLAY filemerger-app-env:latest bash
        ```
    *   Example for Windows (using VcXsrv, assuming it's listening on `your_host_ip:0.0`):
        ```bash
        # In PowerShell on the host, before running docker:
        # $env:DISPLAY="your_host_ip:0.0" 
        # (or use host.docker.internal:0.0 if it works for your Docker version)
        docker run -it --rm -v "${PWD}:/app" -e DISPLAY=host.docker.internal:0.0 filemerger-app-env:latest bash
        ```
    *   Example for macOS (using XQuartz, replace `your_host_ip`):
        ```bash
        # On the host, get your IP: ifconfig en0 (or similar)
        # In XQuartz terminal: socat TCP-LISTEN:6000,reuseaddr,fork UNIX-CONNECT:/tmp/.X11-unix/X0 &
        # (This makes XQuartz listen on TCP port 6000, change if needed)
        # Then run docker with:
        docker run -it --rm -v "${PWD}:/app" -e DISPLAY=your_host_ip:0 filemerger-app-env:latest bash
        ```
        (Note: macOS X11 forwarding can be tricky due to security settings and network configurations).

4.  **Inside the container:**
    After compiling your GUI application (e.g., `FileMergerApp`), you should be able to run it, and it *should* display on your host's X server.
    ```bash
    ./FileMergerApp
    ```

**Important Considerations:**
*   X11 forwarding can be slow and sometimes insecure if not configured (correctly).
*   The exact commands and setup vary significantly based on your host OS, Docker version, and X server configuration.
*   This method is generally more for development or testing purposes rather than for distributing GUI apps to end-users via Docker.

This Docker setup provides a self-contained environment for building and testing your Qt application and its tests, ensuring consistency across different machines.

## 依赖

*   Qt Core
*   Qt GUI
*   Qt Widgets

## Cross-Platform Deployment for the GUI Application

Beyond the Docker environment for building and testing, here's how to deploy the `FileMergerApp` GUI application on different platforms:

### Windows Deployment
(This section already exists and is well-detailed, no changes needed here if it's accurate)

### macOS Deployment

To create a standalone application bundle (`.app`) on macOS that can run on other Macs without a Qt development environment:

1.  **Build your application** on a macOS machine with Qt installed. This will produce a `FileMergerApp.app` bundle (typically in your build directory).
2.  **Open Terminal** on your macOS machine.
3.  **Use `macdeployqt`**: This command-line tool is part of the Qt installation (usually found in `QT_INSTALL_DIR/VERSION/clang_64/bin/macdeployqt`).
    Navigate to the directory containing `FileMergerApp.app` or provide the full path:
    ```bash
    macdeployqt FileMergerApp.app
    ```
    You might want to add options like `-dmg` to create a disk image:
    ```bash
    macdeployqt FileMergerApp.app -dmg
    ```
4.  **Distribution**: The `FileMergerApp.app` bundle (now containing necessary Qt frameworks and plugins) can be copied to other macOS machines and run. If you created a `.dmg` file, that's a common way to distribute macOS applications.

**Notes for `macdeployqt`:**
*   Ensure that the Qt version used for `macdeployqt` matches the one used to build the application.
*   You may need to code-sign your application for distribution on newer macOS versions to avoid Gatekeeper issues.

### Linux Deployment

Deploying Qt applications on Linux has several approaches:

1.  **Rely on System-Installed Qt (Recommended for many scenarios):**
    *   Most Linux distributions provide Qt runtime libraries through their package managers (e.g., `apt` on Debian/Ubuntu, `yum`/`dnf` on Fedora/CentOS).
    *   **Your responsibility**: Clearly document the Qt version your application was built against and the required Qt modules (e.g., Core, Gui, Widgets, Svg).
    *   **User's responsibility**: Install the necessary Qt runtime packages. For example, on an Ubuntu-based system:
        ```bash
        sudo apt update
        sudo apt install qt6-base-runtime # Or a more specific package if available
        # May also need packages for plugins like libqt6svg6, etc.
        ```
    *   **Distribution**: You distribute your compiled executable (`FileMergerApp`). Users place it wherever they like and run it, provided system Qt libraries are present and compatible.
    *   **Pros**: Smaller application package, leverages system updates for Qt.
    *   **Cons**: Relies on the user having the correct Qt version.

2.  **Bundle Qt Libraries:**
    *   Copy the necessary Qt shared library files (`.so` files like `libQt6Core.so.6`, etc., and plugin directories) alongside your application executable.
    *   Create a shell script to launch your application. This script should set the `LD_LIBRARY_PATH` environment variable to point to your bundled libraries directory before executing your application.
        Example `run_filemerger.sh`:
        ```bash
        #!/bin/sh
        APP_DIR="$(dirname "$(readlink -f "$0")")"
        export LD_LIBRARY_PATH="$APP_DIR/lib:$LD_LIBRARY_PATH" 
        "$APP_DIR/FileMergerApp" "$@" 
        ```
        (Place your Qt `.so` files in a `lib` subdirectory next to `FileMergerApp` and this script).
    *   **Pros**: More self-contained, less reliant on system Qt.
    *   **Cons**: Larger application package, you are responsible for updating bundled Qt libraries if security issues arise. Potential for conflicts if the user also has system Qt libraries.

3.  **Modern Packaging Formats (AppImage, Flatpak, Snap):**
    *   These formats aim to create truly universal Linux packages by bundling all dependencies (including Qt) into a single package that runs in a sandboxed environment.
    *   **AppImage**: Creates a single executable file. Relatively easy to get started with.
    *   **Flatpak/Snap**: More integrated with desktop environments and software stores.
    *   **Pros**: Highly portable, dependencies are self-contained, better isolation.
    *   **Cons**: Steeper learning curve to create these packages. Packages can be larger.

Choose the Linux deployment strategy that best fits your needs and your target audience's technical capabilities. For general distribution, relying on system Qt with good documentation or providing an AppImage are often good starting points.

This Docker setup provides a self-contained environment for building and testing your Qt application and its tests, ensuring consistency across different machines. 