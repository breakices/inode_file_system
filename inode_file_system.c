#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 512
#define MAX_FILENAME_LENGTH 12
#define ENTRY_NUMBER 32
#define BLOCK_NUMBER 128
#define INODE_NUMBER 128

struct Inode {
    int inodeNumber;  // 索引节点号
    int blockID;  // 数据块号
    int fileType;  // 文件类型：0表示目录，1表示普通文件
};

struct DirectoryBlock {
    char fileName[ENTRY_NUMBER][MAX_FILENAME_LENGTH];  // 文件名数组
    int inodeID[ENTRY_NUMBER];  // 索引节点号数组
    //inodeID[i]===数组中索引i位置上的Inode的inodeNumber
};

struct FileBlock {
    char content[BLOCK_SIZE];  // 文件内容
};

struct Inode inodeMem[INODE_NUMBER];  // 索引节点内存
struct FileBlock blockMem[BLOCK_NUMBER];  // 数据块内存
char blockBitmap[BLOCK_NUMBER / 8];  // 数据块位图

//关于各个结构体的关系
//总之，inodeID 用于识别和访问特定的 Inode 结构体，
//而 blockID 是 Inode 结构体中的字段，用于指向包含文件内容或目录列表的实际数据块。
//通过 inodeID 可以找到其对应的 Inode，
//然后通过 Inode 的 blockID 可以找到存储数据的 FileBlock 或 DirectoryBlock。
//这是一个两级查找过程，首先通过 inodeID 定位索引节点，然后通过索引节点中的 blockID 定位数据。

void createDirectory() {
    int i, j, flag;
    char path[100];
    char* directory, * parent, * target;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    printf("Input the path:\n");
    scanf("%s", path);

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("It is the root directory!\n");
        return;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或不是目录文件
                printf("The path does not exist!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 创建目标目录
    int entry = -1, n_block = -1, n_inode = -1;
    target = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], target) == 0) {
            printf("The directory already exists!\n");
            return;
        }
        if (block->inodeID[i] == -1) {//找到空的block可以用来创建目录
            entry = i;
            break;
        }
    }
    if (entry >= 0) {
        // 找到一个未使用的数据块
        for (i = 0; i < BLOCK_NUMBER / 8; i++) {
            for (j = 0; j < 8; j++) {
                if ((blockBitmap[i] & (1 << j)) == 0) {
                    n_block = i * 8 + j;
                    break;
                }
            }
            if (n_block != -1) {
                break;
            }
        }
        if (n_block == -1) {
            printf("The block is full!\n");
            return;
        }

        // 找到一个未使用的索引节点
        flag = 0;
        for (i = 0; i < INODE_NUMBER; i++) {
            if (inodeMem[i].blockID == -1) {
                flag = 1;
                inodeMem[i].blockID = n_block;
                inodeMem[i].fileType = 0;
                n_inode = i;
                break;
            }
        }
        if (n_inode == -1) {
            printf("The inode is full!\n");
            return;
        }

        // 初始化新目录文件
        block->inodeID[entry] = n_inode;
        strcpy(block->fileName[entry], target);
        blockBitmap[n_block / 8] |= 1 << (n_block % 8);
        block = (struct DirectoryBlock*)&blockMem[n_block];
        for (i = 0; i < ENTRY_NUMBER; i++) {
            block->inodeID[i] = -1;
        }
    }
    else {
        printf("The directory is full!\n");
    }
    return;
}

//deleteDirectory函数的两个辅助函数
void deleteFileByInode(int inodeID) {
    // 清除文件项和释放索引节点和数据块
    int blockID = inodeMem[inodeID].blockID;
    inodeMem[inodeID].blockID = -1;
    inodeMem[inodeID].fileType = -1;

    memset(&blockMem[blockID], 0, sizeof(struct FileBlock));
    blockBitmap[blockID / 8] &= ~(1 << (blockID % 8));
}

void recursiveDelete(int inodeID) {
    struct DirectoryBlock* block = (struct DirectoryBlock*)&blockMem[inodeMem[inodeID].blockID];

    // 递归删除子目录和文件
    for (int i = 0; i < ENTRY_NUMBER; i++) {
        if (block->inodeID[i] != -1) {
            if (inodeMem[block->inodeID[i]].fileType == 1) {
                // 如果是文件，删除文件
                deleteFileByInode(block->inodeID[i]);
            }
            else {
                // 如果是目录，递归删除目录
                recursiveDelete(block->inodeID[i]);
            }
        }
    }

    // 清除目录项和释放索引节点和数据块
    int blockID = inodeMem[inodeID].blockID;
    inodeMem[inodeID].blockID = -1;
    inodeMem[inodeID].fileType = -1;

    memset(&blockMem[blockID], 0, sizeof(struct FileBlock));
    blockBitmap[blockID / 8] &= ~(1 << (blockID % 8));
}

