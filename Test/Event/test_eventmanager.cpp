#include "test_eventmanager.h"
#include "Module/Util/event_handle.h"
#include <iostream>
using namespace pf::eventhandler;
int test_eventmanager() {

	static Handle de =Subscribe(pf::eventhandler::EVENT_THREAD_SAFE_POINT,[=](uint64_t userdata) {
			std::cout << " Subscribe Iput Funiton Param : "<< userdata << std::endl;
		});

	
	pf::eventhandler::FireEvent(pf::eventhandler::EVENT_THREAD_SAFE_POINT,0);
	return 0;
}