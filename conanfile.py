from conans import ConanFile, CMake, tools


class SteamControllerConan(ConanFile):
    name = "steam_controller"
    version = "0.1"
    license = "MIT"
    author = "Marius Elvert marius.elvert@googlemail.com"
    url = "https://github.com/ltjax/steam_controller"
    description = "Access the Steam Controller without Steam"
    topics = ("hardware", "gamepad")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = "cmake"

    def _configured_cmake(self):
        cmake = CMake(self)
        cmake.configure(source_folder=".")
        return cmake

    def build(self):
        self._configured_cmake().build()

    def package(self):
        self._configured_cmake().install()

    def package_info(self):
        self.cpp_info.libs = ["steam_controller"]

