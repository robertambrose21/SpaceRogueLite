from conan import ConanFile
from conan.tools.scm import Git
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy
import os
import glob


class SDLShadercrossConan(ConanFile):
    name = "sdl_shadercross"
    version = "main"
    license = "Zlib"
    url = "https://github.com/libsdl-org/SDL_shadercross"
    description = "Shader translation library for SDL's GPU API"
    settings = "os", "compiler", "build_type", "arch"

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        self.requires("sdl/3.2.20")
        # Override mpg123 to use newer version with GCC 15+ fixes
        self.requires("mpg123/1.33.0", override=True)

    def source(self):
        git = Git(self)
        git.clone("https://github.com/libsdl-org/SDL_shadercross.git",
                  target=str(self.source_folder),
                  args=["--recurse-submodules"])
        # Unfortunately, SDL_shadercross has no tags/branches - pin to a commit hash instead!
        git.checkout("3e572c3219ea438bff849cebea34f3aad7e1859b")

    def configure(self):
        # Disable OpenGLES to avoid missing GLES headers issue
        self.options["sdl"].opengles = False
        # Disable Wayland to avoid libdecor linking issues
        self.options["sdl"].wayland = False

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.variables["SDLSHADERCROSS_VENDORED"] = True
        tc.variables["SDLSHADERCROSS_CLI"] = True
        tc.variables["SDLSHADERCROSS_SHARED"] = False
        tc.variables["SDLSHADERCROSS_STATIC"] = True
        tc.variables["SDLSHADERCROSS_SPIRVCROSS_SHARED"] = False
        tc.variables["SDLSHADERCROSS_DXC"] = True
        tc.variables["SDLSHADERCROSS_INSTALL"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

        # Copy DXC shared libraries from build directory to package lib folder
        # DXC libraries are built in the vendored external directory
        build_folder = self.build_folder
        pkg_lib = os.path.join(self.package_folder, "lib")
        os.makedirs(pkg_lib, exist_ok=True)

        # Find and copy libdxcompiler.so* from the build tree
        for pattern in ["**/libdxcompiler.so*", "**/libdxil.so*"]:
            for lib_file in glob.glob(os.path.join(build_folder, pattern), recursive=True):
                if os.path.isfile(lib_file):
                    copy(self, os.path.basename(lib_file),
                         src=os.path.dirname(lib_file),
                         dst=pkg_lib)

    def package_info(self):
        bin_path = os.path.join(self.package_folder, "bin")
        lib_path = os.path.join(self.package_folder, "lib")

        # Set for build environment (when used as tool_requires)
        self.buildenv_info.prepend_path("PATH", bin_path)
        self.buildenv_info.prepend_path("LD_LIBRARY_PATH", lib_path)

        # Also set for runtime environment (for test package and dependencies)
        self.runenv_info.prepend_path("PATH", bin_path)
        self.runenv_info.prepend_path("LD_LIBRARY_PATH", lib_path)
