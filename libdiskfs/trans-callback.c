/*
   Copyright (C) 1995, 96, 97, 98 Free Software Foundation, Inc.
   Written by Michael I. Bushnell.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "priv.h"
#include <fcntl.h>

/* Callback function needed for calls to fshelp_fetch_root.  See
   <hurd/fshelp.h> for the interface description.  */
static error_t
_diskfs_translator_callback1_fn (void *cookie1, void *cookie2,
				 uid_t *uid, gid_t *gid,
				 char **argz, size_t *argz_len)
{
  error_t err;
  struct node *np = cookie1;

  if (! (np->dn_stat.st_mode & S_IPTRANS))
    return ENOENT;

  err = diskfs_get_translator (np, argz, (u_int *) argz_len);
  if (err)
    return err;

  *uid = np->dn_stat.st_uid;
  *gid = np->dn_stat.st_gid;

  return 0;
}

/* Callback function needed for calls to fshelp_fetch_root.  See
   <hurd/fshelp.h> for the interface description.  */
static error_t
_diskfs_translator_callback2_fn (void *cookie1, void *cookie2,
				 int flags,
				 mach_port_t *underlying,
				 mach_msg_type_name_t *underlying_type)
{
  struct node *np = cookie1;
  struct protid *cred;
  error_t err;
  struct idvec *uids, *gids;

  uids = make_idvec ();
  gids = make_idvec ();
  idvec_set_ids (uids, &np->dn_stat.st_uid, 1);
  idvec_set_ids (gids, &np->dn_stat.st_gid, 1);

  err =
    diskfs_create_protid (diskfs_make_peropen (np, flags, cookie2),
			  iohelp_create_iouser (uids, gids), &cred);
  if (! err)
    {
      *underlying = ports_get_right (cred);
      *underlying_type = MACH_MSG_TYPE_MAKE_SEND;
      ports_port_deref (cred);
    }
  return err;
}

fshelp_fetch_root_callback1_t _diskfs_translator_callback1 =
  _diskfs_translator_callback1_fn;
fshelp_fetch_root_callback2_t _diskfs_translator_callback2 =
  _diskfs_translator_callback2_fn;