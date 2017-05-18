# QLForth 

QLForth is a Forth working environment implemented in C language. Its goal is to provide engineers with a SOC verification, testing and firmware development platform using Forth as a Domain Specific Language (DSL). With its The natural characteristics of simple, easy to learn and full-featured, QLForth can be used immediately for embedded processor development and testing, and can be used with personal creative by C language extension.

Forth language is invented by Charles Moore in the 1960s. It is based on the stack, interactive with a simple philosophy of computer programming language and work mode and has been widely used in embedded systems and firmware development. Forth alternated zero Operators stack-based computer architecture is the main preferred structure of today's non-von Norman computer systems.

QLForth only requires the user to understand some basic principles of Forth and work mode, not to become a Forth expert first. QLForth is based on F83 model, with C language to achieve the core, extended vocabulary and cross compiler tool. QLForth implements 32-bit floating point operations. The interpreter directly identifies floating-point constants, internal shared data stacks and standard stack operations, supporting floating point operations during compile.

The QLForth cross compiler generates an internal custom-format simple syntax tree, SST, which can be used to optimize the program and generate object code based on the optimized SST. For a different architecture, users only need to change the target code generator and the simulator. The standard release version already contains a simulator for FPGA J1 (http://www.excamera.com/sphinx/fpga-j1.html), this can be used for experience and as an example for architecture simulation.

QLForth supports interactive testing with Forth mode. The actual project consists of a single source code, which can be edited using any text editor tool and loaded at start-up. Then the user can edit a small text file, each contains only several colon definitions, repeatedly load and compile the file and test these words. QLForth's command-line interaction simulates or actually executes the Forth words, automatically synchronizes the contents of the QLForth system and the target processor's data stack. After each word execution, the value of top of stack will be display automatically.

QLForth is compatible with Windows, Linux, Mac OS X 32/64 bits system. The C source code has been compiled by VS 2015 and Tiny CC (Windows), GCC (Linux) and LLVM (Mac OS X) compiler. Users can edit the Forth program with standard editor on these platforms and simulate or actual execution the target code.

The QLForth is on GitHub, please visit https://github.com/forthchina/QLForth

# Run

The 'bin' subdirectory of GitHub QLForth project is the latest version release of the Windows 7+ 32-bit application, which is compiled by the MS VS2015 Community Edition. The EXE file can be executed directly on Windows terminal.

# Source Compilation

- Download the latest version of the source code

>   Git clone https://github.com/forthchina/QLForth QLForth

- Windows MSVS project files are located in the Windows subdirectory. On systems with VS2015 is installed, double-click the QLForth.sln file to perform the compilation operation.

- Windows platform can use the Tiny CC to compile, in the installation of Tiny CC Windows environment, double-click the Windows directory MakeByTinyCC.bat file to complete the compilation and operation.

- Linux users will enter the 'Linux' subdirectory, use 'make' to complete the compilation. We compile and run the QLForth-Linux under VirtualBox + Cent OS 7. The target file is in Linux/bin/ sub-directory with the name - QLForth.

- The QLForth for Mac OS X platform is compiled with xcode, and the project files are located in the Mac subdirectory.
          
# About Naming
QL is the abbreviation of Qinling (Qinling Mountains). Qinling provides a natural boundary between North and South China and support a huge variety of plant and wildlife, some of which is found nowhere else on Earth.

----------


# QLForth  [**in Simple Chinese**, 简体中文] 

QLForth 是用 C语言实现的 Forth 固件工作环境，它的目标是为工程师提供一个使用 Forth 作为领域专用语言（DSL）的SOC处理器验证、测试和固件开发平台。 QLForth 结构简单、功能完整、能够立即用于嵌入式处理器开发和测试，同时可根据个人创意使用C 语言进行扩展。

Forth语言是 Charles Moore 在20世纪60年代发明的基于堆栈、交互式、具有简单性哲学思想的计算机编程语言和工作模式，曾被广泛地应用于嵌入式系统和固件开发，与 Forth 语言互生的零操作数基于堆栈的计算机体系结构是当今非冯诺曼计算机体系的主要优选结构。

QLForth 只要求使用者理解基本的Forth 原理和工作方式而不需要成为 Forth专家。QLForth 核心以F83语法为蓝本，用C 语言实现了核心、扩展字汇和目标处理器交叉编译工具。QLForth 实现32位浮点操作，解释器直接识别浮点数常量，内部共享数据堆栈和标准堆栈操作，支持单独的浮点操作命令。

QLForth 核心编译器生成内部自定义格式简单语法树 SST，后续的代码生成器能够对STT进行优化并根据优化后的 SST 生成目标代码。对于不同的体系结构用户创意，只需要变更目标代码生成器和模拟器才是用户需要变更的部分。标准发行版本已经包含有 FPGA J1 （http://www.excamera.com/sphinx/fpga-j1.html）的模拟器。

QLForth 支持Forth 工作模式的交互式测试。QLForth 实际项目由单一的源代码文本组成，这个文本可以使用任何文本编辑工具进行编辑并在启动时一次装入进行调试。接着用户就可以每次编辑一个短小的文本文件，反复装入这个小文件，调试其中的定义。QLForth 的命令行交互以Forth字为单位模拟或者实际运行调试，自动同步 QLForth 系统和目标处理器数据堆栈内容，且自动显示栈顶元素。

QLForth 与目前流行的主机操作系统兼容，C 语言源代码能够在 Windows、Linux、Mac OS X 32/64 位系统上编译运行，用户可借助这些平台上的标准程序编辑工具编辑 Forth 源程序并模拟或者连接目标芯片实际执行。

QLForth 项目驻留在 GitHub 上，请访问 https://github.com/forthchina/QLForth

# 运行

GitHub QLForth 项目的 bin 子目录为最新版本的 Windows 7+ 32 位应用程序，该文件现在由 MS VS2015 社区版编译。在 Windows 终端可直接执行这个 EXE 文件。

# 源码编译

- 下载源代码的最新版本

	git clone https://github.com/forthchina/QLForth QLForth

- Windows MSVS 工程文件位于 Windows 子目录中，在安装有VS2015 的系统中，双击 QLForth.sln 文件执行编译操作。

- Windows 平台能够使用 Tiny CC 工具编译，在安装好 Tiny CC 的 Windows 环境中，双击 Windows 目录下的 MakeByTinyCC.bat 文件即可完成编译和运行。

- Linux 用户进入 Linux 子目录，使用  make 即可完成编译， 我们在 VirtualBox + Cent OS 7 下编译运行。目标文件 为 Linux/bin/QLForth。

- Mac OS X 平台使用 xcode 进行编译，项目工程文件位于 Mac 子目录。

# 命名
QL是秦岭（汉语拼音QinLing）的缩写。秦岭是长江流域与黄河流域的一个分水岭，生长着一些世界独有的野生动植物。

# 更多
中文用户可以通过微信公众号以获取更多的信息。
