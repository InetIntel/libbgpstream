/*
 * This file is part of bgpstream
 *
 * Copyright (C) 2015 The Regents of the University of California.
 * Authors: Alistair King, Chiara Orsini
 *
 * All rights reserved.
 *
 * This code has been developed by CAIDA at UC San Diego.
 * For more information, contact bgpstream-info@caida.org
 *
 * This source code is proprietary to the CAIDA group at UC San Diego and may
 * not be redistributed, published or disclosed without prior permission from
 * CAIDA.
 *
 * Report any bugs, questions or comments to bgpstream-info@caida.org
 *
 */

#ifndef _BGPSTREAM_DATASOURCE_H
#define _BGPSTREAM_DATASOURCE_H

#include "bgpstream_constants.h"
#include "bgpstream_input.h"
#include "bgpstream_filter.h"

#include <stdlib.h>
#include <stdio.h>

#include "bgpstream_datasource_mysql.h"
#include "bgpstream_datasource_singlefile.h"
#include "bgpstream_datasource_csvfile.h"
#include "bgpstream_datasource_sqlite.h"



typedef enum {
  BGPSTREAM_DATASOURCE_STATUS_ON,    /* current data source is on */
  BGPSTREAM_DATASOURCE_STATUS_OFF,   /* current data source is off */
  BGPSTREAM_DATASOURCE_STATUS_ERROR  /* current data source generated an error */
} bgpstream_datasource_status_t;



typedef struct struct_bgpstream_datasource_mgr_t {  
  bgpstream_data_interface_id_t datasource;
  // datasources available
  bgpstream_mysql_datasource_t *mysql_ds;
  bgpstream_singlefile_datasource_t *singlefile_ds;
  bgpstream_csvfile_datasource_t *csvfile_ds;
  bgpstream_sqlite_datasource_t *sqlite_ds;
  
  // datasource specific_options
  char *mysql_dbname;
  char *mysql_user;
  char *mysql_password;
  char *mysql_host;
  unsigned int mysql_port;
  char *mysql_socket;
  char *mysql_dump_path;
  char *singlefile_rib_mrtfile;
  char *singlefile_upd_mrtfile;
  char *csvfile_file;
  char *sqlite_file;
  
  // blocking options
  int blocking;
  int backoff_time;
  bgpstream_datasource_status_t status;
} bgpstream_datasource_mgr_t;


/* allocates memory for datasource_mgr */
bgpstream_datasource_mgr_t *bgpstream_datasource_mgr_create();

void bgpstream_datasource_mgr_set_data_interface(bgpstream_datasource_mgr_t *datasource_mgr,
						 const bgpstream_data_interface_id_t datasource);

void bgpstream_datasource_mgr_set_data_interface_option(bgpstream_datasource_mgr_t *datasource_mgr,
							const bgpstream_data_interface_option_t *option_type,
							const char *option_value);

/* init the datasource_mgr and start/init the selected datasource */
void bgpstream_datasource_mgr_init(bgpstream_datasource_mgr_t *datasource_mgr,
				   bgpstream_filter_mgr_t *filter_mgr);

void bgpstream_datasource_mgr_set_blocking(bgpstream_datasource_mgr_t *datasource_mgr);

int bgpstream_datasource_mgr_update_input_queue(bgpstream_datasource_mgr_t *datasource_mgr,
						bgpstream_input_mgr_t *input_mgr);

/* stop the active data source */
void bgpstream_datasource_mgr_close(bgpstream_datasource_mgr_t *datasource_mgr);

/* destroy the memory allocated for the datasource_mgr */
void bgpstream_datasource_mgr_destroy(bgpstream_datasource_mgr_t *datasource_mgr);




#endif /* _BGPSTREAM_DATASOURCE */
