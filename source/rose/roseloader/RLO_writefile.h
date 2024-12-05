#ifndef RLO_WRITEFILE_H
#define RLO_WRITEFILE_H

#ifdef __cplusplus
extern "C" {
#endif

struct Main;

bool RLO_write_file(struct Main *main, const char *filepath, int flag);

#ifdef __cplusplus
}
#endif

#endif	// RLO_WRITEFILE_H
