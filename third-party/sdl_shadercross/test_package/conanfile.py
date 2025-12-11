from conan import ConanFile
from conan.tools.build import can_run


class SDLShadercrossTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires(self.tested_reference_str)

    def test(self):
        if can_run(self):
            # Use conanrun environment which now includes both PATH and LD_LIBRARY_PATH
            self.run("shadercross --help", env="conanrun")
