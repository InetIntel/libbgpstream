/*
 * This file is part of bgpstream
 *
 * CAIDA, UC San Diego
 * bgpstream-info@caida.org
 *
 * Copyright (C) 2012 The Regents of the University of California.
 * Authors: Alistair King, Chiara Orsini
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "bgpstream_int.h"
#include "bgpdump_lib.h"
#include "bgpstream_debug.h"
#include "utils.h"

/* TEMPORARY STRUCTURES TO FAKE DATA INTERFACE PLUGIN API */

#ifdef WITH_DATA_INTERFACE_SQLITE
static bgpstream_data_interface_info_t bgpstream_sqlite_info = {
  BGPSTREAM_DATA_INTERFACE_SQLITE, "sqlite",
  "Retrieve metadata information from a sqlite database",
};
#endif


#ifdef WITH_DATA_INTERFACE_SQLITE
static bgpstream_data_interface_option_t bgpstream_sqlite_options[] = {
  /* SQLITE database file name */
  {
    BGPSTREAM_DATA_INTERFACE_SQLITE, 0, "db-file",
    "sqlite database (default: " STR(BGPSTREAM_DI_SQLITE_DB_FILE) ")",
  },
};
#endif


/* allocate memory for a new bgpstream interface
 */
bgpstream_t *bgpstream_create()
{
  bgpstream_debug("BS: create start");
  bgpstream_t *bs = (bgpstream_t *)malloc(sizeof(bgpstream_t));
  if (bs == NULL) {
    return NULL; // can't allocate memory
  }
  bs->filter_mgr = bgpstream_filter_mgr_create();
  if (bs->filter_mgr == NULL) {
    bgpstream_destroy(bs);
    bs = NULL;
    return NULL;
  }
  bs->di_mgr = bgpstream_di_mgr_create(bs->filter_mgr);
  if (bs->di_mgr == NULL) {
    bgpstream_destroy(bs);
    return NULL;
  }
  /* create an empty input mgr
   * the input queue will be populated when a
   * bgpstream record is requested */
  bs->input_mgr = bgpstream_input_mgr_create();
  if (bs->input_mgr == NULL) {
    bgpstream_destroy(bs);
    bs = NULL;
    return NULL;
  }
  bs->reader_mgr = bgpstream_reader_mgr_create(bs->filter_mgr);
  if (bs->reader_mgr == NULL) {
    bgpstream_destroy(bs);
    bs = NULL;
    return NULL;
  }
  /* memory for the bgpstream interface has been
   * allocated correctly */
  bs->status = BGPSTREAM_STATUS_ALLOCATED;
  bgpstream_debug("BS: create end");
  return bs;
}
/* side note: filters are part of the bgpstream so they
 * can be accessed both from the input_mgr and the
 * reader_mgr (input_mgr use them to apply a coarse-grained
 * filtering, the reader_mgr applies a fine-grained filtering
 * of the data provided by the input_mgr)
 */

/* configure filters in order to select a subset of the bgp data available */
void bgpstream_add_filter(bgpstream_t *bs, bgpstream_filter_type_t filter_type,
                          const char *filter_value)
{
  bgpstream_debug("BS: set_filter start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return; // nothing to customize
  }
  bgpstream_filter_mgr_filter_add(bs->filter_mgr, filter_type, filter_value);
  bgpstream_debug("BS: set_filter end");
}

void bgpstream_add_rib_period_filter(bgpstream_t *bs, uint32_t period)
{
  bgpstream_debug("BS: set_filter start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return; // nothing to customize
  }
  bgpstream_filter_mgr_rib_period_filter_add(bs->filter_mgr, period);
  bgpstream_debug("BS: set_filter end");
}

void bgpstream_add_recent_interval_filter(bgpstream_t *bs, const char *interval,
                                          uint8_t islive)
{

  uint32_t starttime, endtime;
  bgpstream_debug("BS: set_filter start");

  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return; // nothing to customize
  }

  if (bgpstream_time_calc_recent_interval(&starttime, &endtime, interval) ==
      0) {
    bgpstream_log_err("Failed to determine suitable time interval");
    return;
  }

  if (islive) {
    bgpstream_set_live_mode(bs);
    endtime = BGPSTREAM_FOREVER;
  }

  bgpstream_filter_mgr_interval_filter_add(bs->filter_mgr, starttime, endtime);
  bgpstream_debug("BS: set_filter end");
}

void bgpstream_add_interval_filter(bgpstream_t *bs, uint32_t begin_time,
                                   uint32_t end_time)
{
  bgpstream_debug("BS: set_filter start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return; // nothing to customize
  }
  if (end_time == BGPSTREAM_FOREVER) {
    bgpstream_set_live_mode(bs);
  }
  bgpstream_filter_mgr_interval_filter_add(bs->filter_mgr, begin_time,
                                           end_time);
  bgpstream_debug("BS: set_filter end");
}

int bgpstream_get_data_interfaces(bgpstream_t *bs,
                                  bgpstream_data_interface_id_t **if_ids)
{
  return bgpstream_di_mgr_get_data_interfaces(bs->di_mgr, if_ids);
}

bgpstream_data_interface_id_t
bgpstream_get_data_interface_id_by_name(bgpstream_t *bs, const char *name)
{
  return bgpstream_di_mgr_get_data_interface_id_by_name(bs->di_mgr, name);
}

bgpstream_data_interface_info_t *
bgpstream_get_data_interface_info(bgpstream_t *bs,
                                  bgpstream_data_interface_id_t if_id)
{
  return bgpstream_di_mgr_get_data_interface_info(bs->di_mgr, if_id);
}

