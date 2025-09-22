## Installation

From the root directory run the following:

```bash
scripts/install.sh # An optional --clean flag may be included. WARNING: This will currently wipe all local conan packages
scripts/build_packages.sh --build-type=[Debug|Release] # These can also be run as a VSCode task
scripts/build.sh
```

**Notes:**

- VSCode may need to be restarted after performing a clean install
- When a new header-only package is added, it's directory should be added to the `HEADER_PROJECTS` list in `install.sh` to allow the install script to continue to function correctly

## Intellisense

#### settings.json

The `settings.json` file under `.vscode` performs the majority of the intellisense for the project. When the project is built, it will generate a top-level `compile_commands.json` file which is a merged copy of all packe `compile_commands.json` files. You should point at this top-level file instead of any others. An of the cmake configuration for `settings.json` is as follows:

```JSON
"C_Cpp.default.compileCommands": "${workspaceFolder}/compile_commands.json",
  "cmake.configureSettings": {
    "CMAKE_TOOLCHAIN_FILE": "${workspaceFolder}/build/release/conan_toolchain.cmake",
    "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
  },
  "cmake.buildDirectory": "${workspaceFolder}/build/release",
```

#### Troubleshooting

- Try running `C/C++: Reset Intellisense Database` and restarting VSCode if intellisense does not work on first try
