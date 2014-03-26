/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2012-2013 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

/*
 * BUILD:
 *
 *      cc -o minimal minimal.c -lcouchbase
 *      cl /DWIN32 /Iinclude minimal.c lib\libcouchbase.lib
 *
 * RUN:
 *
 *      valgrind -v --tool=memcheck  --leak-check=full --show-reachable=yes ./minimal
 *      ./minimal <host:port> <bucket> <passwd>
 *      mininal.exe <host:port> <bucket> <passwd>
 */
//#include <snappy-c.h>
#include <stdio.h>
#include <libcouchbase/couchbase.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#define PRIu64 "I64u"
#else
#include <inttypes.h>
#endif

static void error_callback(lcb_t instance, lcb_error_t error, const char *errinfo)
{
    fprintf(stderr, "ERROR: %s (0x%x), %s\n",
            lcb_strerror(instance, error), error, errinfo);
    exit(EXIT_FAILURE);
}

static void store_callback(lcb_t instance, const void *cookie,
                           lcb_storage_t operation,
                           lcb_error_t error,
                           const lcb_store_resp_t *item)
{
    if (error == LCB_SUCCESS) {
        fprintf(stderr, "STORED \"");
        fwrite(item->v.v0.key, sizeof(char), item->v.v0.nkey, stderr);
        fprintf(stderr, "\" CAS: %"PRIu64"\n", item->v.v0.cas);
    } else {
        fprintf(stderr, "STORE ERROR: %s (0x%x)\n",
                lcb_strerror(instance, error), error);
        exit(EXIT_FAILURE);
    }
    (void)cookie;
    (void)operation;
}

static void get_callback(lcb_t instance, const void *cookie, lcb_error_t error,
                         const lcb_get_resp_t *item)
{
    if (error == LCB_SUCCESS) {
        fprintf(stderr, "GOT \"");
        fwrite(item->v.v0.key, sizeof(char), item->v.v0.nkey, stderr);
        fprintf(stderr, "\" CAS: %"PRIu64" FLAGS:0x%x SIZE:%lu\n",
                item->v.v0.cas, item->v.v0.flags, (unsigned long)item->v.v0.nbytes);
        fwrite(item->v.v0.bytes, sizeof(char), item->v.v0.nbytes, stderr);
        fprintf(stderr, "\n");
        char uncompressed[256];
        size_t uncompressed_len = 256;
        //snappy_status status;
        //status=snappy_uncompress(item->v.v0.bytes, item->v.v0.nbytes, uncompressed, &uncompressed_len);
        fprintf(stderr, "uncompressed\n");
        fwrite(uncompressed, sizeof(char), uncompressed_len, stderr);         
        fprintf(stderr,"\n");
  } else {
        fprintf(stderr, "GET ERROR: %s (0x%x)\n",
                lcb_strerror(instance, error), error);
    }
    (void)cookie;
}

