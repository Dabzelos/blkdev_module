
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
/*TODO:

MINORS
codestyle
утечка памяти при инициализации и убрать bioendio

*/
static char *blkdev_name;
static char *shift_disk_name; 
static int major;
static int minor = 0;
struct block_device *bdev;
struct bdev_handle *current_bdev_handle;
struct gendisk *shift_disk;


static int __init blk_init(void)
{
	pr_info("blk init\n");
	major = register_blkdev(0, "blkmod");
	if (major < 0) {
        pr_err("unable to register block device major\n");
        return -1;
    }
	pr_info("major: %d\n", major);
	return 0;
}

static void __exit blk_exit(void)
{
	pr_info("blk exit\n");
}


static int bdev_name_set(const char *arg, const struct kernel_param *kp)
{
	ssize_t name_len = strlen(arg)+1;
	if (blkdev_name) {
		pr_info("name set\n");
		kfree(blkdev_name);
		blkdev_name = NULL;
	}

	blkdev_name = kzalloc(sizeof(char)* name_len, GFP_KERNEL);
	if (!blkdev_name) {
		return -ENOMEM;
	}

	strcpy(blkdev_name,arg);
	return 0;
}


static int bdev_name_get(char *buf, const struct kernel_param *kp)
{
	ssize_t len;

	if (!blkdev_name)
		return -EINVAL;
	len = strlen(blkdev_name);

	strcpy(buf, blkdev_name);

	return len;
}


static const struct kernel_param_ops bdev_name_ops = 
{
	.set = bdev_name_set,
	.get = bdev_name_get,
};


/*checks if blkdev is in system*/
static int bdev_is_in_sys(const char *buf, const struct kernel_param *kp)
{
	dev_t dev;
	int err;
	
	pr_info("in sys start\n");
	if(!blkdev_name) {
		pr_info("no name\n");
		return -EINVAL;
	}
	
	pr_info("path\n");
	err = lookup_bdev(blkdev_name, &dev);
	if (!err) {
		pr_info("BLKdevice is in sys, devnum: %d \n", dev);
		return 0;
	} else {
		pr_info("ERROR %d\n", err);
		return -EINVAL;
	}
}


/*opens block device by its name*/
static int bdev_open(const char *buf, const struct kernel_param *kp)
{
	current_bdev_handle = bdev_open_by_path(blkdev_name, BLK_OPEN_READ | BLK_OPEN_WRITE, NULL, NULL);
	if (current_bdev_handle && !IS_ERR(current_bdev_handle)) {
		bdev = current_bdev_handle->bdev;
		pr_info("bdev %d \n",bdev->bd_dev);
		return 0;
	} 
	pr_info("ERROR\n");
	return -EINVAL;
}


/*
бдев клози не пашет)))*/
static int bdev_close(char *arg, const struct kernel_param *kp)
{
	if (current_bdev_handle && !IS_ERR(current_bdev_handle)) {
		bdev_release(current_bdev_handle);
		bdev = NULL;
		strcpy(arg, "");
		pr_info("bdev released\n");
		return 0;
	} 
	pr_info("ERROR\n");
	return -EINVAL;
}


static void my_bio_end_io(struct bio *bio)
{
	bio_endio(bio->bi_private);
	bio_put(bio);
}


/* sunmits bio, main idea to redirect io from shift change disk to main */
static void shift_submit_bio(struct bio *bio) 
{

    if (!shift_disk) {
        pr_info("no shift disk\n");
        return;
    }

    struct bio *clone;

    clone = bio_alloc_clone(current_bdev_handle->bdev, bio, GFP_KERNEL, bio->bi_pool);

    if (!clone) {
        pr_info("bio fail\n");
        return;
    }

	clone->bi_private = bio;
	clone->bi_end_io = my_bio_end_io;
    submit_bio(clone);
}


static const struct block_device_operations shift_bio_ops = {
        .owner = THIS_MODULE,
        .submit_bio = shift_submit_bio,
};


/*this function creates blk dev which will 
be shift change between io and base blkdev in sys */
static struct gendisk *shift_blkdev_create(char * shift_name)
{
		

	pr_info("major: %d\n",major);
	
	struct gendisk *new_disk;
	new_disk = blk_alloc_disk(NUMA_NO_NODE);

    new_disk->major = major;
    new_disk->first_minor = minor;
	minor += 1;
    new_disk->minors = 1;
    new_disk->flags = GENHD_FL_NO_PART;
    new_disk->fops = &shift_bio_ops;
	
	set_capacity(new_disk, get_capacity(current_bdev_handle->bdev->bd_disk));

	if(shift_name){
		strcpy(new_disk->disk_name, shift_name); 
	} else {
		pr_info("no name");
	}
	int err;
	err = add_disk(new_disk);
	if (err){
		put_disk(new_disk);
	}

	return new_disk;
}


static int shift_blkdev_close(char *arg, const struct kernel_param *kp)
{
	if(!shift_disk){
		pr_info("no disk to delete\n");
		return -ENOMEM;
	} else{
		put_disk(shift_disk);
		kfree(shift_disk);
		shift_disk = NULL;
		return 1;
	}
}


static int shift_blkdev_name(const char *arg, const struct kernel_param *kp) 
{
    ssize_t len = strlen(arg);   
	shift_disk_name = kzalloc(sizeof(char) * (len), GFP_KERNEL);
	
    if (!shift_disk_name) {
		return -ENOMEM;
    }

	strncpy(shift_disk_name, arg, len);
	shift_disk_name[len - 1] = '\0';
    
    if (!shift_disk_name) {
       return -ENOMEM;
    }

    pr_info("disk name: %s set up succesfully\n", shift_disk_name);
	shift_disk = shift_blkdev_create(shift_disk_name);
	
	if(!shift_disk_name){
		pr_info("no shift disk\n");
		return -ENOMEM;
	}
	return 0;
}


static const struct kernel_param_ops bdev_is_in_sys_ops = {
	.set = bdev_is_in_sys,
	.get = NULL,
};


static const struct kernel_param_ops bdev_ops = {
	.set = bdev_open,
	.get = bdev_close,
};


static const struct kernel_param_ops shift_disk_ops = {
	.set = shift_blkdev_name,
	.get = shift_blkdev_close,
};



MODULE_PARM_DESC(dev_name, "Device name");
module_param_cb(dev_name, &bdev_name_ops, NULL, S_IRUGO | S_IWUSR);


MODULE_PARM_DESC(in_sys, "In sys");
module_param_cb(in_sys, &bdev_is_in_sys_ops, NULL, S_IWUSR);


MODULE_PARM_DESC(bdev, "bldev");
module_param_cb(bdev, &bdev_ops, NULL,S_IRUGO | S_IWUSR);


MODULE_PARM_DESC(shift_disk_name, "shift change disk");
module_param_cb(shift_disk_name, &shift_disk_ops, NULL, S_IWUSR);


module_init(blk_init);
module_exit(blk_exit);


MODULE_AUTHOR("Danila Rudnev-Stepanyan <rudnevda@yandex.ru>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("blk module");