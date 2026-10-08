#include <sys/prx.h>

#include "include/types.h"
#include "include/common.h"
#include "include/thread.h"
#include "include/timer.h"

#include <semaphore.h>

int ratchetron_start(size_t args, void *argp);
int ratchetron_stop(void);

SYS_MODULE_INFO(Ratchetron, 0, 1, 1);
SYS_MODULE_START(ratchetron_start);
SYS_MODULE_STOP(ratchetron_stop);
SYS_MODULE_EXIT(ratchetron_stop);

#include "include/vsh_notify.h"
#include "include/process.h"
#include "include/ps3mapi.h"
#include "include/ratchetron.h"


int ratchetron_start(size_t args, void *argp)
{
	
    sys_ppu_thread_create(&thread_id_ps3mapi, ps3mapi_thread, NULL, THREAD_PRIO, THREAD_STACK_SIZE_PS3MAPI_SVR, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_PS3MAPI);

	_sys_ppu_thread_exit(0); // remove for ccapi compatibility  // left this comment from original webman source, don't know what they're trying to say because this works fine with ccapi installed

	return SYS_PRX_RESIDENT;
}

static void ratchetron_stop_thread(u64 arg) {
	// Tell the server and every client to stop. Without this the server thread sits in accept()
	// forever and the join below never returns, which is why unloading did nothing and why a
	// normal (XMB) power off hung.
	ratchetron_shutdown();

	u64 exit_code;
	if(thread_id_ps3mapi != SYS_PPU_THREAD_NONE)
	{
		sys_ppu_thread_join(thread_id_ps3mapi, &exit_code);
		thread_id_ps3mapi = SYS_PPU_THREAD_NONE;
	}

	sys_ppu_thread_exit(0);
}

int ratchetron_stop(void)
{
	sys_ppu_thread_t t_id;
	int ret = sys_ppu_thread_create(&t_id, ratchetron_stop_thread, NULL, THREAD_PRIO_STOP, THREAD_STACK_SIZE_STOP_THREAD, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);

	u64 exit_code;
	if(ret == CELL_OK)
	{
		sys_ppu_thread_join(t_id, &exit_code);  // was joining thread_id_ps3mapi, which never exited
	}

	sys_ppu_thread_usleep(500000);  // let exiting threads leave the module's code

	unload_prx_module();

	_sys_ppu_thread_exit(0);

	return SYS_PRX_STOP_OK;
}
