#include <stddef.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FLASH_ADDR 0
#define DISK_ADDR 0
#define BLOB_HEADER_SIZE 16

#define BLOB_SIZE_SMALL 48
#define BLOB_SIZE_MIDDLE 240
#define BLOB_SIZE_LARGE 1008

#define BLOB_TYPE_SMALL 3
#define BLOB_TYPE_MIDDLE 2
#define BLOB_TYPE_LARGE 1

#define DISK_BLOB_SIZE 64 * 1024
#define MAGIC_NUMBER 0x12345678
#define DEFAULT_FILE_ID 0xFFFFFFFF
typedef unsigned int disk_addr_t;
typedef unsigned char disk_data_t;
typedef void (*sys_write_imp)(unsigned char *buff, unsigned int start_write_addr, unsigned short write_bytes);

typedef void (*sys_read_imp)(unsigned char *buff, unsigned int start_read_addr, unsigned short write_bytes);

typedef struct _disk_info
{
    unsigned int magic_num;
    disk_addr_t disk_addr;

    unsigned int file_total_count;
    unsigned int file_count;
    disk_addr_t file_addr;

    unsigned int small_blob_count;
    disk_addr_t small_blob_addr;

    unsigned int mid_blob_count;
    disk_addr_t mid_blob_addr;

    unsigned int large_blob_count;
    disk_addr_t large_blob_addr;

    unsigned char extra[24];
} disk_info; //64 bytes

typedef struct _disk_file
{
    unsigned char file_name[32]; //32bytes
    unsigned int file_id;        //4bytes
    unsigned int length;         //4bytes
    disk_addr_t blob_addr;       //4bytes
    union {
        unsigned char data[16];
    } extension;
} disk_file; //64byte

typedef struct _file_blob_header
{
    unsigned int fileid; //4bytes
    union _type_info {
        struct
        {
            unsigned short length;
            unsigned char reserve;
            unsigned char type;
        } type_str;
        //12bits
        unsigned int data;
    } typeinfo;            //2bytes
    disk_addr_t next_blob; //4bytes
    disk_addr_t pre_blob;  //4bytes
} file_blob_header;
//64 bytes
typedef struct _file_blob_small
{
    struct _file_blob_header header;   //14bytes
    disk_data_t blob[BLOB_SIZE_SMALL]; //50bytes
} file_blob_small;
//256 bytes
typedef struct _file_blob_mid
{
    struct _file_blob_header header;    //14bytes
    disk_data_t blob[BLOB_SIZE_MIDDLE]; //50bytes
} file_blob_mid;
//1024 bytes
typedef struct _file_blob_large
{
    struct _file_blob_header header;   //14bytes
    disk_data_t blob[BLOB_SIZE_LARGE]; //50bytes
} file_blob_large;

typedef enum _file_mode
{
    OPEN = 0x01,
    CREATE = 0x01 << 1,
    OPEN_OR_CREATE = OPEN & CREATE,
    TRUNCATE = 0x01 << 2,
    APPEND = 0x01 << 3,
} file_mode;

typedef struct _file
{
    struct _disk *disk_info;
    disk_file file_info;
    int (*write)(struct _file *file, disk_data_t *data, int offset, int length);
    int (*read)(struct _file *file, disk_data_t *data, int offset, int length);
    void (*seek)(struct _file *file, int offset, int pos);
    int read_position;
    int write_postion;
    int file_mode;
} file;

typedef struct _disk
{
    disk_info info;
    file *(*open)(struct _disk *disk, const char *filename, file_mode mode);
    void (*close)(struct _disk *disk, file *file);
} disk;

//1 Blob 64KB , 1 disk_file 64B , 1 Blob = 1024 disk_file
disk *create_disk_info(disk_addr_t start, unsigned short file_blob_count, unsigned short data_blob_count);


void implement_write_method(sys_write_imp write_img);
void implement_read_method(sys_read_imp read_img);