int bgpstream_get_data_interface_options(
  bgpstream_t *bs, bgpstream_data_interface_id_t if_id,
  bgpstream_data_interface_option_t **opts)
{
  return bgpstream_di_mgr_get_data_interface_options(bs->di_mgr, if_id, opts);
}

bgpstream_data_interface_option_t *bgpstream_get_data_interface_option_by_name(
  bgpstream_t *bs, bgpstream_data_interface_id_t if_id, const char *name)
{
  bgpstream_data_interface_option_t *options;
  int opt_cnt = 0;
  int i;

  opt_cnt = bgpstream_get_data_interface_options(bs, if_id, &options);

  if (options == NULL || opt_cnt == 0) {
    return NULL;
  }

  for (i = 0; i < opt_cnt; i++) {
    if (strcmp(options[i].name, name) == 0) {
      return &options[i];
    }
  }

  return NULL;
}

/* configure the data interface options */

int bgpstream_set_data_interface_option(
  bgpstream_t *bs, bgpstream_data_interface_option_t *option_type,
  const char *option_value)
{
  return bgpstream_di_mgr_set_data_interface_option(bs->di_mgr,
                                                    option_type, option_value);
}

/* configure the interface so that it connects
 * to a specific data interface
 */
void bgpstream_set_data_interface(bgpstream_t *bs,
                                  bgpstream_data_interface_id_t di)
{
  bgpstream_di_mgr_set_data_interface(bs->di_mgr, di);
}

bgpstream_data_interface_id_t bgpstream_get_data_interface_id(bgpstream_t *bs)
{
  return bgpstream_di_mgr_get_data_interface_id(bs->di_mgr);
}

/* configure the interface so that it blocks
 * waiting for new data
 */
void bgpstream_set_live_mode(bgpstream_t *bs)
{
  bgpstream_di_mgr_set_blocking(bs->di_mgr);
}

/* turn on the bgpstream interface, i.e.:
 * it makes the interface ready
 * for a new get next call
*/
int bgpstream_start(bgpstream_t *bs)
{
  // validate the filters that have been set
  int rc;
  if ((rc = bgpstream_filter_mgr_validate(bs->filter_mgr)) != 0) {
    return rc;
  }

  // turn on data interface
  // turn on data interface
  if (bgpstream_di_mgr_start(bs->di_mgr) != 0) {
    bs->status = BGPSTREAM_STATUS_ALLOCATED;
    bgpstream_debug("BS: init warning: check if data interface provided is ok");
    bgpstream_debug("BS: init end: not ok");
    return -1;
  }

  bs->status = BGPSTREAM_STATUS_ON;
  return 0;
}

/* this function returns the next available record read
 * if the input_queue (i.e. list of files connected from
 * an external source) or the reader_cqueue (i.e. list
 * of bgpdump currently open) are empty then it
 * triggers a mechanism to populate the queues or
 * return 0 if nothing is available
 */
int bgpstream_get_next_record(bgpstream_t *bs, bgpstream_record_t *record)
{
  bgpstream_debug("BS: get next");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ON)) {
    return -1; // wrong status
  }

  int num_query_results = 0;
  bgpstream_input_t *bs_in = NULL;

  // if bs_record contains an initialized bgpdump entry we destroy it
  bgpstream_record_clear(record);

  while (bgpstream_reader_mgr_is_empty(bs->reader_mgr)) {
    bgpstream_debug("BS: reader mgr is empty");
    // get new data to process and set the reader_mgr
    while (bgpstream_input_mgr_is_empty(bs->input_mgr)) {
      bgpstream_debug("BS: input mgr is empty");
      /* query the external source and append new
       * input objects to the input_mgr queue */
      num_query_results =
        bgpstream_di_mgr_get_queue(bs->di_mgr, bs->input_mgr);
      if (num_query_results == 0) {
        bgpstream_debug("BS: no (more) data are available");
        return 0; // no (more) data are available
      }
      if (num_query_results < 0) {
        bgpstream_debug("BS: error during di_mgr_update_input_queue");
        return -1; // error during execution
      }
      bgpstream_debug("BS: got results from data_interface");
    }
    bgpstream_debug("BS: input mgr not empty");
    bs_in = bgpstream_input_mgr_get_queue_to_process(bs->input_mgr);
    bgpstream_reader_mgr_add(bs->reader_mgr, bs_in, bs->filter_mgr);
    bgpstream_input_mgr_destroy_queue(bs_in);
    bs_in = NULL;
  }
  bgpstream_debug("BS: reader mgr not empty");
  /* init the record with a pointer to bgpstream */
  record->bs = bs;
  return bgpstream_reader_mgr_get_next_record(bs->reader_mgr, record,
                                              bs->filter_mgr);
}

/* turn off the bgpstream interface TODO: remove me */
static void bgpstream_stop(bgpstream_t *bs)
{
  bgpstream_debug("BS: close start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ON)) {
    return; // nothing to close
  }
  bs->status = BGPSTREAM_STATUS_OFF; // interface is off
  bgpstream_debug("BS: close end");
}

/* destroy a bgpstream interface istance
 */
void bgpstream_destroy(bgpstream_t *bs)
{
  bgpstream_debug("BS: destroy start");
  if (bs == NULL) {
    return; // nothing to destroy
  }
  bgpstream_stop(bs);
  bgpstream_input_mgr_destroy(bs->input_mgr);
  bs->input_mgr = NULL;
  bgpstream_reader_mgr_destroy(bs->reader_mgr);
  bs->reader_mgr = NULL;
  bgpstream_filter_mgr_destroy(bs->filter_mgr);
  bs->filter_mgr = NULL;
  bgpstream_di_mgr_destroy(bs->di_mgr);
  bs->di_mgr = NULL;
  free(bs);
  bgpstream_debug("BS: destroy end");
}
