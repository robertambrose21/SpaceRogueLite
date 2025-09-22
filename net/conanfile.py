from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy
import os

class netRecipe(ConanFile):
    name = "net"
    version = "1.0.0-prealpha"
    package_type = "header-library"

    license = "<Put the package license here>"
    author = "robertambrose21@gmail.com"
    url = "https://github.com/robertambrose21/SpaceRogueLite"
    description = "SpaceRogueLite core library"

    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "include/*", "dummy.cpp", "CMakeLists.txt"
    no_copy_source = True

    def requirements(self):
        self.requires("yojimbo/v1.2.5")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        CMakeToolchain(self).generate()
        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "*.h", src=os.path.join(self.source_folder, "include"),
             dst=os.path.join(self.package_folder, "include"))

    def package_info(self):
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []

