#include <resolve.h>
#include <debug.h>

#define VERSION "1.0.0b"
#define DEBUG_SOCKET
#define DEBUG_ADDR IP(192, 168, 1, 155);
#define DEBUG_PORT 5655

void touch_file(char *destfile)
{
	int fd = f_open(destfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd != -1)
		f_close(fd);
}


int sock;
int nthread_run;
int xfer_pct;
char *cfile;
int isxfer;
int folders;
int files;
size_t total_size_folder = 0;
// int nb_folder_copied = 0;
// int nb_file_copied = 0;
size_t progress = 0;
char dir_curent[64];
int progress_total_by_folder;

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
	f_strcpy(noti_buffer.uri, "cxml://user/data/notifi/setting.png");

	f_sceKernelSendNotificationRequest(0, (SceNotificationRequest *)&noti_buffer, sizeof(noti_buffer), 0);
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

void size_file(char addr[1024])
{
	FILE *fichier;
	size_t size = 0;
	fichier = f_fopen(addr, "rb");

	if (fichier)
	{
		f_fseek(fichier, 0, SEEK_END);
		size = f_ftell(fichier);
		total_size_folder += size;
		printfsocket("[OPEN ADD SIZE FILE] %lu Octets\n", size);
		f_fclose(fichier);
	}
	else
	{
		printfsocket("[ERROR] %s\n\n", addr);
	}
	return;
}
void check_folder_size(char *sourcedir)
{
	DIR *dir = f_opendir(sourcedir);
	struct dirent *dp;
	struct stat info;
	char src_path[1024];

	if (!dir)
	{
		return;
	}

	while ((dp = f_readdir(dir)) != NULL)
	{
		if (!f_strcmp(dp->d_name, ".") || !f_strcmp(dp->d_name, ".."))
		{
			continue;
		}
		else
		{
			f_sprintf(src_path, "%s/%s", sourcedir, dp->d_name);
			if (!f_stat(src_path, &info))
			{
				if (S_ISDIR(info.st_mode))
				{
					folders++;
					check_folder_size(src_path);
				}
				else if (S_ISREG(info.st_mode))
				{
					files++;
					size_file(src_path);
				}
			}
		}
	}
	f_closedir(dir);
}
void copy_file(char *sourcefile, char *destfile)
{
	int src = f_open(sourcefile, O_RDONLY, 0);

	if (src != -1)
	{
		int out = f_open(destfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
		if (out != -1)
		{
			cfile = sourcefile;
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
					{
						bytes_copied = bytes_size;
					}
					xfer_pct = bytes_copied * 100 / bytes_size;
					printfsocket("Copy of: %s | %i%%\n", cfile, xfer_pct);

					if (bytes_size > 1073741824) // 1Go
					{
						progress += bytes_copied;
						f_sprintf(dir_curent, "%s", cfile);
						progress_total_by_folder = xfer_pct;

					}
				}
				if (bytes_size < 1073741824) // 1Go
				{
					progress += bytes_copied;
					progress_total_by_folder = progress * 100 / total_size_folder;

					printfsocket("Total Progress | %i%% \n", progress_total_by_folder);
				}

				f_free(buffer);
			}
			f_close(out);
			isxfer = 0;
			xfer_pct = 0;
		}
		f_close(src);
	}
}

void copy_dir(char *sourcedir, char *destdir)
{
	DIR *dir = f_opendir(sourcedir);
	struct dirent *dp;
	struct stat info;
	char src_path[1024];
	char dst_path[1024];

	if (!dir)
	{
		return;
	}
	f_mkdir(destdir, 0777);

	while ((dp = f_readdir(dir)) != NULL)
	{
		if (!f_strcmp(dp->d_name, ".") || !f_strcmp(dp->d_name, ".."))
		{
			continue;
		}
		else
		{
			f_sprintf(src_path, "%s/%s", sourcedir, dp->d_name);
			f_sprintf(dst_path, "%s/%s", destdir, dp->d_name);

			if (!f_stat(src_path, &info))
			{
				if (S_ISDIR(info.st_mode))
				{

					copy_dir(src_path, dst_path);
				}
				else if (S_ISREG(info.st_mode))
				{
					copy_file(src_path, dst_path);
				}
			}
		}
	}
	f_closedir(dir);
}

int wait_for_usb(char *usb_name, char *usb_path)
{
	char probe[64];
	f_sprintf(probe, "%s", "/mnt/usb0/.probe");
	touch_file(probe);

	if (!file_exists(probe))
	{
		printfsocket("Usb Not Found\n");
		return 0;
	}
	else
	{
		f_unlink(probe);
		f_sprintf(usb_name, "%s", "usb0");
		f_sprintf(usb_path, "%s", "/mnt");
		printfsocket("Usb Found\n");

		return 1;
	}
}

