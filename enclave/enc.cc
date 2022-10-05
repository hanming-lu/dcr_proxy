// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <stdio.h>
#include <openssl/evp.h>

#include "crypto.hpp"

// Include the trusted dcr_proxy header that is generated
// during the build. This file is generated by calling the
// sdk tool oeedger8r against the dcr_proxy.edl file.
#include "dcr_proxy_t.h"

// This is the function that the host calls. It prints
// a message in the enclave before calling back out to
// the host to print a message from there too.
void enclave_dcr_proxy()
{
    // Print a message from the enclave. Note that this
    // does not directly call fprintf, but calls into the
    // host and calls fprintf from there. This is because
    // the fprintf function is not part of the enclave
    // as it requires support from the kernel.
    fprintf(stdout, "Host called into enclave to print: Running enclave code!\n");

    // Call back into the host
    oe_result_t result = host_dcr_proxy();
    if (result != OE_OK)
    {
        fprintf(
            stderr,
            "Call to host_dcr_proxy failed: result=%u (%s)\n",
            result,
            oe_result_str(result));
    }
}

void enc_handle_write(const char* msg, unsigned int msg_len, unsigned char* digest, unsigned int* dilen)
{
    fprintf(stdout, 
        "enc_handle_write called with msg %s\n",
        msg
    );
    
    hmac_sha256(msg, msg_len, digest, dilen);
    fprintf(stdout, 
        "after hmac_sha256: %s\n",
        digest
    );

    return;
}

void enc_handle_ack(const char* msg)
{
    fprintf(stdout, "enc_handle_ack called\n");
    return;
}