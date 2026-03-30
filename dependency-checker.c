#ifdef PLUGINS_NEW


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../plugins.h"

typedef struct {
        uint64_t total_instr;
        uint64_t total_long;
        uint64_t total_expensive;

} thread_data_t;



int dependency_checker_pre_thread(mambo_context *ctx){
    fprintf(stderr, "dependency_checker: pre thread callback\n");
    thread_data_t *t_data = (thread_data_t *)mambo_alloc(ctx, sizeof(thread_data_t));

    assert(t_data != NULL);

    memset(t_data, 0, sizeof(thread_data_t));

    int ret = mambo_set_thread_plugin_data(ctx,t_data);
    assert(ret == MAMBO_SUCCESS);

    fprintf(stderr, "[dep_chain] thread %d started — data allocated at %p\n",
        mambo_get_thread_id(ctx), (void *)t_data);

    return 0;
}

int dependency_checker_post_thread(mambo_context *ctx){

   fprintf(stderr, "dependency_checker: post thread callback\n");

   thread_data_t *t_data = (thread_data_t *)mambo_get_thread_plugin_data(ctx);

   assert(t_data != NULL);

   fprintf(stderr,
        "[dep_chain] thread %d exited — "
        "total=%"PRIu64" long=%"PRIu64" expensive=%"PRIu64"\n",
        mambo_get_thread_id(ctx),
        t_data->total_instr,
        t_data->total_long,
        t_data->total_expensive);


return 0;


__attribute__((constructor)) void dependency_checker_init(void) {

    mambo_context *ctx = mambo_register_plugin();
    int ret;
    ret = mambo_register_pre_thread_cb(ctx, dependency_checker_pre_thread);
    assert(ret == MAMBO_SUCCESS);


    ret = mambo_register_post_thread_cb(ctx, dependency_checker_post_thread);
    assert(ret == MAMBO_SUCCESS);

}




#endif /* PLUGINS_NEW */