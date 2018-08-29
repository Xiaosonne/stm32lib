# 基本定义 #
##  读取 ##
    typedef void (*sys_write_imp)(unsigned char *buff, unsigned int start_write_addr, unsigned short write_bytes);

## 写入 ##
    typedef void (*sys_read_imp)(unsigned char *buff, unsigned int start_read_addr, unsigned short write_bytes);

## 文件类型定义 ##
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

## 注册实现 ##
	implement_read_method(sys_read_imp imp_method);
    implement_write_method(sys_write_imp imp_method);
# 初始化 #
## 初始化分区 打开文件 ##
 	disk *disk = create_disk_info(
			0xff00ff00, //起始地址
			2, 			//文件区块大小
			10);		//数据区块大小
    file *f = disk->open(disk, "filename", OPEN_OR_CREATE);

## 读取写入数据 ##
 	char data[] = "1234567890";
    char data2[] = "asdfghjkl";
    char data3[] = "qwerty";
    char *dataread = (char *)malloc(50);
    f->write(f, (disk_data_t *)data, 0, 10);
    f->write(f, (disk_data_t *)data2, 0, 9);
    f->read(f, (disk_data_t *)dataread, 0, 19);
 
