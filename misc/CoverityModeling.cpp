namespace std
{
	template<typename _Tp> class shared_ptr
	{
	};
}

namespace BaseLib
{
	class FileDescriptor
	{
	};

	class FileDescriptorManager
	{
		std::shared_ptr<FileDescriptor> add(int fileDescriptor)
		{
			__coverity_panic__();
		}
	};
}