void deleteDirectory() {
    char path[100];
    char* directory, * parent, * target;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    printf("Input the path to the directory to delete:\n");
    scanf("%s", path);

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("It is the root directory and cannot be deleted!\n");
        return;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            int flag = 0;
            for (int i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                printf("The path does not exist or is not a directory!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 删除目标目录及其内容
    int entry = -1;
    target = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (int i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], target) == 0) {
            entry = i;
            break;
        }
    }
    if (entry < 0) {
        printf("The directory does not exist!\n");
        return;
    }

    int inodeID = block->inodeID[entry];

    if (inodeMem[inodeID].fileType == 1) {
        // 如果是文件，删除文件
        deleteFileByInode(inodeID);
    }
    else {
        // 如果是目录，递归删除目录
        recursiveDelete(inodeID);
    }

    // 清除目录项
    block->inodeID[entry] = -1;
    strcpy(block->fileName[entry], "");

    printf("Directory '%s' has been deleted!\n", target);
}


void listFiles() {
    int i, flag;
    char path[100];
    char* directory;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;  // Start from the root inode
    struct DirectoryBlock* block;

    printf("Input the path:\n");
    scanf("%s", path);
    printf("The directory includes following files\n");

    directory = strtok(path, delimiter);  // Get the first directory in the path
    while (directory != NULL) {
        block = (struct DirectoryBlock*)&blockMem[pointer->blockID];  // Get the block of the current directory
        flag = 0;
        for (i = 0; i < ENTRY_NUMBER; i++) {
            // Search for the directory name in the current directory block
            if (strcmp(block->fileName[i], directory) == 0) {
                flag = 1;  // Found the directory
                pointer = &inodeMem[block->inodeID[i]];  // Point to the inode of the found directory
                break;
            }
        }
        if (flag == 0 || pointer->fileType == 1) {
            // If the directory does not exist or it is not a directory
            printf("The path does not exist or it is not a directory!\n");
            return;
        }
        directory = strtok(NULL, delimiter);  // Get the next part of the path
    }

    // Now pointer points to the inode of the final directory in the path
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];  // Get the block of the final directory
    printf("INode\tisDir\tFile Name\n");
    for (i = 0; i < ENTRY_NUMBER; i++) {
        // List all files and directories in the final directory block
        if (block->inodeID[i] != -1) {  // Check if the inodeID is valid
            struct Inode *fileInode = &inodeMem[block->inodeID[i]];  // Get the inode of the file/directory
            printf("%d\t%d\t%s\n", fileInode->inodeNumber, 1 - fileInode->fileType, block->fileName[i]);  // Print the details
        }
    }
}


void createFile() {
    int i, flag;
    char path[100];
    char* directory, * filename, * parent;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    printf("Input the path for the new file:\n");
    scanf("%s", path);

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("The root directory does not allow file creation!\n");
        return;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或不是目录文件
                printf("The path does not exist or is not a directory!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 创建新文件
    int entry = -1, n_inode = -1, n_block = -1;
    filename = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], filename) == 0) {
            printf("The file already exists!\n");
            return;
        }
        if (block->inodeID[i] == -1) {
            entry = i;
            break;
        }
    }
    if (entry >= 0) {
        // 找到一个未使用的索引节点
        for (i = 0; i < INODE_NUMBER; i++) {
            if (inodeMem[i].blockID == -1) {
                // 找到一个未使用的数据块
                for (int j = 0; j < BLOCK_NUMBER; j++) {
                    int byteIndex = j / 8;
                    int bitIndex = j % 8;
                    if (!(blockBitmap[byteIndex] & (1 << bitIndex))) {
                        n_block = j;
                        blockBitmap[byteIndex] |= (1 << bitIndex); // 标记数据块为已使用
                        break;
                    }
                }
                if (n_block == -1) {
                    printf("No blocks available.\n");
                    return;
                }
                inodeMem[i].fileType = 1; // 设置文件类型为普通文件
                inodeMem[i].blockID = n_block; // 分配数据块
                n_inode = i; // 记录索引节点号
                break;
            }
        }
        if (n_inode == -1) {
            printf("The inode is full!\n");
            return;
        }

        // 初始化新文件项
        block->inodeID[entry] = n_inode;
        strcpy(block->fileName[entry], filename);
    }
    else {
        printf("The directory is full!\n");
    }
}


void deleteFile() {
    int i;
    char path[100];
    char* directory, * filename, * parent;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    printf("Input the path of the file to be deleted:\n");
    scanf("%s", path);

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("The root directory cannot be deleted!\n");
        return;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            int flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                printf("The path does not exist or is not a directory!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 删除目标文件
    int entry = -1;
    filename = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], filename) == 0) {
            entry = i;
            break;
        }
    }
    if (entry >= 0) {
        int inodeID = block->inodeID[entry];
        int blockID = inodeMem[inodeID].blockID;

        if (inodeMem[inodeID].fileType != 1) {
            printf("The path is not a file!\n");
            return;
        }

        // 清除文件内容
        memset(&blockMem[blockID], 0, sizeof(struct FileBlock));

        // 更新位图以表示数据块现在是空闲的
        int byteIndex = blockID / 8;
        int bitOffset = blockID % 8;
        blockBitmap[byteIndex] &= ~(1 << bitOffset);

        // 清除目录项
        block->inodeID[entry] = -1;
        memset(block->fileName[entry], 0, MAX_FILENAME_LENGTH);

        // 重置Inode
        inodeMem[inodeID].blockID = -1;
        inodeMem[inodeID].fileType = -1; // -1 或其他特定值表示这个inode未被使用

        printf("File '%s' has been deleted.\n", filename);
    }
    else {
        printf("The file does not exist!\n");
    }
}


