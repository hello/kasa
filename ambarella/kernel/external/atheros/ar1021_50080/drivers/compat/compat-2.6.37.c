/*
 * Copyright 2010    Hauke Mehrtens <hauke@hauke-m.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Compatibility file for Linux wireless for kernels 2.6.37.
 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37))
#include <linux/compat.h>
#include <linux/netdevice.h>
#include <net/sock.h>
#include <linux/nsproxy.h>
#include <linux/vmalloc.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
static const void *net_current_ns(void)
{
	return current->nsproxy->net_ns;
}

static const void *net_initial_ns(void)
{
	return &init_net;
}

static const void *net_netlink_ns(struct sock *sk)
{
	return sock_net(sk);
}

struct kobj_ns_type_operations net_ns_type_operations = {
	.type = KOBJ_NS_TYPE_NET,
	.current_ns = net_current_ns,
	.netlink_ns = net_netlink_ns,
	.initial_ns = net_initial_ns,
};
EXPORT_SYMBOL_GPL(net_ns_type_operations);

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)*/ 

#undef genl_info
#undef genl_unregister_family

static LIST_HEAD(compat_nl_fam);

static struct genl_ops *genl_get_cmd(u8 cmd, struct genl_family *family)
{
	struct genl_ops *ops;

	list_for_each_entry(ops, &family->family.ops_list, ops.ops_list)
		if (ops->cmd == cmd)
			return ops;

	return NULL;
}


static int nl_doit_wrapper(struct sk_buff *skb, struct genl_info *info)
{
	struct compat_genl_info compat_info;
	struct genl_family *family;
	struct genl_ops *ops;
	int err;

	list_for_each_entry(family, &compat_nl_fam, list) {
		if (family->id == info->nlhdr->nlmsg_type)
			goto found;
	}
	return -ENOENT;

found:
	ops = genl_get_cmd(info->genlhdr->cmd, family);
	if (!ops)
		return -ENOENT;

	memset(&compat_info.user_ptr, 0, sizeof(compat_info.user_ptr));
	compat_info.info = info;
#define __copy(_field) compat_info._field = info->_field
	__copy(snd_seq);
	__copy(snd_pid);
	__copy(genlhdr);
	__copy(attrs);
#undef __copy
	if (family->pre_doit) {
		err = family->pre_doit(ops, skb, &compat_info);
		if (err)
			return err;
	}

	err = ops->doit(skb, &compat_info);

	if (family->post_doit)
		family->post_doit(ops, skb, &compat_info);

	return err;
}

int compat_genl_register_family_with_ops(struct genl_family *family,
					 struct genl_ops *ops, size_t n_ops)
{
	int i, ret;

#define __copy(_field) family->family._field = family->_field
	__copy(id);
	__copy(hdrsize);
	__copy(version);
	__copy(maxattr);
	strncpy(family->family.name, family->name, sizeof(family->family.name));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32))
	__copy(netnsok);
#endif
#undef __copy

	ret = genl_register_family(&family->family);
	if (ret < 0)
		return ret;

	family->attrbuf = family->family.attrbuf;
	family->id = family->family.id;

	for (i = 0; i < n_ops; i++) {
#define __copy(_field) ops[i].ops._field = ops[i]._field
		__copy(cmd);
		__copy(flags);
		__copy(policy);
		__copy(dumpit);
		__copy(done);
#undef __copy
		if (ops[i].doit)
			ops[i].ops.doit = nl_doit_wrapper;
		ret = genl_register_ops(&family->family, &ops[i].ops);
		if (ret < 0)
			goto error_ops;
	}
	list_add(&family->list, &compat_nl_fam);

	return ret;

error_ops:
	compat_genl_unregister_family(family);
	return ret;
}
EXPORT_SYMBOL(compat_genl_register_family_with_ops);

int compat_genl_unregister_family(struct genl_family *family)
{
	int err;
	err = genl_unregister_family(&family->family);
	list_del(&family->list);
	return err;
}
EXPORT_SYMBOL(compat_genl_unregister_family);

#endif
