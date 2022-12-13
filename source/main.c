#include <utils.h>

#define VERSION "1.0.3b"
#define DEBUG_SOCKET
#define DEBUG_ADDR IP(192, 168, 1, 155);
#define DEBUG_PORT 5655
#define INIT_FILE "conf.ini"
#define ERROR_FILE "log_error.txt"
#define SANDBOX "/mnt/sandbox/pfsmnt"
#define OCT_TO_GO / 1024 / 1024 / 1024

int sock;
int nthread_run, nthread_run_delete, folders, files, tmp_folders, tmp_files;
int tmpcnt, isxfer, isdel, xfer_pct;
long xfer_speed;
size_t folder_size_current, tmp_bytes_copied, total_bytes_copied, tmp_total_bytes_copied;
char dir_current[64], current_copied[64];
char *cfile, error_file_path[256];

void copy_file(char *src_path, char *dst_path)
{
	int src = f_open(src_path, O_RDONLY, 0);
	if (src != -1)
	{
		int out = f_open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
		if (out != -1)
		{
			cfile = src_path;
			isxfer = 1;
			char *buffer = f_malloc(4194304);
			
			if (buffer != NULL)
			{
				size_t bytes, bytes_size, bytes_copied = 0;

				f_lseek(src, 0L, SEEK_END);
				bytes_size = f_lseek(src, 0, SEEK_CUR);
				f_lseek(src, 0L, SEEK_SET);

				while ((bytes = f_read(src, buffer, 4194304)) > 0)
				{
					f_write(out, buffer, bytes);
					bytes_copied += bytes;
					if (bytes_copied > bytes_size)
						bytes_copied = bytes_size;

					tmp_bytes_copied = bytes_copied;
					total_bytes_copied = tmp_total_bytes_copied + bytes_copied;
					xfer_speed += bytes;
					xfer_pct = bytes_copied * 100 / bytes_size;
					printfsocket("Copy of: %s | %i%% \n", cfile, xfer_pct);
				}
				f_free(buffer);

				tmp_total_bytes_copied = tmp_total_bytes_copied + tmp_bytes_copied;
				total_bytes_copied = tmp_total_bytes_copied;
				printfsocket("[Total Progress] %i%% \n", total_bytes_copied * 100 / folder_size_current);
			}
		}
		f_close(out);
		isxfer = 0;
		xfer_pct = 0;
	}
	f_close(src);
}


void copy_dir_current(char *dir_current, char *out_dir_current, char (*ignored_lang)[3])
{
	DIR *dir = f_opendir(dir_current);
	struct dirent *dp;
	struct stat info;
	char src_path[1024], dst_path[1024], error[1024];
	int attempt = -1;
	int hasfound_lang = -1;

	if (!dir)
	{
		return;
	}
	f_mkdir(out_dir_current, 0777);
	if (dir_exists(out_dir_current))
	{
		while ((dp = f_readdir(dir)) != NULL)
		{
			if (!f_strcmp(dp->d_name, ".") || !f_strcmp(dp->d_name, ".."))
			{
				continue;
			}
			else
			{
				f_sprintf(src_path, "%s/%s", dir_current, dp->d_name);
				f_sprintf(dst_path, "%s/%s", out_dir_current, dp->d_name);

				if (!f_stat(src_path, &info))
				{
					if (S_ISDIR(info.st_mode))
					{
						tmp_folders++;
						copy_dir_current(src_path, dst_path, ignored_lang);
					}
					else if (S_ISREG(info.st_mode))
					{
						// Check extention file
						char *lastDotPos = f_strrchr(src_path, '.');
						if (lastDotPos != NULL)
						{
							int i;
							// browse the languages ​​to be ignored
							for (i = 0; i < 16; i++)
							{
								if (!f_strcmp(ignored_lang[i], lastDotPos + 1))
								{
									f_sprintf(error, "\n[FILE LANGUE IGNORE]%s\n", src_path);
									write_error(error_file_path, error);
									hasfound_lang = 0;
									continue;
								}
							}
							// if the language extension is not to be ignored
							if (hasfound_lang != 0)
								copy_file(src_path, dst_path);
						}// if no extension
						else
						{
							copy_file(src_path, dst_path);
						}
							
						if (file_compare(src_path, dst_path))
						{
							tmp_files++;
						}
						else
						{
							if ( attempt == -1)
							{
								total_bytes_copied = (tmp_total_bytes_copied - tmp_bytes_copied);
								f_sprintf(error, "[ATTEMPT TO WRITE FILE] because file do not match ->  %s\n", dst_path);
								printfsocket(error);
								write_error(error_file_path, error);
								attempt = 0;
								copy_file(src_path, dst_path);
							}
							else if (!file_compare(src_path, dst_path) && attempt == 0)
							{
								f_sprintf(error, "[REWRITE ATTEMPT FAILED] Maybe more free space on your usb drive?.\n[FILE IGNORE] -> %s\n\n", src_path);
								printfsocket(error);
								write_error(error_file_path, error);
								attempt = -1;
							}
						}
					}
				}
			}
		}
	}
	f_closedir(dir);
}

