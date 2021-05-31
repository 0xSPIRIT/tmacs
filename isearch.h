#ifndef ISEARCH_H_
#define ISEARCH_H_

#include <stdbool.h>

extern bool isearch_mode;
extern int sx, sy;

void toggle_isearch_mode(void);
void isearch_add_char(int c);
void isearch_update_point(void);

#endif  /* ISEARCH_H_ */
