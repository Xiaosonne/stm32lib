#include "file.h"

static sys_write_imp sys_write = NULL;
static sys_read_imp sys_read = NULL;
file *open(disk *disk, const char *filename, file_mode mode);
disk_addr_t get_append_address(file *file, file_blob_header *header, unsigned int left_size, unsigned int *size, unsigned char *type);
void file_close(disk *disk, file *file);
void file_truncate(file *file);
void file_append(file *file);
void file_seek(file *file, int offset, int pos);
void init_disk_info(disk *disk);
/************
 * write from "data+offset" to "file" ,total "length" bytes
 * **********/
int file_write(file *file, disk_data_t *data, int offset, int length);
/************
 * read from "file" to "data+offset" ,total "length" bytes
 * **********/
int file_read(file *file, disk_data_t *data, int offset, int length);

void init_disk_info(disk *disk)
{
    disk_info info;
    memset(&info, 0, sizeof(info));
    sys_read((disk_data_t *)(&info), disk->info.disk_addr, sizeof(disk_info));
    if (info.magic_num == MAGIC_NUMBER && disk->info.disk_addr == info.disk_addr)
    {
        disk->info = info;
        return;
    }
    sys_write((disk_data_t *)(&disk->info), disk->info.disk_addr, sizeof(disk_info));
}
//Blob  1   16  sectors
//sector 1 512B disk_info   3584B disk_files    (56 files)
//sector 2                  4096B disk_files    (64 files)
//........                  4096B disk_files    (64 files)
//sector 16                 4096B disk_files    (64 files)
//total                                         (1016 files)

//Blob  2   16  sectors                         (1024 files)
//Blob  3   16  sectors                         (1024 files)
//Blob  4   16  sectors                         (1024 files)
disk *create_disk_info(unsigned int start, unsigned short file_blob_count, unsigned short data_blob_count)
{
    disk *dsk = (disk *)malloc(sizeof(disk));
    printf("disk size %d", sizeof(disk));
    dsk->open = open;
    dsk->close = file_close;
    disk_info *info = &dsk->info;
    info->disk_addr = start;
    info->magic_num = MAGIC_NUMBER;
    info->file_count = 1016 + (file_blob_count - 1) * 1024;
    info->file_total_count = 0;
    info->file_addr = start + 512;
    printf(" addr open addr %x\r\n", (int)(void *)dsk->open);
    printf(" addr close addr %x\r\n", (int)(void *)dsk->close);
    printf(" info->file_total_count %d\r\n", info->file_total_count);
    //1 sector  1024B    64 small   blob
    //1 sector  1024B    4  mid     blob
    //1 sector  1024B    1  large   blob
    //1 Blob = 1024 files=16 sector = 1024 small    blob
    //1 Blob = 1024 files=16 sector = 64   mid      blob
    //1 Blob = 1024 files=16 sector = 16   large    blob
    unsigned short count = (data_blob_count - (data_blob_count % 3)) / 3;
    info->small_blob_count = count;
    info->small_blob_addr = count * DISK_BLOB_SIZE + info->file_addr;

    info->mid_blob_count = count;
    info->mid_blob_addr = count * DISK_BLOB_SIZE + info->small_blob_addr;

    info->large_blob_count = count;
    info->large_blob_addr = count * DISK_BLOB_SIZE + info->mid_blob_addr;
    printf(" info->file_count\t%d\r\n", info->file_count);
    printf(" info->file_addr\t%d\r\n", info->file_addr);
    printf(" info->small_blob_count\t%d %d\r\n", info->small_blob_count, info->small_blob_addr);
    printf(" info->mid_blob_count\t%d %d\r\n", info->mid_blob_count, info->mid_blob_addr);
    printf(" info->large_blob_count\t%d %d\r\n", info->large_blob_count, info->large_blob_addr);
    init_disk_info(dsk);
    return dsk;
}

