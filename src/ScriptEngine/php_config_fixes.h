#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#ifndef HAVE_SSIZE_T
	#define HAVE_SSIZE_T 1
#endif

#define isnan(a) std::isnan(a)
