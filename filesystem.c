#define FUSE_USE_VERSION 26
#define SIZE 4096
#define _FILE_OFFSET_BITS 64

#define DEBUG 
#ifdef DEBUG
	#define TRACE printf("%d\n", __LINE__);
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

char *binary_path = "/home/lithiumdenis/CSF/Filesystem/MyBinaryFile/bigbinary";

typedef struct ifolder_t
{
	char names[10][64];
	unsigned long nodes[10];
}iFolder;

typedef struct ifile_t
{
	int used_count;
	int total_size;
	unsigned long data[49];	
}iFile;

typedef struct inode_s * inode;
struct inode_s 
{
	int type;
	unsigned long next;
	union {
		iFolder is_folder;
		iFile is_file;
	};
};

typedef struct file_node_s * file_node;
struct file_node_s
{
	char data[SIZE];
	int size;
};

typedef struct node_s * node;
struct node_s 
{
	unsigned long index;
	inode inode;
	node parent;
	node next;
	node childs[10];
	char name[64];
};

struct fs_info_s 
{
	int inode_start;
	int inode_size;
	int data_node_size;
	unsigned long dev_size;
	unsigned long data_start;
}fs_info;

extern char* filesys;
node fs_cash;

FILE* fileSystem() 
{
    FILE* sys = fopen(binary_path, "rb+");
    fseek(sys, 0, SEEK_SET);
    return sys;
}

char** split(char* path) 
{
    char* buf = malloc((strlen(path) + 1) * sizeof(char));
    copyName(buf, path);
    char** res;
    if (strlen(buf) > 1) {
        int count = 0;
        int i = 0;
        while(buf[i] != 0)
            if (buf[i++] == '/') 
                count++;
        res = malloc(sizeof(char*)*(count + 2));
        res[count + 1] = 0;
        res[0] = "/";
        char* pointer = strtok(buf, "/");
        i = 1;
        while(pointer) {
            res[i++] = pointer;
            pointer = strtok(NULL, "/");
        }
    } else {
        res = (char**)malloc(sizeof(char*) * 2);
        res[1] = 0;
        res[0] = "/";
    }
    return res;
}

void copyName(char* dest, char* source) 
{
    int length = strlen(source);
    int i = 0;
    for (; i < length; i++)
        dest[i] = source[i];
    dest[i] = NULL;
}

inode readInode(unsigned long index) 
{
    if (index == NULL) 
    {
        return NULL;
    }
    FILE * sys = fileSystem();
    fseek(sys, index, SEEK_SET);
    inode in = malloc(sizeof(struct inode_s));
    fread(in, sizeof(struct inode_s), 1, sys);
    if (in->type == 0) {
        return NULL;
    }
    fclose(sys);
    return in;
}

node readTree(unsigned long index, node parent, char* name) 
{
    if (index == NULL) 
        return NULL;
    inode in = readInode(index);
    if (in == NULL) {
        return NULL;    
    } 
    inode p = in;
    node head = malloc(sizeof(struct node_s));
    node h = head;
    h->parent = parent;
    h->index = index;
    h->inode = p;
    copyName(h->name, name);
    h->next = readTree(p->next, parent, name);
    if (p->type == 1) {
        for (int i = 0; i < 10; i++) 
        {
            if (p->is_folder.nodes[i] != NULL) 
            {
                h->childs[i] = readTree(p->is_folder.nodes[i], head, p->is_folder.names[i]);
            } else {
                h->childs[i] = NULL;
            }
        }
    }
    return h;
}

void loadFileSystem() 
{
    FILE * sys = fileSystem();
    fread(&fs_info, sizeof(struct fs_info_s), 1, sys);
    fs_cash = readTree(fs_info.inode_start, NULL, "/");
    fclose(sys);
}

void showNode(node nd) 
{
    printf("Node\n");
    printf("Index: %lu\n", nd->index);
    if (nd->inode->type == 1) 
    {
        printf("This is directory\n");
        for (int i = 0; i < 10; i++) 
        {
            printf("Child [%d] is NULL %d\n", i, nd->childs[i] == NULL);
            printf("Child of inode [%d] is NULL %d\n", i, nd->inode->is_folder.nodes[i] == NULL);
            if (nd->inode->is_folder.nodes[i] != NULL) 
                printf("Child's name [%d] %s\n", i, nd->inode->is_folder.names[i]);
        }
    }
}