void implement_write_method(sys_write_imp write_img)
{
    sys_write = write_img;
}
void implement_read_method(sys_read_imp read_img)
{
    sys_read = read_img;
}
void file_close(disk *disk, file *file)
{
    free(file);
}
file *new_file_obj(disk *disk, file_mode mode)
{
    file *fret = (file *)malloc(sizeof(file));
    fret->disk_info = disk;
    fret->read = file_read;
    fret->write = file_write;
    fret->seek = file_seek;
    fret->read_position = 0;
    fret->write_postion = 0;
    fret->file_mode = mode;
    memset(&fret->file_info, 0, sizeof(fret->file_info));
    return fret;
}
void delete_file_obj(file *file)
{
    free(file);
}
file *open(disk *disk, const char *filename, file_mode mode)
{
    switch (mode)
    {
    case CREATE:
    case OPEN:
    {
        file *fret = new_file_obj(disk, mode);
        if (mode == OPEN)
        {
            if (disk->info.file_total_count <= 0)
            {
                free(fret);
                return NULL;
            }
        }
        int file_index = 0;
        while (file_index < disk->info.file_total_count)
        {
            sys_read((disk_data_t *)&(fret->file_info), disk->info.file_addr, sizeof(disk_file));
            if (strcasecmp((const char *)fret->file_info.file_name, filename) == 0)
            {
                return fret;
            }
            file_index++;
        }
        delete_file_obj(fret);
        return NULL;
    }
    case OPEN_OR_CREATE:
    {
        file *ret1 = open(disk, filename, OPEN);
        if (ret1 != NULL)
        {
            return ret1;
        }
        file *fret = new_file_obj(disk, mode);
        fret->file_mode = OPEN_OR_CREATE;
        strcpy((char *)fret->file_info.file_name, filename);                                              //1 复制文件名
        fret->file_info.file_id = disk->info.file_addr + disk->info.file_total_count * sizeof(disk_file); //2 赋值文件地址
        sys_write((unsigned char *)&fret->file_info, fret->file_info.file_id, sizeof(fret->file_info));   //3 写入磁盘信息
        disk->info.file_total_count++;                                                                    //4 更新磁盘文件
        sys_write((unsigned char *)&disk->info, disk->info.disk_addr, sizeof(disk->info));                //5 保存磁盘信息
        return fret;
    }
    case TRUNCATE:
    {
        file *ret1 = open(disk, filename, OPEN);
        if (ret1 == NULL)
        {
            return NULL;
        }
        file_truncate(ret1);
        return ret1;
    }
    case APPEND:
    {
        file *ret1 = open(disk, filename, OPEN);
        if (ret1 == NULL)
        {
            return NULL;
        }
        file_append(ret1);
        return ret1;
    }
    break;
    }
    return NULL;
}

disk_addr_t get_unwrite_addr(file *file, file_blob_header *empty_blob_header, unsigned int blobSize)
{
    int blob = 0;
    char find = 0;
    disk_addr_t addr0 = 0;
    disk_addr_t base_addr = 0;
    int base_blob_count;
    int base_blob_type = 0;
    switch (blobSize)
    {
    case BLOB_SIZE_LARGE:
        base_blob_type = BLOB_TYPE_LARGE;
        base_addr = file->disk_info->info.large_blob_addr;
        base_blob_count = file->disk_info->info.large_blob_count;
        break;
    case BLOB_SIZE_MIDDLE:
        base_blob_type = BLOB_TYPE_MIDDLE;
        base_addr = file->disk_info->info.mid_blob_addr;
        base_blob_count = file->disk_info->info.mid_blob_count;

        break;
    case BLOB_SIZE_SMALL:
        base_blob_type = BLOB_TYPE_SMALL;
        base_addr = file->disk_info->info.small_blob_addr;
        base_blob_count = file->disk_info->info.small_blob_count;
        break;
    }

    addr0 = base_addr + blob * 1024;
    while (blob < base_blob_count)
    {
        sys_read((disk_data_t *)empty_blob_header, addr0, sizeof(file_blob_header));
        if (empty_blob_header->fileid == DEFAULT_FILE_ID || empty_blob_header->fileid == file->file_info.file_id && empty_blob_header->typeinfo.type_str.length < blobSize)
        {
            if (empty_blob_header->fileid == DEFAULT_FILE_ID)
            {
                empty_blob_header->typeinfo.type_str.type = base_blob_type;
            }
            find = 1;
            break;
        }
        blob++;
        addr0 = base_addr + blob * 1024;
    }

    return find == 1 ? addr0 : 0;
}
unsigned short get_blob_size_by_type(unsigned char type)
{
    unsigned short blob_size = 0;
    switch (type)
    {
    case BLOB_TYPE_LARGE:
        blob_size = BLOB_SIZE_LARGE;
        break;
    case BLOB_TYPE_MIDDLE:
        blob_size = BLOB_SIZE_MIDDLE;
        break;
    case BLOB_TYPE_SMALL:
        blob_size = BLOB_SIZE_SMALL;
        break;
    default:
        break;
    }
    return blob_size;
}
/***************************************************************************
 * writed_size  要写入的字节数
 * addr         要写入的地址
 * header       当前的header
 * data         当前的数据
 * return       写入的数据
 * **************************************************************************/
