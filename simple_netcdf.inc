#ifndef SIMPLE_NETCDF_INC
#define SIMPLE_NETCDF_INC

#define SNC_CF_GRID_CREATE(...) snc_cf_grid_create(__VA_ARGS__, \
        src_file = __FILE__, src_line = __LINE__)
#define SNC_CF_GRID_VERTICAL(...) snc_cf_grid_vertical(__VA_ARGS__, \
        src_file = __FILE__, src_line = __LINE__)
#define SNC_CF_DEF_VAR(...) snc_cf_def_var(__VA_ARGS__, src_file = __FILE__, \
        src_line = __LINE__)
#define SNC_CF_WRITE_COORDS(...) snc_cf_write_coords(__VA_ARGS__, \
        src_file = __FILE__, src_line = __LINE__)

#define SNC_OPEN(path) snc_open(path, src_file = __FILE__, src_line = __LINE__)
#define SNC_CREATE(path) snc_create(path, src_file = __FILE__, src_line = __LINE__)

#define SNC_DEF_DIM(...) snc_def_dim(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define SNC_GET_DIM(...) snc_get_dim(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)

#define SNC_INQ_VAR(...) snc_inq_var(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define SNC_DEF_VAR(...) snc_def_var(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define SNC_READ(...) snc_read(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define SNC_WRITE(...) snc_write(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)

#define SNC_GET_ATT(...) snc_get_att(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define SNC_PUT_ATT(...) snc_put_att(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define SNC_GET_GLOBAL_ATT(...) snc_get_global_att(__VA_ARGS__, \
        src_file = __FILE__, src_line = __LINE__)
#define SNC_PUT_GLOBAL_ATT(...) snc_put_global_att(__VA_ARGS__, \
        src_file = __FILE__, src_line = __LINE__)

#define SNC_ENDDEF(file) snc_enddef(file, src_file = __FILE__, src_line = __LINE__)

#define SNC_CLOSE(file) snc_close(file, src_file = __FILE__, src_line = __LINE__)

#define SNC_HANDLE_ERROR(...) snc_handle_error(__VA_ARGS__, \
        src_file = __FILE__, src_line = __LINE__)

#endif
