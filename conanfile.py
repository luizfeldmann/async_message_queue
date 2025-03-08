from conan import ConanFile
from conan.tools.files import load
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
import os, shutil, json

class Recipe(ConanFile):
    # Binary configuration
    package_type = "header-library"
    settings = "os", "compiler", "build_type", "arch"

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports = "package.json"
    exports_sources = [
        "README.md",
        "package.json",
        "CMakeLists.txt",
        "include/*",
        "tests/*" ]

    @property
    def package_json(self) -> dict:
        return json.loads(load(self, os.path.join(self.recipe_folder, "package.json")))

    def set_name(self):
        self.name = self.package_json["name"].lower()

    def set_version(self):
        self.version = self.package_json["version"]

    def requirements(self):
        self.requires("boost/1.81.0")

    def build_requirements(self):
        self.test_requires("gtest/[^1.14.0]")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        # Import dependencies
        deps = CMakeDeps(self)
        deps.generate()

        # Generate toolchain with conan configurations for VS
        tc = CMakeToolchain(self, generator="Ninja Multi-Config")

        # Do not generate user presets due to unsupported schema in VS2019
        tc.user_presets_path = None

        # Support older versions of the JSON schema
        tc.cache_variables["CMAKE_TOOLCHAIN_FILE"] = os.path.join(self.generators_folder, tc.filename)
        tc.cache_variables["CMAKE_INSTALL_PREFIX"] = "${sourceDir}/install"

        # Generate the CMake
        tc.generate()

        # Link the generated presets to the root
        presets_gen = os.path.join(self.generators_folder, "CMakePresets.json")
        presets_usr = os.path.join(self.source_folder, "CMakeUserPresets.json")

        shutil.copyfile(src=presets_gen, dst=presets_usr)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_id(self):
        self.info.clear()

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
