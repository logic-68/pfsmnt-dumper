#ifndef UTILS_H
#define UTILS_H

#include <resolve.h>
#include <debug.h>

#define LANG                                                                                                 \
    {                                                                                                        \
        "us", "se", "ru", "pt", "pl", "no", "nl", "la", "jp", "it", "fr", "fi", "es", "dk", "de", "br", "ar" \
    }
void language_ignored(char (*ignored_lang)[3], char *lang);
int erase_folder(char *dest_path);
void write_error(char *error_file_path, char *error);
void printf_notification(const char *fmt, ...);
void check_current_folder(char *dir_current, char (*ignored_lang)[3]);
void make_file_error(char *error_file_path);
void make_file_conf(char *init_file_path);
void init_lang(char *usb_mount_path);
void touch_file(char *destfile);
char *check_lang_in_conf_init();
char *getusbpath();
int substring(char *haystack, char *needle);
int file_compare(char *fname1, char *fname2);
int if_option(char *init_file_path, char *option);
int wait_for_game(char *title_id);
int file_exists(char *fname);
int dir_exists(char *dname);
#endif