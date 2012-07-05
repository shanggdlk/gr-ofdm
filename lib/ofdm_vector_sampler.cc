/* -*- c++ -*- */
/*
* Copyright 2004 Free Software Foundation, Inc.
*
* This file is part of GNU Radio
*
* GNU Radio is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* GNU Radio is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with GNU Radio; see the file COPYING.  If not, write to
* the Free Software Foundation, Inc., 51 Franklin Street,
* Boston, MA 02110-1301, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_block.h>

#include <string.h>


#include <ofdm_vector_sampler.h>
#include <gr_io_signature.h>

#include <iostream>
#include <algorithm>

#define DEBUG(x) x

ofdm_vector_sampler_sptr ofdm_make_vector_sampler (int itemsize,int vlen)
{
  return ofdm_vector_sampler_sptr (new ofdm_vector_sampler (itemsize,vlen));
}

ofdm_vector_sampler::ofdm_vector_sampler (int itemsize,int vlen)
  : gr_block ("vector_sampler",
          gr_make_io_signature2 (2, 2, itemsize, sizeof(char)),
          gr_make_io_signature  (1, 1, itemsize*vlen)),
    d_itemsize(itemsize),d_vlen(vlen)
{
  set_history(d_vlen);
}

void ofdm_vector_sampler::forecast (int noutput_items, gr_vector_int &ninput_items_required)
{
//  int nreqd = d_vlen + noutput_items - 1;
  int const nreqd = d_vlen;

  for (unsigned i = 0; i < ninput_items_required.size(); i++)
    ninput_items_required[i] = nreqd;
}

int ofdm_vector_sampler::general_work (
    int noutput_items,
    gr_vector_int &ninput_items,
    gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items)
{
  const char *iptr = static_cast<const char *>(input_items[0]);
  const char *trigger = static_cast<const char*>(input_items[1]);
  char *optr = static_cast<char *>(output_items[0]);

  int produced = 0;

  // item count constraint by shortest input stream
  int i_max = std::min(ninput_items[0],ninput_items[1]) - d_vlen+1;

  int i;
  trigger += d_vlen-1;
  for(i = 0; i < i_max; ++i) {
    if(trigger[i] == 1) {
      if(produced < noutput_items) {
        // rearrange interval [i, i+d_fft_length) from input stream to output vector item
        memcpy(optr,iptr+i*d_itemsize,d_vlen*d_itemsize);
        ++produced; optr += d_vlen*d_itemsize;
      } else {
        // consume up to this point
        break;
      }
    }
  }

//  std::cout << "SAMPLER " << unique_id() << ": produced=" << produced
//            << " consumed=" << i << " nout=" << noutput_items
//            << " nin=" << i_max << " nin0=" << ninput_items[0]
//            << " nin1=" << ninput_items[1] << std::endl;

  // example: fft_length = 2, nouput_items = 1
  // iptr:     1 2 3 4 5
  // trigger: 0  1 0 1 1
  //                ^ i = 2, break, consume 2 items
  // out:      (1 2)

  // next step: noutput_items = 2
  // iptr:    3 4 5 6 7 8
  // trigger: 0 1 1 0 0 0
  //            ^ i = 0
  // out:     (3 4) (4 5)
  // consume everything except the last (fft_length)-1 = i_max items
  // i = i_max after for-loop

  consume_each(i);
  return produced;
}




int
ofdm_vector_sampler::noutput_forecast( gr_vector_int &ninput_items,
    int available_space, int max_items_avail, std::vector<bool> &input_done )
{

  // give us every input item we have. we will produce at maximum nout.
  // both streams are consumed at the same rate.
  // we need at least d_vlen items to produce at least 1 item.

  int min_items = std::min( ninput_items[0], ninput_items[1] );
  int nout = std::max( min_items - d_vlen + 1, 0 );

  // if any upstream block is done and there are no more buffered items,
  // say goodbye

  if( nout == 0 && ( input_done[0] || input_done[1] ) )
  {
    return -1;
  }

  // if we have no input items, signal that we are blocked in
  if( ninput_items[0] == 0 || ninput_items[1] == 0 ){
    return 0;
  }

  return nout;
}
