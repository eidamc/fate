// SPDX-License-Identifier: GPL-2.0
/*
 * Fate - A program providing alternative, spiritually accurate diagnostics for
 * Linux PIDs.
 *
 * Copyright (c) 2026 Joshua Crofts <joshua.crofts1@gmail.com>
 */

#include <errno.h>
#include <getopt.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef FATE_DATA_DIR
#define FATE_DATA_DIR "/usr/share/fate/"
#endif

#ifndef FATE_VERSION
#define FATE_VERSION "err"
#endif

#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261u

struct process_info {
        int pid;
        char state;
        int pgrp;
        int processor;
        unsigned long long utime;
        unsigned long long start_time;
        char name[64];
};

struct option flags[] = {
        { "version", no_argument, 0, 'v'},
        { "help", no_argument, 0, 'h'},
        { "predict", required_argument, 0, 'p'},
        { 0, 0, 0, 0}
};

static int get_process_info(struct process_info *info)
{
        char path[64];
        FILE *f;
        int ret;

        snprintf(path, sizeof(path), "/proc/%d/stat", info->pid);
        f = fopen(path, "r");
        if (!f)
                return -ENOENT;

        ret = fscanf(f, "%d %s %c %d %lld %lld %d",
                     &info->pid,
                     info->name,
                     &info->state,
                     &info->pgrp,
                     &info->utime,
                     &info->start_time,
                     &info->processor);

        if (ret < 7) {
                fclose(f);
                return -1;
        }

        fclose(f);
        return 0;
}

static char* get_syscall()
{
        char line[32] = {0};
        FILE *f;
        
        int inx = rand() % (414);

        f = fopen(FATE_DATA_DIR "syscalls.txt", "r");
        if (!f)
                return NULL;

        for (int i = 0; i < inx; i++)
                fgets(line, sizeof(line), f);
        
        fclose(f);

        line[strcspn(line, "\n")] = '\0';

        return strdup(line);
}

static void print_result(struct process_info *info)
{
        char *lucky_call = get_syscall();
        char *unlucky_call = get_syscall();

        printf("\U0001F52E Ah, %s, a wise entity on this silicon plane.\n", info->name);
        printf("\U0001F52E Born in the epoch %lld.\n", info->start_time);
        printf("\U0001F52E Lucky syscall: %s.\n", lucky_call);
        printf("\U0001F52E Unlucky syscall: %s.\n", unlucky_call);

        free(lucky_call);
        free(unlucky_call);
}

/*
 * Fowler-No-Voll hash implementation
 *
 */
unsigned int get_fnv_hash(int pid, int year, int month, int day)
{
        int hash = FNV_OFFSET_BASIS;
        char pid_timestamp[64];

        snprintf(pid_timestamp, sizeof(pid_timestamp), "%d%d%d%d", pid, year, month, day);

        for (int i = 0; pid_timestamp[i] != '\0'; i++) {
                hash ^= (unsigned char)pid_timestamp[i];
                hash *= FNV_PRIME;
        }
        
        return hash;
}

int main(int argc, char **argv)
{
        struct process_info info;
        struct tm *tm;
        time_t t;
        int options_inx = 0;
        int option;
        int ret;
        
        setlocale(LC_ALL, "");

        while ((option = getopt_long(argc, argv, "p:hv", flags, &options_inx)) != -1) {
                switch (option) {
                case 'v':
                        printf("fate " FATE_VERSION "\n");
                        return 0;
                case 'p':
                        info.pid = atoi(optarg);
                        break;
                case 'h':
                default:
                        printf("Usage: fate [-hvp] [PID_VALUE]\n");
                        return 0;
                }
        }
        
        if (optind < argc)
                info.pid = atoi(argv[optind]);

        ret = get_process_info(&info);
        if (ret) {
                puts("\U0001F52E The stars haven't aligned, try again next time...");
                return ret;
        }
       
        t = time(NULL);
        tm = localtime(&t);

        srand(get_fnv_hash(info.pid, tm->tm_year, tm->tm_mon, tm->tm_mday)); 

        print_result(&info);

        return 0;
}
