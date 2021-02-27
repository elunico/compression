#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUT
#define INOUT

#ifdef C_USE_LOG
#include <math.h>
int ilog10(int r) { return (int)(log10(r)); }
#endif

char *stringify_int(int a) {
  char *buf = malloc(sizeof(char) * 10);
  snprintf(buf, 10, "%d", a);
  return buf;
}

char *resize_string(char *original, int csize, INOUT int *max) {
  int nsize = (int)((*max * 1.51) + 1);
  *max = nsize;
  char *next = malloc(sizeof(char) * (nsize + 1)); // null byte
  memcpy(next, original, csize);
  free(original);
  return next;
}

void append_run(INOUT char **s, INOUT int *csize, INOUT int *maxsize, int run,
                char prev) {
// use the log function slow, but no overallocating
#ifdef C_LOG_USE
  //   @     digits     \0   a/b
  if ((1 + ilog10(run) + 1 + 1) > *csize) {
    s = resize_string(s, *csize, &max);
  }
#else
  //   @  digits a/b
  if (((1 + 10 + 1) + *csize) >= *maxsize) {
    *s = resize_string(*s, *csize, maxsize);
  }
#endif
  (*s)[*csize] = '@';
  (*csize)++;
  int count = snprintf((*s) + (*csize), 10, "%d", run);
  assert(count < 10);
  (*csize) += count;
  (*s)[*csize] = (prev == 0 ? 'a' : 'b');
  (*csize)++;
}

// caller owns return value; must free
// returns NULL if length is 0
char *compress_bin_arr(int arr[], size_t length, unsigned minrun) {
  assert(minrun > 0);
  if (length == 0)
    return NULL;
  if (length == 1)
    return stringify_int(arr[0]);

  int last_run = 0;

  int maxsize = 256;
  char *s = malloc((maxsize + 1) * sizeof(char)); // nullbyte
  int csize = 0;

  int prev = arr[0];
  unsigned run = 1;
  int i;
  for (i = 1; i < length; i++) {
    assert(csize < maxsize);
    if (arr[i] == prev) {
      run++;
    } else {
    almost_done:
      if (run < minrun) {
        if ((run + csize) >= maxsize) {
          s = resize_string(s, csize, &maxsize);
        }
        for (int i = 0; i < run; i++) {
          s[csize + i] = (prev + '0');
        }
        csize += run;
      } else {
        append_run(&s, &csize, &maxsize, run, prev);
      }
      prev = arr[i];
      run = 1;
    }
  }
  if (!last_run) {
    last_run = 1;
    goto almost_done;
  }
  s[csize] = '\0';
  return s;
}

int *resize_int_arr(int *arr, int csize, int *maxsize) {
  int nsize = (int)((*maxsize * 1.51) + 1);
  *maxsize = nsize;
  int *next = malloc(sizeof(int) * (nsize));
  memcpy(next, arr, csize * sizeof(int));
  free(arr);
  return next;
}

// caller owns result. must call free
int *decompress_bin_arr(char *str, size_t len) {
  int *result = malloc(sizeof(int) * 256);
  int maxsize = 256;
  int idx = 0;
  for (int i = 0; i < len; i++) {
    char c = str[i];
    if (c == '@') {
      int w = i + 1;
      while (w < len && str[w] != 'a' && str[w] != 'b') {
        w++;
      }
      char old = *(str + w);
      *(str + w) = '\0';
      int num = atoi((str + i + 1));
      *(str + w) = old;
      int value = str[w] == 'a' ? 0 : 1;

      if ((num + idx) >= maxsize) {
        result = resize_int_arr(result, idx, &maxsize);
      }

      for (int j = 0; j < num; j++)
        result[idx + j] = value;
      i = w; // do NOT pass the a/b part because the for loop is going to i++
      idx += num;
    } else {
      if (idx >= maxsize) {
        result = resize_int_arr(result, idx, &maxsize);
      }
      result[idx++] = str[i] - '0';
    }
  }
  return result;
}

int main(int argc, char const *argv[]) {
  int arr[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,
               0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1};

  int arr2[65536];
  for (int i = 0; i < 65536; i++) {
    arr2[i] = (int)(rand() % 2);
    // printf("%d, ", arr2[i]);
    // arr2[i] = 0;
  }
  arr2[1623] = 1;

  puts("\n");

  char *s = compress_bin_arr(arr, sizeof(arr) / sizeof(int), 4);
  char *r = compress_bin_arr(arr2, sizeof(arr2) / sizeof(int), 4);

  // printf("%s\n", s);
  printf("%s\n", r);

  int *darr = decompress_bin_arr(s, strlen(s));
  int *darr2 = decompress_bin_arr(r, strlen(r));

  for (int i = 0; i < sizeof(arr) / sizeof(int); i++) {
    if (arr[i] != darr[i]) {
      fprintf(stderr, "1. Error at %d: %d != %d\n", i, arr[i], darr[i]);
    }
  }

  for (int i = 0; i < sizeof(arr2) / sizeof(int); i++) {
    if (arr2[i] != darr2[i]) {
      fprintf(stderr, "2. Error at %d: %d != %d\n", i, arr2[i], darr2[i]);
    }
  }

  printf("Original: %d, Compressed: %lu\n", 65536, strlen(r));
  printf("Ratio: %.2lf%%\n", 100 * (1 - (strlen(r) / 65536.0)));

  free(s);
  free(r);

  free(darr);
  free(darr2);

  return 0;
}
