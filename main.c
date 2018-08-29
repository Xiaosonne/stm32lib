
#include "Storage/file.h"
void imp_sys_read_imp(disk_data_t *buff, disk_addr_t start_read_addr, unsigned short write_bytes)
{
    disk_data_t *addrStart = (disk_data_t *)start_read_addr;
    memcpy(buff, addrStart, write_bytes);
}
void imp_sys_write_imp(disk_data_t *buff, disk_addr_t start_write_addr, unsigned short write_bytes)
{
    disk_data_t *addrStart = (disk_data_t *)start_write_addr;
    memcpy(addrStart, buff, write_bytes);
}
int main(int argn, char **args)
{
    implement_read_method(imp_sys_read_imp);
    implement_write_method(imp_sys_write_imp);

    unsigned char *virtualDisl = (unsigned char *)malloc(64 * 1024 * 12);
    disk *disk = create_disk_info((disk_addr_t)virtualDisl, 2, 10);

    file *f = disk->open(disk, "fuckyou", OPEN_OR_CREATE);

    char data[] = "1234567890";
    char data2[] = "asdfghjkl";
    char data3[] = "qwerty";
    char *dataread = (char *)malloc(50);
    memset(dataread, 0, 50);

    if (f->file_info.length > 0)
    {
        f->read(f, (disk_data_t *)dataread, 0, f->file_info.length);
        printf("dataread %s\r\n", dataread); //should be 1234567890asdfghjkl1234567890
                                             //file *f2 = disk->open(disk, "fuckyou", APPEND);
                                             //f2->write(f2,(disk_data_t *)data3,0,6);
    }
    else
    {
        f->write(f, (disk_data_t *)data, 0, 10);
        f->write(f, (disk_data_t *)data2, 0, 9);
        f->read(f, (disk_data_t *)dataread, 0, 19);
        printf("dataread %s\r\n", dataread); //should be 1234567890asdfghjkl
        f->read_position = 10;
        memset(dataread, 0, 50);
        f->read(f, (disk_data_t *)dataread, 0, 9);
        printf("dataread %s\r\n", dataread); //should be asdfghjkl

        memset(dataread, 0, 50);
        f->write(f, (disk_data_t *)data, 0, 10);
        f->read_position = 0;
        f->read(f, (disk_data_t *)dataread, 0, 29);
        printf("dataread %s\r\n", dataread); //should be 1234567890asdfghjkl1234567890
        memset(dataread, 0, 50);
        file *f2 = disk->open(disk, "fuckyou", APPEND);
        f2->write(f2, (disk_data_t *)data3, 0, 6);
        f->read_position = 0;
        f->read(f, (disk_data_t *)dataread, 0, 35);
        printf("dataread %s\r\n", dataread); //should be 1234567890asdfghjkl1234567890qwerty
    }
    return 0;
}