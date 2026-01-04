#include "test_timer.h"
#include "Module/Util/p_timer.h"
#include <iostream>
int test_timer() {

	pf::Timer timer;
	timer.record();


	double detal = timer.elapsed_seconds();

	std::cout << " timer record : " << detal << " ms" << std::endl;
	return 0;
}