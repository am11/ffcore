#pragma once

namespace ff
{
	class ResourceHandle
	{
	public:
		const String &GetName() const;
		IUnknown *GetObject() const;

		void AddRef();
		void Release();

	protected:
		ResourceHandle(StringRef name);
		ResourceHandle(ResourceHandle &rhs);
		ResourceHandle(ResourceHandle &&rhs);
		virtual ~ResourceHandle();

		ResourceHandle &operator=(const ResourceHandle &rhs);
		void SetObject(IUnknown *value);
		virtual void OnObjectChanged(IUnknown *value);

	private:
		friend class ResourceLoader;

		String _name;
		ComPtr<IUnknown> _object;
		long _refs;
	};

	template<typename T>
	class TypedResourceHandle
	{
	public:
		TypedResourceHandle(const String &name);
		TypedResourceHandle(TypedResourceHandle<T> &rhs);
		TypedResourceHandle(TypedResourceHandle<T> &&rhs);

		TypedResourceHandle<T> &operator=(const TypedResourceHandle<T> &rhs);

		T *GetTypedObject() const;

	protected:
		virtual void OnObjectChanged(IUnknown *value) override;

	private:
		ComPtr<T> _typed;
	};
}

template<typename T>
T *ff::TypedResourceHandle<T>::GetTypedObject() const
{
	return _typed;
}

template<typename T>
void ff::TypedResourceHandle<T>::OnObjectChanged(IUnknown *value)
{
	_typed = nullptr;
	_typed.QueryFrom(value);
}

template<typename T>
ff::TypedResourceHandle<T>::TypedResourceHandle(const String &name)
	: ff::ResourceHandle(name)
{
}

template<typename T>
ff::TypedResourceHandle<T>::TypedResourceHandle(TypedResourceHandle<T> &rhs)
	: ff::ResourceHandle(rhs)
	, _typed(rhs._typed)
{
}

template<typename T>
ff::TypedResourceHandle<T>::TypedResourceHandle(TypedResourceHandle<T> &&rhs)
	: ff::ResourceHandle(std::move(rhs))
	, _typed(std::move(rhs._typed))
{
}

template<typename T>
ff::TypedResourceHandle<T> &ff::TypedResourceHandle<T>::operator=(const TypedResourceHandle<T> &rhs)
{
	if (this != &rhs)
	{
		__super::operator=(rhs);
		_typed = rhs._typed;
	}

	return *this;
}
