#include "cachelab.h"
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <getopt.h>

typedef struct{
    int valid;
    unsigned long long tag;
    int lru;
}cache_line_t;

typedef struct{
    cache_line_t* lines;
}cache_set_t;

//typedef cache_set_t* cache_t;

int s,E,b;
int miss_cnt,hit_cnt,evict_cnt;
int lru_cnt=0;
cache_set_t* cache=NULL;
char* trace_file=NULL;
int verbose=0;

cache_set_t* init_cache(){
    int S=1<<s;
    cache_set_t* cache=(cache_set_t*)(malloc(sizeof(cache_set_t)*S));

    for(int i=0;i<S;i++){
        cache[i].lines=(cache_line_t*)(malloc(sizeof(cache_line_t)*E));
        for(int j=0;j<E;j++){
            cache[i].lines[j].valid=0;
            cache[i].lines[j].tag=0;
            cache[i].lines[j].lru=0;
        }
    }

    return cache;
}

void data_access(unsigned long long addr){
    unsigned long long mask=(1ULL<<s)-1;
    unsigned long long index=(addr>>b)&mask;
    unsigned long long tag=addr>>(s+b);
    
    cache_set_t* set=&cache[index];

    for(int i=0;i<E;i++){
        if(set->lines[i].valid&&set->lines[i].tag==tag){
            hit_cnt++;
            set->lines[i].lru=++lru_cnt;
            return;    
        }
    }

    miss_cnt++;

    int target=-1,min_lru=INT_MAX;

    for(int i=0;i<E;i++){
        if(!set->lines[i].valid){
            target=i;
            break;
        }
        if(set->lines[i].valid&&set->lines[i].lru<min_lru){
            target=i;
            min_lru=set->lines[i].lru;
        }
    }

    if(set->lines[target].valid){
        evict_cnt++;
    }

    set->lines[target].valid=1;
    set->lines[target].lru=++lru_cnt;
    set->lines[target].tag=tag;
}

int main(int argc,char** argv)
{
    int opt;
    while((opt=getopt(argc,argv,"s:E:b:t:v"))!=-1){
        switch(opt){
            case 's':s=atoi(optarg);break;
            case 'E':E=atoi(optarg);break;
            case 'b':b=atoi(optarg);break;
            case 't':trace_file=optarg;break;
            case 'v':verbose=1;break;
            default:
                fprintf(stderr, "Usage: %s [-v] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
                exit(1);
        }
    }

    cache=init_cache();

    FILE* fp=fopen(trace_file,"r");
    if(fp==NULL){
        fprintf(stderr,"Error opening file %s\n",trace_file);
        exit(1);
    }

    // char op;
    // unsigned long long addr;
    // int size;

    // while(fscanf(fp,"%c %llx,%d",&op,&addr,&size)==3){
    //     if(op=='I') continue;
    //     switch(op){
    //         case 'L':
    //         case 'S':
    //             data_access(addr);
    //             break;
    //         case 'M':
    //             data_access(addr);
    //             data_access(addr);
    //             break;
    //     }
    // }

    char buf[1000];
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        // 忽略指令加载 'I'（注意 Trace 里通常格式是 " I 0040049d,8"）
        if (buf[1] == 'I') continue; 

        char op;
        unsigned long long addr;
        int size;
        
        // 解析格式：操作符 地址,大小 (例如 L 7ff0005a0,8)
        if (sscanf(buf, " %c %llx,%d", &op, &addr, &size) == 3) {
            switch (op) {
                case 'L':
                case 'S':
                    data_access(addr);
                    break;
                case 'M':
                    data_access(addr);
                    data_access(addr);
                    break;
            }
        }
    }

    fclose(fp);

    printSummary(hit_cnt, miss_cnt, evict_cnt);
    return 0;
}
