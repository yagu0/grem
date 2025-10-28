from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
import subprocess, os, glob, pybind11

def needs_rebuild(lib_path, src_dir):
    if not os.path.exists(lib_path):
        return True
    lib_mtime = os.path.getmtime(lib_path)
    for src in glob.glob(os.path.join(src_dir, "*.c")):
        if os.path.getmtime(src) > lib_mtime:
            return True
    return False

# ChatGPT to the rescue...
class CustomBuildExt(build_ext):
    """Call 'make release' inside c_project before pybind11 module compilation."""
    def run(self):
        lib_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "../c_project/bin/libgrem.a"))
        cproj_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "../c_project"))

        if needs_rebuild(lib_path, "../c_project/src"):
            print(f"Compilation of C code in {cproj_dir} --> libgrem.a")
            subprocess.check_call(["make", "release"], cwd=cproj_dir)
        else:
            print(f"C library already up to date: {lib_path}")

        # Then normal build (C++ / pybind11)
        super().run()

ext_modules = [
    Extension(
        "grem._native",
        sources=["../grem/wrapper.cpp"],
        include_dirs=[
            pybind11.get_include(),
            "../c_project/src"
        ],
        language="c++",
        extra_compile_args=["-O3", "-std=c++17"],
        extra_objects=["../c_project/bin/libgrem.a"],
    )
]

setup(
    name="grem",
    version="0.1",
    packages=find_packages(where=".."),
    ext_modules=ext_modules,
    cmdclass={"build_ext": CustomBuildExt},
    package_dir={"": ".."},
    package_data={"grem": ["*.so"]},
    include_package_data=True,
    zip_safe=False,
)