void readFile() {
    int i, flag;
    char path[100];
    char* filename, * directory, * parent;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    printf("Input the path of the file to be read:\n");
    scanf("%s", path);

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("The root directory cannot be read!\n");
        return;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或不是目录文件
                printf("The path does not exist or is not a directory!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 读取目标文件
    int entry = -1;
    filename = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], filename) == 0) {
            entry = i;
            break;
        }
    }
    if (entry >= 0) {
        int inodeID = block->inodeID[entry];
        if (inodeMem[inodeID].fileType == 0) {
            printf("The specified path is not a file!\n");
            return;
        }
        struct FileBlock* fileBlock = (struct FileBlock*)&blockMem[inodeMem[inodeID].blockID];
        printf("File content:\n%s\n", fileBlock->content);
    }
    else {
        printf("The file does not exist!\n");
    }
}

void writeFile() {
    int i, flag;
    char path[100];
    char* filename, * directory, * parent;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    printf("Input the path of the file to be written:\n");
    scanf("%s", path);

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("The root directory cannot be written!\n");
        return;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或不是目录文件
                printf("The path does not exist or is not a directory!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 写入目标文件
    int entry = -1;
    filename = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], filename) == 0) {
            entry = i;
            break;
        }
    }
    if (entry >= 0) {
        int inodeID = block->inodeID[entry];
        if (inodeMem[inodeID].fileType == 0) {
            printf("The specified path is not a file!\n");
            return;
        }
        struct FileBlock* fileBlock = (struct FileBlock*)&blockMem[inodeMem[inodeID].blockID];
        printf("Enter the content to be written (up to %d characters):\n", BLOCK_SIZE);
        scanf(" %[^\n]s", fileBlock->content);
        printf("File content has been updated.\n");
    }
    else {
        printf("The file does not exist!\n");
    }
}

// 保存快照到文件
void saveSnapshot(const char* snapshotFile) {
    FILE* file = fopen(snapshotFile, "wb");
    if (file == NULL) {
        printf("Failed to open snapshot file for writing.\n");
        return;
    }

    // 写入inode数组
    fwrite(inodeMem, sizeof(struct Inode), INODE_NUMBER, file);

    // 写入文件/目录块数组
    fwrite(blockMem, sizeof(struct FileBlock), BLOCK_NUMBER, file);

    // 写入数据块位图
    fwrite(blockBitmap, sizeof(blockBitmap), 1, file);

    fclose(file);
    printf("Snapshot saved to '%s'\n", snapshotFile);
}

// 从快照文件恢复
void restoreFromSnapshot(const char* snapshotFile) {
    FILE* file = fopen(snapshotFile, "rb");
    if (file == NULL) {
        printf("Failed to open snapshot file for reading.\n");
        return;
    }

    // 读取inode数组
    fread(inodeMem, sizeof(struct Inode), INODE_NUMBER, file);

    // 读取文件/目录块数组
    fread(blockMem, sizeof(struct FileBlock), BLOCK_NUMBER, file);

    // 读取数据块位图
    fread(blockBitmap, sizeof(blockBitmap), 1, file);

    fclose(file);
    printf("File system restored from '%s'\n", snapshotFile);
}

int main() {
    // Initialization
    int i;
    for (i = 0; i < INODE_NUMBER; i++) {
        inodeMem[i].inodeNumber = i;
        inodeMem[i].blockID = -1;
    }
    inodeMem[0].blockID = 0;
    blockBitmap[0] |= 1;
    struct DirectoryBlock* root = (struct DirectoryBlock*)&blockMem[0];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        root->inodeID[i] = -1;
    }

    int running = 1;
    int choice;
    while (running == 1) {
        printf("The system supports following commands:\n"
            "1. Create a directory\n"
            "2. Delete a directory\n"
            "3. List Files\n"
            "4. Create a file\n"
            "5. Delete a file\n"
            "6. Read a file\n"
            "7. Write a file\n"
            "8. Save snapshot\n"
            "9. Restore from snapshot\n"
            "0. Exit\n"
            "Input your command number:\n"
        );
        scanf("%i", &choice);
        switch (choice)
        {
        case 1:
            createDirectory();
            break;

        case 2:
            deleteDirectory();
            break;

        case 3:
            listFiles();
            break;

        case 4:
            createFile();
            break;

        case 5:
            deleteFile();
            break;

        case 6:
            readFile();
            break;

        case 7:
            writeFile();
            break;

        case 8:
            saveSnapshot("filesystem_snapshot.bin");
            break;

        case 9:
            restoreFromSnapshot("filesystem_snapshot.bin");
            break;

        case 0:
            running = 0;
            break;

        default:
            printf("Please input a valid command number\n");
            break;
        }
    }

    return 0;
}