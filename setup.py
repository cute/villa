from distutils.core import setup, Extension

include_dirs = []
library_dirs = []
libraries = []
runtime_library_dirs = []
extra_objects = []
define_macros = []

setup(name = "villa",
      version = "0.1",
      author = "Li Guangming",
      license = "MIT",
      url = "https://github.com/cute/villa",
      packages = ["villa"],
      ext_package = "villa",
      ext_modules = [Extension( name = "villa",
                                sources = [
                                "src/pyvilla.c",
                                "src/depot.c",
                                "src/cabin.c",
                                "src/myconf.c",
                                "src/villa.c",
                                ],
                                include_dirs = include_dirs,
                                library_dirs = library_dirs,
                                runtime_library_dirs = runtime_library_dirs,
                                libraries = libraries,
                                extra_objects = extra_objects,
                                define_macros = define_macros
                              )],
      )

