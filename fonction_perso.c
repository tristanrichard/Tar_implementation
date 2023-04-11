#include "lib_tar.h"

/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    if(lseek(tar_fd,0,SEEK_SET)==-1){return -4;}
    int nb_zero=0;
    while(nb_zero<2){
        tar_header_t *header = (tar_header_t*) malloc(512);
        size_t rd = read(tar_fd,header,sizeof(tar_header_t));
        if(rd<512||header==NULL){
            free(header);
            printf("erreur avec read ou malloc");
            return -4;
        }
        //on vérifie si le bloc est vide
        int isZero = 1;

        for(int i=0; i<512; i++){
            unsigned char* bits = (unsigned char*) header;
            if (bits[i] != 0){
                isZero = 0;
                nb_zero = 0;
                break;
            }
        }

        if (isZero){
            nb_zero++;
            continue;
        }
        if(strcmp(header->name,path)==0){//on a trouvé le fichier
            if(header->typeflag==REGTYPE){//le fichier est bien un fichier standart
                if(offset>TAR_INT(header->size)){//offset trop loin
                    free(header);
                    return -2;
                }else{
                    size_t readbytes = TAR_INT(header->size)-offset;//nombre de bits à lire
                    if(readbytes>*len){//buffer pas assez grand
                        rd = read(tar_fd+offset,dest,*len);
                        if(rd==-1){
                            free(header);
                            return -3;
                        }
                        free(header);
                        return readbytes-*len;
                    }else{
                        rd = read(tar_fd+offset,dest,readbytes);
                        if(rd==-1){
                            free(header);
                            return -3;
                        }
                        free(header);
                        *len=readbytes;
                        return 0;
                    }
                }
            }else{//le fichier n'est pas un fichier standart
                free(header);
                return -1;
            }
        }
        //passe au header suivant
        size_t size = TAR_INT(header->size);
        if (size % 512){
            size += 512 - (size % 512);
        }
        if(lseek(tar_fd,size,SEEK_CUR)==-1){
            free(header);
            return -4;
        }
        free(header);        
    }
    return -1;
}


int main(){
    int fd = open("archive.tar",O_RDWR);
    if (fd == -1) {
        perror("open(tar_file)");
        return -1;
    }
    // Allocate an array of char arrays to hold the entry paths
    uint8_t* dest = (uint8_t*) malloc(10000 * sizeof(uint8_t));

    size_t *len = (size_t*) malloc(sizeof(size_t));
    *len=10000;
    int ret = read_file(fd,"lib_tar.c",0,dest,len);

    printf("read returned %d\n", ret);
    printf("len:%d\n", *len);
    free(dest);
    close(fd);
    return 0;
}