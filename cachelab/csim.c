#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cachelab.h"

int h, v, s, E, b, S;
int current_time = 0, hit_count = 0, miss_count = 0, eviction_count = 0;
char t[1000];
FILE *fp;

typedef struct {
  int valid;
  int tag;
  int time;
} cache_line;
typedef cache_line *cache_set;
typedef cache_set *cache;

cache C;

void print_usage() {
  printf(
      "Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n"
      "Options:\n"
      "  -h         Print this help message.\n"
      "  -v         Optional verbose flag.\n"
      "  -s <num>   Number of set index bits.\n"
      "  -E <num>   Number of lines per set.\n"
      "  -b <num>   Number of block offset bits.\n"
      "  -t <file>  Trace file.\n"
      "\n"
      "Examples:\n"
      "  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
      "  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

int get_current_time() { return current_time; }

void update_current_time() { current_time++; }

void parse_flag(int argc, char *argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
    switch (opt) {
      case 'h':
        h = 1;
        break;
      case 'v':
        v = 1;
        break;
      case 's':
        s = atoi(optarg);
        break;
      case 'E':
        E = atoi(optarg);
        break;
      case 'b':
        b = atoi(optarg);
        break;
      case 't':
        strcpy(t, optarg);
        break;
      default:
        print_usage();
        exit(-1);
    }
  }
  if (h) print_usage();
}

void load_file() {
  fp = fopen(t, "r");
  if (fp == NULL) {
    printf("%s: No such file or directory\n", t);
    exit(-1);
  }
}

void close_file() { fclose(fp); }

void init_cache() {
  C = (cache)malloc(sizeof(cache_set) * S);
  for (int i = 0; i < S; i++) {
    C[i] = (cache_set)malloc(sizeof(cache_line) * E);
    for (int j = 0; j < E; j++) {
      C[i][j].valid = 0;
      C[i][j].tag = 0;
      C[i][j].time = get_current_time();
    }
  }
}

void update_cache(unsigned int addr) {
  int set = (addr >> b) & ((1 << s) - 1);
  int tag = addr >> (s + b);

  // check if hit
  for (int i = 0; i < E; i++) {
    if (C[set][i].valid && C[set][i].tag == tag) {
      hit_count++;
      C[set][i].time = get_current_time();
      if (v) printf("hit ");
      return;
    }
  }

  miss_count++;
  if (v) printf("miss ");

  // if miss, check if there is empty line
  for (int i = 0; i < E; ++i) {
    if (!C[set][i].valid) {
      C[set][i].valid = 1;
      C[set][i].tag = tag;
      C[set][i].time = get_current_time();
      return;
    }
  }

  // if miss and no empty line, evict the line with the smallest time
  eviction_count++;
  if (v) printf("eviction ");

  int min_time = C[set][0].time;
  int min_index = 0;
  for (int i = 1; i < E; i++) {
    if (C[set][i].time < min_time) {
      min_time = C[set][i].time;
      min_index = i;
    }
  }
  C[set][min_index].tag = tag;
  C[set][min_index].time = get_current_time();
}

void simulate_cache() {
  char op;
  unsigned int addr;
  int size;

  while (~fscanf(fp, "%c %xu,%d", &op, &addr, &size)) {
    if (op != 'L' && op != 'S' && op != 'M') continue;
    if (v) printf("%c %d,%d ", op, addr, size);
    switch (op) {
      case 'L':
        update_cache(addr);
        break;
      case 'M':
        // the data modify operation is treated as a load followed by a store
        update_cache(addr);
      case 'S':
        update_cache(addr);
        break;
    }
    if (v) puts("");
    update_current_time();
  }
}

void free_cache() {
  for (int i = 0; i < S; i++) {
    free(C[i]);
  }
  free(C);
}

int main(int argc, char *argv[]) {
  // parse flag from command line
  parse_flag(argc, argv);
  S = 1 << s;

  // load trace file
  load_file();

  // init cache
  init_cache();

  // simulate cache
  simulate_cache();

  close_file();
  free_cache();

  printSummary(hit_count, miss_count, eviction_count);
  return 0;
}
