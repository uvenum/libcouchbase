ADD_LIBRARY(couchbase_select STATIC plugin-select.c)
ADD_DEFINITIONS(-DLIBCOUCHBASE_INTERNAL=1)
SET_TARGET_PROPERTIES(couchbase_select
    PROPERTIES
        COMPILE_FLAGS "${CMAKE_C_FLAGS} ${LCB_CORE_CFLAGS}")
INSTALL(
    FILES
        select_io_opts.h
    DESTINATION
        include/libcouchbase/)
