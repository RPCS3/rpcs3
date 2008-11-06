/* dummy.c: a d9n net driver

	The purpose of this driver is to provide a device to point a
	route through, but not to actually transmit packets.

	Why?  If you have a machine whose only connection is an occasional
	PPP/SLIP/PLIP link, you can only connect to your own hostname
	when the link is up.  Otherwise you have to use localhost.
	This isn't very consistent.

	One solution is to set up a d9n link using PPP/SLIP/PLIP,
	but this seems (to me) too much overhead for too little gain.
	This driver provides a small alternative. Thus you can do
	
	[when not running slip]
		ifconfig d9n slip.addr.ess.here up
	[to go to slip]
		ifconfig d9n down
		dip whatever

	This was written by looking at Donald Becker's skeleton driver
	and the loopback driver.  I then threw away anything that didn't
	apply!	Thanks to Alan Cox for the key clue on what to do with
	misguided packets.

			Nick Holloway, 27th May 1994
	[I tweaked this explanation a little but that's all]
			Alan Cox, 30th May 1994
*/

/* To have statistics (just packets sent) define this */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/init.h>
#include <net/tcp.h>
#include <net/udp.h>


static char *edev;
static struct net_device *ndev;

MODULE_PARM(edev, "s");
MODULE_PARM_DESC(edev,
	"name of the device from which to send packets");

static struct net_device dev_d9n;


static int d9n_xmit(struct sk_buff *skb, struct net_device *dev);
static struct net_device_stats *d9n_get_stats(struct net_device *dev);
static int d9n_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
static int d9n_rx_hook(struct sk_buff *skb);
static int (*old_rx_hook)(struct sk_buff *skb);


/* fake multicast ability */
static void set_multicast_list(struct net_device *dev) {
}

#ifdef CONFIG_NET_FASTROUTE
static int d9n_accept_fastpath(struct net_device *dev, struct dst_entry *dst) {
	return -1;
}
#endif

static int __init d9n_init(struct net_device *dev) {

	/* Initialize the device structure. */

	dev->priv = kmalloc(sizeof(struct net_device_stats), GFP_KERNEL);
	if (dev->priv == NULL)
		return -ENOMEM;
	memset(dev->priv, 0, sizeof(struct net_device_stats));

	dev->get_stats = d9n_get_stats;
	dev->hard_start_xmit = d9n_xmit;
	dev->set_multicast_list = set_multicast_list;
	dev->do_ioctl = d9n_ioctl;
#ifdef CONFIG_NET_FASTROUTE
	dev->accept_fastpath = d9n_accept_fastpath;
#endif

	/* Fill in device structure with ethernet-generic values. */
	ether_setup(dev);
	dev->tx_queue_len = 0;
	dev->flags = 0;

	return 0;
}

static int d9n_xmit(struct sk_buff *skb, struct net_device *dev) {
	struct net_device_stats *stats = dev->priv;

	printk("d9n_xmit %d\n", skb->len);

//	dev_queue_xmit(skb);
	stats->tx_packets++;
	stats->tx_bytes+=skb->len;

	dev_kfree_skb(skb);
	return 0;
}

static struct net_device_stats *d9n_get_stats(struct net_device *dev) {
	return dev->priv;
}

static int d9n_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd) {
	printk("d9n_ioctl\n");

	if (!netif_running(dev))
		return -EINVAL;

/*	switch (cmd) {
		case 
	}*/
	return 0;
}

static int d9n_rx_hook(struct sk_buff *skb) {
	struct sk_buff *skb2;
	struct iphdr *iph;
	unsigned char *ptr;
	__u32 saddr;
	int proto;

	printk("d9n_rx_hook: %d\n", skb->len);
	ptr = skb->mac.ethernet->h_dest;
	printk("h_dest:   %02X:%02X:%02X:%02X:%02X:%02X.\n", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
	ptr = skb->mac.ethernet->h_source;
	printk("h_source: %02X:%02X:%02X:%02X:%02X:%02X.\n", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);

	proto = ntohs(skb->mac.ethernet->h_proto);
	if (proto != ETH_P_IP) {
		return NET_RX_SUCCESS;
	}

	printk("ETH_P_IP\n");
	iph = (struct iphdr*)skb->data;
	printk("saddr=%08x, daddr=%08x\n", iph->saddr, iph->daddr);

	skb2 = skb_clone(skb, GFP_ATOMIC);
	skb2->dev = &dev_d9n;
	netif_rx(skb2);

	return NET_RX_SUCCESS;
}

static int __init d9n_init_module(void) {
	int err;

	printk("d9n_init_module\n");
	if (edev == NULL) {
		printk(KERN_ERR "dev9net: please specify network device (dev='device'), aborting.\n"); 
		return -1;
	}
	ndev = dev_get_by_name(edev);
	if (ndev == NULL) {
		printk(KERN_ERR "dev9net: network device %s does not exists, aborting.\n", edev); 
		return -1;
	}

	old_rx_hook = ndev->rx_hook;
	ndev->rx_hook = d9n_rx_hook;

	dev_d9n.init = d9n_init;
	SET_MODULE_OWNER(&dev_d9n);

	/* Find a name for this unit */
	err=dev_alloc_name(&dev_d9n,"d9n");
	if(err<0)
		return err;
	err = register_netdev(&dev_d9n);
	printk("register_netdev: %d\n", err);
	if (err<0)
		return err;
	return 0;
}

static void __exit d9n_cleanup_module(void) {
	unregister_netdev(&dev_d9n);
	kfree(dev_d9n.priv);

	memset(&dev_d9n, 0, sizeof(dev_d9n));
	dev_d9n.init = d9n_init;
	ndev->rx_hook = old_rx_hook;
}

module_init(d9n_init_module);
module_exit(d9n_cleanup_module);
MODULE_LICENSE("GPL");
