import os
import sys
import warnings

sys.path.insert(0, os.path.abspath("../.."))

project = "FuncCraft"
copyright = "2026, Khoirul Faiq Muzakka"
author = "Khoirul Faiq Muzakka"
release = "0.1.0"

extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.mathjax",
    "sphinx.ext.viewcode",
    "breathe",
]

templates_path = ["_templates"]
exclude_patterns = ["_build", "**.ipynb_checkpoints"]

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]
autodoc_mock_imports = ["funccraft._funccraft"]

autodoc_default_options = {
    "members": True,
    "undoc-members": False,
    "private-members": False,
    "special-members": "__call__",
    "inherited-members": True,
    "show-inheritance": True,
}

breathe_projects = {
    "FuncCraft": "../xml",
}
breathe_default_project = "FuncCraft"

try:
    from sphinx.deprecation import RemovedInSphinx80Warning

    warnings.filterwarnings(
        "ignore",
        category=RemovedInSphinx80Warning,
        module=r"breathe\.project",
    )
except Exception:
    pass
