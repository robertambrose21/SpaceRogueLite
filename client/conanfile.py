from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from pathlib import Path


class clientRecipe(ConanFile):
    name = "client"
    version = "1.0.0-prealpha"
    package_type = "application"

    # Optional metadata
    license = "<Put the package license here>"
    author = "robertambrose21@gmail.com"
    url = "https://github.com/robertambrose21/SpaceRogueLite"
    description = "SpaceRogueLite server application"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*", "../scripts/merge_compile_commands.py"

    def requirements(self):
        self.requires("yojimbo/v1.2.5")
        self.requires("net/1.0.0-prealpha")
        self.requires("core/1.0.0-prealpha")

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

        workspace_root = Path(self.source_folder).parent.resolve()
        script_path = workspace_root / "scripts" / "merge_compile_commands.py"
        output_path = workspace_root / "compile_commands.json"

        if script_path.exists():
            self.run(f'python "{script_path}" --root "{workspace_root}" --output "{output_path}"', env=None)
        else:
            self.output.warning(f"Merge script not found at {script_path}")

    def package(self):
        cmake = CMake(self)
        cmake.install()

    

    
