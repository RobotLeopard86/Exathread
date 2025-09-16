import os
from textwrap import dedent

version = os.environ.get("GITHUB_RELEASE", default="latest")
project = 'Exathread'
copyright = '2025 RobotLeopard86'
author = 'RobotLeopard86'
release = version

extensions = [
	'breathe',
	'exhale',
	'myst_parser'
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'README.md', '.venv']

html_theme = 'pydata_sphinx_theme'
html_title = "Exathread Documentation"
html_favicon = "../exathread_logo.png"
html_permalinks_icon = "<span/>"
html_use_index = False
html_domain_indices = False
html_copy_source = False
html_static_path = ["assets"]
html_css_files = ["fonts.css"]

breathe_projects = {
    "Exathread": "./.doxygen/xml"
}
breathe_default_project = "Exathread"

exhale_args = {
    "containmentFolder":     "./api",
    "rootFileName":          "index.rst",
    "doxygenStripFromPath":  "../",
    "rootFileTitle":         "API Reference",
    "createTreeView":        True,
    "exhaleExecutesDoxygen": True,
    "afterTitleDescription": "Welcome to the Exathread documentation. Here you can find comprehensive API information. Check out the API map below.",
    "exhaleDoxygenStdin": dedent('''
									INPUT = ../include/exathread.hpp
                                    EXCLUDE_SYMBOLS = std,exathread::details*,exathread::corowrap*
									HIDE_UNDOC_MEMBERS = YES
									MAX_INITIALIZER_LINES = 0
									''')
}

primary_domain = 'cpp'
highlight_language = 'cpp'

html_context = {
   "default_mode": "dark"
}

html_theme_options = {
    "logo": {
        "text": "Exathread Documentation",
        "image_light": html_favicon,
        "image_dark": html_favicon
    },
    "icon_links": [
        {
            "name": "GitHub",
            "url": "https://github.com/RobotLeopard86/Exathread",
            "icon": "fa-brands fa-github",
            "type": "fontawesome",
        }
   ],
   "navbar_start": ["navbar-logo", "version-switcher"],
   "switcher": {
        "version_match": version,
        "json_url": "https://raw.githubusercontent.com/RobotLeopard86/Exathread/main/docs/switcher.json"
    }
}
