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
