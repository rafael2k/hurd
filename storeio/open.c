/* Per-open information for storeio

   Copyright (C) 1995, 1996 Free Software Foundation, Inc.

   Written by Miles Bader <miles@gnu.ai.mit.edu>

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

#include <hurd.h>
#include <stdio.h>

#include "open.h"
#include "dev.h"

/* Returns a new per-open structure for the device DEV in OPEN.  If an error
   occurs, the error-code is returned, otherwise 0.  */
error_t
open_create (struct dev *dev, struct open **open)
{
  *open = malloc (sizeof (struct open));
  if (*open == NULL)
    return ENOMEM;

  (*open)->dev = dev;
  (*open)->offs = 0;
  mutex_init (&(*open)->lock);

  return 0;
}

/* Free OPEN and any resources it holds.  */
void
open_free (struct open *open)
{
  free (open);
}

/* Writes up to LEN bytes from BUF to OPEN's device at device offset OFFS
   (which may be ignored if the device doesn't support random access),
   and returns the number of bytes written in AMOUNT.  If no error occurs,
   zero is returned, otherwise the error code is returned.  */
error_t
open_write (struct open *open, off_t offs, void *buf, size_t len,
	    vm_size_t *amount)
{
  error_t err;
  if (offs < 0)
    /* Use OPEN's offset.  */
    {
      mutex_lock (&open->lock);
      err = dev_write (open->dev, open->offs, buf, len, amount);
      if (! err)
	open->offs += *amount;
      mutex_unlock (&open->lock);
    }
  else
    err = dev_write (open->dev, offs, buf, len, amount);
  return err;
}    

/* Reads up to AMOUNT bytes from the device into BUF and LEN using the
   standard mach out-array convention.  If no error occurs, zero is returned,
   otherwise the error code is returned.  */
error_t
open_read (struct open *open, off_t offs, size_t amount,
	   void **buf, vm_size_t *len)
{
  error_t err;
  if (offs < 0)
    /* Use OPEN's offset.  */
    {
      mutex_lock (&open->lock);
      err = dev_read (open->dev, open->offs, amount, buf, len);
      if (! err)
	open->offs += *len;
      mutex_unlock (&open->lock);
    }
  else
    err = dev_read (open->dev, offs, amount, buf, len);
  return err;
}   

/* Set OPEN's location to OFFS, interpreted according to WHENCE as by seek.
   The new absolute location is returned in NEW_OFFS (and may not be the same
   as OFFS).  If no error occurs, zero is returned, otherwise the error code
   is returned.  */
error_t
open_seek (struct open *open, off_t offs, int whence, off_t *new_offs)
{
  error_t err = 0;

  mutex_lock (&open->lock);

  switch (whence)
    {
    case SEEK_SET:
      open->offs = offs; break;
    case SEEK_CUR:
      open->offs += offs; break;
    case SEEK_END:
      open->offs = open->dev->store->size - offs; break;
    default:
      err = EINVAL;
    }

  if (! err)
    *new_offs = open->offs;

  mutex_unlock (&open->lock);

  return err;
}
