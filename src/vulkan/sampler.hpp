#pragma once

#include "cookie.hpp"
#include "vulkan_common.hpp"
#include "vulkan_headers.hpp"
#include "utils/object_pool.hpp"

namespace Vulkan
{
	enum class StockSampler
	{
		NearestClamp,
		LinearClamp,
		TrilinearClamp,
		NearestWrap,
		LinearWrap,
		TrilinearWrap,
		NearestShadow,
		LinearShadow,
		LinearYUV420P,
		LinearYUV422P,
		LinearYUV444P,
		Count
	};

	//Essentially typedef from VkSamplerCreateInfo
	struct SamplerCreateInfo
	{
		VkFilter mag_filter;
		VkFilter min_filter;
		VkSamplerMipmapMode mipmap_mode;
		VkSamplerAddressMode address_mode_u;
		VkSamplerAddressMode address_mode_v;
		VkSamplerAddressMode address_mode_w;
		float mip_lod_bias;
		VkBool32 anisotropy_enable;
		float max_anisotropy;
		VkBool32 compare_enable;
		VkCompareOp compare_op;
		float min_lod;
		float max_lod;
		VkBorderColor border_color;
		VkBool32 unnormalized_coordinates;
	};

	class Sampler;
	struct SamplerDeleter
	{
		void operator()(Sampler* sampler);
	};

	//Ref-counted wrapper class for vksampler
	class Sampler : public Util::IntrusivePtrEnabled<Sampler, SamplerDeleter, HandleCounter>,
		public Cookie, public InternalSyncEnabled
	{
	public:
		friend struct SamplerDeleter;
		~Sampler();

		VkSampler get_sampler() const
		{
			return sampler;
		}

		const SamplerCreateInfo& get_create_info() const
		{
			return create_info;
		}

	private:
		friend class Util::ObjectPool<Sampler>;
		Sampler(Device* device, VkSampler sampler, const SamplerCreateInfo& info);

		Device* device;
		VkSampler sampler;
		SamplerCreateInfo create_info;
	};
	using SamplerHandle = Util::IntrusivePtr<Sampler>;
}
