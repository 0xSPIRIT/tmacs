#ifndef LINE_H_
#define LINE_H_

struct Line {
    char *string;
    int length, capacity;
};

struct Line *line_new(struct Line *line);
void line_insert_char(struct Line *line, int c);
void line_insert_str(struct Line *line, char *str);
void line_cut_char(struct Line *line, int n);
void line_insert_char_at(struct Line *line, int c, int k);
void line_cut_str(struct Line *line, int n, int k);
void line_center(struct Line *line, int char_w);

#endif /* LINE_H_ */
