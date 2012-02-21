#ifndef SIMPLE_NETCDF_INC
#define SIMPLE_NETCDF_INC

#define SNC_OPEN(path) snc_open(path, __FILE__, __LINE__)
#define SNC_CREATE(path) snc_create(path, __FILE__, __LINE__)

#define SNC_DEF_DIM(...) snc_def_dim(__VA_ARGS__, __FILE__, __LINE__)
#define SNC_GET_DIM(...) snc_get_dim(__VA_ARGS__, __FILE__, __LINE__)

#define SNC_INQ_VAR(...) snc_inq_var(__VA_ARGS__, __FILE__, __LINE__)
#define SNC_DEF_VAR(...) snc_def_var(__VA_ARGS__, __FILE__, __LINE__)
#define SNC_READ(...) snc_read(__VA_ARGS__, __FILE__, __LINE__)
#define SNC_WRITE(...) snc_write(__VA_ARGS__, __FILE__, __LINE__)

#define SNC_GET_ATT(...) snc_get_att(__VA_ARGS__, __FILE__, __LINE__)
#define SNC_PUT_ATT(...) snc_put_att(__VA_ARGS__, __FILE__, __LINE__)

#define SNC_ENDDEF(file) snc_enddef(file, __FILE__, __LINE__)

#define SNC_CLOSE(file) snc_close(file, __FILE__, __LINE__)

#define SNC_HANDLE_ERROR(...) snc_handle_error(__VA_ARGS__, __FILE__, __LINE__)

#endif
