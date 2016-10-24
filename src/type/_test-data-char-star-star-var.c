#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *helium_heap_addr[BUFSIZ];
int helium_heap_size[BUFSIZ];
int helium_heap_top = 0;
// the size of the found address. Should reset everytime before checking.
int helium_heap_target_size=-1;

void input() {
  int helium_size;
  char **var;
  
  // HELIUM_INPUT_BEGIN
  scanf("%d", &helium_size);
  if (helium_size == 0) {
    var = NULL;
  } else {
    var = (char**)malloc(sizeof(char*)*helium_size);
    printf("malloc size for addr: %p is %d\n", (void*)var, helium_size);
    helium_heap_addr[helium_heap_top]=var;
    helium_heap_size[helium_heap_top]=helium_size;
    helium_heap_top++;
    for (int i=0,helium_size_tmp=helium_size;i<helium_size_tmp;i++) {
      // PointerType::GetInputCode: var
      scanf("%d", &helium_size);
      if (helium_size == 0) {
        var[i] = NULL;
      } else {
        var[i] = (char*)malloc(sizeof(char)*helium_size);
        printf("malloc size for addr: %p is %d\n", (void*)var[i], helium_size);
        helium_heap_addr[helium_heap_top]=var[i];
        helium_heap_size[helium_heap_top]=helium_size;
        helium_heap_top++;
        scanf("%s", var[i]);
      }
    }
  }
  // HELIUM_INPUT_END
}

void output() {
  char **var=NULL;
  // HELIUM_OUTPUT_BEGIN
  if (var == NULL) {
    printf("isnull_var = %d\n", 1);
    fflush(stdout);
  } else {
    printf("isnull_var = %d\n", 0);
    fflush(stdout);

    helium_heap_target_size = -1;
    for (int i=0;i<helium_heap_top;i++) {
      if (var == helium_heap_addr[i]) {
        helium_heap_target_size = helium_heap_size[i];
        break;
      }
    }
    if (helium_heap_target_size != -1) {
      printf("int_var_heapsize = %d\n", helium_heap_target_size);
      fflush(stdout);
      for (int i=0,helium_heap_target_size_tmp=helium_heap_target_size;i<helium_heap_target_size_tmp;i++) {
        // PointerType::GetOutputCode: var contained type: CharTypelevel: 1
        if (var[i] == NULL) {
          printf("isnull_var[%d] = %d\n", i, 1);
          fflush(stdout);
        } else {
          printf("isnull_var[%d] = %d\n", i, 0);
          fflush(stdout);
          printf("int_var[%d].strlen = %ld\n", i, strlen(var[i]));
          fflush(stdout);
          printf("addr_var[%d] = %p\n", i, (void*)var[i]);
          fflush(stdout);
          helium_heap_target_size = -1;
          for (int ii=0;ii<helium_heap_top;ii++) {
            if (var[i] == helium_heap_addr[ii]) {
              helium_heap_target_size = helium_heap_size[ii];
              break;
            }
          }
          if (helium_heap_target_size != -1) {
            printf("int_var[%d]_heapsize = %d\n", i, helium_heap_target_size);
            fflush(stdout);
            for (int ii=0,helium_heap_target_size_tmp=helium_heap_target_size;ii<helium_heap_target_size_tmp;ii++) {
            }
          }
        }
      }
    }
  }
  // HELIUM_OUTPUT_END
}
