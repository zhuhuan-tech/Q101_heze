#pragma once
#ifdef IQMETRICS_EXPORTS
#define IQMETRICS_API __declspec(dllexport)
#else
#define IQMETRICS_API __declspec(dllimport)
#endif
#include <embed.h>

namespace MLIQMetrics {

	class IQMETRICS_API PythonManager {
	public:
		static PythonManager& instance() {
			static PythonManager inst;
			return inst;
		}

	private:
		PythonManager() {
			interpreter = std::make_unique<pybind11::scoped_interpreter>();
		}
		//pybind11::scoped_interpreter guard{};
		std::unique_ptr<pybind11::scoped_interpreter> interpreter;
	};
	class ML_PythonUtl
	{
	};
}