int wait_for_game(char *title_id)
{
	int res = 0;

	DIR *dir;
	struct dirent *dp;

	dir = f_opendir("/mnt/sandbox/pfsmnt");
	if (!dir)
	{
		printfsocket("No game launch\n");
		return 0;
	}

	while ((dp = f_readdir(dir)) != NULL)
	{
		if (f_strstr(dp->d_name, "-app0") != NULL)
		{
			f_sscanf(dp->d_name, "%[^-]", title_id);
			printfsocket("Game %s start\n", title_id);
			res = 1;
			break;
		}
	}
	f_closedir(dir);

	return res;
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
				printf_notification("Copy %s \nProgress:  %i%%", dir_curent, progress_total_by_folder);
			}
		}
		else
		{
			t1 = 0;
		}
		f_sceKernelSleep(5);
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

	printf_notification("Welcome to PFSMNT-DUMPER V%s", VERSION);
	f_sceKernelSleep(7);

	int nb_folders_app0;
	int nb_files_app0;
	size_t total_app0 = 0;
	char usb_name[256] = {0};
	char usb_path[256] = {0};
	char title_id[64];
	char src_path[1024], dst_path[1024];

	isxfer = 0;
	nthread_run = 1;

	ScePthread nthread;
	f_scePthreadCreate(&nthread, NULL, nthread_func, NULL, "nthread");
	if (wait_for_game(title_id) == 0)
	{
		do
		{
			printf_notification("Please start the PSN Game and minimize it before running the payload...");
			f_sceKernelSleep(7);
		} while (wait_for_game(title_id) == 0);
	}

	if (wait_for_usb(usb_name, usb_path) == 0)
	{
		do
		{
			printf_notification("Please insert USB media in exfat format");
			f_sceKernelSleep(7);
		} while (wait_for_usb(usb_name, usb_path) == 0);
	}

	f_sprintf(src_path, "/mnt/sandbox/pfsmnt/%s-app0", title_id);
	if (file_exists(src_path))
	{
		f_sprintf(dir_curent, "%s-app0", title_id);
		printf_notification("Calcul size of %s-app0", title_id);
		printfsocket("src_path -> %s\n", src_path);

		check_folder_size(src_path);
		total_app0 = total_size_folder;
		printfsocket("[FOLDER CONTENT] %s | FOLDER = %d | FILES = %ld | Size = %lu Octets\n", src_path, folders, files, total_app0);
		f_sceKernelSleep(7);

		f_sprintf(dst_path, "%s/%s/%s", usb_path, usb_name, title_id);
		printfsocket("[Create Path]-> %s\n", dst_path);
		f_mkdir(dst_path, 0777);

		f_sprintf(dst_path, "%s/%s-%s", dst_path, title_id, "app0");
		printfsocket("[Create Folder]-> %s\n", dst_path);

		printf_notification("Copy of %s \nStart...", dir_curent);
		f_sceKernelSleep(7);

		copy_dir(src_path, dst_path);

		printf_notification("%s \nCopied", dir_curent);
		f_sceKernelSleep(7);
	}
	nb_folders_app0 = folders;
	nb_files_app0 = files;

	total_size_folder = 0;
	progress_total_by_folder = 0;

	f_sprintf(src_path, "/mnt/sandbox/pfsmnt/%s-patch0", title_id);

	if (file_exists(src_path))
	{
		folders = 0;
		files = 0;
		f_sprintf(dir_curent, "%s-patch0", title_id);
		printf_notification("Calcul size of %s-patch0", title_id);
		printfsocket("src_path -> %s\n", src_path);

		check_folder_size(src_path);
		printfsocket("[FOLDER CONTENT] %s | FOLDER = %d | FILES = %ld | Size = %lu Octets\n", src_path, folders, files, total_size_folder);
		f_sceKernelSleep(7);

		f_sprintf(dst_path, "%s/%s/%s/%s-%s", usb_path, usb_name, title_id, title_id, "patch0");
		printfsocket("[Create Folder]-> %s\n", dst_path);

		f_mkdir(dst_path, 0777);

		printf_notification("Copy of %s \nStart...", dir_curent);
		f_sceKernelSleep(7);

		copy_dir(src_path, dst_path);

		printf_notification("%s \nCopied", dir_curent);
		f_sceKernelSleep(7);
	}
	printfsocket("[APP_CONTENT] %s | FOLDER = %d | FILES = %ld | Size = %lu Octets\n", title_id, nb_folders_app0 + folders, nb_files_app0 + files, total_size_folder + total_app0);
	nthread_run = 0;

	printfsocket("Copy END");
	return 0;
}
