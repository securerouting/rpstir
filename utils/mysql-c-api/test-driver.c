#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <my_global.h>
#include <mysql.h>

#include "connect.h"
#include "rtr.h"
#include "test-driver.h"
#include "util.h"


/*==============================================================================
 * Use this for temporary test calls.
------------------------------------------------------------------------------*/
void useDbConn(void *connp) {
    (void) connp;  // to avoid -Wunused-parameter
//    uint16_t nonce;
//    getCacheNonce(connp, &nonce);
//    printf("nonce = %hu\n", nonce);

//    setCacheNonce(connp, 3434);

//    int ret;
//    uint32_t sn = 0;
//    ret = getLatestSerialNumber(connp, &sn);
//    if (ret == 0) {
//        printf("serial number found:  %" PRIu32 "\n", sn);
//    } else if (ret == 1) {
//        puts("no serial numbers in the db\n");
//    } else if (ret == -1) {
//        puts("some error occurred\n");
//    }

//    uint32_t ser_num = 0xfffffffc;
//    addNewSerNum(connp, &ser_num);
//    printf("serial number = %u\n", ser_num);

//    addNewSerNum(connp, NULL);

//    deleteSerNum(connp, 99);

//    deleteAllSerNums(connp);

//    void **ptr = NULL;
//    startSerialQuery(connp, ptr, 5);
}


/*==============================================================================
------------------------------------------------------------------------------*/
int main() {
    void *connp = 0;
//    const char host[] = "localhost";
//    const char user[] = "rpki";
//    const char pass[] = "validator";
//    const char db[] = "rpkidb7";

    OPEN_LOG();

//    if ((connp = connectDb(host, user, pass, db)) == NULL) {
    if ((connp = connectDbDefault()) == NULL) {
        CLOSE_LOG();
        return(-1);
    }

    useDbConn(connp);

    disconnectDb(connp);

    CLOSE_LOG();

    return EXIT_SUCCESS;
}
