#ifndef CFG_PARSER_H
#define CFG_PARSER_H

void cp_reset_parser(void);
int cp_parse_file(char * file_name);
int cp_add_key(char * main_key,char * sub_key,char * value);
int cp_get_key(char * main_key,char * sub_key,char * value);
void cp_print_all_pair(void);

#endif
