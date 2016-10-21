#ifndef HC_SYSCALLS_H
#define HC_SYSCALLS_H

#include <hc.hpp>
#include <array>

struct kfd_sc;

class syscalls {
	using arg_array = ::std::array<uint64_t, 6>;
	using status_t  = ::std::atomic_uint;

	kfd_sc *syscalls_ = NULL;
	size_t elements_ = 0;
	const bool wave_control_;

	syscalls(const syscalls&) = delete;
	syscalls(bool use_wave_control);
	~syscalls();

	kfd_sc &get_slot() const [[hc]];
	static status_t &get_atomic_status(kfd_sc &slot) [[hc]] [[cpu]];
	int send_common(kfd_sc &slot, int sc, const arg_array &args) [[hc]];
	uint64_t wait_get_ret(kfd_sc &slot) [[hc]];
public:
	static syscalls& get() [[cpu]]; //[[hc]]
	void wait_all() const [[cpu]];
	void wait_one_free() [[hc]];
	uint64_t wait_get_ret() [[hc]]
	{
		return wait_get_ret(get_slot());
	}
	uint64_t send(int sc, const arg_array &args = {}) [[hc]]
	{
		kfd_sc &slot = get_slot();
		uint64_t ret = send_common(slot, sc, args);
		if (!ret)
			ret = wait_get_ret(slot);
		return ret;
	}
	int send_nonblock(int sc, const arg_array &args = {}) [[hc]];
};
#endif
