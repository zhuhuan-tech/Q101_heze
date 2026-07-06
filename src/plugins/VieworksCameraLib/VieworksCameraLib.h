#pragma region Macros and defines
// Export functions through shared library
#  if defined(EXPORTING_VIEWORKSCAMERA)
#    define VIEWORKS_EXPORT __declspec(dllexport)
#  else
#    define VIEWORKS_EXPORT __declspec(dllimport)
#  endif

