#include "test_job_system.h"
#include "Module/Util/job_system.h"
#include <iostream>
#include <thread>

pf::jobsystem::context ctx;

int test_jobsystem() {
	pf::jobsystem::Initialize();


	for (int i = 0; i < 100; i++) {
		pf::jobsystem::Execute(ctx, [=](pf::jobsystem::JobArgs args) {
				std::cout << " Current index : " << i << std::endl;
			});
	}
	std::thread([] {
		pf::jobsystem::Wait(ctx);
		std::cout << " \n[wi::initializer] Wicked Engine Initialized (%d ms)"<< std::endl;
		//wilog("\n[wi::initializer] Wicked Engine Initialized (%d ms)", (int)std::round(timer.elapsed()));
		}).detach();



	//pf::jobsystem::Wait(ctx);
	std::cout << " Begin ShutDown " << std::endl;
	pf::jobsystem::ShutDown();
	return 0;
}