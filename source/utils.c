#include <utils.h>

// https://github.com/OSM-Made/PS4-Notify
void printf_notification(const char *fmt, ...)
{
	SceNotificationRequest noti_buffer;

	va_list args;
	va_start(args, fmt);
	f_vsprintf(noti_buffer.message, fmt, args);
	va_end(args);

	noti_buffer.type = 0;
	noti_buffer.unk3 = 0;
	noti_buffer.use_icon_image_uri = 1;
	noti_buffer.target_id = -1;
	f_strcpy(noti_buffer.uri, "cxml://psnotification/tex_icon_system");

	f_sceKernelSendNotificationRequest(0, (SceNotificationRequest *)&noti_buffer, sizeof(noti_buffer), 0);
}
int fgetc_pointer(int fp)
{
	char c;
	if (f_read(fp, &c, 1) == 0)
	{
		return (-1);
	}
	return (c);
}
void touch_file(char *destfile)
{
	int fd = f_open(destfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd != -1)
		f_close(fd);
}
int file_exists(char *fname)
{
	FILE *file = f_fopen(fname, "rb");
	if (file)
	{
		f_fclose(file);
		return 1;
	}
	return 0;
}
int dir_exists(char *dname)
{
	DIR *dir = f_opendir(dname);
	if (dir)
	{
		f_closedir(dir);
		return 1;
	}
	return 0;
}
int wait_for_game(char *title_id)
{
	int res = 0;
	DIR *dir;
	struct dirent *dp;
	dir = f_opendir("/mnt/sandbox/pfsmnt");
	if (!dir)
	{
		printfsocket("\n[NO GAME LAUNCH]\n");
		return 0;
	}

	while ((dp = f_readdir(dir)) != NULL)
	{
		if (f_strstr(dp->d_name, "-app0") != NULL)
		{
			f_sscanf(dp->d_name, "%[^-]", title_id);
			printfsocket("\n[GAME %s START OK]\n", title_id);
			res = 1;
			break;
		}
	}
	f_closedir(dir);

	return res;
}
void make_file_conf(char *init_file_path)
{
	if (!file_exists(init_file_path))
	{
		int ini = f_open(init_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
		char *buffer;
		buffer = "Uncomment the line below for a full backup of pfsmnt\r\n//FULL_DUMP\r\n\r\nUncomment the line below for a backup of the app0-nest folder\r\n//app0-nest\r\n\r\nUncomment the line below for a backup of the /user/app/Title_ID/app.pkg\r\n//pkg\r\n\r\nUncomment the line below for a backup of the of/user/appmeta/Title_ID\r\n//appmeta\r\n\r\nEdit the line below with your current system language:\r\nus,se,ru,pt,pl,no,nl,la,jp,it,fr,fi,es,dk,de,br,ar\r\nLANGUAGE=fr";
		f_write(ini, buffer, f_strlen(buffer));
		f_close(ini);
		printf_notification("Generated conf.in file.\nYou can change the default language.\nWait 20s...");
		f_sceKernelSleep(20);
		char *usb_mnt_path = getusbpath();
		if (usb_mnt_path == NULL)
		{
			do
			{
				printf_notification("Please insert USB media in exfat format");
				f_sceKernelSleep(7);
				usb_mnt_path = getusbpath();
			} while (usb_mnt_path == NULL);
		}
	}
	printfsocket("[INIT FILE CONF.INI] -> %s\n", init_file_path);
}
void make_file_error(char *error_file_path)
{

	int ini = f_open(error_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	char *buffer;
	buffer = "\0";
	f_write(ini, buffer, f_strlen(buffer));
	f_close(ini);
	printfsocket("[INIT FILE ERROR.TXT] -> %s\n", error_file_path);
}
char *read_string(int f)
{
	char *string = f_malloc(sizeof(char) * 65536);
	int c;
	int length = 0;
	if (!string)
	{
		return string;
	}
	while ((c = fgetc_pointer(f)) != -1)
	{
		string[length++] = c;
	}
	string[length++] = '\0';

	return f_realloc(string, sizeof(char) * length);
}
int substring(char *haystack, char *needle)
{
	if (f_strlen(haystack) >= f_strlen(needle))
	{
		for (int i = f_strlen(haystack) - f_strlen(needle); i >= 0; i--)
		{
			int found = 1;
			for (size_t d = 0; d < f_strlen(needle); d++)
			{
				if (haystack[i + d] != needle[d])
				{
					found = 0;
					break;
				}
			}
			if (found == 1)
			{
				return i;
			}
		}
	}
	return -1;
}
size_t size_file(char *src_file)
{
	FILE *file;
	size_t size = 0;
	file = f_fopen(src_file, "rb");
	if (file)
	{
		f_fseek(file, 0, SEEK_END);
		size = f_ftell(file);
	}
	f_fclose(file);
	return size;
}
int file_compare(char *fname1, char *fname2)
{
	int res = 0;
	int file1 = f_open(fname1, O_RDONLY, 0);
	int file2 = f_open(fname2, O_RDONLY, 0);
	char *buffer1 = f_malloc(65536);
	char *buffer2 = f_malloc(65536);

	if (file1 && file2 && buffer1 != NULL && buffer2 != NULL)
	{
		f_lseek(file1, 0, SEEK_END);
		f_lseek(file2, 0, SEEK_END);
		long size1 = f_lseek(file1, 0, SEEK_CUR);
		long size2 = f_lseek(file2, 0, SEEK_CUR);
		f_lseek(file1, 0L, SEEK_SET);
		f_lseek(file2, 0L, SEEK_SET);
		if (size1 == size2)
		{
			int lastBytes = 100;
			if (size1 < lastBytes)
			{
				lastBytes = size1;
			}
			f_lseek(file1, -lastBytes, SEEK_END);
			f_lseek(file2, -lastBytes, SEEK_END);
			int bytesRead1 = f_read(file1, buffer1, sizeof(char));
			int bytesRead2 = f_read(file2, buffer2, sizeof(char));
			if (bytesRead1 > 0 && bytesRead1 == bytesRead2)
			{
				res = 1;
				for (int i = 0; i < bytesRead1; i++)
				{
					if (buffer1[i] != buffer2[i])
					{
						res = 0;
						break;
					}
				}
			}
		}
	}
	f_free(buffer1);
	f_free(buffer2);
	f_close(file1);
	f_close(file2);
	return res;
}
int compare_ext(char *src_file, char (*ignored_lang)[3])
{
	char *lastDotPos = f_strrchr(src_file, '.');
	if (lastDotPos != NULL)
	{
		int i;
		for (i = 0; i < 16; i++)
			if (!f_strcmp(ignored_lang[i], lastDotPos + 1))
				return 1;
	}
	return 0;
}
void language_ignored(char (*ignored_lang)[3], char *lang)
{
	char tab[17][3] = LANG;
	char tmplang[3];
	int i, j;
	int found = 0;
	for (i = 0; i < 17; i++)
	{
		for (j = 0; j < 1; j++)
		{
			if (j % 2 == 0)
			{
				f_strcat(tmplang, &tab[i][j]);
				if (!f_strcmp(tmplang, lang) && found == 0)
				{
					found = 1;
					continue;
				}
				else
					f_strcpy(ignored_lang[i], tmplang);
				if (found != 0)
					f_strcpy(ignored_lang[i - 1], tmplang);
			}
			else
				f_strcpy(tmplang, &tab[i][j]);
		}
		tmplang[0] = '\0';
	}
}
int check_if_lang_exist(char *lang)
{
	char language[3];
	char arrayLang[34][3] = LANG;
	int i, j;
	for (i = 0; i < 17; i++)
	{
		for (j = 0; j < 1; j++)
		{
			if (j % 2 == 0)
				f_strcat(language, &arrayLang[i][j]);
			else
				f_strncpy(language, &arrayLang[i][j], 1);

			if (!f_strcmp(language, lang))
				return 1;
		}
		language[0] = '\0';
	}
	return 0;
}
char *check_lang_in_conf_init(char *init_file_path)
{
	char lang[3];
	char *retval = f_malloc(sizeof(char) * 3);
	if (file_exists(init_file_path))
	{
		int cfile = f_open(init_file_path, O_RDONLY, 0);
		char *idata = read_string(cfile);
		f_close(cfile);

		if (f_strstr(idata, "LANGUAGE=") != NULL)
		{
			int spos = substring(idata, "LANGUAGE=");
			f_strncpy(lang, idata + spos + 9, 3);
			f_strcpy(retval, lang);
			printfsocket("[LANGUAGE FOUND] %s\n\n", retval);
			int language = check_if_lang_exist(retval);
			if (language != 0)
			{
				return retval;
			}
		}
	}
	return NULL;
}
int if_option(char *init_file_path, char *option)
{
	if (file_exists(init_file_path))
	{
		int cfile = f_open(init_file_path, O_RDONLY, 0);
		char *idata = read_string(cfile);
		char *tmp_isuncomment = f_strrchr(option, '/');
		char *isuncomment = f_strrchr(tmp_isuncomment, '/');
		f_close(cfile);
		if (f_strlen(idata) != 0)
		{
			if (f_strstr(idata, option) != NULL)
				return 0;
			else if (f_strstr(idata, isuncomment + 1) != NULL)
				return 1;
		}
	}
	return 0;
}
char *getusbpath()
{
	char tmppath[64];
	char tmpusb[64];
	tmpusb[0] = '\0';
	char *retval = f_malloc(sizeof(char) * 10);

	for (int x = 0; x <= 7; x++)
	{
		f_sprintf(tmppath, "/mnt/usb%i/.probe", x);
		touch_file(tmppath);
		if (file_exists(tmppath))
		{
			f_unlink(tmppath);
			f_sprintf(tmpusb, "/mnt/usb%i", x);
			printfsocket("[USB MOUNT PATH]: %s\n", tmpusb);

			f_strcpy(retval, tmpusb);
			return retval;
		}
		tmpusb[0] = '\0';
	}
	printfsocket("[NO USB FOUND.Wait...]\n");
	return NULL;
}
void write_log(char *error_file_path, char *error)
{
	int error_log = f_open(error_file_path, O_WRONLY, 0777);
	f_lseek(error_log, 0, SEEK_END);
	f_write(error_log, error, f_strlen(error));
	f_close(error_log);
}
int erase_folder(char *dest_path)
{
	DIR *dir = f_opendir(dest_path);
	struct dirent *dp;
	struct stat info;
	char src_file[1024];
	if (dir)
	{
		while ((dp = f_readdir(dir)) != NULL)
		{
			if (!f_strcmp(dp->d_name, ".") || !f_strcmp(dp->d_name, ".."))
				continue;
			else
			{
				f_sprintf(src_file, "%s/%s", dest_path, dp->d_name);
				if (!f_stat(src_file, &info))
				{
					if (S_ISDIR(info.st_mode))
						erase_folder(src_file);
					else if (S_ISREG(info.st_mode))
						printfsocket("[DELETE FILE] -> %s\n", src_file);
					f_unlink(src_file);
				}
			}
		}
	}
	f_closedir(dir);
	f_rmdir(dest_path);
	if (!dir_exists(dest_path))
		return 0;
	else
		return 1;
}