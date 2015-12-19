#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64
#define NLENGTH 512
#define FREEBLOCK 0
#define FOLDERBLOCK 1
#define FILEBLOCK 2

#ifdef DEBUG
#define TRACE printf("[DEBUG] FILE:%s LINE:%d\n", __FILE__, __LINE__);
#else
#define TRACE
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>

static const char *binary_path = "/home/lithiumdenis/CSF/Filesystem/MyBinaryFile/bigbinary";
const int blockSize = 4096;
int fileDest = -1;
const int isRootBlock = 0;

//Данные о файле
typedef struct stat stat_t;
typedef struct node inode_t;
struct node
{
    char name[NLENGTH];
    char data[0];
    char status;
    stat_t stat;
};

//Создаём блок
void *blockCreator()
{
    return calloc(blockSize, sizeof(char));
}

//Записываем блок
int blockWriter(int number, void *block)
{
    int result = -1;
    if (number >= 0 && lseek(fileDest, blockSize * number, SEEK_SET) >= 0)
    {
        if (write(fileDest, block, blockSize) == blockSize)
        {
            result = 0;
        }
    }
    return result;
}

//Создаем корневой элемент
int rootCreator()
{
    int res = -1;
    inode_t *rootNode = (inode_t *)blockCreator();
    if (rootNode != NULL)
    {
        rootNode->name[0] = '\0';
        rootNode->status = FOLDERBLOCK;
        rootNode->stat.st_mode = S_IFDIR | 0777;
        rootNode->stat.st_nlink = 2;
        if (blockWriter(isRootBlock, rootNode) == 0)
        {
            res = 0;
        }
        free(rootNode);
    }
    return res;
}

//Создаём файл или пытаемся работать с известным
void makeBinaryFile()
{
    //Попытаемся открыть файл
    fileDest = open(binary_path, O_RDWR, 0666);
    if (fileDest < 0)
    {
        //Если не нашли, создадим новый
        fileDest = open(binary_path, O_CREAT | O_RDWR, 0666);
        rootCreator();
    }
}

/*//Получение атрибутов файла
static int lithiumdenis_getattr(const char *path, struct stat *stbuf)
{
    inode n = FindINode(path);
    if (n != NULL)
    {
        if (n->type == 1) 
        {
            stbuf->st_mode = 0777 | S_IFDIR;
            stbuf->st_nlink = 2;
            return 0;
        }
    }
    else
        return -ENOENT;
}

//Получение содержимого папки
static int lithiumdenis_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
    inode n = FindINode("/");
    PrintiNode(n);
    if (n == NULL) 
    {
        return -ENOENT;
    } 
    else 
    { 
        if (n->type == 1) 
        {
	        for (int i = 0; i < 10; i++) 
	        {
                if (n->folder.nodes[i] != NULL)
       	         	filler(buf, n->folder.name[i], NULL, 0);
        	}
        	return 0;
        }
        return -ENOENT;
    }
}

//Создание папки
static int lithiumdenis_mkdir(const char* path, mode_t mode) 
{
	inode parent = ReadiNode(fileSystem.istart);

	inode child = malloc(sizeof(struct str_iNode));
	child->type = 1;

	char* name = strtok(path, "/");

	AddChildiNode(parent, fileSystem.istart, child, name);
	PrintiNode(parent);
	return 0;
}*/

//Изменение размера файла
static int lithiumdenis_truncate(const char * path, off_t offset) {
	return 0;
}

static struct fuse_operations lithiumdenis_operations = {
	//.getattr  = lithiumdenis_getattr,
	//.readdir  = lithiumdenis_readdir,
	//.mkdir    = lithiumdenis_mkdir,
        .truncate = lithiumdenis_truncate
};


int main(int argc, char *argv[])
{
        makeBinaryFile();
	return fuse_main(argc, argv, &lithiumdenis_operations, NULL);
}