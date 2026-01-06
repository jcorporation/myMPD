# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'myMPD'
copyright = '2018-2026, Juergen Mang'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx_copybutton',
]

templates_path = ['_templates']
exclude_patterns = ['_includes/*']

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_book_theme'
html_static_path = [
    'assets',
]
html_favicon = 'assets/favicon.ico'
html_css_files = [
    'mympd.css',
]
html_theme_options = {
    "repository_url": "https://github.com/jcorporation/myMPD",
    "repository_branch": "master",
    "path_to_docs": "docs/",
    "use_source_button": True,
    "use_repository_button": True,
    "home_page_in_toc": True,
    "logo": {
        "image_light": "assets/logo-light.svg",
        "image_dark": "assets/logo-dark.svg",
    },
}