// calcul the size of a file and increments the size of the current folder
void size_file(char addr[1024])
{
	FILE *file;
	size_t size = 0;
	file = f_fopen(addr, "rb");
	if (file)
	{
		f_fseek(file, 0, SEEK_END);
		size = f_ftell(file);
		// increment the size of the current folder
		folder_size_current += size;
		printfsocket("[OPEN AND CALCUL SIZE] %s | %lu Octets\n", addr, size);
		f_fclose(file);
	}
}

// calculate the total size of the current folder
void check_size_folder_current(char *dir_current, char (*ignored_lang)[3])
{
	DIR *dir = f_opendir(dir_current);
	struct dirent *dp;
	struct stat info;
	char src_file[1024], error[1024];
	int hasfound_lang = -1;

	if (!dir)
		return;
	while ((dp = f_readdir(dir)) != NULL)
	{
		if (!f_strcmp(dp->d_name, ".") || !f_strcmp(dp->d_name, ".."))
			continue;
		else
		{
			f_sprintf(src_file, "%s/%s", dir_current, dp->d_name);
			if (!f_stat(src_file, &info))
			{
				if (S_ISDIR(info.st_mode))
				{
					folders++;
					check_size_folder_current(src_file, ignored_lang);
				}
				else if (S_ISREG(info.st_mode))
				{

					// Check extention file
					char *lastDotPos = f_strrchr(src_file, '.');
					if (lastDotPos != NULL)
					{
						int i;
						// browse the languages ​​to be ignored
						for (i = 0; i < 16; i++)
						{
							if (!f_strcmp(ignored_lang[i], lastDotPos + 1))
							{
								f_sprintf(error, "[FILE IGNORE]%s\n", src_file);
								write_error(error_file_path, error);
								hasfound_lang = 0;
								continue;
							}
						} // if the language extension is not to be ignored
						if (hasfound_lang != 0)
						{
							files++;
							size_file(src_file);
						}
					} // if no extension
					else
					{
						files++;
						size_file(src_file);
					}
				}
			}
		}
		hasfound_lang = -1;
	}
	f_closedir(dir);
}
void check_current_folder(char *dir_current, char (*ignored_lang)[3])
{
	printfsocket("\n[CALCUL CURRENT FOLDER] -> %s\n", dir_current);
	printf_notification("Calcul size of:\n\n %s", dir_current);
	f_sceKernelSleep(7);
	check_size_folder_current(dir_current, ignored_lang);
	printfsocket("\n[FOLDER CONTENT] %s | FOLDER = %d | FILES = %ld | Size = %lu Octets | %.2f Go\n\n", dir_current, folders, files, folder_size_current, (double)folder_size_current OCT_TO_GO);
	printf_notification("Size of:\n\n%s\n%.2f Go", dir_current, (double)folder_size_current OCT_TO_GO);
	f_sceKernelSleep(7);
}

