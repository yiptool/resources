/* vim: set ai noet ts=4 sw=4 tw=115: */
//
// Copyright (c) 2014 Nikolay Zapolnov (zapolnov@gmail.com).
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include "resource_loader.h"
#include <android/asset_manager.h>
#include <sstream>
#include <stdexcept>
#include <memory>

namespace Yip
{
	AAssetManager * g_AssetManager_8a09e478cb;
}

namespace
{
	class AssetStreamBuf : public std::streambuf
	{
	public:
		AssetStreamBuf(AAssetManager * manager, const std::string & name)
			: m_Name(name)
		{
			m_Asset = AAssetManager_open(manager, name.c_str(), AASSET_MODE_STREAMING);
			if (!m_Asset)
			{
				std::stringstream ss;
				ss << "unable to open asset '" << name << "'.";
				throw std::runtime_error(ss.str());
			}
		}

		~AssetStreamBuf()
		{
			AAsset_close(m_Asset);
		}

		int underflow() override
		{
			if (gptr() == egptr())
			{
				int result = AAsset_read(m_Asset, m_Buffer, sizeof(m_Buffer));
				if (result > 0)
					setg(m_Buffer, m_Buffer, m_Buffer + result);
				else if (result == 0)
					return std::char_traits<char>::eof();
				else
				{
					std::stringstream ss;
					ss << "error reading asset '" << m_Name << "'.";
					throw std::runtime_error(ss.str());
				}
			}

			return std::char_traits<char>::to_int_type(*gptr());
		}

	private:
		AAsset * m_Asset;
		std::string m_Name;
		char m_Buffer[16384];

		AssetStreamBuf(const AssetStreamBuf &) = delete;
		AssetStreamBuf & operator=(const AssetStreamBuf &) = delete;
	};

	class AssetStream : public std::istream
	{
	public:
		AssetStream(std::unique_ptr<AssetStreamBuf> && buf)
			: std::istream(buf.get()),
			  m_StreamBuf(std::move(buf))
		{
		}

	private:
		std::unique_ptr<AssetStreamBuf> m_StreamBuf;

		AssetStream(const AssetStream &) = delete;
		AssetStream & operator=(const AssetStream &) = delete;
	};
}

std::string Resource::Loader::loadResource(const std::string & name)
{
	return loadResourceFromStream(openResource(name), name);
}

Resource::StreamPtr Resource::Loader::openResource(const std::string & name)
{
	if (!Yip::g_AssetManager_8a09e478cb)
		throw std::runtime_error("asset manager has not been initialized yet.");

	std::unique_ptr<AssetStreamBuf> buf(new AssetStreamBuf(Yip::g_AssetManager_8a09e478cb, name));
	return std::make_shared<AssetStream>(std::move(buf));
}
