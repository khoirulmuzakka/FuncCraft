from skbuild import setup
from pathlib import Path
import shutil


def _remove_stale_package_builds():
    for path in Path("_skbuild").glob("*/setuptools/lib*/funccraft"):
        shutil.rmtree(path, ignore_errors=True)
    for path in Path("_skbuild").glob("*/setuptools/bdist*/wheel/funccraft"):
        shutil.rmtree(path, ignore_errors=True)


def _filter_wheel_manifest(files):
    """Keep wheels limited to the importable Python package.

    MANIFEST.in is intentionally broader because source distributions need the
    C++ sources and CMake files to build from source. Wheels should only contain
    installed package files produced under funccraft/.
    """
    return [path for path in files if path.startswith("funccraft/")]


_remove_stale_package_builds()

setup(
    include_package_data=False,
    packages=["funccraft"],
    cmake_args=[
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_CXX_STANDARD=17",
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
        "-DBUILD_PYTHON=ON",
    ],
    cmake_process_manifest_hook=_filter_wheel_manifest,
)
