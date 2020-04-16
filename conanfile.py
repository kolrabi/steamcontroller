from conans import ConanFile, CMake


class SteamControllerConan(ConanFile):
    name = "steam_controller"
    version = "1.1"
    license = "MIT"
    author = "Marius Elvert marius.elvert@googlemail.com"
    url = "https://github.com/ltjax/steam_controller"
    description = "Access the Steam Controller without Steam"
    topics = ("hardware", "gamepad")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = "cmake"
    exports_sources = "source/*", "externals/*",\
                      "!externals/hidapi/LICENSE-*.txt",\
                      "include/*", "CMakeLists.txt",\
                      "demo/*"

    def _configured_cmake(self):
        cmake = CMake(self)
        cmake.configure(source_folder=".", defs={
            'steam_controller_INCLUDE_CONAN': True
        })
        return cmake

    def build(self):
        self._configured_cmake().build()

    def package(self):
        self._configured_cmake().install()

    def package_info(self):
        self.cpp_info.libs = ["steam_controller"]
        if self.settings.os == "Windows":
            self.cpp_info.libs.append("Setupapi")
        if self.settings.os == "Linux":
            self.cpp_info.libs.append("udev")
        if self.settings.os == "Macos":
            self.cpp_info.exelinkflags.append("-framework CoreFoundation")
            self.cpp_info.exelinkflags.append("-framework IOKit")


