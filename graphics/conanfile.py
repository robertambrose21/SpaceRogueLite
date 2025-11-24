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
        self.requires("sdl_image/3.2.4", transitive_headers=True)
        self.requires("sdl_ttf/3.2.2", transitive_headers=True)
        # Override mpg123 to use newer version with GCC 15+ fixes
        self.requires("mpg123/1.33.0", override=True)

        self.requires("imgui/1.92.4", transitive_headers=True)

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

        # Copy ImGui backends to include/backends directory
        import shutil
        from pathlib import Path

        imgui_dep = self.dependencies["imgui"]
        imgui_res_dir = Path(imgui_dep.package_folder) / "res" / "bindings"
        backends_dir = Path(self.source_folder) / "include" / "backends"

        # Create backends directory if it doesn't exist
        backends_dir.mkdir(parents=True, exist_ok=True)

        backends = [
            "imgui_impl_sdl3.h",
            "imgui_impl_sdl3.cpp",
            "imgui_impl_sdlgpu3.h",
            "imgui_impl_sdlgpu3.cpp",
            "imgui_impl_sdlgpu3_shaders.h",
        ]

        for backend in backends:
            src_file = imgui_res_dir / backend
            dst_file = backends_dir / backend
            if src_file.exists():
                shutil.copy2(src_file, dst_file)
                self.output.info(f"Copied {backend} to include/backends/")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["graphics"]