int copy_verification(char *out_dir_current)
{
	folders = 0;
	files = 0;
	printfsocket("\n[OUT FOLDER CONTENT] %s | FOLDER = %d | FILES = %ld | Size = %lu/%lu Octets | %.2f/%.2f Go\n\n", out_dir_current, tmp_folders, tmp_files, total_bytes_copied, folder_size_current, (double)total_bytes_copied OCT_TO_GO, (double)folder_size_current OCT_TO_GO);
	if (total_bytes_copied == folder_size_current)
	{
		printfsocket("[COPY SUCCESSFULLY]\n");
		printf_notification("Copy of:\n%s\nSuccessfully!\n%.2f Go", out_dir_current, (double)folder_size_current OCT_TO_GO);
		f_sceKernelSleep(7);
		return 1;
	}
	else
	{
		printfsocket("[ERROR WHILE COPYING]\n");
		printf_notification("Error while copying\nCheck the error.txt file on USB");
		f_sceKernelSleep(7);
		return 0;
	}
	tmp_folders = 0;
	tmp_files = 0;
	folder_size_current = 0;
}
void *nthread_delete(void *arg)
{
	UNUSED(arg);
	time_t t1, t2;
	t1 = 0;
	while (nthread_run_delete)
	{
		if (isdel)
		{
			t2 = f_time(NULL);
			if ((t2 - t1) >= 7)
			{
				t1 = t2;
				printf_notification("Deletion in progress please wait...");
			}
		}
		else
			t1 = 0;

		f_sceKernelSleep(5);
	}

	return NULL;
}
void *nthread_func(void *arg)
{
	UNUSED(arg);
	time_t t1, t2;
	t1 = 0;
	while (nthread_run)
	{
		if (isxfer)
		{
			t2 = f_time(NULL);
			if ((t2 - t1) >= 20)
			{
				t1 = t2;
				if (tmpcnt >= 1048576)
					printf_notification("Copy of:\n%s \nProgress:  %i%% / Speed: %u MB/s\nCopied %.2fGo/%.2fGo", current_copied, total_bytes_copied * 100 / folder_size_current, tmpcnt / 1048576, (double)total_bytes_copied OCT_TO_GO, (double)folder_size_current OCT_TO_GO);

				else if (tmpcnt >= 1024)
					printf_notification("Copy of:\n%s \nProgress:  %i%% / Speed: %u KB/s\nCopied %.2fGo/%.2fGo", current_copied, total_bytes_copied * 100 / folder_size_current, tmpcnt / 1024, (double)total_bytes_copied OCT_TO_GO, (double)folder_size_current OCT_TO_GO);
				else
					printf_notification("Copy of:\n%s \nProgress:  %i%% / Speed: %u B/s\nCopied %.2fGo/%.2fGo", current_copied, total_bytes_copied * 100 / folder_size_current, tmpcnt, (double)total_bytes_copied OCT_TO_GO, (double)folder_size_current OCT_TO_GO);
			}
		}
		else
			t1 = 0;
		f_sceKernelSleep(5);
	}

	return NULL;
}
void *sthread_func(void *arg)
{
	while (nthread_run)
	{
		if (isxfer)
		{
			tmpcnt = xfer_speed;
			xfer_speed = 0;
		}
		f_sceKernelSleep(1);
	}

	return NULL;
}