void saveNode(node nd) 
{
    FILE * sys = fileSystem();
    fseek(sys, nd->index, SEEK_SET);
    if (nd->inode->type != 0)
        fwrite(nd->inode, sizeof(struct inode_s), 1, sys);
    else 
    {
        int stat = NULL;
        fwrite(&stat, 4, 1, sys);
    }
    fclose(sys);
}

void add(node parent, node child) 
{
    int i = 0;
    while (i < 10 && parent->childs[i] != NULL) i++;
    if (i < 10) 
    {
        parent->childs[i] = child;
        child->parent = parent;
        parent->inode->is_folder.nodes[i] = child->index;
        copyName(parent->inode->is_folder.names[i], child->name);
        saveNode(child);
        saveNode(parent);
    }
}

void clearChild(node parent, node child) 
{
    if (parent == NULL) return;
    node n = parent;
    if (parent->inode->type != 1) return;
    do {
        for (int i = 0; i < 10; i++) 
        {
            if (n->inode->is_folder.nodes[i] == child->index) 
            {
                n->inode->is_folder.nodes[i] = NULL;
                n->childs[i] = NULL;
                saveNode(n);
                return;
            }
        }
        n = n->next;
    } while(n != NULL);
}

void delete(node nd) 
{
    clearChild(nd->parent, nd);
    freeNode(nd);
}

node searchChildByName(char* name, node parent) 
{
    node n = parent;
    do 
    {
        for (int i = 0; i < 10; i++) 
        {
            if (n->childs[i] != NULL) 
            {
                if (strcmp(n->childs[i]->name, name) == 0)
                    return n->childs[i];
            }
        }
        n = n->next;
    } while (n != NULL);
    return NULL;
}

node searchParent(char* path) 
{
    char** pathDecomp = split(path);
    int i = 0, count = 0;
    while(pathDecomp[i++] != NULL) 
    {
        count++;
    }
    if (count == 2) 
    {
        if (strcmp(pathDecomp[0], "/") == 0) 
        {
            return fs_cash;
        }
        else 
        {
            return NULL;
        }
    } else 
      {
        printf(">>Search \n");
        node nd = fs_cash;
        i = 1;
        while (pathDecomp[i+1] != NULL) 
        {
            nd = searchChildByName(pathDecomp[i], nd);
            if (nd == NULL) return NULL;
            i++;
        }
        return nd;
    }
}

node searchByName(char* path) 
{
    char** pathDecomp = split(path);
    int i = 0, count = 0;
    while(pathDecomp[i++] != NULL) 
    {
        count++;
    }
    printf("Count %d\n", count);
    if (count == 1) {
        if (strcmp(path, "/") == 0) 
        {
            return fs_cash;
            printf("This is root, inode_start %d\n", fs_info.inode_start);
        }
        else 
        {
            printf("ERROR! Not root\n");
            return NULL;
        }
    } else 
    {
        node folder = searchParent(path);
        if (folder == NULL) return NULL;
        return searchChildByName(pathDecomp[count - 1], folder);
    }
}

file_node dataOfNode(unsigned long index) 
{
    FILE * sys = fileSystem();
    file_node n = malloc(sizeof(struct file_node_s));
    fseek(sys, index, SEEK_SET);
    int stat;
    fread(&stat, 4, 1, sys);
    if (stat == NULL) return NULL;
    fseek(sys, index, SEEK_SET);
    fread(n, sizeof(struct file_node_s), 1, sys);
    fclose(sys);
    return n;
}

unsigned long searchFreeDataNode() 
{
    FILE * sys = fileSystem();
    fseek(sys, fs_info.data_start, SEEK_SET);
    unsigned long pos = fs_info.data_start;
    int stat;
    do 
    {
        fread(&stat, 4, 1, sys);
        pos += fs_info.data_node_size;
        fseek(sys, pos, SEEK_SET);
    } while (stat != NULL);
    fclose(sys);
    return pos - fs_info.data_node_size;
}

void freeNode(node nd) 
{
    do 
    {
        if (nd->inode->type == 1) 
        {
            for (int i = 0; i < 0; i++) 
            {
                if (nd->childs[i] != NULL) 
                {
                    freeNode(nd->childs[i]);
                }
            }
        }
        nd->inode->type = 0;
        saveNode(nd);
        free(nd->inode);
        node next = nd->next;
        free(nd);
        nd = next;
    } while (nd != NULL);
}

file_node dataOfEmptyNode() 
{
    file_node fn = malloc(sizeof(struct file_node_s));
    for (int i = 0; i < SIZE; i++) 
        fn->data[i] = NULL;
    fn->size = 0;
}

