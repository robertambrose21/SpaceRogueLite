from conan import ConanFile
from conan.tools.scm import Git
from conan.tools.files import copy
from conan.tools.cmake import cmake_layout, CMake
import os

class FastWFCConan(ConanFile):
    name = "fast-wfc"
    version = "1.0.0"
    license = "MIT"
    url = "https://github.com/math-fehr/fast-wfc"
    description = "Fast Wave Function Collapse algorithm implementation"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def layout(self):
        cmake_layout(self, src_folder="src")

    def source(self):
        git = Git(self)
        git.clone("git@github.com:math-fehr/fast-wfc.git", target=str(self.source_folder))
        git.folder = str(self.source_folder)
        git.checkout("0edfccf8f354da79b00ebfea7b1dbee5271e9c1f")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        pkg_include = os.path.join(self.package_folder, "include")
        pkg_lib = os.path.join(self.package_folder, "lib")

        os.makedirs(pkg_include, exist_ok=True)
        os.makedirs(pkg_lib, exist_ok=True)

        # Copy headers from src/include/
        copy(self, "*.hpp",
            src=os.path.join(self.source_folder, "src", "include"),
            dst=pkg_include,
            keep_path=True)

        # Copy built libraries
        copy(self, "*.a", src=self.build_folder, dst=pkg_lib, keep_path=False)
        copy(self, "*.so", src=self.build_folder, dst=pkg_lib, keep_path=False)

    def package_info(self):
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libs = ["fastwfc_static"]
