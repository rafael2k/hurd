/* Interface functions for the socket.defs interface.
   Copyright (C) 1995,96,97,99,2000 Free Software Foundation, Inc.
   Written by Michael I. Bushnell, p/BSG.

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include <hurd/trivfs.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>

#include "pfinet.h"
#include "socket_S.h"

#include <linux/sched.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <net/sock.h>


error_t
S_socket_create (struct trivfs_protid *master,
		 int sock_type,
		 int protocol,
		 mach_port_t *port,
		 mach_msg_type_name_t *porttype)
{
  struct sock_user *user;
  struct socket *sock;
  error_t err;

  if (!master)
    return EOPNOTSUPP;

  /* Don't allow bogus SOCK_PACKET here. */

  if ((sock_type != SOCK_STREAM
       && sock_type != SOCK_DGRAM
       && sock_type != SOCK_SEQPACKET
       && sock_type != SOCK_RAW)
      || protocol < 0)
    return EINVAL;

  __mutex_lock (&global_lock);

  become_task_protid (master);

  sock = sock_alloc ();

  sock->type = sock_type;

  err = - (*net_families[PF_INET]->create) (sock, protocol);
  if (err)
    sock_release (sock);
  else
    {
      user = make_sock_user (sock, master->isroot, 0, 1);
      *port = ports_get_right (user);
      *porttype = MACH_MSG_TYPE_MAKE_SEND;
      ports_port_deref (user);
    }

  __mutex_unlock (&global_lock);

  return err;
}


/* Listen on a socket. */
error_t
S_socket_listen (struct sock_user *user, int queue_limit)
{
  error_t err;

  if (!user)
    return EOPNOTSUPP;

  __mutex_lock (&global_lock);
  become_task (user);
  err = - (*user->sock->ops->listen) (user->sock, queue_limit);
  __mutex_unlock (&global_lock);

  return err;
}

error_t
S_socket_accept (struct sock_user *user,
		 mach_port_t *new_port,
		 mach_msg_type_name_t *new_port_type,
		 mach_port_t *addr_port,
		 mach_msg_type_name_t *addr_port_type)
{
  struct sock_user *newuser;
  struct socket *sock, *newsock;
  error_t err;

  if (!user)
    return EOPNOTSUPP;

  sock = user->sock;

  __mutex_lock (&global_lock);

  become_task (user);

  newsock = sock_alloc ();
  if (!newsock)
    err = ENOMEM;
  else
    {
      newsock->type = sock->type;

      err = - (*sock->ops->dup) (newsock, sock);
      if (!err)
	err = - (*sock->ops->accept) (sock, newsock, sock->flags);

      if (!err)
	/* In Linux there is a race here with the socket closing before the
	   ops->getname call we do in make_sockaddr_port.  Since we still
	   have the world locked, this shouldn't be an issue for us.  */
	err = make_sockaddr_port (newsock, 1, addr_port, addr_port_type);

      if (!err)
	{
	  newuser = make_sock_user (newsock, user->isroot, 0, 1);
	  *new_port = ports_get_right (newuser);
	  *new_port_type = MACH_MSG_TYPE_MAKE_SEND;
	  ports_port_deref (newuser);
	}

      if (err)
	sock_release (newsock);
    }

  __mutex_unlock (&global_lock);

  return err;
}

error_t
S_socket_connect (struct sock_user *user,
		  struct sock_addr *addr)
{
  struct socket *sock;
  error_t err;

  if (!user || !addr)
    return EOPNOTSUPP;

  sock = user->sock;

  __mutex_lock (&global_lock);

  become_task (user);

  err = - (*sock->ops->connect) (sock, &addr->address, addr->address.sa_len,
				 sock->flags);

  __mutex_unlock (&global_lock);

  /* MiG should do this for us, but it doesn't. */
  if (!err)
    mach_port_deallocate (mach_task_self (), addr->pi.port_right);

  return err;
}

error_t
S_socket_bind (struct sock_user *user,
	       struct sock_addr *addr)
{
  error_t err;

  if (!user)
    return EOPNOTSUPP;
  if (! addr)
    return EADDRNOTAVAIL;

  __mutex_lock (&global_lock);
  become_task (user);
  err = - (*user->sock->ops->bind) (user->sock,
				    &addr->address, addr->address.sa_len);
  __mutex_unlock (&global_lock);

