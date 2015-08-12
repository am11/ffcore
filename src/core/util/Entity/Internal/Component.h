#pragma once

namespace ff
{
	namespace details
	{
		/// Internally, all entity components derive from this class.
		/// You should ignore this and derive from ComponentBase<YourClass> instead.
		struct Component
		{
		protected:
			/// Used by GetFactoryIndex() static functions in each component class
			UTIL_API static size_t GetNextFactoryIndex();
		};
	}
}