void saveData(file_node nd, unsigned long index) 
{
    FILE * sys = fileSystem();
    fseek(sys, index, SEEK_SET);
    fwrite(nd, sizeof(struct file_node_s), 1, sys);
    fclose(sys);
}

node createNodeEmptyInode(inode ind, unsigned long index) 
{
    node head = malloc(sizeof(struct node_s));
    head->index = index;
    head->inode = ind;
    head->parent = NULL;
    head->next = NULL;
    for (int i = 0; i < 10; i++) 
    {
        head->childs[i] = NULL;
    }
    return head;
}

inode emptyInode(int type) 
{
    inode in = malloc(sizeof(struct inode_s));
    in->type = type;
    in->next = NULL;
    if (type == 1) 
    {
        for (int i = 0; i < 10; i++) 
        {
            in->is_folder.nodes[i] = NULL;
        }
    } else if (type == 2) 
      {
        for (int i = 0; i < 49; i++) 
        {
            in->is_file.data[i] = NULL;
        }
        in->is_file.used_count = 0;
        in->is_file.total_size = 0;
    } else 
      {
        return NULL;
      }
    return in;
}


unsigned long searchFreeInode() 
{
    FILE * sys = fileSystem();
    fseek(sys, fs_info.inode_start, SEEK_SET);
    unsigned long pos = fs_info.inode_start;
    int stat;
    do 
    {
        fread(&stat, 4, 1, sys);
        pos += fs_info.inode_size;
        fseek(sys, pos, SEEK_SET);
    } while (stat != NULL);
    fclose(sys);
    return pos - fs_info.inode_size;
}

void FillBinaryFile() 
{
        //Заполняем файл нулями
        FILE* f = fopen(binary_path, "wt");
	for (int i = 0; i < 80000000; i++)
	{
		fprintf(f, "%d\n", 0);
		fprintf(f, "\n");
	}
	fclose(f);

        //Форматируем для работы с iNode
        FILE * sys = fileSystem();
        int zero = NULL;
        fpos_t pos;
        fgetpos(sys, &pos);
        fseek(sys,0,SEEK_END);   
        int size = ftell(sys); 
        fseek(sys, 0, SEEK_SET);
        fs_info.inode_size = sizeof(struct inode_s);
        fs_info.inode_start = sizeof(struct fs_info_s);
        fs_info.dev_size = size;
        fs_info.data_node_size = sizeof(struct file_node_s);
        fs_info.data_start = (int)(size * 0.05);
        fwrite(&fs_info, sizeof(struct fs_info_s), 1, sys);

        float last = 0, old = 0;
        for (int i = fs_info.inode_start; i < fs_info.data_start; i+=fs_info.inode_size) 
        {
            fseek(sys,i,SEEK_SET);  
            fwrite(&zero, 4, 1, sys);
        }
        last = 0;
        for (int i = fs_info.data_start; i < fs_info.dev_size; i+=fs_info.data_node_size) 
        {
            fseek(sys,i,SEEK_SET);
            fwrite(&zero, 4, 1, sys);
        }

        inode ind = emptyInode(1);
        node n = createNodeEmptyInode(ind, fs_info.inode_start);
        saveNode(n);
        fclose(sys);
}

//Изменение размера файла
static int lithiumdenis_truncate(const char * path, off_t newsize) 
{       
        int rt = 0;
        rt = truncate(path, newsize);
	return rt;
}

//Получение атрибутов файла
static int lithiumdenis_getattr(const char *path, struct stat *stbuf) 
{
	node nd = searchByName(path);
	if (nd == NULL) 
        {
		return -ENOENT;
	} 
        else 
        {
	    if (nd->inode->type == 2) 
            {
                //Файл
	        stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		stbuf->st_size = nd->inode->is_file.total_size;
		return 0;
	    } 
            else
            {
	        //Директория
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 3;
		return 0;
            }
	}
}

//Получение содержимого папки
static int lithiumdenis_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) 
{
	node nd = searchByName(path);
	if (nd == NULL) 
        {
	    //Файл не открыт
	    return -ENOENT;
	} 
        else 
        { 
	    if (nd->inode->type == 1) 
            {
	        filler(buf, ".", NULL, 0);	
		filler(buf, "..", NULL, 0);
		node n1 = nd;
		do 
                {
		    for (int i = 0; i < 10; i++) 
                    {
		        if (nd->childs[i] != NULL)
			    filler(buf, nd->childs[i]->name, NULL, 0);
		    }
		    nd = nd->next;
		} while (nd != NULL);
		    return 0;
	    }
	    return -ENOENT;
	}
}