  /* MiG should do this for us, but it doesn't. */
  if (!err)
    mach_port_deallocate (mach_task_self (), addr->pi.port_right);

  return err;
}

error_t
S_socket_name (struct sock_user *user,
	       mach_port_t *addr_port,
	       mach_msg_type_name_t *addr_port_name)
{
  if (!user)
    return EOPNOTSUPP;

  __mutex_lock (&global_lock);
  become_task (user);
  make_sockaddr_port (user->sock, 0, addr_port, addr_port_name);
  __mutex_unlock (&global_lock);
  return 0;
}

error_t
S_socket_peername (struct sock_user *user,
		   mach_port_t *addr_port,
		   mach_msg_type_name_t *addr_port_name)
{
  error_t err;

  if (!user)
    return EOPNOTSUPP;

  __mutex_lock (&global_lock);
  become_task (user);
  err = make_sockaddr_port (user->sock, 1, addr_port, addr_port_name);
  __mutex_unlock (&global_lock);

  return err;
}

error_t
S_socket_connect2 (struct sock_user *user1,
		   struct sock_user *user2)
{
  error_t err;

  if (!user1 || !user2)
    return EOPNOTSUPP;

  __mutex_lock (&global_lock);

  become_task (user1);

  if (user1->sock->type != user2->sock->type)
    err = EINVAL;
  else if (user1->sock->state != SS_UNCONNECTED
	   && user2->sock->state != SS_UNCONNECTED)
    err = EISCONN;
  else
    err = - (*user1->sock->ops->socketpair) (user1->sock, user2->sock);

  __mutex_unlock (&global_lock);

  /* MiG should do this for us, but it doesn't. */
  if (!err)
    mach_port_deallocate (mach_task_self (), user2->pi.port_right);

  return err;
}

error_t
S_socket_create_address (mach_port_t server,
			 int sockaddr_type,
			 char *data,
			 mach_msg_type_number_t data_len,
			 mach_port_t *addr_port,
			 mach_msg_type_name_t *addr_port_type)
{
  error_t err;
  struct sock_addr *addrstruct;
  const struct sockaddr *const sa = (void *) data;

  if (sockaddr_type != AF_INET)
    return EAFNOSUPPORT;
  if (sa->sa_family != sockaddr_type
      || data_len < offsetof (struct sockaddr, sa_data))
    return EINVAL;

  err = ports_create_port (addrport_class, pfinet_bucket,
			   (offsetof (struct sock_addr, address)
			    + data_len), &addrstruct);
  if (err)
    return err;

  memcpy (&addrstruct->address, data, data_len);

  /* BSD does not require incoming sa_len to be set, so we don't either.  */
  addrstruct->address.sa_len = data_len;

  *addr_port = ports_get_right (addrstruct);
  *addr_port_type = MACH_MSG_TYPE_MAKE_SEND;
  ports_port_deref (addrstruct);
  return 0;
}

error_t
S_socket_fabricate_address (mach_port_t server,
			    int sockaddr_type,
			    mach_port_t *addr_port,
			    mach_msg_type_name_t *addr_port_type)
{
  return EOPNOTSUPP;
}

error_t
S_socket_whatis_address (struct sock_addr *addr,
			 int *type,
			 char **data,
			 mach_msg_type_number_t *datalen)
{
  if (!addr)
    return EOPNOTSUPP;

  *type = addr->address.sa_family;
  if (*datalen < addr->address.sa_len)
    *data = mmap (0, addr->address.sa_len,
		  PROT_READ|PROT_WRITE, MAP_ANON, 0, 0);
  *datalen = addr->address.sa_len;
  memcpy (*data, &addr->address, addr->address.sa_len);

  return 0;
}

error_t
S_socket_shutdown (struct sock_user *user,
		   int direction)
{
  error_t err;

  if (!user)
    return EOPNOTSUPP;

  __mutex_lock (&global_lock);
  become_task (user);
  err = - (*user->sock->ops->shutdown) (user->sock, direction);
  __mutex_unlock (&global_lock);

  return err;
}

error_t
S_socket_getopt (struct sock_user *user,
		 int level,
		 int option,
		 char **data,
		 u_int *datalen)
{
  error_t err;

  if (! user)
    return EOPNOTSUPP;

  __mutex_lock (&global_lock);
  become_task (user);

  err = - (level == SOL_SOCKET ? sock_getsockopt
	   : *user->sock->ops->getsockopt)
    (user->sock, level, option, *data, datalen);

  __mutex_unlock (&global_lock);

  /* XXX option data not properly typed, needs byte-swapping for netmsgserver.
     Most options are ints, some like IP_OPTIONS are bytesex-neutral.  */

  return err;
}

