from conan import ConanFile
from conan.tools.scm import Git
from conan.tools.files import copy
from pathlib import Path
import shutil, os

class YojimboConan(ConanFile):
    name = "yojimbo"
    version = "v1.2.5"
    license = "BSD-3-Clause"
    url = "https://github.com/mas-bandwidth/yojimbo"
    description = "A network library for client/server games"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "premake5.lua"

    def source(self):
        src = Path(self.source_folder)

        # Clean the source folder first
        for child in src.iterdir():
            if child.is_file():
                child.unlink()
            else:
                shutil.rmtree(child)

        git = Git(self)
        git.clone("https://github.com/mas-bandwidth/yojimbo.git", target=str(src))
        git.checkout(self.version)

    def requirements(self):
        # Core dependencies
        self.requires("libsodium/1.0.20")
        self.requires("mbedtls/3.6.4")

    def build(self):
         # Pick the premake generator depending on platform
        if self.settings.os == "Windows":
            premake_cmd = "premake5 vs2022"
        else:
            premake_cmd = "premake5 gmake"

        self.run(premake_cmd, cwd=self.source_folder)
        self.run(f"make -j", cwd=self.source_folder)

    def package(self):
        pkg_include = os.path.join(self.package_folder, "include")
        pkg_lib = os.path.join(self.package_folder, "lib")
        pkg_bin = os.path.join(self.package_folder, "bin")

        os.makedirs(pkg_include, exist_ok=True)
        os.makedirs(pkg_lib, exist_ok=True)
        os.makedirs(pkg_bin, exist_ok=True)

        # Copy top-level headers (yojimbo.h, serialize.h, etc.)
        copy(self, "*.h",
            src=self.source_folder,
            dst=pkg_include,
            keep_path=False)

        # Copy headers from "include/" subfolder
        copy(self, "*.h",
            src=os.path.join(self.source_folder, "include"),
            dst=pkg_include,
            keep_path=True)

        build_dir = os.path.join(self.source_folder, "bin")
        copy(self, "*.a", src=build_dir, dst=pkg_lib, keep_path=False)
        copy(self, "*.so", src=build_dir, dst=pkg_lib, keep_path=False)
        copy(self, "*.dll", src=build_dir, dst=pkg_bin, keep_path=False)

        copy(self, "libtlsf.a", src=os.path.join(self.build_folder, "bin"), dst=pkg_lib, keep_path=False)

    def package_info(self):
        self.cpp_info.requires = ["libsodium::libsodium", "mbedtls::mbedtls"]
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libs = ["yojimbo", "netcode", "reliable", "tlsf", "sodium-builtin"]