int main(int argc, char *argv[])
{
    
    fprintf(stderr,"first line.."); 
    lcb_uint32_t tmo = 350000000;
    const lcb_store_cmd_t *commands[1];
    FILE *file20MB;
    char *membuffer;
    long numbytes;
    file20MB = fopen("output.dat","r");
    fseek(file20MB,0L,SEEK_END);
    numbytes=ftell(file20MB);
    fseek(file20MB,0L,SEEK_SET);
    membuffer = (char*)calloc(numbytes,sizeof(char));
    if(membuffer==NULL)assert(1==2);
    fread(membuffer, sizeof(char),numbytes,file20MB);
    fclose(file20MB); 
    lcb_error_t err;
    lcb_t instance;
    struct lcb_create_st create_options;
    fprintf(stderr,"before memset.."); 
    memset(&create_options, 0, sizeof(create_options));
    
    fprintf(stderr,"after memset..host is %s", argv[1]);
 
    if (argc > 1) {
        create_options.v.v0.host = argv[1];
    }
    if (argc > 2) {
        create_options.v.v0.user = argv[2];
        create_options.v.v0.bucket = argv[2];
    }
    if (argc > 3) {
        create_options.v.v0.passwd = argv[3];
    }
    fprintf(stderr,"after argc section");
    
    err = lcb_create(&instance, &create_options);
    fprintf(stderr,"after lcb create section");
    
    if (err != LCB_SUCCESS) {
        fprintf(stderr, "Failed to create libcouchbase instance: %s\n",
                lcb_strerror(NULL, err));
        return 1;
    }
    /*setting timeout for set/get*/
    lcb_cntl(instance, LCB_CNTL_SET, LCB_CNTL_OP_TIMEOUT, &tmo);
    (void)lcb_set_error_callback(instance, error_callback);
    /* Initiate the connect sequence in libcouchbase */
    if ((err = lcb_connect(instance)) != LCB_SUCCESS) {
        fprintf(stderr, "Failed to initiate connect: %s\n",
                lcb_strerror(NULL, err));
        lcb_destroy(instance);
        return 1;
    }
    (void)lcb_set_get_callback(instance, get_callback);
    (void)lcb_set_store_callback(instance, store_callback);
    /* Run the event loop and wait until we've connected */
    fprintf(stderr,"after setting callbacks");
    lcb_wait(instance);
    {
        fprintf(stderr,"before lcb_hello...");
        err = lcb_hello(instance, NULL);
        fprintf(stderr,"after hello call..");
        if (err != LCB_SUCCESS) {
            fprintf(stderr, "Failed to hello: %s\n", lcb_strerror(NULL, err));
            return 1;
        }
    }
    fprintf(stderr,"after hello");
    const char inflated[] = "aaaaaaaaabbbbbbbccccccdddddd";
    size_t inflated_len = strlen(inflated);
    /*char deflated[256];
    size_t deflated_len = 256;
    //snappy_status status;
    fprintf(stderr, "before compression: "); 
    fwrite(inflated, sizeof(char), inflated_len, stderr);         
    //status = snappy_compress(inflated, inflated_len,
                            deflated, &deflated_len);
    fprintf(stderr, "\nafter compression: "); 
    fwrite(deflated, sizeof(char), deflated_len, stderr);  */       
    /*lcb_wait(instance);
    {
        lcb_store_cmd_t cmd;
        const lcb_store_cmd_t *commands[1];

        commands[0] = &cmd;
        memset(&cmd, 0, sizeof(cmd));
        cmd.v.v0.operation = LCB_SET;
        cmd.v.v0.datatype = LCB_BINARY_DATATYPE_COMPRESSED;
        cmd.v.v0.key = "foo";
        cmd.v.v0.nkey = 3;
        cmd.v.v0.bytes = deflated;
        cmd.v.v0.nbytes = deflated_len;
        fprintf(stderr,"before store");
        err = lcb_store(instance, NULL, 1, commands);
        if (err != LCB_SUCCESS) {
            fprintf(stderr, "Failed to store: %s\n", lcb_strerror(NULL, err));
            return 1;
        }
        fprintf(stderr,"after store");
    }*/
    lcb_wait(instance);
    {
        lcb_store_cmd_t cmd;
        commands[0] = &cmd;
        memset(&cmd, 0, sizeof(cmd));
        cmd.v.v0.operation = LCB_SET;
        cmd.v.v0.datatype = LCB_BINARY_DATATYPE_COMPRESSED;
        cmd.v.v0.key = "foo20M";
        cmd.v.v0.nkey = 6;
        cmd.v.v0.bytes = membuffer;
        cmd.v.v0.nbytes = numbytes;
        fprintf(stderr,"before store 20MB");
        err = lcb_store(instance, NULL, 1, commands);
        if (err != LCB_SUCCESS) {
            fprintf(stderr, "Failed to store 20MB: %s\n", lcb_strerror(NULL, err));
            return 1;
        }
        fprintf(stderr,"after store 20MB");
    }
    lcb_wait(instance);
    {
        lcb_get_cmd_t cmd;
        const lcb_get_cmd_t *commands[1];
        commands[0] = &cmd;
        memset(&cmd, 0, sizeof(cmd));
        cmd.v.v0.key = "foo20M";
        cmd.v.v0.nkey = 6;
        err = lcb_get(instance, NULL, 1, commands);
        if (err != LCB_SUCCESS) {
            fprintf(stderr, "Failed to get: %s\n", lcb_strerror(NULL, err));
            return 1;
        }
    }
    lcb_wait(instance);
    lcb_destroy(instance);

    return 0;
}
