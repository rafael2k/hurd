/* 
   Copyright (C) 1994 Free Software Foundation

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "priv.h"
#include "io_S.h"
#include <mach/default_pager.h>

/* Implement io_map_cntl as described in <hurd/io.defs>. */
error_t
diskfs_S_io_map_cntl (struct protid *cred,
		      memory_object_t *ctlobj)
{
  if (!cred)
    return EOPNOTSUPP;
  
  assert (__vm_page_size >= sizeof (struct shared_io));
  mutex_lock (&cred->po->np->lock);
  if (!cred->mapped)
    {
      default_pager_object_create (diskfs_default_pager, &cred->shared_object,
				   __vm_page_size);
      vm_map (mach_task_self (), (u_int *)&cred->mapped, __vm_page_size, 0, 1, 
	      cred->shared_object, 0, 0, VM_PROT_READ|VM_PROT_WRITE,
	      VM_PROT_READ|VM_PROT_WRITE, 0);
      cred->mapped->conch_status = USER_HAS_NOT_CONCH;
      spin_lock_init (&cred->mapped->lock);
      *ctlobj = cred->shared_object;
      mutex_unlock (&cred->po->np->lock);
      return 0;
    }
  else
    {
      mutex_unlock (&cred->po->np->lock);
      return EBUSY;
    }
}
