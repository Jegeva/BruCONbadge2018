#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc,char** argv)
{
    int fd = open(argv[1],O_RDONLY);
    struct stat mstat;
    fstat(fd,&mstat);
    unsigned char * mem = (char*)calloc(mstat.st_size,sizeof(char));

    read(fd,mem,mstat.st_size);
    unsigned char * ptr = mem ;
    uint16_t res,res0,res1;
    int i;

    i=122;
    printf("uint16_t %s[%d]={\n",argv[1],(mstat.st_size-122)/3);

    while( i < (mstat.st_size-3) ){
        res = (*(ptr+i)>>2) | (*(ptr+i+1)>>2)<<4 | (*(ptr+i+2)>>2)<<8  ;

        printf("0x%x",res);

        i+=3;
        if(i<mstat.st_size-3)
            putchar(',');
        if((i-122)%131==0)
            putchar('\n');
    }
    printf("};\n");

    free(mem);
    close(fd);

}
