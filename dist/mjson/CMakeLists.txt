add_library(mjson "")

target_sources(mjson
  PRIVATE
    mjson.c
  PUBLIC
    mjson.h
)

target_compile_options(mjson
  PRIVATE
    "-Wno-unused-but-set-variable"
    "-DMJSON_ENABLE_PRINT=0"
    "-DMJSON_ENABLE_BASE64=0"
    "-DMJSON_ENABLE_RPC=0"
    "-DMJSON_ENABLE_PRETTY=0"
    "-DMJSON_ENABLE_MERGE=0"
)
