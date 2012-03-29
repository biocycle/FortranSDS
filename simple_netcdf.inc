#ifndef SIMPLE_NETCDF_INC
#define SIMPLE_NETCDF_INC

#define snc_cf_write_coords(...) snc_cf_write_coords(__VA_ARGS__, \
        src_file = __FILE__, src_line = __LINE__)

#define snc_open_var(...) snc_open_var(__VA_ARGS__, \
        src_file = __FILE__, src_line = __LINE__)

#define snc_open(path) snc_open(path, src_file = __FILE__, src_line = __LINE__)
#define snc_create(path) snc_create(path, src_file = __FILE__, src_line = __LINE__)

#define snc_def_dim(...) snc_def_dim(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define snc_get_dim(...) snc_get_dim(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)

#define snc_inq_var(...) snc_inq_var(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define snc_def_var(...) snc_def_var(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define snc_read(...) snc_read(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define snc_write(...) snc_write(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)

#define snc_get_att(...) snc_get_att(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define snc_put_att(...) snc_put_att(__VA_ARGS__, src_file = __FILE__, src_line = __LINE__)
#define snc_get_global_att(...) snc_get_global_att(__VA_ARGS__, \
        src_file = __FILE__, src_line = __LINE__)
#define snc_put_global_att(...) snc_put_global_att(__VA_ARGS__, \
        src_file = __FILE__, src_line = __LINE__)

#define snc_enddef(file) snc_enddef(file, src_file = __FILE__, src_line = __LINE__)

#define snc_close(file) snc_close(file, src_file = __FILE__, src_line = __LINE__)

#define snc_handle_error(...) snc_handle_error(__VA_ARGS__, \
        src_file = __FILE__, src_line = __LINE__)

#endif
