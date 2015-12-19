#define FUSE_USE_VERSION 26

#define DEBUG 
#ifdef DEBUG
	#define TRACE printf("%d\n", __LINE__);
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static const char *binary_path = "/home/lithiumdenis/CSF/Filesystem/MyBinaryFile/bigbinary";

typedef struct str_iFolder 
{ 
	char name[10][5];
	int nodes[10];
}iFolder;

typedef struct str_iNode *inode;
struct str_iNode
{
	int type;
	iFolder folder;
};

struct str_FileSystem 
{
	int istart;
	int iend;
	int isize;
}fileSystem;

int KnowSizeFile()
{
	FILE* fs = fopen(binary_path, "rb+");
	fseek(fs, 0, SEEK_END);   
	int size = ftell(fs);
	return size; 	
}

void FillBinaryFile() 
{
        //Заполняем файл нулями
        FILE* f = fopen(binary_path, "wt");
	for (int i = 0; i < 10000000; i++)
	{
		fprintf(f, "%d\n", 0);
		fprintf(f, "\n");
	}
	fclose(f);
        
        //Форматируем для работы с iNode
	int size = KnowSizeFile();
	FILE* fs = fopen(binary_path, "rb+"); 
	fseek(fs, 0, SEEK_SET);
	fileSystem.isize = sizeof(struct str_iNode);
	fileSystem.istart = sizeof(struct str_FileSystem);
	fileSystem.iend = size;
	fwrite(&fileSystem, sizeof(struct str_FileSystem), 1, fs);
	for (int i = fileSystem.istart; i < fileSystem.iend - fileSystem.isize; i += fileSystem.isize) 
	{
		fseek(fs, i, SEEK_SET);  
		int x = 0;
		fwrite(&x, 4, 1, fs);
	}
	int type = 1;
	inode in = malloc(sizeof(struct str_iNode));
	in->type = type;
	for (int i = 0; i < 10; i++) 
	{
		in->folder.nodes[i] = NULL;
	}
	fseek(fs, fileSystem.istart, SEEK_SET);
	fwrite(in, sizeof(struct str_iNode), 1, fs);

	fclose(fs);
}

inode ReadiNode(int index) 
{
    FILE* fs = fopen(binary_path, "rb+");
    fseek(fs, index, SEEK_SET);
    inode in = malloc(sizeof(struct str_iNode));
	fread(in, sizeof(struct str_iNode), 1, fs);
    fclose(fs);
    return in;
}

void PrintiNode(inode n) 
{
    if (n->type == 1) 
    {
        for (int i = 0; i < 10; i++) 
        {
	        printf("%d, %d, %s, %d\n", i, n->folder.nodes[i], n->folder.name[i], n->type);
        }
    }
}

int AddChildiNode(inode parent, int parentIndex, inode child, const char* nameChild)
{
	int pos = FindFreeiNode();
	FILE *fs = fopen(binary_path, "rb+");	
	if (pos < 0 || parent->type != 1)
	{
		return -1;
	}
	int i = 0;
	while (i < 10 && parent->folder.nodes[i] != NULL)
	{
		i++;
	}
	if (i == 10)
	{
		return -1;
	}
	parent->folder.nodes[i] = pos;
	CopyName(parent->folder.name[i], nameChild);	
	fseek(fs, parentIndex, SEEK_SET);
	fwrite(parent, sizeof(struct str_iNode), 1, fs);
	fseek(fs, pos, SEEK_SET);
	fwrite(child, sizeof(struct str_iNode), 1, fs);
	fclose(fs);
	return pos;
}

void CopyName(char* dest, char* source) 
{
    int len = strlen(source);
    int i = 0;
    for (i; i < len; i++)
    {
        dest[i] = source[i];
    }
    dest[i] = NULL;
}

inode FindINode(char* path)
{   
    if (strcmp(path, "/") == 0)
    {
        return ReadiNode(fileSystem.istart);
    }
    else
    {
	if (strcmp(path, "/qq") == 0)
    	{
    		inode n = ReadiNode(fileSystem.istart + fileSystem.isize);
    		if (n->type == 0) 
    			return NULL;
       		return ReadiNode(fileSystem.istart + fileSystem.isize);
    	}
        return NULL;
    }
}

int FindFreeiNode()
{
	FILE *fs = fopen(binary_path, "rb+");
	inode n = malloc(sizeof(struct str_iNode));	
	int pos = fileSystem.istart;
	do
	{
		if (pos >= fileSystem.iend - fileSystem.isize)
		{
			return -1;
		}
		fseek(fs, pos, SEEK_SET);
		fread(n, sizeof(struct str_iNode), 1, fs);
		if (n->type == 0)
		{
			return pos;
		}
		pos += fileSystem.isize;
	}while (pos < fileSystem.iend);
	return pos;
}

void LoadFileSystem() 
{
    FILE* fs = fopen(binary_path, "rb+");
    fseek(fs, 0, SEEK_SET);
    fread(&fileSystem, sizeof(struct str_FileSystem), 1, fs);
    fclose(fs);
}

//Получение атрибутов файла
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
}

//Изменение размера файла
static int lithiumdenis_truncate(const char * path, off_t offset) {
	return 0;
}

static struct fuse_operations lithiumdenis_operations = {
	.getattr  = lithiumdenis_getattr,
	.readdir  = lithiumdenis_readdir,
	.mkdir    = lithiumdenis_mkdir,
        .truncate = lithiumdenis_truncate
};


int main(int argc, char *argv[])
{
        FillBinaryFile();
        LoadFileSystem();
	return fuse_main(argc, argv, &lithiumdenis_operations, NULL);
}