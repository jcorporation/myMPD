add_library(sds "")

target_sources(sds
  PRIVATE
    sds.c
    sdsalloc.h
  PUBLIC
    sds.h
)

target_compile_options(sds
  PRIVATE
    "-Wno-format-nonliteral"
    "-Wno-conversion"
)
