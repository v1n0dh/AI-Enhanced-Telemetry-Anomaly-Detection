// telemetry_module.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinodh");
MODULE_DESCRIPTION("Telemetry Kernel Module exposing real CPU, battery, network stats");
MODULE_VERSION("0.3");

static int cpu_temp = 0;       // in Celsius
static int battery_level = 0;  // percentage
static int network_throughput = 0; // Mbps

static struct kobject *telemetry_kobj;
static struct task_struct *telemetry_thread;

// Helper: Read integer from file
static int read_int_from_file(const char *path) {
    struct file *f;
    mm_segment_t oldfs;
    char buf[16];
    int val = 0;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    f = filp_open(path, O_RDONLY, 0);
    if (!IS_ERR(f)) {
        if (f->f_op->read) {
            f->f_op->read(f, buf, sizeof(buf) - 1, &f->f_pos);
            buf[15] = '\0';
            kstrtoint(buf, 10, &val);
        }
        filp_close(f, NULL);
    }

    set_fs(oldfs);
    return val;
}

// Sysfs show functions
static ssize_t cpu_temp_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", cpu_temp);
}

static ssize_t battery_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", battery_level);
}

static ssize_t network_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", network_throughput);
}

// Sysfs attributes
static struct kobj_attribute cpu_temp_attr = __ATTR_RO(cpu_temp);
static struct kobj_attribute battery_attr = __ATTR_RO(battery_level);
static struct kobj_attribute network_attr = __ATTR_RO(network_throughput);

// Update telemetry periodically
static int update_telemetry(void *data) {
    u64 prev_rx = 0, prev_tx = 0, curr_rx, curr_tx;
    const char *iface = "eth0"; // change if needed

    // Initial network bytes
    prev_rx = read_int_from_file("/sys/class/net/eth0/statistics/rx_bytes");
    prev_tx = read_int_from_file("/sys/class/net/eth0/statistics/tx_bytes");

    while (!kthread_should_stop()) {
        // CPU temperature (millidegree -> degree)
        cpu_temp = read_int_from_file("/sys/class/thermal/thermal_zone0/temp") / 1000;

        // Battery level from sysfs (example, adjust to your hardware)
        battery_level = read_int_from_file("/sys/class/power_supply/BAT0/capacity");

        // Network throughput in Mbps
        curr_rx = read_int_from_file("/sys/class/net/eth0/statistics/rx_bytes");
        curr_tx = read_int_from_file("/sys/class/net/eth0/statistics/tx_bytes");
        network_throughput = ((curr_rx - prev_rx) + (curr_tx - prev_tx)) * 8 / 1000000; // Mbps
        prev_rx = curr_rx;
        prev_tx = curr_tx;

        ssleep(2);
    }
    return 0;
}

// Init & exit
static int __init telemetry_init(void) {
    int retval;

    telemetry_kobj = kobject_create_and_add("telemetry", kernel_kobj);
    if (!telemetry_kobj)
        return -ENOMEM;

    retval = sysfs_create_file(telemetry_kobj, &cpu_temp_attr.attr);
    retval |= sysfs_create_file(telemetry_kobj, &battery_attr.attr);
    retval |= sysfs_create_file(telemetry_kobj, &network_attr.attr);

    telemetry_thread = kthread_run(update_telemetry, NULL, "telemetry_thread");

    printk(KERN_INFO "Telemetry module loaded.\n");
    return retval;
}

static void __exit telemetry_exit(void) {
    if (telemetry_thread)
        kthread_stop(telemetry_thread);

    kobject_put(telemetry_kobj);
    printk(KERN_INFO "Telemetry module unloaded.\n");
}

module_init(telemetry_init);
module_exit(telemetry_exit);
