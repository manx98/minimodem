from os.path import join

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, get, replace_in_file, rmdir


# 类名这个可以随意更改
class MiniModemRecipe(ConanFile):
    # 包名这个需要保证全小写
    name = "mini_modem"
    # 版本号（这个可以删除，然后在构建的时候--version传进去）
    version = "0.24-1"
    # 包类型（可选）
    # application：包是一个应用程序。
    # library：包是一个通用库。
    # 更多请参考 https://docs.conan.io/2/reference/conanfile/attributes.html#package-type
    package_type = "application"
    # 元数据信息（可选）
    license = "<Put the package license here>"
    author = "kamal@whence.com"
    url = "http://www.whence.com/minimodem"
    description = "general-purpose software audio FSK modem"
    topics = ("audio", "FSK", "modem")
    # settings里面的配置是全局配置
    # 通常是通过profile进行配置,保存系统环境信息
    settings = "os", "compiler", "build_type", "arch"
    # options 当前包的配置(非全局)
    # shared 这个选项是必须配置的，其它选项可以自由添加
    # 结构为 "选项名": [可选值...], 可选值有个特殊的值 "ANY" 表示任意值
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_alsa": [True, False],
        "with_sendfile": [True, False],
        "with_pulseaudio": [True, False],
        "with_benchmarks": [True, False],
    }
    # default_options 配置options的默认值
    # 这里也能设置依赖包的配置值,可以使用*来表示通配符
    # "包名/版本:配置名": 配置值
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_alsa": True,
        "with_sendfile": True,
        "with_pulseaudio": False,
        "with_benchmarks": True,
    }

    # config_options 方法用于在为包中的可用选项分配值之前对其进行配置或约束。典型的用例是删除给定平台中的选项。例如fPIC标志在Windows中不存在，因此应在此方法中将其删除
    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")
            self.options.rm_safe("with_alsa")
            self.options.rm_safe("with_sendfile")
            self.options.rm_safe("with_pulseaudio")

    # configure 方法应用于配方中的settings和options，以便稍后在generate()、build()或package()方法在构建依赖图并扩展包依赖项的同时执行。
    # 这意味着当该方法执行时，依赖项仍然不存在，它们不存在，并且无法访问self.dependencies
    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.settings.rm_safe("compiler.libcxx")

    # 在 layout()方法中，您可以调整 self.folders 和 self.cpp
    # 详细用法请参考 https://docs.conan.io/2/reference/conanfile/methods/layout.html
    # 这里直接使用cmake_layout加载cmake项目布局，如果CMakeList.txt文件和conanfile.py不在同一级,你需要传入src_folder（默认当前目录）指向其所在的目录
    def layout(self):
        cmake_layout(self, src_folder=".")

    def requirements(self):
        self.requires("fftw/3.3.10", headers=True, libs=True)
        if self.settings.os == "Linux":
            if self.options.with_alsa:
                self.requires("libalsa/1.2.10", headers=True, libs=True)
            if self.options.with_pulseaudio:
                self.requires("pulseaudio/17.0", headers=True, libs=True)
            if self.options.with_sendfile:
                self.requires("libsndfile/1.2.2", headers=True, libs=True)

    # generate 方法将在依赖图的计算和安装之后运行。这意味着它将在 conan install 命令之后运行，或者当在缓存中构建包时，它将在调用 build() 方法之前运行。
    def generate(self):
        # CMakeDeps生成器为每个依赖项生成必要的文件，以便能够使用cmake的find_package() 函数来查找依赖项。
        deps = CMakeDeps(self)
        deps.generate()
        # 覆盖构建环境中已设置的环境变量的值
        buildenv = VirtualBuildEnv(self)
        buildenv.generate()
        # CMakeToolchain是CMake 的工具链生成器。它生成的工具链文件可用于 CMake 的命令行调用-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake。
        # 此生成器将当前的包配置、设置和选项转换为 CMake 工具链语法。
        tc = CMakeToolchain(self)
        if self.settings.os == "Linux":
            tc.cache_variables["USE_ALSA"] = True if self.options.with_alsa else False
            tc.cache_variables["USE_PULSEAUDIO"] = True if self.options.with_pulseaudio else False
            tc.cache_variables["USE_SNDFILE"] = True if self.options.with_sendfile else False
        tc.cache_variables["USE_BENCHMARKS"] = True if self.options.with_benchmarks else False
        tc.cache_variables["USE_SNDIO"] = False
        # 应用上面设置的环境变量
        tc.presets_build_environment = buildenv.environment()
        tc.generate()

    # build() 方法用于定义从包的源代码构建。实际上，这意味着调用一些构建系统，这可以显式完成或使用 Conan 提供的任何构建助手：
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
