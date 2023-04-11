#include "lib_tar.h"

/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 *         -4 problème avec une fonction interne(read ou malloc)
 */
int check_archive(int tar_fd) {//correct
    int nb_headers = 0;//commence à 0 parce que contient tj un header null pour spécifier la fin de l'archive
    int nb_zero=0;
    lseek(tar_fd,0,SEEK_SET);
    while (nb_zero<2) {
        tar_header_t *header =(tar_header_t*) malloc(512);
        size_t rd= read(tar_fd, header, sizeof(tar_header_t));
        if(rd==-1||header==NULL){
            free(header);
            printf("erreur dans read ou malloc");
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

        //Verifie la magic value
        if(strncmp(header->magic, TMAGIC, TMAGLEN) != 0) {
            free(header);
            return -1;
        }

        //Verifie la version value
        if (strncmp(header->version, TVERSION, TVERSLEN) != 0) {
            free(header);
            return -2;
        }

        //On vérifie la checksum
        int checksum = 0;
        int checksum_true = TAR_INT(header->chksum);
        for(int i = 0;i<8;i++){// on remplace les bits de checksum par des espaces car lorsque que checksum est calculé il ne connait pas sa propre valeur
            header->chksum[i]=' ';
        }
        for (size_t i = 0; i < sizeof(tar_header_t); i++) {
            checksum += ((char*) header)[i];
        }
        if (checksum_true != checksum) {
            free(header);
            return -3;
        }
        //On passe au header suivant
        size_t size = TAR_INT(header->size);
        if (size % 512){
            size += 512 - (size % 512);
        }
        lseek(tar_fd,size,SEEK_CUR);
        free(header);
        nb_headers++;
    }
   return nb_headers;
}

/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path) {//est ce qu'on doit trouver un fichier dans un directory?
    if(lseek(tar_fd,0,SEEK_SET)==-1){return -4;}
    int nb_zero=0;
    while(nb_zero<2){
        tar_header_t *header = (tar_header_t*) malloc(512);
        size_t rd = read(tar_fd,header,sizeof(tar_header_t));
        if(rd<512||header==NULL){
            free(header);
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

        if(strcmp(header->name,path)==0){
            return 1;//on a trouvé le fichier
        }
        //si c'est on trouve un directory et qu'on doit les gérer il faudrait sauvegarder dir/ et relancer la recherche sur dir/path
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
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path) {
    if(lseek(tar_fd,0,SEEK_SET)==-1){return -4;}
    int nb_zero=0;
    while(nb_zero<2){
        tar_header_t *header = (tar_header_t*) malloc(512);
        size_t rd = read(tar_fd,header,sizeof(tar_header_t));
        if(rd<512||header==NULL){
            free(header);
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


        if(strcmp(header->name,path)==0){//on a trouvé le fichier, format d'un directory: path=dir/
            if(header->typeflag==DIRTYPE){
                free(header);
                return 1;//le fichier est bien un directory
            }else{
                free(header);
                return 0;//le fichier n'est pas un directory
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
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path) {
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
            if(header->typeflag==REGTYPE){
                free(header);
                return 1;//le fichier est bien un fichier standart
            }else{
                free(header);
                return 0;//le fichier n'est pas un fichier standart
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
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path) {
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
            if(header->typeflag==SYMTYPE){
                free(header);
                return 1;//le fichier est bien un symlink
            }else{
                free(header);
                return 0;//le fichier n'est pas un symlink
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
    return 0;
}

/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */
int list(int tar_fd, char *path, char **entries, size_t *no_entries) {
    int tar_fd_init=tar_fd;
    if(lseek(tar_fd,0,SEEK_SET)==-1){return -4;}
    int nb_zero=0;
    bool find = false;
    char *previous =(char*) malloc(100);// un name fait 100 
    *previous=' ';
    int i=0;
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

        if(find){
            if(strncmp(header->name,path,strlen(path))==0&&strncmp(header->name,previous,strlen(previous))!=0){//on vérifie si on le header commence par dir/
                if(header->typeflag==DIRTYPE){
                    strcpy(previous,header->name);//on ne veut pas prendre les éléments de ce sous-dossier
                }
                if(i==*no_entries){
                    free(header);
                    break;
                }
                strcpy(entries[i],header->name);
                i++;
            }if(strncmp(header->name,path,strlen(path))!=0){
                free(header);
                free(previous);
                *no_entries=i;
                return 1;
            }
        }

        if(strcmp(header->name,path)==0&&!find){//on a trouvé le fichier
            if(header->typeflag==DIRTYPE){
                find=true;
            }else if(header->typeflag==SYMTYPE){
                char *name = malloc(strlen(header->linkname) + 2);
                strcpy(name,header->linkname);
                strcat(name,"/");
                free(header);
                free(previous);
                return list(tar_fd_init,name,entries,no_entries);//on relance la recherhe

            }else{
                free(header);
                free(previous);
                *no_entries=i;
                return 0;
            }
        }
        //passe au header suivant
        size_t size = TAR_INT(header->size);
        if (size % 512){
            size += 512 - (size % 512);
        }
        if(lseek(tar_fd,size,SEEK_CUR)==-1){
            free(header);
            free(previous);
            *no_entries=i;
            return -4;
        }
        free(header);        
    }
    *no_entries=i;
    free(previous);
    return 0;
}

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
                        lseek(tar_fd,offset,SEEK_CUR);
                        rd = read(tar_fd,dest,*len);
                        if(rd==-1){
                            free(header);
                            return -3;
                        }
                        free(header);
                        return readbytes-*len;
                    }else{
                        lseek(tar_fd,offset,SEEK_CUR);
                        rd = read(tar_fd,dest,readbytes);
                        if(rd==-1){
                            free(header);
                            return -3;
                        }
                        free(header);
                        *len=readbytes;
                        return 0;
                    }
                }
            }if(header->typeflag==SYMTYPE){
                char *name = malloc(strlen(header->linkname) + 2);
                strcpy(name,header->linkname);
                free(header);
                return read_file(tar_fd,name,offset,dest,len);//on relance
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

