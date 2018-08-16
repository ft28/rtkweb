#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rtkweb.h"


int main(int argc, char* argv[])
{
  int num = 0;
  Record record;

  char *basedir = "./sample";  
  int tracelevel = 0;
  int outstat = 0;

  if (argc == 2) {
    basedir = argv[1];
  } else if (argc == 3) {
    basedir = argv[1];
    tracelevel = atoi(argv[2]);
  } else if (argc == 4) {
    basedir = argv[1];    
    tracelevel = atoi(argv[2]);    
    outstat = atoi(argv[3]);
  }

  open_rtksvr(basedir, tracelevel, outstat);
  start_rtksvr();
  while (num < 1000000) {
    update_record(&record);
    num += 1;
    print_record(&record);
    printf("loop %d\n", num);
    sleep(2);
  }
  stop_rtksvr();
  close_rtksvr(); 

  return 0;
}
