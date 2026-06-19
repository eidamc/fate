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
#include <stdint.h>
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

const char *filenames[] = {
        "subject.txt",
        "action.txt",
        "target.txt",
        "advice.txt"
};

const char *filenames_dat[] = {
        "subject.txt.dat",
        "action.txt.dat",
        "target.txt.dat",
        "advice.txt.dat"
};

struct option flags[] = {
        { "version", no_argument, 0, 'v' },
        { "help", no_argument, 0, 'h' },
        { "predict", required_argument, 0, 'p' },
        { 0, 0, 0, 0 }
};

static int get_process_info(struct process_info *info)
{
        char path[32];
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

int get_rand_dat_byte_index(const char * file)
{
        uint32_t ver, numstr, longlen, shortlen, flags;
        uint32_t start_byte;
        uint32_t *offsets;
        int rand_inx;
        char path[32] = {0};
        char delim;

        snprintf(path, sizeof(path), FATE_DATA_DIR "/%s", file);

        FILE *f = fopen(path, "r");
        if (!f)
                return -ENOENT;

        fread(&ver, sizeof(uint32_t), 1, f);
        fread(&numstr, sizeof(uint32_t), 1, f);
        fread(&longlen, sizeof(uint32_t), 1, f);
        fread(&shortlen, sizeof(uint32_t), 1, f);
        fread(&flags, sizeof(uint32_t), 1, f);
        fread(&delim, sizeof(char), 1, f);

        ver = __builtin_bswap32(ver);
        numstr = __builtin_bswap32(numstr);
        longlen = __builtin_bswap32(longlen);
        shortlen = __builtin_bswap32(shortlen);
        flags = __builtin_bswap32(flags);

        fseek(f, 24, SEEK_SET);

        offsets = malloc((numstr + 1) * sizeof(uint32_t));
        if (!offsets) {
                fclose(f);
                return -ENOMEM;
        }

        fread(offsets, sizeof(uint32_t), numstr + 1, f);

        rand_inx = rand() % numstr;
        start_byte = __builtin_bswap32(offsets[rand_inx]);

        free(offsets);
        fclose(f);

        return start_byte;
}

int read_and_print_from_file(const char *filename, int start_byte)
{
        char path[32] = {0};
        char c;

        snprintf(path, sizeof(path), FATE_DATA_DIR "/%s", filename);

        FILE *f = fopen(path, "r");
        if (!f)
                return -ENOENT;

        fseek(f, start_byte, SEEK_SET);

        while ((c = fgetc(f)) != '\n')
                putchar(c);

        fclose(f);
        return 0;
}

void get_daily_fortune()
{
       int byte_inx;

       for (int i = 0; i < 4; i++) {
               if (i == 3)
                       printf("\n\U0001F52E ");

               byte_inx = get_rand_dat_byte_index(filenames_dat[i]);
               read_and_print_from_file(filenames[i], byte_inx);
               putchar(' ');
       }

       putchar('\n');
}

void get_syscall()
{
        int byte_inx;

        byte_inx = get_rand_dat_byte_index("syscalls.txt.dat");
        read_and_print_from_file("syscalls.txt", byte_inx);
}

static void print_result(struct process_info *info)
{
        printf("\U0001F52E DAILY HOROSCOPE FOR: %s\n\U0001F52E ", info->name);
        get_daily_fortune();
        printf("\U0001F52E Lucky syscall: ");
        get_syscall();
        printf("\n\U0001F52E Unlucky syscall: ");
        get_syscall();
        putchar('\n');
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
