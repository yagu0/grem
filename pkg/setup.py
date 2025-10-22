from setuptools import setup, Extension, find_packages
from glob import glob
import pybind11

ext_modules = [
    Extension(
        "grem._native",
        sources=glob("../c_project/src/*.c") + ["../grem/wrapper.cpp"],
        include_dirs=[
            pybind11.get_include(),
            "../src"
        ],
        language="c++",
        extra_compile_args=["-O3", "-std=c++17"],
    )
]

setup(
    name="grem",
    version="0.1",
    packages=find_packages(where=".."),
    ext_modules=ext_modules,
    package_dir={"": ".."},
    package_data={"grem": ["*.so"]},
    include_package_data=True,
    zip_safe=False,
)