//Создание папки
static int lithiumdenis_mkdir(const char* path, mode_t mode) 
{
	int index = searchFreeInode();
	char** names = split(path);
	int i = 0, count = 0;
	while(names[i++] != NULL) 
	    count++;
	char* name = names[count - 1];
	node parent = searchParent(path);
	inode ind = emptyInode(1);
	node nd = createNodeEmptyInode(ind, index);
	copyName(nd->name, name);
	add(parent, nd);
	return 0;
}

//Свойства открытия файла
static int lithiumdenis_open(const char *path, struct fuse_file_info *fi) 
{
	node nd = searchByName(path);
	if (nd == NULL) 
		return -ENOENT;
	if (nd->inode->type != 2)
		return -ENOENT;
	return 0;
}

//Свойства открытия директории
static int lithiumdenis_opendir(const char *path, struct fuse_file_info *fi) 
{
	node nd = searchByName(path);
	if (nd == NULL) 
		return -ENOENT;
	if (nd->inode->type != 1)
		return -ENOENT;
	return 0;
}

//Удаление директории
static int lithiumdenis_rmdir(const char *path) 
{
	node nd = searchByName(path);
	showNode(nd);
	delete(nd);
	return 0;
}

//Создание
static int lithiumdenis_create(const char *path, mode_t mode, struct fuse_file_info *fi) 
{
	int index = searchFreeInode();
	char** names = split(path);
	int i = 0, count = 0;
	while(names[i++] != NULL)
		count++;
	char* name = names[count - 1];
	node parent = searchParent(path);
	inode in = emptyInode(2);
	node n = createNodeEmptyInode(in, index);
	copyName(n->name, name);
	add(parent, n);
	return 0;
}

//Чтение из файла
static int lithiumdenis_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *finfo) 
{
	node file = searchByName(path);
        //А файл ли это
	if (file -> inode -> type != 2) 
        {
		return -ENOENT;
	}
	int block_num, node_num;
	int node_capacity = SIZE * 49;
	node_num = offset / node_capacity;
	offset -= node_num * node_capacity;
	block_num = offset / SIZE;
	offset -= block_num * SIZE;
	int nind = node_num;
	while (nind > 0) 
        {
		if (file -> next == NULL) 
                {
			//Следующий null
			return 0;
		}
		file = file -> next;
		nind--;
	}
	
	if (file -> inode -> is_file.data[block_num] == NULL) 
        {
		//Базовый не существует
		return 0;
	}
	
	file_node n = dataOfNode(file -> inode -> is_file.data[block_num]);
	if (n == NULL) 
        {
		//Ошибка чтения
		return 0;
	}

	if (n->size == 0) 
        {
		//Пустые данные
		return 0;
	}
	
        //В случае, если всё хорошо
	size_t read_size = size;
	size_t readed_size = 0;
	size_t len = n -> size;
	if (offset < len) 
        {
		do 
                {
			//Начинаем чтение
			if (offset + size > len)
				read_size = len - offset;
			memcpy(buf + readed_size, n->data + offset, read_size);
			size -= read_size;
			readed_size += read_size;
			if (len < SIZE - 1) return readed_size;
			//Данные отправлены
			if ((int)size > 0) 
                        {
				if (block_num < 48) 
                                {
					//Переходим к следующему блоку
					block_num++;
				} 
                                else 
                                {
					//Переходим к следующему inode
					file = file -> next;
					block_num = 0;
					if (file == NULL)
						return readed_size;
				}
				offset = 0;
				if (file->inode->is_file.data[block_num] == NULL) 
                                {
					//Дальнейшие данные не существуют
					return readed_size;
				}
				n = dataOfNode(file->inode->is_file.data[block_num]);
				if (n == NULL) 
                                {
					//Ошибка чтения
					return readed_size;
				}
				if (n -> size == 0) 
                                {
					//Пустые данные
					return readed_size;
				}
			}
		} while ((int)size > 0);
	} 
        else
	    readed_size = 0;
	return readed_size;
}

//Удаление файла
static int lithiumdenis_unlink(const char *path) 
{
	node nd = searchByName(path);
	showNode(nd);
	delete(nd);
	return 0;
}

//Вызывается, когда нет ссылок на открытый файл
static int lithiumdenis_release(const char *path, struct fuse_file_info *fi)
{
	int rt = 0;
	int efile = fi->fh;
        printf("\nrelease(path = \"%s\", fi = 0x%08x)\n", path, fi);
        rt = close(efile);
	return 0;
}