int payload_main(struct payload_args *args)
{
	dlsym_t *dlsym = args->dlsym;

	int libKernel = 0x2001;

	dlsym(libKernel, "sceKernelSleep", &f_sceKernelSleep);
	dlsym(libKernel, "sceKernelLoadStartModule", &f_sceKernelLoadStartModule);
	dlsym(libKernel, "sceKernelDebugOutText", &f_sceKernelDebugOutText);
	dlsym(libKernel, "sceKernelSendNotificationRequest", &f_sceKernelSendNotificationRequest);
	dlsym(libKernel, "sceKernelUsleep", &f_sceKernelUsleep);
	dlsym(libKernel, "scePthreadMutexLock", &f_scePthreadMutexLock);
	dlsym(libKernel, "scePthreadMutexUnlock", &f_scePthreadMutexUnlock);
	dlsym(libKernel, "scePthreadExit", &f_scePthreadExit);
	dlsym(libKernel, "scePthreadMutexInit", &f_scePthreadMutexInit);
	dlsym(libKernel, "scePthreadCreate", &f_scePthreadCreate);
	dlsym(libKernel, "scePthreadMutexDestroy", &f_scePthreadMutexDestroy);
	dlsym(libKernel, "scePthreadJoin", &f_scePthreadJoin);
	dlsym(libKernel, "socket", &f_socket);
	dlsym(libKernel, "bind", &f_bind);
	dlsym(libKernel, "listen", &f_listen);
	dlsym(libKernel, "accept", &f_accept);
	dlsym(libKernel, "open", &f_open);
	dlsym(libKernel, "read", &f_read);
	dlsym(libKernel, "write", &f_write);
	dlsym(libKernel, "close", &f_close);
	dlsym(libKernel, "stat", &f_stat);
	dlsym(libKernel, "fstat", &f_fstat);
	dlsym(libKernel, "rename", &f_rename);
	dlsym(libKernel, "rmdir", &f_rmdir);
	dlsym(libKernel, "mkdir", &f_mkdir);
	dlsym(libKernel, "getdents", &f_getdents);
	dlsym(libKernel, "unlink", &f_unlink);
	dlsym(libKernel, "readlink", &f_readlink);
	dlsym(libKernel, "lseek", &f_lseek);
	dlsym(libKernel, "puts", &f_puts);
	dlsym(libKernel, "mmap", &f_mmap);
	dlsym(libKernel, "munmap", &f_munmap);

	dlsym(libKernel, "sceKernelReboot", &f_sceKernelReboot);

	int libNet = f_sceKernelLoadStartModule("libSceNet.sprx", 0, 0, 0, 0, 0);
	dlsym(libNet, "sceNetSocket", &f_sceNetSocket);
	dlsym(libNet, "sceNetConnect", &f_sceNetConnect);
	dlsym(libNet, "sceNetHtons", &f_sceNetHtons);
	dlsym(libNet, "sceNetAccept", &f_sceNetAccept);
	dlsym(libNet, "sceNetSend", &f_sceNetSend);
	dlsym(libNet, "sceNetInetNtop", &f_sceNetInetNtop);
	dlsym(libNet, "sceNetSocketAbort", &f_sceNetSocketAbort);
	dlsym(libNet, "sceNetBind", &f_sceNetBind);
	dlsym(libNet, "sceNetListen", &f_sceNetListen);
	dlsym(libNet, "sceNetSocketClose", &f_sceNetSocketClose);
	dlsym(libNet, "sceNetHtonl", &f_sceNetHtonl);
	dlsym(libNet, "sceNetInetPton", &f_sceNetInetPton);
	dlsym(libNet, "sceNetGetsockname", &f_sceNetGetsockname);
	dlsym(libNet, "sceNetRecv", &f_sceNetRecv);
	dlsym(libNet, "sceNetErrnoLoc", &f_sceNetErrnoLoc);
	dlsym(libNet, "sceNetSetsockopt", &f_sceNetSetsockopt);

	int libC = f_sceKernelLoadStartModule("libSceLibcInternal.sprx", 0, 0, 0, 0, 0);
	dlsym(libC, "vsprintf", &f_vsprintf);
	dlsym(libC, "memset", &f_memset);
	dlsym(libC, "sprintf", &f_sprintf);
	dlsym(libC, "snprintf", &f_snprintf);
	dlsym(libC, "snprintf_s", &f_snprintf_s);
	dlsym(libC, "strcat", &f_strcat);
	dlsym(libC, "free", &f_free);
	dlsym(libC, "memcpy", &f_memcpy);
	dlsym(libC, "strcpy", &f_strcpy);
	dlsym(libC, "strncpy", &f_strncpy);
	dlsym(libC, "sscanf", &f_sscanf);
	dlsym(libC, "scanf", &f_scanf);
	dlsym(libC, "malloc", &f_malloc);
	dlsym(libC, "calloc", &f_calloc);
	dlsym(libC, "strlen", &f_strlen);
	dlsym(libC, "strcmp", &f_strcmp);
	dlsym(libC, "strchr", &f_strchr);
	dlsym(libC, "strrchr", &f_strrchr);
	dlsym(libC, "gmtime_s", &f_gmtime_s);
	dlsym(libC, "time", &f_time);
	dlsym(libC, "localtime", &f_localtime);
	dlsym(libC, "closedir", &f_closedir);
	dlsym(libC, "opendir", &f_opendir);
	dlsym(libC, "readdir", &f_readdir);
	dlsym(libC, "fclose", &f_fclose);
	dlsym(libC, "fopen", &f_fopen);
	dlsym(libC, "strstr", &f_strstr);
	dlsym(libC, "fseek", &f_fseek);
	dlsym(libC, "ftell", &f_ftell);
	dlsym(libC, "fread", &f_fread);
	dlsym(libC, "usleep", &f_usleep);
	dlsym(libC, "fputs", &f_fputs);
	dlsym(libC, "fgetc", &f_fgetc);
	dlsym(libC, "feof", &f_feof);
	dlsym(libC, "fprintf", &f_fprintf);
	dlsym(libC, "realloc", &f_realloc);
	dlsym(libC, "seekdir", &f_seekdir);

	int libNetCtl = f_sceKernelLoadStartModule("libSceNetCtl.sprx", 0, 0, 0, 0, 0);
	dlsym(libNetCtl, "sceNetCtlInit", &f_sceNetCtlInit);
	dlsym(libNetCtl, "sceNetCtlTerm", &f_sceNetCtlTerm);
	dlsym(libNetCtl, "sceNetCtlGetInfo", &f_sceNetCtlGetInfo);

	struct sockaddr_in server;
	server.sin_len = sizeof(server);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = DEBUG_ADDR;
	server.sin_port = f_sceNetHtons(DEBUG_PORT);
	f_memset(server.sin_zero, 0, sizeof(server.sin_zero));
	sock = f_sceNetSocket("debug", AF_INET, SOCK_STREAM, 0);
	f_sceNetConnect(sock, (struct sockaddr *)&server, sizeof(server));

	char usb_mount_path[256], init_file_path[256], lang[3];
	char title_id[64], out_dir_current[64], dest_path[1024];

	xfer_speed = 0;
	isxfer = 0;
	nthread_run = 1;
	nthread_run_delete = 1;

	ScePthread nthread;
	f_scePthreadCreate(&nthread, NULL, nthread_func, NULL, "nthread");
	ScePthread nthread2;
	f_scePthreadCreate(&nthread2, NULL, nthread_delete, NULL, "nthreaddel");
	ScePthread sthread;
	f_scePthreadCreate(&sthread, NULL, sthread_func, NULL, "sthread");

	printf_notification("Welcome to PFSMNT-DUMPER V%s", VERSION);
	f_sceKernelSleep(7);

	if (wait_for_game(title_id) != 1)
	{
		do
		{
			printf_notification("Please start the PSN Game and minimize it before running the payload...");
			f_sceKernelSleep(7);
		} while (wait_for_game(title_id) != 1);
	}
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
	f_sprintf(usb_mount_path, "%s", usb_mnt_path);
	f_free(usb_mnt_path);
	f_sprintf(init_file_path, "%s/%s", usb_mount_path, INIT_FILE);
	make_file_conf(init_file_path);
	f_sprintf(error_file_path, "%s/%s", usb_mount_path, ERROR_FILE);
	make_file_error(error_file_path);
	char *langue = check_lang_in_conf_init(init_file_path);
	f_sprintf(lang, "%s", langue);

	if (langue != NULL)
	{
		f_free(langue);
		printf_notification("Your selected language: %s", lang);
		f_sceKernelSleep(7);
		char(*ignored_lang)[3];
		ignored_lang = f_malloc(32 * sizeof(*ignored_lang));
		if (ignored_lang != NULL)
		{
			language_ignored(ignored_lang, lang);
			f_sprintf(dest_path, "%s/%s", usb_mount_path, title_id);
			if (dir_exists(dest_path))
			{
				printf_notification("Application already present on USB\n\nDeletion.Wait...");
				f_sceKernelSleep(7);
				isdel = 1;
				if (!erase_folder(dest_path))
					isdel = 0;
				nthread_run_delete = 0;
				f_sceKernelSleep(7);
				if (!dir_exists(dest_path))
				{
					printf_notification("Application:\n\n%s\nDelete successfully.Wait...", dest_path);
					f_sceKernelSleep(7);
				}
			}

			f_mkdir(dest_path, 0777);

			char full[32] = "//FULL_DUMP";
			int int_full = if_option(init_file_path, full);
			char nest[32] = "//app0-nest";
			int int_nest = if_option(init_file_path, nest);
			if (int_full || int_nest)
			{
				if (int_full)
					f_sprintf(dir_current, "%s", SANDBOX);
				else if (int_nest)
					f_sprintf(dir_current, "%s/%s-app0-nest", SANDBOX, title_id);
				printfsocket("\n%s -> Selected", dir_current);
				printf_notification("%s\n\nSelected!", dir_current);
				f_sprintf(current_copied, dir_current);
				f_sceKernelSleep(7);
				check_current_folder(dir_current, ignored_lang);
				f_sprintf(out_dir_current, "%s", dest_path);
				printf_notification("Copy of\n\n%s\nStart...", dir_current);
				f_sceKernelSleep(7);
				copy_dir_current(dir_current, out_dir_current, ignored_lang);
				if (!copy_verification(out_dir_current))
					;
			}
			else
			{
				f_sprintf(dir_current, "%s/%s-app0", SANDBOX, title_id);
				f_sprintf(current_copied, dir_current);
				check_current_folder(dir_current, ignored_lang);
				f_sprintf(out_dir_current, "%s/%s-app0", dest_path, title_id);
				printf_notification("Copy of:\n\n%s\nStart...", dir_current);
				f_sceKernelSleep(7);
				copy_dir_current(dir_current, out_dir_current, ignored_lang);
				f_sceKernelSleep(7);
				if (copy_verification(out_dir_current) != 0)
				{
					f_sprintf(dir_current, "%s/%s-patch0", SANDBOX, title_id);
					if (dir_exists(dir_current))
					{
						f_sprintf(current_copied, dir_current);
						check_current_folder(dir_current, ignored_lang);
						f_sprintf(out_dir_current, "%s/%s-patch0", dest_path, title_id);
						printf_notification("Copy of:\n\n%s\nStart...", dir_current);
						f_sceKernelSleep(7);
						copy_dir_current(dir_current, out_dir_current, ignored_lang);
						f_sceKernelSleep(7);
						if (!copy_verification(out_dir_current))
						{
							char error[128] = "Failed to copy, see error file on usb";
							printf_notification("%s", error);
							write_error(error_file_path, error);
							f_sceKernelSleep(7);
						}
					}
					else
					{
						printf_notification("%s\nNot found", dir_current);
						f_sceKernelSleep(7);
					}
				}
			}
		}
		else
		{
			char error[128] = "Memory allocation error";
			printf_notification("%s", error);
			write_error(error_file_path, error);
			f_sceKernelSleep(7);
		}
		f_free(ignored_lang);
	}
	else
	{
		f_free(langue);
		char error[128] = "ERROR:\nIn the conf.ini file the language does\nnot match. Correct\n";
		printf_notification("%s", error);
		write_error(error_file_path, error);
		f_sceKernelSleep(7);
	}
	nthread_run = 0;
	printf_notification("Goodbye!");
	printfsocket("EXIT");
	return 0;
}