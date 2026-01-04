#include "test_job_system.h"
#include "Module/Util/job_system.h"
#include <iostream>

int test_jobsystem() {
	pf::jobsystem::Initialize();
	pf::jobsystem::context ctx;

	for (int i = 0; i < 100; i++) {
		pf::jobsystem::Execute(ctx, [=](pf::jobsystem::JobArgs args) {
				std::cout << " Current index : " << i << std::endl;
			});
	}

	pf::jobsystem::Wait(ctx);
	pf::jobsystem::ShutDown();
	return 0;
}