#ifndef _FSTRCMP_H
#define _FSTRCMP_H

  /* GNU gettext - internationalization aids
  Copyright (C) 1995 Free Software Foundation, Inc.

  This file was written by Peter Miller <pmiller@agso.gov.au>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#define PARAMS(proto) proto

class FuzzyStrCmp
{
public:
  FuzzyStrCmp();
  virtual ~FuzzyStrCmp();

  double fstrcmp (const char *__s1, const char *__s2, double __minimum);

protected:
  /*
   * Data on one input string being compared.
   */
  struct string_data
  {
    /* The string to be compared. */
    const char *data;
    /* The length of the string to be compared. */
    int data_length;
    /* The number of characters inserted or deleted. */
    int edit_count;
  };

  int diag (int xoff, int xlim, int yoff, int ylim, int minimal, struct partition *part);
  void compareseq (int xoff, int xlim, int yoff, int ylim, int minimal);

  int heuristic;
  int max_edits;

  /* Vector, indexed by diagonal, containing 1 + the X coordinate of the
   point furthest along the given diagonal in the forward search of the
   edit matrix.  */
  int *fdiag;

  /* Vector, indexed by diagonal, containing the X coordinate of the point
   furthest along the given diagonal in the backward search of the edit
   matrix.  */
  int *bdiag;

  /* Edit scripts longer than this are too expensive to compute.  */
  int too_expensive;
  struct string_data string[2];

  int *fdiag_buf;
  size_t fdiag_max;
};

double fstrcmp (const char *__s1, const char *__s2, double __minimum);

#endif
