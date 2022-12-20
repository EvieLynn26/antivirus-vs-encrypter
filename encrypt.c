#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <unistd.h>
#include <utime.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int encrypt_files(const char *path, char key)
{
  int status = 0;
  const int n = PATH_MAX;
  struct dirent *entry = NULL;
  DIR *dir = NULL;
  DIR *sub_dir = NULL;
  FILE *fp = NULL;
  unsigned int sz = 0;
  unsigned int sz_read = 0;
  unsigned int sz_write = 0;
  char *fdata = NULL;
  char *abs_path = NULL;
  dir = opendir(path);
  if (dir)
  {
    abs_path = (char *)malloc(n * sizeof(char));
    if (abs_path == NULL)
    {
      status = errno;
      printf("Memory allocation error, error code %d\n", status);
      return status;
    }

    while (entry = readdir(dir))
    {
      sub_dir = NULL;
      fp = NULL;
      memset(abs_path, 0, n);
      if ((entry->d_name[0] == '.' && strlen(entry->d_name) == 1) || ((strlen(entry->d_name) == 2) && (entry->d_name[0] == '.') && (entry->d_name[1] == '.')))
        continue;

      snprintf(abs_path, n, "%s/%s", path, entry->d_name);

      struct stat buff;
      FILE *fp;
      if ((fp = fopen(abs_path, "r")) == NULL)
      {
        printf("Cannot open file\n");
        return 1;
      }
      fstat(fileno(fp), &buff);

      // printf("AFTER LSTAT for file: %s \n", abs_path);

      if (S_ISDIR(buff.st_mode))
      {
        fclose(fp);
        encrypt_files(abs_path, key);
        continue;
      }

      sz = buff.st_size;

      size_t chunk_sz = 1000;

      unsigned int chunk_cnt = 0;
      if (sz % chunk_sz == 0)
      {
        chunk_cnt = sz / chunk_sz;
      }
      else
      {
        chunk_cnt = sz / chunk_sz + 1;
      }
      char *fdata = (char *)calloc(chunk_sz, sizeof(char));
      if (!fdata)
      {
        status = errno;
        printf("Allocation for %d bytes failed wit herror %d!\n", sz, status);
        return status;
      }
      fclose(fp);

      int save_pos = 0;
      for (int i = 0; i < chunk_cnt; ++i)
      {
        // printf("i = %d\n", i);
        if (i == chunk_cnt - 1)
        {
          chunk_sz = sz - chunk_sz * (chunk_cnt - 1);
        }
        // printf("CHUNK SIZE = %ld \n", chunk_sz);

        if ((fp = fopen(abs_path, "r")) == NULL)
        {
          printf("Cannot open file\n");
          return 1;
        }

        fseek(fp, save_pos, SEEK_SET);
        printf("SAVE POS = %d \n", save_pos);

        sz_read = fread(fdata, 1, chunk_sz, fp);

        for (int i = 0; i < chunk_sz; ++i)
        {
          fdata[i] ^= key;
        }

        fclose(fp);

        if ((fp = fopen(abs_path, "w")) == NULL)
        {
          printf("Cannot open file\n");
          return 1;
        }

        fseek(fp, save_pos, SEEK_SET);

        sz_write = fwrite(fdata, 1, chunk_sz, fp);
        save_pos = ftell(fp);

        fclose(fp);
      }
      free(fdata);
    }
    free(abs_path);
  }
  return status;
}

int main(int argc, char *argv[])
{
  int ret = 0;
  /* Check cmd params*/
  if (argc != 2)
  {
    printf("usage: %s path\n", argv[0]);
    return 1;
  }

  const char key = 7;
  ret = encrypt_files(argv[1], key);
  printf("encrypting finished with code %d\n", ret);
  return ret;
}