error_t
S_socket_setopt (struct sock_user *user,
		 int level,
		 int option,
		 char *data,
		 u_int datalen)
{
  error_t err;

  if (! user)
    return EOPNOTSUPP;

  /* XXX option data not properly typed, needs byte-swapping for netmsgserver.
     Most options are ints, some like IP_OPTIONS are bytesex-neutral.  */

  __mutex_lock (&global_lock);
  become_task (user);

  err = - (level == SOL_SOCKET ? sock_setsockopt
	   : *user->sock->ops->setsockopt)
    (user->sock, level, option, data, datalen);

  __mutex_unlock (&global_lock);

  return err;
}

error_t
S_socket_send (struct sock_user *user,
	       struct sock_addr *addr,
	       int flags,
	       char *data,
	       u_int datalen,
	       mach_port_t *ports,
	       u_int nports,
	       char *control,
	       u_int controllen,
	       mach_msg_type_number_t *amount)
{
  int sent;
  struct iovec iov = { data, datalen };
  struct msghdr m = { msg_name: addr ? &addr->address : 0,
		      msg_namelen: addr ? addr->address.sa_len : 0,
		      msg_flags: flags,
		      msg_controllen: 0, msg_iov: &iov, msg_iovlen: 1 };

  if (!user)
    return EOPNOTSUPP;

  /* Don't do this yet, it's too bizarre to think about right now. */
  if (nports != 0 || controllen != 0)
    return EINVAL;

  __mutex_lock (&global_lock);
  become_task (user);
  if (user->sock->flags & O_NONBLOCK)
    m.msg_flags |= MSG_DONTWAIT;
  sent = (*user->sock->ops->sendmsg) (user->sock, &m, datalen, 0);
  __mutex_unlock (&global_lock);

  /* MiG should do this for us, but it doesn't. */
  if (addr && sent >= 0)
    mach_port_deallocate (mach_task_self (), addr->pi.port_right);

  if (sent >= 0)
    {
      *amount = sent;
      return 0;
    }
  else
    return (error_t)-sent;
}

error_t
S_socket_recv (struct sock_user *user,
	       mach_port_t *addrport,
	       mach_msg_type_name_t *addrporttype,
	       int flags,
	       char **data,
	       u_int *datalen,
	       mach_port_t **ports,
	       mach_msg_type_name_t *portstype,
	       u_int *nports,
	       char **control,
	       u_int *controllen,
	       int *outflags,
	       mach_msg_type_number_t amount)
{
  error_t err;
  union { struct sockaddr_storage storage; struct sockaddr sa; } addr;
  int alloced = 0;
  struct iovec iov;
  struct msghdr m = { msg_name: &addr.sa, msg_namelen: sizeof addr,
		      msg_controllen: 0, msg_iov: &iov, msg_iovlen: 1 };

  if (!user)
    return EOPNOTSUPP;

  /* Instead of this, we should peek and the socket and only
     allocate as much as necessary. */
  if (amount > *datalen)
    {
      *data = mmap (0, amount, PROT_READ|PROT_WRITE, MAP_ANON, 0, 0);
      alloced = 1;
    }

  iov.iov_base = *data;
  iov.iov_len = amount;

  __mutex_lock (&global_lock);
  become_task (user);
  if (user->sock->flags & O_NONBLOCK)
    flags |= MSG_DONTWAIT;
  err = (*user->sock->ops->recvmsg) (user->sock, &m, amount, flags, 0);
  __mutex_unlock (&global_lock);

  if (err < 0)
    err = -err;
  else
    {
      *datalen = err;
      if (alloced && round_page (*datalen) < round_page (amount))
	munmap (*data + round_page (*datalen),
		round_page (amount) - round_page (*datalen));
      err = S_socket_create_address (0, addr.sa.sa_family,
				     (void *) &addr.sa, m.msg_namelen,
				     addrport, addrporttype);
      if (err && alloced)
	munmap (*data, *datalen);

      *outflags = m.msg_flags;
      *nports = 0;
      *portstype = MACH_MSG_TYPE_COPY_SEND;
      *controllen = 0;
    }

  return err;
}