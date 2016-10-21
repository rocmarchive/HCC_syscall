#include <iostream>
#include <fstream>
#include <thread>
#include <hsakmt.h>
#include <linux/kfd_sc.h>

#include <hc.hpp>
#include <hc_syscalls.h>

#include <unistd.h>
#include <fcntl.h>

static const char *module_param =
	"/sys/module/amdkfd/parameters/sc_use_wave_management";

static bool use_wave_control()
{
	::std::fstream f(module_param, ::std::ios_base::in);
	char setting = 'x';
	f >> setting;
	if (setting != 'Y' && setting != 'N') {
		::std::cerr << "ERROR: Failed to determine wave control mode\n";
		exit(1);
	}
	return setting == 'Y';
}

syscalls& syscalls::get() [[cpu]] // [[hc]]
{
	static syscalls instance(use_wave_control());
	return instance;
}

syscalls::syscalls(bool use_wave_control): wave_control_(use_wave_control)
{
	void *sc_area = NULL;
	HSAuint32 elements = 0;
	hsaKmtOpenKFD();
	HSAKMT_STATUS s = hsaKmtGetSyscallArea(&elements, &sc_area);
	if (sc_area) {
		syscalls_ = static_cast<kfd_sc*>(sc_area);
		elements_ = elements;
	}
}

syscalls::~syscalls()
{
	if (syscalls_)
		hsaKmtFreeSyscallArea();
	hsaKmtCloseKFD();
}

extern "C" void __hsa_sendmsg(uint32_t msg)[[hc]];
extern "C" void __hsa_sendmsghalt(uint32_t msg)[[hc]];
extern "C" void __hsa_fence(void)[[hc]];

extern "C" uint32_t __hsail_get_lane_id(void)[[hc]];
extern "C" uint32_t __hsa_gethwid(void)[[hc]];

kfd_sc  &syscalls::get_slot() const [[hc]]
{
	uint32_t id = __hsa_gethwid();
	unsigned slot =
		(((id >> 4) & 0x3) | ((id >> 6) & 0x1fc)) * 10 + (id & 0xf);
	int idx = (slot * 64) + __hsail_get_lane_id();
	return syscalls_[idx % elements_];
}

syscalls::status_t &syscalls::get_atomic_status(kfd_sc &slot) [[hc]] [[cpu]]
{
	//this is ugly and probably broken
	status_t &status = *(status_t*)&slot.status;
	return status;
}

uint64_t syscalls::wait_get_ret(kfd_sc& slot) [[hc]]
{
	status_t &status = get_atomic_status(slot);
	while (status != KFD_SC_STATUS_FINISHED) ;
	uint64_t ret = slot.arg[0];
	status = KFD_SC_STATUS_FREE;
	return ret;
}

void syscalls::wait_one_free() [[hc]]
{
	kfd_sc &slot = get_slot();
	status_t &status = get_atomic_status(slot);
	while (status != KFD_SC_STATUS_FREE);
	//TODO we can probably use s_sleep here
}

void syscalls::wait_all() const [[cpu]]
{
	for (unsigned i = 0; i < elements_; ++i) {
		status_t &status = get_atomic_status(syscalls_[i]);
		while (status != KFD_SC_STATUS_FINISHED &&
		       status != KFD_SC_STATUS_FREE)
			::std::this_thread::yield();
	}
}
int syscalls::send_nonblock(int sc, const arg_array &args) [[hc]]
{
	return send_common(get_slot(), sc | KFD_SC_NORET_FLAG, args);
}

int syscalls::send_common(kfd_sc &slot, int sc, const arg_array &args) [[hc]]
{
	if (syscalls_ == NULL || elements_ == 0)
		return EINVAL;

	status_t &status = get_atomic_status(slot);

	uint32_t expected = KFD_SC_STATUS_FREE;
	if (!status.compare_exchange_strong(expected, KFD_SC_STATUS_GPU))
		return EAGAIN;
	slot.sc_num = sc;
	// std::copy should work here if it were implemented for AMP
	for (int i = 0; i < args.size(); ++i)
		slot.arg[i] = args[i];

	// This fence might be unnecessary, MUBUF intructions complete
	// in-order. However, FLAT instructions do not. It might be the
	// case that the ordering is maintained in this case. However,
	// the specs explicitly say:
	// "the only sensible S_WAITCNT value to use after FLAT instructions
	// is zero" (CI ISA p. 85)
	__hsa_fence();
	status = KFD_SC_STATUS_READY;
	// Make sure the status update is visible before issuing interrupt
	// These are scalar, so they get executed only once per wave.
	__hsa_fence();
	if ((sc & KFD_SC_NORET_FLAG) != 0 || !wave_control_)
		__hsa_sendmsg(0);
	else
		__hsa_sendmsghalt(0);
	return 0;
}
