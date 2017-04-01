# QLForth 

QLForth is a Forth environment implemented fully using C language and runs on Windows PC. It provides a simple, full-functional ready-to-use tool and an easy-to-adapted framework for creative System-On-Chip (SOC) verification, testing and firmware development.

Forth is invented by Charles Moore in 1960s. It has been widely used in embedded systems and firmware development. Forth is the inspiration source for many well-known non-Von Norman computers and the only available firmware development tool for these hardware.

QLForth takes Forth as the Domain Specific Language (DSL) for firmware development of Smart IOT Node and artificial intelligence. Users need only understand the Forth principle and work mode, no need to become an expert-level Forther. The C language is enough customizing the personalized architecture simulation and analysis validation toolkit at the top level.

QLForth consists of three main parts: the Core, the Target and the User Interpreter.

+ The QLForth core takes F83 model with C Function Thread. The entire QLForth interpreter execute as an independent Windows thread. All the core and extended vocabulary words are implemented by build-in C function, no meta-compile process is needed. QLForth supports traditional dictionary and vocabulary with hash searching algorithm. QLForth provides a lot of floating point words to support floating point operations. The interpreter identifies floating point constants, the 32-bits float data shares data stack with integer CELLs. 

+ The target includes cross-compiler, object code generator / simulator and the USB HID hardware connection. All cross-compile words are implemented using C functions and placed into a dedicated Forth vocabulary as immediate word. The user needs not distinguish between host and target operations specifically. The target compiler always generates an internal simple syntax tree, SST, for subsequent optimization. The object code generator will perform code generation based on the optimized SST for simulator or hardware processor. For creative architecture developer, the generator and simulator is the only parts needing to be modified.

+ QLForth implements a source code editor, an interactive interpreter and TTY-styled text output window. For every QLForth application, only one source file is necessary. The huge file or library can be divided into multiple small text files and be imported by the LOAD command to the project source. By the great Scintilla Control, the source code editor has achieved syntax highlighting, font scaling and in-line error prompts. In interpret window, the space-bar-separated Forth word can be typed and executed one-by-one with topmost data stack item displayed automatically. QLForth implements Forth's philosophical incremental compilation. The modified region will be automatically interpreted as the source code editor recognizes that this area is after a particular separation point, 

QLForth is built with VS2015 Community, only C and Windows SDK features has been used. No need for C++ and .Net support. 

# QLForth Startup
## Running the executable directly 
+ Download the executable file from the release folder. This folder contains some older date of QLForth. We shall regularly updates the release, but does not guarantee the synchronization between the execute and the latest source code.

## Build from Source
+   Download the latest version of the source code by

>         git clone https://github.com/forthchina/QLForth QLForth

+   Go to QLForth folder, double-click the file QLForth.sln on a computer with the VS2015 community version installed then perform the rebuild operation.

## Running QLForth
+   The latest QLForth has been tested on 32 bits Windows 7+ PC. 
+   QLForth is compiled using 32-bits mode in each iteration, and the execute release is able to run on 64-bit Windows.
+   The editor control SciLexer.dll is already included in the release and can be downloaded from https://www.scintilla.org/
          
# About Naming
QL is the abbreviation of Qinling (Qinling Mountains). Qinling provides a natural boundary between North and South China and support a huge variety of plant and wildlife, some of which is found nowhere else on Earth.

[in Simple Chinese]

# QLForth  [**in Simple Chinese**, 简体中文] 

QLForth 是完全用 C语言实现的、运行在Windows PC 上的Forth语言工作环境，它的目标是为工程师提供一个结构简单、功能完整并立即可用的嵌入式处理器开发和测试工具和一个可根据个人创意进行高层扩展的平台框架，用以支持 SOC处理器验证、测试和固件开发过程。

Forth语言是 Charles Moore 在20世纪60年代发明的基于堆栈、交互式、具有简单性哲学思想的计算机编程语言和工作模式，曾被广泛地应用于嵌入式系统和固件开发，与 Forth 语言互生的零操作数基于堆栈的计算机体系结构是当今非冯诺曼计算机体系的主要优选结构。

QLForth 把 Forth 定位成为智能物联网结点和人工智能固件开发的领域专用语言（DSL），使用者只需要理解Forth 原理和工作方式而不需要成为 Forth专家。C语言是实现高层次个人化定制体系结构、平台模拟和性能分析验证的所需要的基本工具。

QLForth 由核心、目标支持和用户交互三个主要部分组成。

+ QLForth 核心是运行在 PC 平台上的 Forth 内核，以F83语法为蓝本，用C 语言实现全部核心和扩展字汇。QLForth 支持传统的字典和字汇支操作，并实现基于字汇支的字散列查找算法。QLForth 实现32位浮点操作，解释器直接识别浮点数常量，内部共享数据堆栈和标准堆栈操作，支持单独的浮点操作命令。QLForth核心使用C函数串线解释方式， 整个Forth 解释器使用一个 Windows 独立线程运行，减少了对其它部分的干扰。

+ QLForth 的目标处理器支持模块包括交叉编译器、目标代码生成和模拟执行器以及硬件USB HID接口模块。所有的目标交叉编译字使用 C 函数实现并放置在一个专门的Forth 字汇支中作为立即字执行，用户不需要特别区分主机和目标操作。QLForth 目标编译器首先生成内部自定义格式简单语法树 SST 以方便后续优化，目标代码生成器根据优化后的 SST 生成代码，提供给模拟器执行或者下载到硬件处理器上执行。对于不同的体系结构创意，只有目标代码生成器和模拟器才是用户需要变更的部分。

+ QLForth 实现了满足Forth 工作方法和软件工程要求的源代码编辑、交互命令行界面和TTY 风格的文本输出窗口。QLForth 的每个应用只需要一个独立的源文件，巨大的文本文件或者库文件可以分隔成多个小文件并使用 LOAD命令合并引入。使用 Scintilla 控件的编辑器实现了语法加亮、字体缩放和内嵌式的语法错误提示。交互式命令窗口支持Forth单字调试并自动显示数据堆栈上前两个单元的内容。QLForth 实现源于Forth 哲学思想的增量编译，当源代码编辑模块识别用户在一个特别的分隔点之后编辑文本时，将自动针对这个区域的文本进行解释。

QLForth 使用 C 和 Windows SDK 实现，不需要特别的 .NET库，也没有使用 C++ 语言。

# QLForth 运行
## 直接运行可执行文件
   在 release 目录下载可执行文件。我们将定期更新发行版本，但并不保证发行的程序与最新的源代码同步。

## 源码编译
   下载源代码的最新版本

>    git clone https://github.com/forthchina/QLForth QLForth

   在安装有 VS2015 社区版本的电脑上，进入 QLForth 子目录，双击 QLForth.sln 文件，接着执行编译操作。

## 运行
+ QLForth 在32位 Windows 7+ 平台上开发测试。
+ QLForth 在每次开发测试迭代过程中使用 32位模式编译，但其发行程序经测试可以运行在 64位 Windows 上。
+ 编辑器控件 SciLexer.dll 已经包含在Release 发行中，用户可以从 www.scintilla.org/ 下载
          
# 命名
QL是秦岭（Qin Ling）的缩写。秦岭是长江流域与黄河流域的一个分水岭，生长着一些世界独有的野生动植物。

# 更多
  中文用户可以通过微信公众号以获取更多的信息。

