/* Implementation of memory_object_data_unlock for pager library
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
#include "memory_object_S.h"
#include <stdio.h>

/* Implement kernel requests for access as described in 
   <mach/memory_object.defs>. */
kern_return_t
_pager_seqnos_memory_object_data_unlock (mach_port_t object, 
					 mach_port_seqno_t seqno,
					 mach_port_t control,
					 vm_offset_t offset,
					 vm_size_t length,
					 vm_prot_t access)
{
  struct pager *p;
  volatile int err;
  
  if (!(p = ports_check_port_type (object, pager_port_type)))
    return EOPNOTSUPP;

  mutex_lock (&p->interlock);
  _pager_wait_for_seqno (p, seqno);
  _pager_release_seqno (p, seqno);
  mutex_unlock (&p->interlock);

  if (p->pager_state != NORMAL)
    {
      printf ("pager in wrong state for unlock\n");
      goto out;
    }

  if (control != p->memobjcntl)
    {
      printf ("incg data unlock: wrong control port\n");
      goto out;
    }
  /* The only thing we ever block is writes */
  if ((access & VM_PROT_WRITE) == 0)
    {
      printf ("incg data unlock: not unlock writes\n");
      goto out;
    }
  if (offset % __vm_page_size)
    {
      printf ("incg data unlock: misaligned request\n");
      goto out;
    }
  if (length != __vm_page_size)
    {
      printf ("incg data unlock: bad length size %d\n", length);
      goto out;
    }

  err = pager_unlock_page (p->upi, offset);
  
  if (!err)
    /* We can go ahead and release the lock.  */
    _pager_lock_object (p, offset, length, MEMORY_OBJECT_RETURN_NONE, 0,
			VM_PROT_NONE, 0);
  else
    {
      /* Flush the page, and set a bit so that m_o_data_request knows
	 to issue an error.  */
      _pager_lock_object (p, offset, length, MEMORY_OBJECT_RETURN_NONE, 1,
			  VM_PROT_WRITE, 1);
      _pager_mark_next_request_error (p, offset, length, err);
    }
 out:
  ports_done_with_port (p);
  return 0;
}

