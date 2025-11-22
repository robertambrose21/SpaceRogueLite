from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class graphicsRecipe(ConanFile):
    name = "graphics"
    version = "1.0.0-prealpha"
    package_type = "library"

    # Optional metadata
    license = "<Put the package license here>"
    author = "robertambrose21@gmail.com"
    url = "https://github.com/robertambrose21/SpaceRogueLite"
    description = "SpaceRogueLite graphics library"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*", "include/*"

    def requirements(self):
        self.requires("sdl/3.2.20")
        self.requires("sdl_image/3.2.4")
        self.requires("sdl_ttf/3.2.2")
        # Override mpg123 to use newer version with GCC 15+ fixes
        self.requires("mpg123/1.33.0", override=True)

        self.requires("core/1.0.0-prealpha")

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        # Disable OpenGLES to avoid missing GLES headers issue
        self.options["sdl"].opengles = False
        # Disable Wayland to avoid libdecor linking issues
        self.options["sdl"].wayland = False

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["graphics"]