//Делает возможным процесс кэширования
static int lithiumdenis_flush(const char *path, struct fuse_file_info *fi)
{
	int rt = 0;
	printf("\nflush(path = \"%s\", file = 0x%08x)\n", path, fi);
	return rt;
}

//Запись в файл
static int lithiumdenis_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
	node file = searchByName(path);
	node root = file;
	int block_num, node_num;
	int node_capacity = SIZE * 49;
	node_num = offset / node_capacity;
	offset -= node_num * node_capacity;
	block_num = offset / SIZE;
	offset -= block_num * SIZE;
	int nind = node_num;
	
        while (nind > 0) 
        {
		//Для следующего inode
		if (file -> next == NULL) 
                {
			//Создадим inode
			inode in = emptyInode(2);
			unsigned long pos = searchFreeInode();
			if (pos < 0) 
                            return 0;
			node new_node = createNodeEmptyInode(in, pos);
			file -> next = new_node;
			file -> inode->next = pos;
			saveNode(new_node);
			saveNode(file);
			block_num = 0;
		} 
		file = file -> next;
		nind--;
	}
        
        //Проверим, файл ли это
	if (file -> inode -> type != 2) 
        {
		return -ENOENT;
	}
	
	file_node n;
	if (file -> inode -> is_file.data[block_num] == NULL) 
        {
		//Это новый файл
		n = dataOfEmptyNode();
		file -> inode -> is_file.data[block_num] = searchFreeDataNode();
		if (file -> inode -> is_file.data[block_num] < 0) 
                    return 0;
		file -> inode -> is_file.used_count++;
                //Место для записи найдено
	} 
        else 
        {
		//Не новый, просто загрузим
		n = dataOfNode(file -> inode -> is_file.data[block_num]);
	}
	
	if (n == NULL) 
        {
		//Не удалось загрузить
		return 0;
	}

	size_t write_size = size;
	size_t writted_size = 0;
	size_t len = n -> size;
	if (offset <= len) {
		do 
                {
			//Производим запись
			if (size > SIZE)
				write_size = SIZE - offset;
			memcpy(n->data + offset, buf + writted_size, write_size);
			size -= write_size;
			writted_size += write_size;
			n -> size += write_size;
			len = n -> size;
			saveNode(file);
			saveData(n, file->inode->is_file.data[block_num]);
			if (len < SIZE - 1) 
                        {
				root -> inode -> is_file.total_size += writted_size;
				saveNode(root);
				return writted_size;
			}
			//Данные отправлены
			if ((int)size > 0) 
                        {
				if (block_num < 48) 
                                {
					//Переходим к следующему блоку
					block_num++;
				} 
                                else 
                                {
					//Переходим к следующему inode
					inode in = emptyInode(2);
					unsigned long pos = searchFreeInode();
					if (pos < 0) 
                                            return writted_size;
					node new_node = createNodeEmptyInode(in, pos);
					file -> next = new_node;
					file -> inode -> next = pos;
					saveNode(new_node);
					saveNode(file);
					file = new_node;
					block_num = 0;
				}
				offset = 0;
				n = dataOfEmptyNode();
				file -> inode -> is_file.data[block_num] = searchFreeDataNode();
				if (file -> inode -> is_file.data[block_num] < 0) 
                                    return writted_size;
				file -> inode -> is_file.used_count++;
				
			}
		} while ((int)size > 0);
	} 
        else
            writted_size = 0;
	root -> inode -> is_file.total_size += writted_size;
	saveNode(root);
	return writted_size;
}

static struct fuse_operations lithiumdenis_operations = {
	.getattr  = lithiumdenis_getattr,
	.readdir  = lithiumdenis_readdir,
	.mkdir    = lithiumdenis_mkdir,
        .truncate = lithiumdenis_truncate,
        .open     = lithiumdenis_open,
        .opendir  = lithiumdenis_opendir,
        .rmdir    = lithiumdenis_rmdir,
        .create   = lithiumdenis_create,
        .read     = lithiumdenis_read,
        .unlink   = lithiumdenis_unlink,
        .release  = lithiumdenis_release,
        .flush    = lithiumdenis_flush,
        .write    = lithiumdenis_write
};

int main(int argc, char *argv[])
{
        //Проверка наличия параметра - надо ли создавать новую ФС
        if(argc > 2)
        {
            if(strstr(argv[2], "newfs"))
            {
                FillBinaryFile();
            } 
            argc = 2;
        }

        loadFileSystem();
	searchFreeInode();
	return fuse_main(argc, argv, &lithiumdenis_operations, NULL);
}
