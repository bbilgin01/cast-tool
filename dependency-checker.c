#ifdef PLUGINS_NEW


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "../plugins.h"

int dependency_checker_pre_thread(mambo_context *ctx){
    fprintf(stderr, "dependency_checker: pre thread callback\n");
    return 0;   
}

int dependency_checker_post_thread(mambo_context *ctx){
    fprintf(stderr, "dependency_checker: post thread callback\n");
    return 0;
}

__attribute__((constructor)) void dependency_checker_init(void) {

    mambo_context *ctx = mambo_register_plugin();
    mambo_register_pre_thread_cb(ctx, dependency_checker_pre_thread);
    mambo_register_post_thread_cb(ctx, dependency_checker_post_thread);
}




#endif /* PLUGINS_NEW */