int write_at_blob(file_blob_header *header, disk_addr_t addr, disk_data_t *data, unsigned int writed_size, char update_header)
{
    unsigned short used_size = header->typeinfo.type_str.length;
    unsigned short blob_size = get_blob_size_by_type(header->typeinfo.type_str.type);
    unsigned short unused_size = blob_size - used_size;
    if (unused_size == 0)           //当前的块已经写满
        return 0;                   //没有可用的空间了
    if (writed_size >= unused_size) //可用的磁盘很充分
    {
        header->typeinfo.type_str.length = blob_size;
        if (update_header == 1)
            sys_write((disk_data_t *)header, addr, BLOB_HEADER_SIZE);
        sys_write(data, addr + BLOB_HEADER_SIZE + used_size, unused_size);
        writed_size -= unused_size;
        return unused_size;
    }
    else
    {
        header->typeinfo.type_str.length = (used_size + writed_size);
        if (update_header == 1)
            sys_write((disk_data_t *)header, addr, BLOB_HEADER_SIZE);
        sys_write(data, addr + BLOB_HEADER_SIZE + used_size, writed_size);
        return writed_size;
    }
}

disk_addr_t get_append_address(
    file *file,
    file_blob_header *header,
    unsigned int left_size,
    unsigned int *size,
    unsigned char *type)
{
    disk_addr_t empty_blob_addr = 0;
    if (left_size >= (BLOB_SIZE_LARGE - BLOB_SIZE_MIDDLE))
    {
        empty_blob_addr = get_unwrite_addr(file, header, BLOB_SIZE_LARGE);
        *size = BLOB_SIZE_LARGE;
        *type = BLOB_TYPE_LARGE;
    }
    else if (left_size < BLOB_SIZE_LARGE && left_size >= (BLOB_SIZE_MIDDLE - BLOB_SIZE_SMALL))
    {
        empty_blob_addr = get_unwrite_addr(file, header, BLOB_SIZE_MIDDLE);
        *size = BLOB_SIZE_MIDDLE;
        *type = BLOB_TYPE_MIDDLE;
    }
    else if (left_size < BLOB_SIZE_MIDDLE && left_size >= BLOB_SIZE_SMALL)
    {
        empty_blob_addr = get_unwrite_addr(file, header, BLOB_SIZE_SMALL);
        *size = BLOB_SIZE_SMALL;
        *type = BLOB_TYPE_SMALL;
    }
    else
    {
        empty_blob_addr = get_unwrite_addr(file, header, BLOB_SIZE_SMALL);
        *size = left_size;
        *type = BLOB_TYPE_SMALL;
    }
    return empty_blob_addr;
}
void file_append(file *file)
{
    file->file_mode = APPEND;
    file->read_position = file->file_info.length;
}
void file_truncate(file *file)
{
    file->read_position = 0;
    file->file_info.length = 0;
    disk_addr_t addr = file->file_info.blob_addr;
    file->file_info.blob_addr = 0;
    file_blob_header header;
    while (addr != 0)
    {
        memset(&header, 0, BLOB_HEADER_SIZE);
        sys_read((disk_data_t *)&header, addr, BLOB_HEADER_SIZE);
        addr = header.next_blob;
        memset(&header, 0, BLOB_HEADER_SIZE);
        sys_write((disk_data_t *)&header, addr, BLOB_HEADER_SIZE);
    }
    sys_write((unsigned char *)&file->file_info, file->file_info.file_id, sizeof(file->file_info));
    file->file_mode = TRUNCATE;
}
void file_seek(file *file, int offset, int pos)
{
}
int file_write(file *file, disk_data_t *data, int offset, int length)
{
    int left_size = length;
    int total_write_bytes = 0;

    disk_addr_t end_blob_addr = file->file_info.blob_addr;
    file_blob_header header, empty_blob_header, end_blob_header;
    memset(&end_blob_header, 0, BLOB_HEADER_SIZE);
    memset(&empty_blob_header, 0, BLOB_HEADER_SIZE);
    int is_append = 1;
    switch (file->file_mode)
    {
    case OPEN:
    case CREATE:
    case OPEN_OR_CREATE:
        is_append = 0;
    case APPEND:
        file->write_postion = file->file_info.length;
        break;
    }
    int pos = file->write_postion;
    //偏移到指定的位置
    //追加
    if (end_blob_addr == 0)
    {
        empty_blob_header.fileid = file->file_info.file_id; //file.1 该文件没有被写过数据
    }                                                       //file.2 先从最后一块写
    else                                                    //file.3 如果没有被写满
    {
        disk_addr_t addr_pre_temp = 0;
        if (is_append == 0)
        {
            int offset = file->write_postion;
            while (end_blob_addr)                                                             //不是追加是重写
            {                                                                                 //覆盖已写入的区块
                sys_read((disk_data_t *)&empty_blob_header, end_blob_addr, BLOB_HEADER_SIZE); //读取header信息
                if (offset > 0)
                {
                    if (offset > empty_blob_header.typeinfo.type_str.length)
                    {
                        offset -= empty_blob_header.typeinfo.type_str.length;
                        end_blob_addr = empty_blob_header.next_blob;
                        continue;
                    }
                }
                unsigned short old_length = empty_blob_header.typeinfo.type_str.length;                                          //备份原始长度
                empty_blob_header.typeinfo.type_str.length = offset;                                                             //设置当前偏移量
                offset = 0;                                                                                                      //从区块的第一个字节写入
                int update = left_size != old_length ? 1 : 0;                                                                    //如果 left_size> blob_size
                int sz = write_at_blob(&empty_blob_header, end_blob_addr, data + offset + total_write_bytes, left_size, update); //更新区块信息
                total_write_bytes += sz;
                left_size -= sz;
                if (length == total_write_bytes)
                {
                    file->write_postion += total_write_bytes;
                    file->file_info.length += file->write_postion - pos;
                    sys_write((disk_data_t *)&file->file_info, file->file_info.file_id, sizeof(disk_file));
                    return total_write_bytes;
                }
                end_blob_addr = empty_blob_header.next_blob;
                end_blob_header = empty_blob_header;
            }
        }
        else
        {
            while (end_blob_addr != 0)
            {                                                                                 //找到最后一块
                sys_read((disk_data_t *)&empty_blob_header, end_blob_addr, BLOB_HEADER_SIZE); //如果最后一块可用
                end_blob_header = empty_blob_header;
                if (addr_pre_temp == empty_blob_header.pre_blob && empty_blob_header.next_blob != 0) //覆盖已写入的数据
                {                                                                                    //找到最后一块空闲的blob
                    addr_pre_temp = end_blob_addr;
                    end_blob_addr = empty_blob_header.next_blob;
                }
                else
                {
                    break;
                }
            }
            int sz = write_at_blob(&empty_blob_header, end_blob_addr, data + offset + total_write_bytes, left_size, 1);
            total_write_bytes += sz;
            left_size -= sz;
        }
    }
    while (left_size)
    {
        disk_addr_t empty_blob_addr = 0;
        unsigned int size = 0;
        unsigned char type = 1;
        empty_blob_addr = get_append_address(file, &empty_blob_header, left_size, &size, &type);

        if (end_blob_addr == 0)
        {
            file->file_info.blob_addr = empty_blob_addr;
        }
        else
        {

            if (end_blob_header.fileid != 0)                                             //a-1 有上一个头信息
                end_blob_header.next_blob = empty_blob_addr;                             //a-2 更新上一个头的blob信息
            sys_write((disk_data_t *)&end_blob_header, end_blob_addr, BLOB_HEADER_SIZE); //a-3 上一个头信息 更新 写入磁盘
        }

        file_blob_header header_temp;
        header_temp.fileid = file->file_info.file_id;
        header_temp.pre_blob = end_blob_addr;
        header_temp.next_blob = 0;

        size = left_size > size ? size : left_size;
        header_temp.typeinfo.type_str.length = size;
        header_temp.typeinfo.type_str.type = type;

        sys_write((disk_data_t *)&header_temp, empty_blob_addr, BLOB_HEADER_SIZE);              //1.1写入头信息
        sys_write(data + offset + total_write_bytes, empty_blob_addr + BLOB_HEADER_SIZE, size); //1.2写入数据信息

        end_blob_header = header_temp;
        end_blob_addr = empty_blob_addr;
        left_size -= size;
        total_write_bytes += size;
        printf("find:%x2\t type:%d\t total_write_bytes:%d\r\n", empty_blob_addr, empty_blob_header.typeinfo.type_str.type, size);
    }
    file->write_postion += total_write_bytes;
    file->file_info.length += file->write_postion - pos;
    sys_write((disk_data_t *)&file->file_info, file->file_info.file_id, sizeof(disk_file));
    return total_write_bytes;
}
int file_read(file *file, disk_data_t *data, int offset, int length)
{
    int read_size = 0, left_size = 0;
    unsigned int blob_size = 0;
    disk_addr_t end_blob_addr = file->file_info.blob_addr;
    file_blob_header end_blob_header;
    memset(&end_blob_header, 0, BLOB_HEADER_SIZE);
    //todo goto read_position
    int disk_offset = 0;
    while (end_blob_addr != 0)
    {
        sys_read((disk_data_t *)&end_blob_header, end_blob_addr, BLOB_HEADER_SIZE);
        if ((disk_offset + end_blob_header.typeinfo.type_str.length) < file->read_position)
        {
            end_blob_addr = end_blob_header.next_blob;
            disk_offset += end_blob_header.typeinfo.type_str.length;
        }
        else
        {
            break;
        }
    }
    disk_offset = file->read_position - disk_offset;

    while (end_blob_addr != 0 && read_size < length)
    {
        sys_read((disk_data_t *)&end_blob_header, end_blob_addr, BLOB_HEADER_SIZE);
        if (end_blob_header.fileid != file->file_info.file_id)
        {
            return -1;
        }
        left_size = length - read_size;
        blob_size = end_blob_header.typeinfo.type_str.length;
        if (blob_size > 0)
        {
            unsigned int read = left_size > blob_size ? blob_size : left_size;
            sys_read(data + offset + read_size, end_blob_addr + BLOB_HEADER_SIZE + disk_offset, read);
            disk_offset = 0;
            read_size += read;
            file->read_position += read;
            if (end_blob_header.next_blob == 0)
            {
                if (read_size < length)
                {
                    return -1;
                }
            }
        }
        else
        {
            return -1;
        }
        end_blob_addr = end_blob_header.next_blob;
    }
    return 0;
}