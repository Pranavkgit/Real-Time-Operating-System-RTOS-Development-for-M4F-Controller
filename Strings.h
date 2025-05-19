#ifndef STRINGS_H_
#define STRINGS_H_

#include<stdio.h>
#include<stdint.h>
#include <inttypes.h>

int length_str(char *str);
int cmp_str(char *str1,char *str2);
char *str_cat(char *str1,char *str2,char *final_str);
char *itoa(uint32_t pid, char *str);
char *tohex(uint32_t num);
char *percentage(char *cpu);

#endif /* STRINGS_H_ */
